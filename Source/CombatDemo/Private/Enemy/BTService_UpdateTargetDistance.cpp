// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemy/BTService_UpdateTargetDistance.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UBTService_UpdateTargetDistance::UBTService_UpdateTargetDistance()
{
    NodeName = TEXT("Update Target Distance");
    Interval = 0.5f;
}

void UBTService_UpdateTargetDistance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard)
        return;
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController || !AIController->GetPawn())
        return;
    
    UObject* TargetObj = Blackboard->GetValueAsObject(TargetKey.SelectedKeyName);
    AActor* Target = Cast<AActor>(TargetObj);
    if (Target)
    {
        float Dist = FVector::Dist(AIController->GetPawn()->GetActorLocation(), Target->GetActorLocation());
        Blackboard->SetValueAsFloat(DistanceKey.SelectedKeyName, Dist);
    }
    else
    {
        Blackboard->ClearValue(DistanceKey.SelectedKeyName);
    }
}