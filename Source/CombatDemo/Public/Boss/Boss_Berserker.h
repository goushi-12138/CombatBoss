#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h" // 确保包含了 FOnAttributeChangeData 的定义
#include "Boss_Berserker.generated.h"

class UAbilitySystemComponent;
class UBossAttributeSet;
class UStaticMeshComponent;
class UBossGameplayAbility;

UCLASS()
class COMBATDEMO_API ABoss_Berserker : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABoss_Berserker();

	// GAS 接口
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 属性集获取
	UFUNCTION(BlueprintCallable, Category = "Boss|GAS")
	UBossAttributeSet* GetBossAttributeSet() const;

	// 阶段切换回调，由属性集触发，通知AI控制器
	void OnPhaseChanged();

	// 柱子的静态网格体组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
	UStaticMeshComponent* PillarMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Abilities")
	TArray<TSubclassOf<UBossGameplayAbility>> InitialAbilities;

	// 一阶段和二阶段的移动速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Movement")
	float Phase1WalkSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Movement")
	float Phase2RunSpeed = 500.0f;

	// 供属性集调用的阶段切换处理
	void HandlePhaseTransition();

	// 供蓝图调用的移动速度切换
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void SetMovementSpeedForPhase(int32 NewPhase);

	// 是否正在执行攻击
	UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
	bool bIsAttacking = false;
protected:
	virtual void BeginPlay() override;

	// 当前是否已经执行过转阶段（防止重复触发）
	bool bPhaseTransitioned = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|GAS")
	UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY()
	UBossAttributeSet* BossAttributeSet;

	// 用于绑定属性变化回调
	void BindAttributeChangeCallbacks();
	// 修正声明，类型为 const FOnAttributeChangeData&
	virtual void HealthChanged(const FOnAttributeChangeData& Data);

};