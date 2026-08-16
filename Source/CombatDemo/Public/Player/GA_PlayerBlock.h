#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "GA_PlayerBlock.generated.h"

class USoundBase;

UCLASS()
class COMBATDEMO_API UGA_PlayerBlock : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_PlayerBlock();

    /** 盾牌防御姿势蒙太奇（循环） */
    UPROPERTY(EditDefaultsOnly, Category = "Block")
    UAnimMontage* BlockMontage;

    /** 盾牌被击中音效（Boss命中防御中的玩家时，从玩家盾牌组件位置播放） */
    UPROPERTY(EditDefaultsOnly, Category = "Block")
    USoundBase* BlockHitSound;

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
    /** 移动输入恢复标记 */
    bool bWasMovementIgnored = false;

    /** 收到 Boss 的 Event.Player.BlockHit 事件 → 在盾牌位置播放格挡音效 */
    UFUNCTION()
    void OnBlockHitEvent(FGameplayEventData Payload);
};