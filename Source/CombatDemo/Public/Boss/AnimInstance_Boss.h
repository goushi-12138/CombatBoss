// AnimInstance_Boss.h

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "AnimInstance_Boss.generated.h"

class UAbilitySystemComponent;

UCLASS()
class COMBATDEMO_API UAnimInstance_Boss : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    // 移动状态变量
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    float Speed = 0.0f;

    // 当前是否在一阶段（用于切换 Blend Space）
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    bool bIsPhaseOne = true;

    // 【新增】是否正在播放转阶段蒙太奇（用于锁定阶段）
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    bool bIsPhaseTransitioning = false;
private:
    UPROPERTY()
    UAbilitySystemComponent* AbilitySystemComponent;

    // 缓存标记
    FGameplayTag PhaseTransitionTag;
};