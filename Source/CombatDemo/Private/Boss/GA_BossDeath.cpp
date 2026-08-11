#include "Boss/GA_BossDeath.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"

UGA_BossDeath::UGA_BossDeath()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Event.Boss.Death")));
    ActivationBlockedTags.Reset();
    CancelAbilitiesWithTag.Reset();
    BlockAbilitiesWithTag.Reset();
}

bool UGA_BossDeath::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    return true;
}

void UGA_BossDeath::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (bHasDied)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ACharacter* Boss = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Boss)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    bHasDied = true;
    
    // 立即停止移动和AI，防止死亡动画期间乱动
    Boss->GetCharacterMovement()->StopMovementImmediately();
    Boss->GetCharacterMovement()->DisableMovement();
    Boss->GetCharacterMovement()->SetMovementMode(MOVE_None);

    if (AAIController* AIC = Cast<AAIController>(Boss->GetController()))
    {
        if (AIC->GetBrainComponent())
        {
            AIC->BrainComponent->StopLogic(TEXT("Death"));
            AIC->BrainComponent->Cleanup();
        }
        AIC->UnPossess();
    }

    // 禁用Tick
    Boss->SetActorTickEnabled(false);
    
    // 3. 禁用所有碰撞（胶囊体和网格体都忽略玩家）
// 先关胶囊体
    if (UCapsuleComponent* Capsule = Boss->GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 再关网格体
    if (USkeletalMeshComponent* Mesh = Boss->GetMesh())
    {
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // 播放死亡蒙太奇
    if (DeathMontage)
    {
        // 让蒙太奇播完后不自动过渡回基础姿势
        DeathMontage->BlendOut.SetBlendTime(999.0f);

        UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, DeathMontage, 1.0f, NAME_None, true);
        MontageTask->ReadyForActivation();
    }
}

void UGA_BossDeath::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}