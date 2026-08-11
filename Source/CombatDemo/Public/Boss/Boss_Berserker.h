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

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintCallable, Category = "Boss|GAS")
    UBossAttributeSet* GetBossAttributeSet() const;

    void OnPhaseChanged();
    void HandlePhaseTransition();
    void OnDeath();

    UFUNCTION(BlueprintCallable, Category = "Boss")
    void SetMovementSpeedForPhase(int32 NewPhase);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Components")
    UStaticMeshComponent* PillarMesh;

    UPROPERTY(EditDefaultsOnly, Category = "Boss|Abilities")
    TArray<TSubclassOf<UBossGameplayAbility>> InitialAbilities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Movement")
    float Phase1WalkSpeed = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Movement")
    float Phase2RunSpeed = 500.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Boss|State")
    bool bIsAttacking = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|GAS")
    UAbilitySystemComponent* AbilitySystemComponent;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    UBossAttributeSet* BossAttributeSet;

    bool bPhaseTransitioned = false;
    bool bIsDead = false;

	// 用于绑定属性变化回调
	void BindAttributeChangeCallbacks();
	// 修正声明，类型为 const FOnAttributeChangeData&
	virtual void HealthChanged(const FOnAttributeChangeData& Data);

};