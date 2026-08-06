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

    // ×ª½×¶Î×¨ÓÃÃÉÌ«Ææ
    UPROPERTY(EditDefaultsOnly, Category = "PhaseTransition")
    UAnimMontage* RoarMontage;

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;
};