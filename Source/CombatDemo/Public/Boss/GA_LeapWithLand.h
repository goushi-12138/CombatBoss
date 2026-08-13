#pragma once

#include "CoreMinimal.h"
#include "Boss/BossGameplayAbility.h"
#include "GA_LeapWithLand.generated.h"

class UAbilitySystemComponent;

UCLASS()
class COMBATDEMO_API UGA_LeapWithLand : public UBossGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_LeapWithLand();

    /** 要播放的跳跃下劈蒙太奇（带根运动） */
    UPROPERTY(EditDefaultsOnly, Category = "Leap")
    UAnimMontage* LeapMontage;

    /** 落地后AOE半径 */
    UPROPERTY(EditDefaultsOnly, Category = "Leap")
    float LandRadius = 250.0f;

    /** 落地直接伤害GE */
    UPROPERTY(EditDefaultsOnly, Category = "Leap|Damage")
    TSubclassOf<UGameplayEffect> DirectDamageEffect;

    /** 落地击倒GE（保留属性兼容旧配置；本受击系统不再应用——改为播放倒地蒙太奇，无"倒地不能动"硬控） */
    UPROPERTY(EditDefaultsOnly, Category = "Leap|Damage")
    TSubclassOf<UGameplayEffect> StunEffect;
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
    /** 落地事件回调 */
    UFUNCTION()
    void OnLandEvent(FGameplayEventData EventData);

    /** 蒙太奇结束回调 */
    UFUNCTION()
    void OnMontageFinished();

    void OnMontageInterrupted();

    /** 执行落地AOE */
    void ExecuteLandAOE();

    /** 向玩家发送倒地事件（正对Boss→DownFront向后倒；背对Boss→DownBack向前倒） */
    void SendDownReactEvent(UAbilitySystemComponent* TargetASC, AActor* Victim, AActor* BossAvatar);

    /** 是否已落地（防止重复触发） */
    bool bHasLanded = false;

    /** 是否已忽略碰撞 */
    bool bCollisionIgnored = false;
};
