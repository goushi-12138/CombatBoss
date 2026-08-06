#include "Boss/BTTask_ExecuteAbility.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"
#include "GameplayTagContainer.h"

UBTTask_ExecuteAbility::UBTTask_ExecuteAbility()
{
	NodeName = TEXT("Execute Boss Ability");
}

EBTNodeResult::Type UBTTask_ExecuteAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	APawn* ControlledPawn = AIC->GetPawn();
	if (!ControlledPawn) return EBTNodeResult::Failed;

	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(ControlledPawn);
	if (!ASCInterface) return EBTNodeResult::Failed;

	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC) return EBTNodeResult::Failed;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;

	// 获取技能 ID
	FName AbilityID = BB->GetValueAsName(FName("CurrentAbilityID"));
	if (AbilityID.IsNone()) return EBTNodeResult::Failed;

	// 构建 Tag，例如 "Ability.Boss.P1_Sweep"
	const FString TagString = FString::Printf(TEXT("Ability.Boss.%s"), *AbilityID.ToString());
	FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
	if (!AbilityTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteAbility: Invalid Ability Tag %s"), *TagString);
		return EBTNodeResult::Failed;
	}

	// 激活能力
	const bool bActivated = ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTag));
	if (!bActivated)
	{
		UE_LOG(LogTemp, Warning, TEXT("BTTask_ExecuteAbility: Failed to activate ability with tag %s"), *TagString);
		return EBTNodeResult::Failed;
	}

	// 监听能力结束
	AbilityEndedHandle = ASC->OnAbilityEnded.AddUObject(this, &UBTTask_ExecuteAbility::OnAbilityEnded, &OwnerComp);
	return EBTNodeResult::InProgress;
}

void UBTTask_ExecuteAbility::OnAbilityEnded(const FAbilityEndedData& EndedData, UBehaviorTreeComponent* OwnerComp)
{
	if (!OwnerComp) return;

	AAIController* AIC = OwnerComp->GetAIOwner();
	if (!AIC) return;

	if (APawn* Pawn = AIC->GetPawn())
	{
		if (IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(Pawn))
		{
			if (UAbilitySystemComponent* ASC = Interface->GetAbilitySystemComponent())
			{
				ASC->OnAbilityEnded.Remove(AbilityEndedHandle);
			}
		}
	}

	FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
}

void UBTTask_ExecuteAbility::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	// 清理委托绑定
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (AIC)
	{
		if (APawn* Pawn = AIC->GetPawn())
		{
			if (IAbilitySystemInterface* Interface = Cast<IAbilitySystemInterface>(Pawn))
			{
				if (UAbilitySystemComponent* ASC = Interface->GetAbilitySystemComponent())
				{
					ASC->OnAbilityEnded.Remove(AbilityEndedHandle);
				}
			}
		}
	}
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}