#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "BTTask_ExecuteAbility.generated.h"

UCLASS()
class COMBATDEMO_API UBTTask_ExecuteAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ExecuteAbility();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

protected:
	void OnAbilityEnded(const FAbilityEndedData& EndedData, UBehaviorTreeComponent* OwnerComp);

private:
	FDelegateHandle AbilityEndedHandle;
};