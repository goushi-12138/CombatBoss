#include "Boss/GA_PhaseTransition.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Boss/BossAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Boss/Boss_Berserker.h"

// 将 AbilityTags.AddTag 替换为 SetAssetTags，仅在构造函数中设置
UGA_PhaseTransition::UGA_PhaseTransition()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    // 正确方式：添加到 AbilityTags，才能被事件激活
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("Event.Boss.PhaseTransition")));
    SetAssetTags(TagContainer);
}

bool UGA_PhaseTransition::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    // 转阶段演出无条件允许激活
    return true;
}

void UGA_PhaseTransition::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    // 手动提交技能（不调用Super，避免基类播放其他蒙太奇）
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 添加转阶段标记，锁定ABP的一阶段姿势
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.PhaseTransition")));
    }

    // 标记攻击状态，防止行为树打断
    if (ABoss_Berserker* Boss = Cast<ABoss_Berserker>(GetAvatarActorFromActorInfo()))
    {
        Boss->bIsAttacking = true;
    }

    // 播放怒吼蒙太奇
    if (RoarMontage)
    {
        UE_LOG(LogTemp, Warning, TEXT("GA_PhaseTransition: Playing RoarMontage"));

        UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, RoarMontage);
        MontageTask->OnCompleted.AddDynamic(this, &UGA_PhaseTransition::OnRoarMontageCompleted);
        MontageTask->OnBlendOut.AddDynamic(this, &UGA_PhaseTransition::OnRoarMontageCompleted);
        MontageTask->OnInterrupted.AddDynamic(this, &UGA_PhaseTransition::OnRoarMontageCompleted);
        MontageTask->ReadyForActivation();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("GA_PhaseTransition: RoarMontage is NULL!"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void UGA_PhaseTransition::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 移除转阶段标记
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.PhaseTransition")));
    }

    // 清除攻击状态
    if (ABoss_Berserker* Boss = Cast<ABoss_Berserker>(GetAvatarActorFromActorInfo()))
    {
        Boss->bIsAttacking = false;

        // 设置黑板值：转阶段已完成
        if (ABossAIController* AIC = Cast<ABossAIController>(Boss->GetController()))
        {
            if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
            {
                BB->SetValueAsBool(FName("bPhaseTransitioned"), true);
            }
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_PhaseTransition::OnRoarMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}