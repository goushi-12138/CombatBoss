// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_ClearBlackboardValue.generated.h"

UCLASS(Blueprintable)
class COMBATDEMO_API UBTTask_ClearBlackboardValue : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ClearBlackboardValue();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual FString GetStaticDescription() const override;

protected:
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector BlackboardKey;
};