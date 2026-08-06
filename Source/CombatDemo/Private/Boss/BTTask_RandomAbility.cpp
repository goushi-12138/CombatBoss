#include "Boss/BTTask_RandomAbility.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_RandomAbility::UBTTask_RandomAbility()
{
	NodeName = TEXT("Random Ability (P1 Mid)");
}

EBTNodeResult::Type UBTTask_RandomAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
		return EBTNodeResult::Failed;

	// Ëæ»úÑ¡ 0 »ò 1
	const int32 Index = FMath::RandRange(0, 1);
	const FName SelectedID = (Index == 0) ? AbilityID_0 : AbilityID_1;

	BB->SetValueAsName(BlackboardKeyName, SelectedID);

	return EBTNodeResult::Succeeded;
}