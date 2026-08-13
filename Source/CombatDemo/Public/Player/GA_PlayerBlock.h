#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_PlayerBlock.generated.h"

UCLASS()
class COMBATDEMO_API UGA_PlayerBlock : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_PlayerBlock();

    /** ∂‹≈∆∑¿”˘◊À ∆√…Ã´∆Ê£®—≠ª∑£© */
    UPROPERTY(EditDefaultsOnly, Category = "Block")
    UAnimMontage* BlockMontage;

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

private:
    /** “∆∂Ø ‰»Îª÷∏¥±Íº« */
    bool bWasMovementIgnored = false;
};