// BTTask_SetAbilityID.cpp
#include "Boss/BTTask_SetAbilityID.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_SetAbilityID::UBTTask_SetAbilityID()
{
    NodeName = TEXT("Set Ability ID");
}

EBTNodeResult::Type UBTTask_SetAbilityID::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    if (!BB) return EBTNodeResult::Failed;

    BB->SetValueAsName(BlackboardKeyName, AbilityID);
    return EBTNodeResult::Succeeded;
}