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

    TimeSinceLastUpdate += DeltaTime;
    if (TimeSinceLastUpdate >= UpdateInterval)
    {
        TimeSinceLastUpdate = 0.0f;
        UpdateBlackboard();
    }

    TimeSinceLastTurn += DeltaTime;

    APawn* ControlledPawn = GetPawn();
    APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!ControlledPawn || !PlayerPawn) return;

    ABoss_Berserker* Boss = Cast<ABoss_Berserker>(ControlledPawn);

    // 检测攻击状态变化
    if (Boss)
    {
        if (Boss->bIsAttacking)
        {
            TimeSinceLastAttack = 0.0f; // 正在攻击，清零计时
        }
        else
        {
            TimeSinceLastAttack += DeltaTime; // 攻击结束，开始计时
        }
    }

    if (!Boss || Boss->bIsAttacking || TimeSinceLastTurn < TurnCooldown) return;

    FVector ToPlayer = PlayerPawn->GetActorLocation() - ControlledPawn->GetActorLocation();
    ToPlayer.Z = 0.0f;
    FVector Forward = ControlledPawn->GetActorForwardVector();
    Forward.Z = 0.0f;
    float Dot = FVector::DotProduct(Forward.GetSafeNormal(), ToPlayer.GetSafeNormal());
    float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));

    if (Angle > 45.0f && !Boss->bIsAttacking && TimeSinceLastAttack > PostAttackTurnLockTime)
    {
        IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(ControlledPawn);
        if (ASCInterface)
        {
            UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
            if (ASC)
            {
                FGameplayTagContainer TurnTag;
                TurnTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Boss.Turn")));
                if (ASC->TryActivateAbilitiesByTag(TurnTag) && TimeSinceLastTurn > TurnCooldown)
                    TimeSinceLastTurn = 0.0f;
            }
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[TurnCheck] Angle: %.2f, bIsAttacking: %d, Cooldown: %.2f"), Angle, Boss->bIsAttacking, TimeSinceLastTurn);
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