#include "Player/GA_PlayerBlock.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

UGA_PlayerBlock::UGA_PlayerBlock()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Block")));

    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Dodge")));

    // 防御时阻止攻击
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Combo")));

    // 添加防御状态标签
    FGameplayTagContainer BlockTag;
    BlockTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Status.Blocking")));
    SetAssetTags(BlockTag); // 或者用 ActivationOwnedTags，下面手动添加更灵活
}

void UGA_PlayerBlock::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 添加防御状态标签
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (ASC)
    {
        ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.Blocking")));
    }

    // 禁用移动输入
    APlayerController* PC = Cast<APlayerController>(Character->GetController());
    if (PC)
    {
        // 记录当前状态
        bWasMovementIgnored = PC->IsMoveInputIgnored();
        PC->SetIgnoreMoveInput(true);
    }

    // 停止移动
    Character->GetCharacterMovement()->StopMovementImmediately();

    // 播放防御蒙太奇（循环）
    if (BlockMontage)
    {
        UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, BlockMontage, 1.0f, NAME_None, true);
        MontageTask->ReadyForActivation(); // 不绑定结束回调，按住期间持续播放
    }
}

void UGA_PlayerBlock::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 移除防御状态标签
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    if (ASC)
    {
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.Blocking")));
    }

    // 恢复移动输入
    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (Character)
    {
        APlayerController* PC = Cast<APlayerController>(Character->GetController());
        if (PC)
        {
            PC->SetIgnoreMoveInput(bWasMovementIgnored);
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}