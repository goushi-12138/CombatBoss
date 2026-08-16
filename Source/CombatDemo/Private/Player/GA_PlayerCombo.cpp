#include "Player/GA_PlayerCombo.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h" // 命中位置计算：取目标胶囊半径

UGA_PlayerCombo::UGA_PlayerCombo()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    FGameplayTagContainer NewTags;
    NewTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Combo")));
    SetAssetTags(NewTags);

    // 命中 GameplayCue 默认值（可在蓝图 Class Defaults 覆盖）
    HitCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Player.Hit"));
}

void UGA_PlayerCombo::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    CurrentComboIndex = 0;
    PlayComboSection(CurrentComboIndex);
}

void UGA_PlayerCombo::PlayComboSection(int32 SectionIndex)
{
    if (!ComboMontage)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    // 清理上一段的所有定时器和任务
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ComboWindowOpenTimer);
        GetWorld()->GetTimerManager().ClearTimer(ComboWindowCloseTimer);
        GetWorld()->GetTimerManager().ClearTimer(DamageTraceTimer);
    }
    if (ComboWaitTask && ComboWaitTask->IsActive())
    {
        ComboWaitTask->EndTask();
    }
    ComboWaitTask = nullptr;

    bComboWindowOpen = false;
    bPendingNextCombo = false;
    bIsDamageWindowActive = false;
    DamagedTargets.Empty();

    // 确定 Section 名称
    FName TargetSection;
    switch (SectionIndex)
    {
    case 0: TargetSection = Section1; break;
    case 1: TargetSection = Section2; break;
    case 2: TargetSection = Section3; break;
    case 3: TargetSection = Section4; break;
    default: TargetSection = Section1; break;
    }

    // 播放蒙太奇，并绑定完成/中断回调
    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, ComboMontage, 1.0f, TargetSection);
    MontageTask->OnCompleted.AddDynamic(this, &UGA_PlayerCombo::OnMontageCompleted);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_PlayerCombo::OnMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_PlayerCombo::OnMontageInterrupted);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_PlayerCombo::OnMontageInterrupted);
    MontageTask->ReadyForActivation();

    bMontagePlaying = true;

    // 获取当前 Section 时长
    float SectionDuration = ComboMontage->GetSectionLength(ComboMontage->GetSectionIndex(TargetSection));
    if (SectionDuration <= 0.0f)
    {
        SectionDuration = ComboMontage->GetPlayLength();
    }

    // 开启输入窗口定时器（结束前 PreInputWindow 秒）
    float OpenDelay = FMath::Max(0.0f, SectionDuration - PreInputWindow);
    GetWorld()->GetTimerManager().SetTimer(
        ComboWindowOpenTimer,
        this,
        &UGA_PlayerCombo::OnComboWindowOpen,
        OpenDelay,
        false
    );

    // 关闭输入窗口定时器（结束后 PostInputWindow 秒）
    float CloseDelay = SectionDuration + PostInputWindow;
    GetWorld()->GetTimerManager().SetTimer(
        ComboWindowCloseTimer,
        this,
        &UGA_PlayerCombo::OnComboWindowClose,
        CloseDelay,
        false
    );

    // 等待伤害开始/结束事件
    UAbilityTask_WaitGameplayEvent* WaitDamageStart = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this, FGameplayTag::RequestGameplayTag(FName("Event.Attack.Start")), nullptr, true, true);
    WaitDamageStart->EventReceived.AddDynamic(this, &UGA_PlayerCombo::OnAttackDamageStart);
    WaitDamageStart->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* WaitDamageEnd = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this, FGameplayTag::RequestGameplayTag(FName("Event.Attack.End")), nullptr, true, true);
    WaitDamageEnd->EventReceived.AddDynamic(this, &UGA_PlayerCombo::OnAttackDamageEnd);
    WaitDamageEnd->ReadyForActivation();
}

void UGA_PlayerCombo::OnComboWindowOpen()
{
    bComboWindowOpen = true;

    // 开始监听连招输入
    ComboWaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this,
        FGameplayTag::RequestGameplayTag(FName("Event.Player.ComboInput")),
        nullptr,
        true,
        true
    );
    ComboWaitTask->EventReceived.AddDynamic(this, &UGA_PlayerCombo::OnComboInputReceived);
    ComboWaitTask->ReadyForActivation();
}

void UGA_PlayerCombo::OnComboWindowClose()
{
    // 只有当没有待处理的下一段且蒙太奇已停止时才结束连招
    if (!bPendingNextCombo && !bMontagePlaying)
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}

void UGA_PlayerCombo::OnComboInputReceived(FGameplayEventData Payload)
{
    if (!bComboWindowOpen) return;

    // 清理定时器和监听任务
    GetWorld()->GetTimerManager().ClearTimer(ComboWindowOpenTimer);
    GetWorld()->GetTimerManager().ClearTimer(ComboWindowCloseTimer);
    bComboWindowOpen = false;

    if (ComboWaitTask && ComboWaitTask->IsActive())
    {
        ComboWaitTask->EndTask();
    }
    ComboWaitTask = nullptr;

    if (bMontagePlaying)
    {
        // 当前段还在播放，设置 pending，等蒙太奇完成后再进入下一段
        bPendingNextCombo = true;
    }
    else
    {
        // 当前段已经结束，处于后摇窗口，立即进入下一段
        CurrentComboIndex = (CurrentComboIndex + 1) % 4;
        PlayComboSection(CurrentComboIndex);
    }
}

