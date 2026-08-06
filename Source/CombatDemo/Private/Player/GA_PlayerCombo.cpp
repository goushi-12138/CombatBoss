#include "Player/GA_PlayerCombo.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystemInterface.h"

UGA_PlayerCombo::UGA_PlayerCombo()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    FGameplayTagContainer NewTags;
    NewTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Combo")));
    SetAssetTags(NewTags);
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

    // 从第一段开始
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

    // 清理上一次的伤害检测
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(DamageTraceTimer);
    }
    bIsDamageWindowActive = false;
    DamagedTargets.Empty();

    bIsPlayingMontage = true;

    FName TargetSection;
    switch (SectionIndex)
    {
    case 0: TargetSection = Section1; break;
    case 1: TargetSection = Section2; break;
    case 2: TargetSection = Section3; break;
    }

    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, ComboMontage, 1.0f, TargetSection);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_PlayerCombo::OnMontageBlendOut);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_PlayerCombo::OnMontageInterrupted);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_PlayerCombo::OnMontageInterrupted);
    MontageTask->ReadyForActivation();

    // 等待伤害开始和结束事件
    UAbilityTask_WaitGameplayEvent* WaitDamageStart = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this, FGameplayTag::RequestGameplayTag(FName("Event.Attack.Start")), nullptr, true, true);
    WaitDamageStart->EventReceived.AddDynamic(this, &UGA_PlayerCombo::OnAttackDamageStart);
    WaitDamageStart->ReadyForActivation();

    UAbilityTask_WaitGameplayEvent* WaitDamageEnd = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this, FGameplayTag::RequestGameplayTag(FName("Event.Attack.End")), nullptr, true, true);
    WaitDamageEnd->EventReceived.AddDynamic(this, &UGA_PlayerCombo::OnAttackDamageEnd);
    WaitDamageEnd->ReadyForActivation();

    UE_LOG(LogTemp, Warning, TEXT("Playing combo section: %s"), *TargetSection.ToString());
}

void UGA_PlayerCombo::OnMontageBlendOut()
{
    bIsPlayingMontage = false;
    // 开始等待连招输入
    StartWaitingForComboInput();
}

void UGA_PlayerCombo::OnMontageInterrupted()
{
    bIsPlayingMontage = false;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_PlayerCombo::StartWaitingForComboInput()
{
    // 设置超时定时器
    GetWorld()->GetTimerManager().SetTimer(
        ComboWindowTimer,
        this,
        &UGA_PlayerCombo::OnComboWindowTimeout,
        ComboWindow,
        false
    );

    // 等待连招输入事件
    UAbilityTask_WaitGameplayEvent* WaitEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this,
        FGameplayTag::RequestGameplayTag(FName("Event.Player.ComboInput")),
        nullptr,
        true,  // 只触发一次
        true   // 精确匹配
    );
    WaitEvent->EventReceived.AddDynamic(this, &UGA_PlayerCombo::OnComboInputReceived);
    WaitEvent->ReadyForActivation();

    UE_LOG(LogTemp, Warning, TEXT("Combo window open, waiting for input..."));
}

void UGA_PlayerCombo::OnComboInputReceived(FGameplayEventData Payload)
{
    // 取消超时定时器
    GetWorld()->GetTimerManager().ClearTimer(ComboWindowTimer);

    // 进入下一段连招
    CurrentComboIndex = (CurrentComboIndex + 1) % 3;
    PlayComboSection(CurrentComboIndex);

    UE_LOG(LogTemp, Warning, TEXT("Combo input received! Next: %d"), CurrentComboIndex);
}

void UGA_PlayerCombo::OnComboWindowTimeout()
{
    UE_LOG(LogTemp, Warning, TEXT("Combo window expired, ending combo."));
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PlayerCombo::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ComboWindowTimer);
        GetWorld()->GetTimerManager().ClearTimer(DamageTraceTimer);
    }
    CurrentComboIndex = 0;
    bIsPlayingMontage = false;
    bIsDamageWindowActive = false;
    DamagedTargets.Empty();
    UGameplayAbility::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_PlayerCombo::OnAttackDamageStart(FGameplayEventData Payload)
{
    bIsDamageWindowActive = true;
    DamagedTargets.Empty();
    // 开启每0.1秒一次的伤害检测
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

    // 计算检测位置：角色前方偏移
    FVector TraceLocation = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * DamageOffset.X
        + Avatar->GetActorRightVector() * DamageOffset.Y
        + FVector(0, 0, DamageOffset.Z);

    // 球形检测
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

            // 检查是否是Boss（或拥有AbilitySystemComponent的敌人）
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
                    UE_LOG(LogTemp, Warning, TEXT("Damage applied to %s"), *Victim->GetName());
                }
            }
        }
    }
}