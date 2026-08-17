#include "Player/GA_PlayerDodge.h"
#include "CombatDemoCharacter.h"
#include "Player/PlayerAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "TimerManager.h"
#include <Kismet\GameplayStatics.h>

UGA_PlayerDodge::UGA_PlayerDodge()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Dodge")));
    SetAssetTags(TagContainer);

    // 翻滚时取消攻击和防御
    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Combo")));
    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Block")));

    // 翻滚期间禁止攻击、防御、再次翻滚
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Combo")));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Block")));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Dodge")));

    InvulnerableTag = FGameplayTag::RequestGameplayTag(FName("Status.Dodging"));
}

bool UGA_PlayerDodge::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    // 精力不足时无法翻滚
    if (const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
    {
        const float Stamina = ASC->GetNumericAttribute(UPlayerAttributeSet::GetStaminaAttribute());
        if (Stamina < StaminaCost)
        {
            UE_LOG(LogTemp, Log, TEXT("[Dodge] Not enough stamina: %.1f < %.1f"), Stamina, StaminaCost);
            return false;
        }
    }

    return true;
}

void UGA_PlayerDodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    // 每次激活重置防重入标志（保证任何提前 return 路径下 EndAbility 都能正常执行清理）
    bIsEnding = false;

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ACharacter* Player = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Player)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 添加无敌标签
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->AddLooseGameplayTag(InvulnerableTag);

        // 扣除翻滚精力（每次翻滚消耗 StaminaCost）
        const float Stamina = ASC->GetNumericAttribute(UPlayerAttributeSet::GetStaminaAttribute());
        ASC->SetNumericAttributeBase(UPlayerAttributeSet::GetStaminaAttribute(), FMath::Max(0.0f, Stamina - StaminaCost));
    }

    // 读取角色上存储的翻滚方向（由 CombatDemoCharacter 设置）
    FVector Direction = FVector::ZeroVector;
    if (ACombatDemoCharacter* MyChar = Cast<ACombatDemoCharacter>(Player))
    {
        Direction = MyChar->DodgeDirection;
    }

    // 获取玩家控制器，用于获取视角方向
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 获取视角的水平朝向
    FRotator ControlRotation = PC->GetControlRotation();
    FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
    FVector CamForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    FVector CamRight = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    // 如果没有方向输入，默认使用视角前方（前翻滚）
    if (Direction.IsNearlyZero())
    {
        Direction = CamForward;
    }

    // ===== 关键修复：动画选择基于"翻滚方向 相对 人物面朝方向"（永劫式闪避）=====
    // Direction 是摄像机基准方向（W=摄像机前, S=后, D=右, A=左）。
    // 但"前滚/后滚/左滚/右滚"动画必须以人物朝向为准：
    // 人物面朝后、W(摄像机前)翻滚时，翻滚方向其实是人物身后 → 应播"向后翻滚"动画，
    // 这样根运动才朝人物后 = 摄像机前（玩家看的方向）。
    const FVector ActorForward = Player->GetActorForwardVector().GetSafeNormal2D();
    const FVector ActorRight = Player->GetActorRightVector().GetSafeNormal2D();

    const float ForwardAmount = FVector::DotProduct(Direction, ActorForward);
    const float RightAmount = FVector::DotProduct(Direction, ActorRight);

    UAnimMontage* MontageToPlay = nullptr;

    if (FMath::Abs(ForwardAmount) >= FMath::Abs(RightAmount))
    {
        // 前后方向占主导
        MontageToPlay = (ForwardAmount >= 0.0f) ? DodgeForward : DodgeBackward;
    }
    else
    {
        // 左右方向占主导
        MontageToPlay = (RightAmount >= 0.0f) ? DodgeRight : DodgeLeft;
    }

    if (!MontageToPlay)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, MontageToPlay, 1.0f, NAME_None, true); // true 应用根运动
    ActiveMontageTask->OnCompleted.AddDynamic(this, &UGA_PlayerDodge::OnMontageCompleted);
    ActiveMontageTask->OnInterrupted.AddDynamic(this, &UGA_PlayerDodge::OnMontageCompleted);
    ActiveMontageTask->OnCancelled.AddDynamic(this, &UGA_PlayerDodge::OnMontageCompleted);
    ActiveMontageTask->ReadyForActivation();

    // 【超时兜底】对障碍物翻滚被碰撞卡住、或蒙太奇循环导致回调不触发时，
    // 超时强制结束技能，避免 GA 永久卡在激活状态（→ 之后翻滚全部失效）。
    const float MontageLen = MontageToPlay->GetPlayLength();
    const float Timeout = FMath::Max(MontageLen + 0.5f, 1.5f);
    GetWorld()->GetTimerManager().SetTimer(
        DodgeTimeoutHandle, this, &UGA_PlayerDodge::ForceEndDodge, Timeout, false);
}

void UGA_PlayerDodge::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PlayerDodge::ForceEndDodge()
{
    UE_LOG(LogTemp, Warning, TEXT("[Dodge] Timeout - forcing EndAbility to prevent stuck"));
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_PlayerDodge::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 防重入：避免 EndAbility 与蒙太奇回调（EndTask→OnCancelled→OnMontageCompleted）互相调用
    if (bIsEnding)
        return;
    bIsEnding = true;

    // 取消超时定时器
    if (DodgeTimeoutHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(DodgeTimeoutHandle);
    }

    if (ActiveMontageTask && ActiveMontageTask->IsActive())
    {
        ActiveMontageTask->EndTask();
    }
    ActiveMontageTask = nullptr;

    // 移除无敌标签
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveLooseGameplayTag(InvulnerableTag);
    }

    // 将 Super::EndAbility(...) 替换为 UGameplayAbility::EndAbility(...) 明确调用父类实现
    UGameplayAbility::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}