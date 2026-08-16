#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_PlayerHitReact.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

UCLASS()
class COMBATDEMO_API UGA_PlayerHitReact : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_PlayerHitReact();

	// ===== 摇晃受击蒙太奇（前五种攻击，均不开根运动）=====
	// 攻击来自玩家前方 → 向后倒
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact|Shake")
	UAnimMontage* HitFront;

	// 攻击来自玩家后方 → 向前倒
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact|Shake")
	UAnimMontage* HitBack;

	// 攻击来自玩家左方 → 向右倒
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact|Shake")
	UAnimMontage* HitLeft;

	// 攻击来自玩家右方 → 向左倒
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact|Shake")
	UAnimMontage* HitRight;

	// ===== 倒地蒙太奇（跳劈专用，均不开根运动）=====
	// 玩家正对Boss → 向后倒地
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact|Down")
	UAnimMontage* DownFront;

	// 玩家背对Boss → 向前倒地
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact|Down")
	UAnimMontage* DownBack;

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

	/** 根据触发事件标签选择蒙太奇 */
	UAnimMontage* SelectMontage(const FGameplayTag& EventTag) const;

	// ===== 蒙太奇回调 =====
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

private:
	// 防重入保护：避免 EndAbility 与 OnMontageCompleted/OnMontageInterrupted 互相调用导致堆栈溢出
	bool bIsEnding = false;

	UAbilityTask_PlayMontageAndWait* ActiveMontageTask = nullptr;

	/** 本次播放的是否为倒地蒙太奇（DownFront/DownBack）。
	    是则播放期间给玩家添加 Status.Downed 标签（禁止移动输入），蒙太奇结束移除。
	    注意：不修改 CharacterMovement 的 MovementMode（MOVE_None 会阻止蒙太奇根运动，
	    导致"被打飞一小段距离"的根运动失效）。 */
	bool bIsDownMontage = false;
};
