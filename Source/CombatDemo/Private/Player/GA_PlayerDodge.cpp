#include "Player/GA_PlayerDodge.h"
#include "CombatDemoCharacter.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
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

void UGA_PlayerDodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
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

    // 计算方向在视角前、右两个轴上的投影
    float ForwardAmount = FVector::DotProduct(Direction, CamForward);
    float RightAmount = FVector::DotProduct(Direction, CamRight);

    UAnimMontage* MontageToPlay = nullptr;

    if (FMath::Abs(ForwardAmount) > 0.5f)
    {
        // 前后方向占主导
        MontageToPlay = (ForwardAmount > 0.0f) ? DodgeForward : DodgeBackward;
    }
    else
    {
        // 左右方向占主导
        MontageToPlay = (RightAmount > 0.0f) ? DodgeRight : DodgeLeft;
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
}

void UGA_PlayerDodge::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PlayerDodge::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
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