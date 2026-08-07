#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BossAIController.generated.h"

class UBlackboardComponent;
class UBehaviorTreeComponent;

UCLASS()
class COMBATDEMO_API ABossAIController : public AAIController
{
	GENERATED_BODY()

public:
	ABossAIController();

	virtual void Tick(float DeltaTime) override;

	// 行为树和黑板资产（在编辑器中指定）
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBlackboardData* BlackboardAsset;

	// 阶段切换调用
	void SetPhase(bool bIsPhaseOne);

protected:
	virtual void BeginPlay() override;

private:
	float TimeSinceLastUpdate = 0.0f;
	static constexpr float UpdateInterval = 0.5f;

	void UpdateBlackboard();

private:
	float TimeSinceLastTurn = 0.0f;
	float TurnCooldown = 0.5f;
};