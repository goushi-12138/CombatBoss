#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_PlayerCombo.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

UCLASS()
class COMBATDEMO_API UGA_PlayerCombo : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_PlayerCombo();

    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    UAnimMontage* ComboMontage;

    // 上一段结束前多久开始接受输入
    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    float PreInputWindow = 0.5f;

    // 上一段结束后多久内接受输入
    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    float PostInputWindow = 0.3f;

    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    FName Section1 = FName("Attack1");
    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    FName Section2 = FName("Attack2");
    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    FName Section3 = FName("Attack3");
    UPROPERTY(EditDefaultsOnly, Category = "Combo")
    FName Section4 = FName("Attack4");

    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    float DamageRadius = 200.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    FVector DamageOffset = FVector(100.0f, 0.0f, 0.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Damage")
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    // ===== 命中特效/音效（GameplayCue）=====
    // 攻击命中目标时，在目标身上执行该 GameplayCue。
    // 特效（Niagara）与打击音效由 GameplayCueNotify_Static 蓝图配置，代码不直接引用资产。
    UPROPERTY(EditDefaultsOnly, Category = "HitFX")
    FGameplayTag HitCueTag;

    UPROPERTY()
    TArray<AActor*> DamagedTargets;

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
    bool bComboWindowOpen = false;
    bool bMontagePlaying = false;
    bool bPendingNextCombo = false;

    FTimerHandle ComboWindowOpenTimer;
    FTimerHandle ComboWindowCloseTimer;

    UAbilityTask_WaitGameplayEvent* ComboWaitTask = nullptr;

    void PlayComboSection(int32 SectionIndex);

    UFUNCTION()
    void OnComboWindowOpen();
    UFUNCTION()
    void OnComboWindowClose();

    UFUNCTION()
    void OnComboInputReceived(FGameplayEventData Payload);

    // 蒙太奇完成/中断回调
    UFUNCTION()
    void OnMontageCompleted();
    UFUNCTION()
    void OnMontageInterrupted();

    // 伤害判定
    UFUNCTION()
    void OnAttackDamageStart(FGameplayEventData Payload);
    UFUNCTION()
    void OnAttackDamageEnd(FGameplayEventData Payload);
    void PerformDamageTrace();

    FTimerHandle DamageTraceTimer;
    bool bIsDamageWindowActive = false;
};