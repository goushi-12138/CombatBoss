#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BossAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class COMBATDEMO_API UBossAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UBossAttributeSet();

	// 当前血量
	UPROPERTY(BlueprintReadOnly, Category = "Boss|Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UBossAttributeSet, Health)

		// 最大血量
		UPROPERTY(BlueprintReadOnly, Category = "Boss|Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UBossAttributeSet, MaxHealth)

		// 阶段（1 或 2）
		UPROPERTY(BlueprintReadOnly, Category = "Boss|Attributes")
	FGameplayAttributeData Phase;
	ATTRIBUTE_ACCESSORS(UBossAttributeSet, Phase)

		// 当属性值变化时调用，用于处理阶段切换
		virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};
