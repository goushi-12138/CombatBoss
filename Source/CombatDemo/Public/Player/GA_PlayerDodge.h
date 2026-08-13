#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_PlayerDodge.generated.h"

class UAbilityTask_PlayMontageAndWait;

UCLASS()
class COMBATDEMO_API UGA_PlayerDodge : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_PlayerDodge();

    // 四个方向翻滚蒙太奇（可在蓝图中指定）
    UPROPERTY(EditDefaultsOnly, Category = "Dodge")
    UAnimMontage* DodgeForward;

    UPROPERTY(EditDefaultsOnly, Category = "Dodge")
    UAnimMontage* DodgeBackward;

    UPROPERTY(EditDefaultsOnly, Category = "Dodge")
    UAnimMontage* DodgeLeft;

    UPROPERTY(EditDefaultsOnly, Category = "Dodge")
    UAnimMontage* DodgeRight;

    // 翻滚无敌标签，翻滚期间添加
    FGameplayTag InvulnerableTag;

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
    UFUNCTION()
    void OnMontageCompleted();

    UAbilityTask_PlayMontageAndWait* ActiveMontageTask;
};