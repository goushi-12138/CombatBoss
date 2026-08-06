#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_GetRandomReachablePoint.generated.h"

UCLASS()
class COMBATDEMO_API UBTTask_GetRandomReachablePoint : public UBTTask_BlackboardBase
{
    GENERATED_BODY()

public:
    UBTTask_GetRandomReachablePoint();

    // ´æ´¢Ëæ»úµãµÄºÚ°å¼ü£¨Ð´Èë£©
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector RandomLocationKey;

    // ËÑË÷°ë¾¶
    UPROPERTY(EditAnywhere, Category = "Parameters", meta = (ClampMin = "1.0"))
    float Radius = 400.0f;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};