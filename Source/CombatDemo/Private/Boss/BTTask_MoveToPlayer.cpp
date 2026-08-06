// BTTask_MoveToPlayer.cpp
#include "Boss/BTTask_MoveToPlayer.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include <Navigation\PathFollowingComponent.h>

UBTTask_MoveToPlayer::UBTTask_MoveToPlayer()
{
	NodeName = TEXT("Move To Player (Dynamic)");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_MoveToPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn) return EBTNodeResult::Failed;

	// 使用 AI 的 MoveToActor，它会持续追踪目标 Actor
	EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(PlayerPawn, AcceptableRadius);

	if (MoveResult == EPathFollowingRequestResult::RequestSuccessful)
	{
		return EBTNodeResult::InProgress; // 关键：任务持续进行
	}
	else
	{
		return EBTNodeResult::Failed;
	}
}

void UBTTask_MoveToPlayer::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

	if (!AIController || !PlayerPawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 检查 Boss 是否已在玩家周围的目标半径内
	float DistanceToPlayer = FVector::Dist(AIController->GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());

	if (DistanceToPlayer <= AcceptableRadius)
	{
		AIController->StopMovement(); // 停止移动
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}