#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "BossGameplayAbility.generated.h"

class UGameplayEffect;
class UAnimMontage;

// 受击方向（决定玩家播放哪个方向的受击蒙太奇）
UENUM(BlueprintType)
enum class EBossHitReactDirection : uint8
{
	FromBoss    UMETA(DisplayName = "FromBoss (Auto)"),
	Front       UMETA(DisplayName = "Front"),
	Back        UMETA(DisplayName = "Back"),
	Left        UMETA(DisplayName = "Left"),
	Right       UMETA(DisplayName = "Right")
};

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

	// 攻击锥角度（度）：只能命中 Boss 前方该角度范围内的目标。
	// 例如 150 = 前方 ±75°；360（默认）或 0 = 不限制（全向）。
	// 左挥砍/右挥砍/二连砍/举砸 蓝图配置为 150；旋转连砍/跳劈保持 360。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossGA|Conditions")
	float AttackConeAngle = 360.0f;

	// 玩家盾牌防御锥角度（度）：玩家举盾时，只有攻击来自玩家前方该角度范围内才算格挡成功
	// （格挡成功：伤害减70% + 不播受击动画 + 播放盾牌格挡音效）。
	// 来自身后的攻击格挡无效（全额伤害 + 正常受击动画）。默认 150 = 前方 ±75°。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossGA|Conditions")
	float ShieldDefenseConeAngle = 150.0f;

	// 格挡冲击初速度（cm/s）：格挡成功时把玩家向后推开的水平初速度。
	// 玩家 BrakingDecelerationWalking=2000 时，滑行距离 ≈ v²/(2*2000)，300 ≈ 22 单位。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossGA|Conditions")
	float BlockPushSpeed = 300.0f;

	// ========== 受击反馈 ==========
	// 命中玩家后播放哪个方向的受击蒙太奇
	// FromBoss = 根据Boss到玩家的实际方向动态计算（推荐，前五种攻击均可使用）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossGA|HitReact")
	EBossHitReactDirection HitReactDirection = EBossHitReactDirection::FromBoss;

	// 连续伤害模式（旋转挥砍）：伤害窗口内可对同一目标造成多次伤害并多次触发受击
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossGA|Damage")
	bool bContinuousDamage = false;

	// 连续伤害模式下，每次伤害判定的间隔（秒）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "BossGA|Damage", meta = (EditCondition = "bContinuousDamage"))
	float RepeatDamageInterval = 0.25f;
	// ========== 受击反馈结束 ==========

protected:
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 【重要修正！】必须加上 UFUNCTION() 宏，且补全之前漏掉的函数
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	/** 设置Boss与玩家之间的碰撞（忽略或恢复），用于跳跃等技能防止踩头 */
	UFUNCTION(BlueprintCallable, Category = "BossGA")
	void SetIgnorePlayerCollision(bool bIgnore);

	/** 向目标发送受击事件（计算方向后下发到目标ASC） */
	void SendHitReactEvent(AActor* Victim);

	/** 判断本次攻击是否被玩家的盾牌有效格挡（玩家防御中 且 攻击来自玩家前方防御锥内） */
	bool IsAttackBlockedByShield(AActor* Victim) const;

	/** 根据 HitReactDirection 配置计算受击方向Tag */
	FGameplayTag GetHitReactDirectionTag(AActor* Victim) const;

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
