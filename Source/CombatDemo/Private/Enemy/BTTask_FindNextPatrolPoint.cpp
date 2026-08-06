// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemy/BTTask_FindNextPatrolPoint.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Enemy/CombatPatrolEnemy.h"

UBTTask_FindNextPatrolPoint::UBTTask_FindNextPatrolPoint()
{
    NodeName = TEXT("Find Next Patrol Point");
}

EBTNodeResult::Type UBTTask_FindNextPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard)
    {
        UE_LOG(LogTemp, Error, TEXT("FindNextPatrolPoint: Blackboard is null!"));
        return EBTNodeResult::Failed;
    }


    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogTemp, Error, TEXT("FindNextPatrolPoint: Blackboard is null!"));
        return EBTNodeResult::Failed;
    }


    ACombatPatrolEnemy* Enemy = Cast<ACombatPatrolEnemy>(AIController->GetPawn());
    if (!Enemy)
    {
        UE_LOG(LogTemp, Error, TEXT("FindNextPatrolPoint: Failed to cast to ACombatPatrolEnemy!"));
        return EBTNodeResult::Failed;
    }

    // 打印巡逻点数量
    UE_LOG(LogTemp, Warning, TEXT("FindNextPatrolPoint: PatrolPoints count = %d"), Enemy->PatrolPoints.Num());

    FVector NextLocation = Enemy->GetNextPatrolPointLocation();
    UE_LOG(LogTemp, Warning, TEXT("FindNextPatrolPoint: NextLocation = %s"), *NextLocation.ToString());
    Blackboard->SetValueAsVector(PatrolLocationKey.SelectedKeyName, NextLocation);

    return EBTNodeResult::Succeeded;
}