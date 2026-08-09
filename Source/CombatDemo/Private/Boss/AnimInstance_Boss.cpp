// AnimInstance_Boss.cpp

#include "Boss/AnimInstance_Boss.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Boss/BossAIController.h"

void UAnimInstance_Boss::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    AActor* Owner = GetOwningActor();
    if (Owner)
    {
        IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Owner);
        if (ASCInterface)
        {
            AbilitySystemComponent = ASCInterface->GetAbilitySystemComponent();
        }
    }

    PhaseTransitionTag = FGameplayTag::RequestGameplayTag(FName("Status.PhaseTransition"));
}

void UAnimInstance_Boss::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    APawn* Pawn = TryGetPawnOwner();
    if (!Pawn) return;

    // 1. 更新移动速度
    Speed = Pawn->GetVelocity().Size2D();

    // 2. 检查是否正在转阶段演出（GAS标签）
    bIsPhaseTransitioning = false;
    if (AbilitySystemComponent)
    {
        bIsPhaseTransitioning = AbilitySystemComponent->HasMatchingGameplayTag(PhaseTransitionTag);
    }

    // 3. 从黑板读取阶段信息（仅在非转阶段演出期间）
    if (!bIsPhaseTransitioning)
    {
        if (ABossAIController* AIC = Cast<ABossAIController>(Pawn->GetController()))
        {
            if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
            {
                bIsPhaseOne = BB->GetValueAsBool(FName("IsPhaseOne"));
            }
        }
    }
    // 如果正在转阶段演出，保持 bIsPhaseOne 不变（维持一阶段姿势）
}