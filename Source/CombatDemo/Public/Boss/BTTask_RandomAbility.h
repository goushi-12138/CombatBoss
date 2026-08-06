#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RandomAbility.generated.h"

UCLASS()
class COMBATDEMO_API UBTTask_RandomAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_RandomAbility();

	// 第一个候选技能 ID
	UPROPERTY(EditAnywhere, Category = "RandomAbility")
	FName AbilityID_0;

	// 第二个候选技能 ID
	UPROPERTY(EditAnywhere, Category = "RandomAbility")
	FName AbilityID_1;

	// 要写入的黑板键名（默认 CurrentAbilityID）
	UPROPERTY(EditAnywhere, Category = "RandomAbility")
	FName BlackboardKeyName = TEXT("CurrentAbilityID");

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};