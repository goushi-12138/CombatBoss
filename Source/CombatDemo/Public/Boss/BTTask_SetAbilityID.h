// BTTask_SetAbilityID.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SetAbilityID.generated.h"

UCLASS()
class COMBATDEMO_API UBTTask_SetAbilityID : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_SetAbilityID();

    UPROPERTY(EditAnywhere, Category = "SetAbility")
    FName AbilityID;

    UPROPERTY(EditAnywhere, Category = "SetAbility")
    FName BlackboardKeyName = TEXT("CurrentAbilityID");

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};