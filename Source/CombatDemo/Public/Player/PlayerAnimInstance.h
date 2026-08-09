#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GameplayTagContainer.h"
#include "PlayerAnimInstance.generated.h"

class UAbilitySystemComponent;

UCLASS()
class COMBATDEMO_API UPlayerAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    bool bIsStunned = false;

    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    float Speed = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Animation")
    bool bIsInAir = false;

private:
    UPROPERTY()
    UAbilitySystemComponent* AbilitySystemComponent;

    FGameplayTag StunnedTag;
};