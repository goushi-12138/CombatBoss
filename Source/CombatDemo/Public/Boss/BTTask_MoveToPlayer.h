// BTTask_MoveToPlayer.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToPlayer.generated.h"

UCLASS()
class COMBATDEMO_API UBTTask_MoveToPlayer : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToPlayer();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "MoveTo")
	float AcceptableRadius = 700.0f;
};
