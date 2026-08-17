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

    // 每次翻滚消耗的精力（默认20，精力不足时无法翻滚）
    UPROPERTY(EditDefaultsOnly, Category = "Dodge")
    float StaminaCost = 20.0f;

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
    UFUNCTION()
    void OnMontageCompleted();

    UAbilityTask_PlayMontageAndWait* ActiveMontageTask;

    /** 翻滚超时兜底定时器：碰撞卡住/蒙太奇循环导致回调不触发时，超时强制结束技能，
        避免 GA 永久卡在激活状态 → 之后翻滚全部失效 */
    FTimerHandle DodgeTimeoutHandle;

    /** 超时强制结束翻滚 */
    void ForceEndDodge();

    /** 防重入：避免 EndAbility 与蒙太奇回调互相调用造成栈溢出 */
    bool bIsEnding = false;
};