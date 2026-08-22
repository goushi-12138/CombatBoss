#include "Player/GA_PlayerBlock.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "CombatDemoCharacter.h"
#include "Components/StaticMeshComponent.h"

UGA_PlayerBlock::UGA_PlayerBlock()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    FGameplayTagContainer BlockingTag;
    BlockingTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Block")));
    SetAssetTags(BlockingTag);

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

    // 【盾牌格挡音效】监听 Boss 的 Event.Player.BlockHit 事件（Boss 命中防御中的玩家时发送）
    UAbilityTask_WaitGameplayEvent* WaitBlockHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this, FGameplayTag::RequestGameplayTag(FName("Event.Player.BlockHit")));
    if (WaitBlockHit)
    {
        WaitBlockHit->EventReceived.AddDynamic(this, &UGA_PlayerBlock::OnBlockHitEvent);
        WaitBlockHit->ReadyForActivation();
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

void UGA_PlayerBlock::OnBlockHitEvent(FGameplayEventData Payload)
{
    if (!BlockHitSound)
    {
        return;
    }

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character)
    {
        return;
    }

    // 优先挂到玩家盾牌组件（ShieldMesh），声音从盾牌位置发出；找不到则挂到网格体
    USceneComponent* AttachTarget = Character->GetMesh();
    if (ACombatDemoCharacter* PlayerChar = Cast<ACombatDemoCharacter>(Character))
    {
        if (PlayerChar->ShieldMesh)
        {
            AttachTarget = PlayerChar->ShieldMesh;
        }
    }

    if (AttachTarget)
    {
        UGameplayStatics::SpawnSoundAttached(
            BlockHitSound,
            AttachTarget,
            NAME_None,
            FVector::ZeroVector,
            EAttachLocation::SnapToTarget,
            true);
        UE_LOG(LogTemp, Log, TEXT("[Block] BlockHit sound played at %s"), *AttachTarget->GetName());
    }
}