void UGA_PlayerCombo::OnMontageCompleted()
{
    bMontagePlaying = false;

    if (bPendingNextCombo)
    {
        bPendingNextCombo = false;
        CurrentComboIndex = (CurrentComboIndex + 1) % 4;
        PlayComboSection(CurrentComboIndex);
    }
    // 否则不做任何事，等待 CloseTimer 触发结束连招
}

void UGA_PlayerCombo::OnMontageInterrupted()
{
    // 被外部打断，直接结束
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_PlayerCombo::OnAttackDamageStart(FGameplayEventData Payload)
{
    bIsDamageWindowActive = true;
    DamagedTargets.Empty();
    GetWorld()->GetTimerManager().SetTimer(
        DamageTraceTimer,
        this,
        &UGA_PlayerCombo::PerformDamageTrace,
        0.1f,
        true
    );
}

void UGA_PlayerCombo::OnAttackDamageEnd(FGameplayEventData Payload)
{
    bIsDamageWindowActive = false;
    GetWorld()->GetTimerManager().ClearTimer(DamageTraceTimer);
}

void UGA_PlayerCombo::PerformDamageTrace()
{
    if (!bIsDamageWindowActive) return;

    AActor* Avatar = GetAvatarActorFromActorInfo();
    if (!Avatar) return;

    FVector TraceLocation = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * DamageOffset.X
        + Avatar->GetActorRightVector() * DamageOffset.Y
        + FVector(0, 0, DamageOffset.Z);

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Avatar);

    bool bHit = GetWorld()->OverlapMultiByObjectType(
        Overlaps,
        TraceLocation,
        FQuat::Identity,
        FCollisionObjectQueryParams(ECC_Pawn),
        FCollisionShape::MakeSphere(DamageRadius),
        Params
    );

    if (bHit)
    {
        for (const FOverlapResult& Result : Overlaps)
        {
            AActor* Victim = Result.GetActor();
            if (!Victim || DamagedTargets.Contains(Victim)) continue;

            IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Victim);
            if (ASCInterface)
            {
                UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent();
                if (TargetASC && DamageEffectClass)
                {
                    FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
                    GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectToTarget(
                        DamageEffectClass->GetDefaultObject<UGameplayEffect>(),
                        TargetASC,
                        1.0f,
                        EffectContext
                    );
                    DamagedTargets.Add(Victim);

                    // ===== 命中位置计算 =====
                    // 注意：这里是球形 Overlap 检测，没有物理意义上的命中点（无 HitResult）。
                    // 用"目标胶囊表面朝向攻击者的点"近似接触位置（砍在目标身上的位置）：
                    const FVector ToVictim = Victim->GetActorLocation() - Avatar->GetActorLocation();
                    const FVector AttackDir = ToVictim.GetSafeNormal2D();

                    float VictimRadius = 0.0f;
                    if (const ACharacter* VictimChar = Cast<ACharacter>(Victim))
                    {
                        VictimRadius = VictimChar->GetCapsuleComponent()->GetScaledCapsuleRadius();
                    }
                    // 接触点：目标中心向攻击者方向回退一个半径（接近胶囊表面），带最小偏移防止与中心重合
                    const FVector HitPoint = Victim->GetActorLocation() - AttackDir * FMath::Max(VictimRadius * 0.8f, 30.0f);
                    const FVector HitNormal = -AttackDir; // 命中法线指向攻击者

                    // ===== 命中 GameplayCue（特效 + 音效）=====
                    // 特效（Niagara）与音效资产都在 GameplayCueNotify 蓝图里配置，代码只发事件。
                    if (HitCueTag.IsValid() && TargetASC)
                    {
                        FGameplayCueParameters CueParams;
                        CueParams.Location = HitPoint;
                        CueParams.Normal = HitNormal;
                        CueParams.Instigator = Avatar;
                        CueParams.EffectCauser = Avatar;
                        TargetASC->ExecuteGameplayCue(HitCueTag, CueParams);

                        UE_LOG(LogTemp, Log, TEXT("[Combo] HitCue %s executed at %s (target %s)"),
                            *HitCueTag.ToString(), *HitPoint.ToString(), *GetNameSafe(Victim));
                    }
                }
            }
        }
    }
}

void UGA_PlayerCombo::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ComboWindowOpenTimer);
        GetWorld()->GetTimerManager().ClearTimer(ComboWindowCloseTimer);
        GetWorld()->GetTimerManager().ClearTimer(DamageTraceTimer);
    }

    if (ComboWaitTask && ComboWaitTask->IsActive())
    {
        ComboWaitTask->EndTask();
    }
    ComboWaitTask = nullptr;

    CurrentComboIndex = 0;
    bComboWindowOpen = false;
    bMontagePlaying = false;
    bPendingNextCombo = false;
    bIsDamageWindowActive = false;
    DamagedTargets.Empty();

    UGameplayAbility::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}