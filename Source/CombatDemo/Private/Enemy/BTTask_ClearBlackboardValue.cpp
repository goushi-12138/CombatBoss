// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemy/BTTask_ClearBlackboardValue.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_ClearBlackboardValue::UBTTask_ClearBlackboardValue()
{
    NodeName = TEXT("Clear Blackboard Value");
}

EBTNodeResult::Type UBTTask_ClearBlackboardValue::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }

    if (!BlackboardKey.IsSet())
    {
        return EBTNodeResult::Failed;
    }

    FName KeyName = BlackboardKey.SelectedKeyName;
    if (KeyName.IsNone())
    {
        return EBTNodeResult::Failed;
    }

    // 将 FName 转换为 FBlackboard::FKey
    FBlackboard::FKey KeyID = BlackboardComp->GetKeyID(KeyName);
    if (!BlackboardComp->IsValidKey(KeyID))
    {
        return EBTNodeResult::Failed;
    }
    // 核心：清除键值
    BlackboardComp->ClearValue(KeyID);

    return EBTNodeResult::Succeeded;
}

FString UBTTask_ClearBlackboardValue::GetStaticDescription() const
{
    FString KeyDesc = BlackboardKey.SelectedKeyName.ToString();
    if (KeyDesc.IsEmpty())
    {
        KeyDesc = TEXT("None");
    }
    return FString::Printf(TEXT("Clear Blackboard Value: %s"), *KeyDesc);
}