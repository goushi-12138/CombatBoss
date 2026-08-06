#include "Enemy/BTTask_GetRandomReachablePoint.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"

UBTTask_GetRandomReachablePoint::UBTTask_GetRandomReachablePoint()
{
    // 父类的 BlackboardKey 用于读取原点（LastKnownLocation）
    // 我们自己再添加一个键用于写入随机点
    bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_GetRandomReachablePoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 获取 AI 控制器和黑板
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController) return EBTNodeResult::Failed;

    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    if (!Blackboard) return EBTNodeResult::Failed;

    // 从父类指定的 BlackboardKey（原点）读取向量
    FVector Origin = Blackboard->GetValueAsVector(GetSelectedBlackboardKey());

    // 获取导航系统
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(AIController->GetWorld());
    if (!NavSys) return EBTNodeResult::Failed;

    // 在原点周围随机查找可达点
    FNavLocation RandomLocation;
    bool bFound = NavSys->GetRandomReachablePointInRadius(Origin, Radius, RandomLocation);

    if (bFound)
    {
        // 写入随机点黑板键
        Blackboard->SetValueAsVector(RandomLocationKey.SelectedKeyName, RandomLocation.Location);
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}