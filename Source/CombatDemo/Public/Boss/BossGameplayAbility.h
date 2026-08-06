#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "BossGameplayAbility.generated.h"

UCLASS()
class COMBATDEMO_API UBossGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UBossGameplayAbility();

	// 技能 ID（对应黑板中的 AbilityID）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossGA")
	FName SkillID;

	// 距离条件
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossGA|Conditions")
	float MinDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossGA|Conditions")
	float MaxDistance = 0.0f;

	// 冷却 GameplayEffect（可选）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossGA|Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	// 要播放的蒙太奇
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossGA|Animation")
	UAnimMontage* MontageToPlay;

	// 伤害 GameplayEffect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossGA|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled) override;

	// 【重要修正！】必须加上 UFUNCTION() 宏，且补全之前漏掉的函数
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

protected:
	// 设置Boss与玩家之间的碰撞（忽略或恢复）
	UFUNCTION(BlueprintCallable, Category = "BossGA")
	void SetIgnorePlayerCollision(bool bIgnore);
private:
	// 用于伤害判定的延迟和事件响应（同样需要补上 UFUNCTION 以保证委托可以绑定）
	UFUNCTION()
	void OnAttackStartEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnAttackEndEvent(FGameplayEventData Payload);

	UFUNCTION()
	void ApplyDamageToTarget();

	FTimerHandle AttackTraceTimer;
	bool bIsAttacking = false;

	TArray<AActor*> DamagedTargets;
};