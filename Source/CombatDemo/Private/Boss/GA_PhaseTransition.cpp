#include "Boss/GA_PhaseTransition.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Boss/BossAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Boss/Boss_Berserker.h"

// 将 AbilityTags.AddTag 替换为 SetAssetTags，仅在构造函数中设置
UGA_PhaseTransition::UGA_PhaseTransition()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    // 设置默认的 AssetTags
    FGameplayTagContainer DefaultTags;
    DefaultTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Event.Boss.PhaseTransition")));
    SetAssetTags(DefaultTags);
}

void UGA_PhaseTransition::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (RoarMontage)
    {
        UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, RoarMontage);
        MontageTask->OnCompleted.AddDynamic(this, &UGA_PhaseTransition::OnMontageCompleted);
        MontageTask->OnBlendOut.AddDynamic(this, &UGA_PhaseTransition::OnMontageCompleted);
        MontageTask->OnInterrupted.AddDynamic(this, &UGA_PhaseTransition::OnMontageInterrupted);
        MontageTask->ReadyForActivation();
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
}

void UGA_PhaseTransition::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 解除 Boss 的攻击状态
    if (AActor* Avatar = GetAvatarActorFromActorInfo())
    {
        if (ABoss_Berserker* Boss = Cast<ABoss_Berserker>(Avatar))
        {
            Boss->bIsAttacking = false;

            // 获取 AI 控制器，设置黑板值
            if (ABossAIController* AIC = Cast<ABossAIController>(Boss->GetController()))
            {
                if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
                {
                    BB->SetValueAsBool(FName("bPhaseTransitioned"), true);
                }
            }
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}