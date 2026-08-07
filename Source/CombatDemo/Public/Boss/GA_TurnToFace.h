#pragma once

#include "CoreMinimal.h"
#include "Boss/BossGameplayAbility.h"
#include "GA_TurnToFace.generated.h"

UCLASS()
class COMBATDEMO_API UGA_TurnToFace : public UBossGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_TurnToFace();

    UPROPERTY(EditDefaultsOnly, Category = "Turn")
    UAnimMontage* TurnLeft90;

    UPROPERTY(EditDefaultsOnly, Category = "Turn")
    UAnimMontage* TurnRight90;

    UPROPERTY(EditDefaultsOnly, Category = "Turn")
    UAnimMontage* TurnLeft180;

    UPROPERTY(EditDefaultsOnly, Category = "Turn")
    UAnimMontage* TurnRight180;

protected:
    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    UFUNCTION()
    void OnTurnMontageCompleted();

    UFUNCTION()
    void OnTurnMontageInterrupted();
};