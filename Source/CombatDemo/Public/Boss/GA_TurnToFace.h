#pragma once

#include "CoreMinimal.h"
#include "Boss/BossGameplayAbility.h"
#include "GA_TurnToFace.generated.h"

UCLASS()
class COMBATDEMO_API UGA_TurnToFace : public UBossGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_TurnToFace();

	// 四个转身蒙太奇，在蓝图中指定
	UPROPERTY(EditDefaultsOnly, Category = "Turn")
	UAnimMontage* TurnLeft90;

	UPROPERTY(EditDefaultsOnly, Category = "Turn")
	UAnimMontage* TurnRight90;

	UPROPERTY(EditDefaultsOnly, Category = "Turn")
	UAnimMontage* TurnLeft180;

	UPROPERTY(EditDefaultsOnly, Category = "Turn")
	UAnimMontage* TurnRight180;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 蒙太奇播放完成回调
	UFUNCTION()
	void OnTurnMontageCompleted();

	UFUNCTION()
	void OnTurnMontageInterrupted();
};
