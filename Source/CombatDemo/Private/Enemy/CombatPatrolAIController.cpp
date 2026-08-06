// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemy/CombatPatrolAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Enemy/CombatPatrolEnemy.h"
#include "GameFramework/CharacterMovementComponent.h"

const FName ACombatPatrolAIController::TargetKey(TEXT("Target"));
const FName ACombatPatrolAIController::TargetLocationKey(TEXT("TargetLocation"));
const FName ACombatPatrolAIController::TargetDistanceKey(TEXT("TargetDistance"));
const FName ACombatPatrolAIController::IsInvestigatingKey(TEXT("IsInvestigating"));
const FName ACombatPatrolAIController::LastKnownLocationKey(TEXT("LastKnownLocation"));

ACombatPatrolAIController::ACombatPatrolAIController()
{
    BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
    PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 1500.0f;
    SightConfig->LoseSightRadius = 1800.0f;
    SightConfig->PeripheralVisionAngleDegrees = 90.0f;
    SightConfig->SetMaxAge(1.0f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

    

    PerceptionComp->ConfigureSense(*SightConfig);
    PerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());
    PerceptionComp->OnPerceptionUpdated.AddDynamic(this, &ACombatPatrolAIController::OnPerceptionUpdated);
}

void ACombatPatrolAIController::BeginPlay()
{
    Super::BeginPlay();
}

void ACombatPatrolAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    UE_LOG(LogTemp, Warning, TEXT("AI Controller has possessed: %s"), *InPawn->GetName())
    if (BlackboardComp && BehaviorTree)
    {
        BlackboardComp->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
    }

    if (BehaviorTree)
    {
        RunBehaviorTree(BehaviorTree);
    }
}

void ACombatPatrolAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
    AActor* DetectedTarget = nullptr;
    float ClosestDist = FLT_MAX;

    for (AActor* Actor : UpdatedActors)
    {
        if (!Actor || Actor == GetPawn())
            continue;

        FActorPerceptionBlueprintInfo Info;
        PerceptionComp->GetActorsPerception(Actor, Info);

        bool bSensed = false;
        for (const FAIStimulus& Stim : Info.LastSensedStimuli)
        {
            if (Stim.WasSuccessfullySensed() && Stim.Type == UAISense::GetSenseID<UAISense_Sight>())
            {
                bSensed = true;
                break;
            }
        }

        if (bSensed)
        {
            float Dist = FVector::Dist(GetPawn()->GetActorLocation(), Actor->GetActorLocation());
            if (Dist < ClosestDist)
            {
                ClosestDist = Dist;
                DetectedTarget = Actor;
            }
        }
    }

    if (BlackboardComp)
    {
        if (DetectedTarget)
        {
            // 重新看到目标：取消调查定时器
            GetWorldTimerManager().ClearTimer(InvestigationTimerHandle);

            BlackboardComp->SetValueAsObject(TargetKey, DetectedTarget);
            float Dist = FVector::Dist(GetPawn()->GetActorLocation(), DetectedTarget->GetActorLocation());
            BlackboardComp->SetValueAsFloat(TargetDistanceKey, Dist);
            BlackboardComp->SetValueAsBool(IsInvestigatingKey, false);
        }
        else
        {
            // 当前帧没感知到任何人，检查是否原本有目标
            UObject* CurrentTarget = BlackboardComp->GetValueAsObject(TargetKey);
            if (CurrentTarget)
            {
                // 立即丢失目标，无缓冲
                ConfirmTargetLost();
            }
        }
    }
}

void ACombatPatrolAIController::UpdateTargetDistance()
{
    if (!BlackboardComp)
        return;

    UObject* TargetObj = BlackboardComp->GetValueAsObject(TargetKey);
    AActor* Target = Cast<AActor>(TargetObj);
    if (Target && GetPawn())
    {
        float Dist = FVector::Dist(GetPawn()->GetActorLocation(), Target->GetActorLocation());
        BlackboardComp->SetValueAsFloat(TargetDistanceKey, Dist);
    }
    else
    {
        BlackboardComp->ClearValue(TargetDistanceKey);
    }
}

void ACombatPatrolAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!GetPawn()) return;
    UBlackboardComponent* BB = GetBlackboardComponent();
    if (!BB) return;

    bool bHasTarget = (BB->GetValueAsObject(TargetKey) != nullptr);
    bool bIsInvestigating = BB->GetValueAsBool(IsInvestigatingKey);

    // 核心逻辑：有目标 → 追击速度；否则（包括调查和巡逻）→ 巡逻速度
    if (bHasTarget)
    {
        if (!bIsChasing)
        {
            SetChaseSpeed();
            bIsChasing = true;
        }
    }
    else
    {
        if (bIsChasing)
        {
            SetPatrolSpeed();
            bIsChasing = false;
        }
       
    }
}

void ACombatPatrolAIController::SetChaseSpeed()
{
    if (ACombatPatrolEnemy* Enemy = Cast<ACombatPatrolEnemy>(GetPawn()))
    {
        if (UCharacterMovementComponent* MoveComp = Enemy->GetCharacterMovement())
        {
            MoveComp->MaxWalkSpeed = ChaseSpeed;
            UE_LOG(LogTemp, Warning, TEXT("切换到追击速度: %.0f"), ChaseSpeed);
        }
    }
}

void ACombatPatrolAIController::SetPatrolSpeed()
{
    if (ACombatPatrolEnemy* Enemy = Cast<ACombatPatrolEnemy>(GetPawn()))
    {
        if (UCharacterMovementComponent* MoveComp = Enemy->GetCharacterMovement())
        {
            MoveComp->MaxWalkSpeed = PatrolSpeed;
            UE_LOG(LogTemp, Warning, TEXT("切换到巡逻速度: %.0f"), PatrolSpeed);
        }
    }
}

void ACombatPatrolAIController::ConfirmTargetLost()
{
    if (!BlackboardComp) return;

    AActor* CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey));
    if (CurrentTarget)
    {
        BlackboardComp->SetValueAsVector(LastKnownLocationKey, CurrentTarget->GetActorLocation());
    }

    BlackboardComp->SetValueAsBool(IsInvestigatingKey, true);
    BlackboardComp->ClearValue(TargetKey);
    BlackboardComp->ClearValue(TargetDistanceKey);


    // 启动调查超时计时器
    GetWorldTimerManager().SetTimer(
        InvestigationTimerHandle,
        this,
        &ACombatPatrolAIController::EndInvestigation,
        InvestigationDuration,
        false
    );

    UE_LOG(LogTemp, Warning, TEXT("进入调查状态，%.1f秒后自动结束"), InvestigationDuration);
}

void ACombatPatrolAIController::EndInvestigation()
{
    if (BlackboardComp)
    {
        BlackboardComp->SetValueAsBool(IsInvestigatingKey, false);
        UE_LOG(LogTemp, Warning, TEXT("调查结束，回归巡逻"));
    }
}