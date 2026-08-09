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

    // Ò»½×¶Î×ªÉíÃÉÌ«Ææ
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase1")
    UAnimMontage* TurnLeft45;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase1")
    UAnimMontage* TurnRight45;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase1")
    UAnimMontage* TurnLeft90;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase1")
    UAnimMontage* TurnRight90;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase1")
    UAnimMontage* TurnLeft135;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase1")
    UAnimMontage* TurnRight135;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase1")
    UAnimMontage* TurnLeft180;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase1")
    UAnimMontage* TurnRight180;

    // ¶þ½×¶Î×ªÉíÃÉÌ«Ææ
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase2")
    UAnimMontage* TurnLeft45_P2;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase2")
    UAnimMontage* TurnRight45_P2;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase2")
    UAnimMontage* TurnLeft90_P2;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase2")
    UAnimMontage* TurnRight90_P2;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase2")
    UAnimMontage* TurnLeft135_P2;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase2")
    UAnimMontage* TurnRight135_P2;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase2")
    UAnimMontage* TurnLeft180_P2;
    UPROPERTY(EditDefaultsOnly, Category = "Turn|Phase2")
    UAnimMontage* TurnRight180_P2;
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