// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemy/BTTask_Attack.h"
#include "AIController.h"
#include "Enemy/CombatPatrolEnemy.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/KismetMathLibrary.h"

UBTTask_Attack::UBTTask_Attack()
{
    NodeName = TEXT("Attack");
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
        return EBTNodeResult::Failed;

    ACombatPatrolEnemy* Enemy = Cast<ACombatPatrolEnemy>(AIController->GetPawn());
    if (!Enemy)
        return EBTNodeResult::Failed;

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (Blackboard)
    {
        AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject("Target"));
        if (Target)
        {
            FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(Enemy->GetActorLocation(), Target->GetActorLocation());
            LookAt.Pitch = 0.0f;
            Enemy->SetActorRotation(LookAt);
        }
    }
    Enemy->Attack();
    return EBTNodeResult::Succeeded;
}