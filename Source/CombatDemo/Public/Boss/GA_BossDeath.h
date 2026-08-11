#pragma once

#include "CoreMinimal.h"
#include "Boss/BossGameplayAbility.h"
#include "GA_BossDeath.generated.h"

UCLASS()
class COMBATDEMO_API UGA_BossDeath : public UBossGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_BossDeath();

    /** À¿Õˆ√…Ã´∆Ê */
    UPROPERTY(EditDefaultsOnly, Category = "Death")
    UAnimMontage* DeathMontage;
protected:
protected:
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

private:
    bool bHasDied = false;
};