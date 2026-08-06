#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_PlayerCombo.generated.h"

UCLASS()
class COMBATDEMO_API UGA_PlayerCombo : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_PlayerCombo();

    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    UAnimMontage* ComboMontage;

    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    float ComboWindow = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    FName Section1 = FName("Attack1");

    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    FName Section2 = FName("Attack2");

    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    FName Section3 = FName("Attack3");

    // 伤害检测参数
    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    float DamageRadius = 200.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    FVector DamageOffset = FVector(100.0f, 0.0f, 0.0f);

    // 伤害GE类
    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    // 已伤害目标列表
    UPROPERTY()
    TArray<AActor*> DamagedTargets;

    // 伤害检测定时器
    FTimerHandle DamageTraceTimer;
    bool bIsDamageWindowActive = false;

    void PerformDamageTrace();

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
    int32 CurrentComboIndex = 0;
    bool bIsPlayingMontage = false;

    void PlayComboSection(int32 SectionIndex);

    UFUNCTION()
    void OnMontageBlendOut();

    UFUNCTION()
    void OnMontageInterrupted();

    void StartWaitingForComboInput();

    UFUNCTION()
    void OnComboInputReceived(FGameplayEventData Payload);

    UFUNCTION()
    void OnComboWindowTimeout();

    FTimerHandle ComboWindowTimer;

    // 伤害事件
    UFUNCTION()
    void OnAttackDamageStart(FGameplayEventData Payload);

    UFUNCTION()
    void OnAttackDamageEnd(FGameplayEventData Payload);
};