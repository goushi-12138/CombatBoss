#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "PlayerAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class COMBATDEMO_API UPlayerAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UPlayerAttributeSet();

    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, Health);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MaxHealth);

    // ===== 精力（翻滚消耗，每秒恢复）=====
    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData Stamina;
    ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, Stamina);

    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData MaxStamina;
    ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MaxStamina);

    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
    /** 判断攻击是否来自玩家前方（用于盾牌格挡减伤：Boss 位于玩家前方 ±75° 内才算格挡） */
    bool IsAttackFromFront() const;
};
