#include "Boss/BossAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Boss/Boss_Berserker.h"
#include "AbilitySystemComponent.h" // 添加此头文件以解决UAbilitySystemComponent不完整类型问题

ABossAIController::ABossAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ABossAIController::BeginPlay()
{
	Super::BeginPlay();

	if (BlackboardAsset)
	{
		// 修复方式：将Blackboard.Get()的结果存入一个变量，传递变量作为左值
		UBlackboardComponent* BlackboardPtr = Blackboard.Get();
		UseBlackboard(BlackboardAsset, BlackboardPtr);
	}

	if (BehaviorTreeAsset)
	{
		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void ABossAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新黑板数据
    TimeSinceLastUpdate += DeltaTime;
    if (TimeSinceLastUpdate >= UpdateInterval)
    {
        TimeSinceLastUpdate = 0.0f;
        UpdateBlackboard();
    }

    // 旋转逻辑：只在非攻击状态下转向玩家
    APawn* ControlledPawn = GetPawn();
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);

    if (ControlledPawn && PlayerPawn)
    {
        // 检查Boss是否正在攻击
        ABoss_Berserker* Boss = Cast<ABoss_Berserker>(ControlledPawn);
        bool bShouldRotate = true;

        if (Boss)
        {
            bShouldRotate = !Boss->bIsAttacking;  // 攻击中不旋转
        }

        if (bShouldRotate)
        {
            FVector DirectionToPlayer = PlayerPawn->GetActorLocation() - ControlledPawn->GetActorLocation();
            DirectionToPlayer.Z = 0.0f;

            if (!DirectionToPlayer.IsNearlyZero())
            {
                FRotator TargetRotation = DirectionToPlayer.Rotation();
                FRotator CurrentRotation = ControlledPawn->GetActorRotation();
                FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 5.0f);
                ControlledPawn->SetActorRotation(NewRotation);
            }
        }
    }
}

void ABossAIController::UpdateBlackboard()
{
    if (!Blackboard) return;

    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    // 玩家位置
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC && PC->GetPawn())
    {
        const FVector PlayerLocation = PC->GetPawn()->GetActorLocation();
        Blackboard->SetValueAsVector(FName("PlayerLocation"), PlayerLocation);

        Blackboard->SetValueAsObject(FName("TargetPlayer"), PC->GetPawn());

        const float Dist = FVector::Dist(PlayerLocation, ControlledPawn->GetActorLocation());
        Blackboard->SetValueAsFloat(FName("DistanceToPlayer"), Dist);

        /*
        // ========== 【新增】强制Boss始终面向玩家 ==========
        FVector DirectionToPlayer = PlayerLocation - ControlledPawn->GetActorLocation();
        DirectionToPlayer.Z = 0.0f;  // 只在水平面旋转

        if (!DirectionToPlayer.IsNearlyZero())
        {
            FRotator TargetRotation = DirectionToPlayer.Rotation();
            FRotator CurrentRotation = ControlledPawn->GetActorRotation();

            // 使用AIController的平滑旋转，避免瞬间转向
            FRotator NewRotation = FMath::RInterpTo(
                CurrentRotation,
                TargetRotation,
                GetWorld()->GetDeltaSeconds(),
                8.0f  // 旋转速度，越大越快
            );

            ControlledPawn->SetActorRotation(NewRotation);
        }
        */
    }
}

void ABossAIController::SetPhase(bool bIsPhaseOne)
{
	if (Blackboard)
	{
		Blackboard->SetValueAsBool(FName("IsPhaseOne"), bIsPhaseOne);
	}

	// 【新增】同步修改Boss的移动速度
	if (ABoss_Berserker* Boss = Cast<ABoss_Berserker>(GetPawn()))
	{
		Boss->SetMovementSpeedForPhase(bIsPhaseOne ? 1 : 2);
	}
}