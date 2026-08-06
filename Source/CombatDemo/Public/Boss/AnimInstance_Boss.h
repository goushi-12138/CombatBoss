#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimInstance_Boss.generated.h"

UCLASS()
class COMBATDEMO_API UAnimInstance_Boss : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    // 暴露给动画蓝图使用的速度变量
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    float Speed = 0.0f;

    // 暴露方向变量（用于转身混合）
    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    float Direction = 0.0f;
private:
    float PreviousYaw = 0.0f;
    bool bHasPreviousYaw = false;

public:
    UFUNCTION(BlueprintCallable)
    void DebugSetDirection(float InDirection) { Direction = InDirection; }

    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    bool bIsPhaseOne = true;

};