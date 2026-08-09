#pragma once

#include "CoreMinimal.h"
#include "Boss/BossGameplayAbility.h"
#include "GA_PhaseTransition.generated.h"

UCLASS()
class COMBATDEMO_API UGA_PhaseTransition : public UBossGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_PhaseTransition();

    // 转阶段专用蒙太奇
    UPROPERTY(EditDefaultsOnly, Category = "PhaseTransition")
    UAnimMontage* RoarMontage;

protected:
    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags,
        const FGameplayTagContainer* TargetTags,
        FGameplayTagContainer* OptionalRelevantTags) const override;
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

    // 播放完毕后回调
    UFUNCTION()
    void OnRoarMontageCompleted();
};