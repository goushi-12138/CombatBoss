#include "Boss/AnimInstance_Boss.h"
#include "Boss/Boss_Berserker.h"
#include "Boss/BossAttributeSet.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h" // 确保包含此头文件
#include "BehaviorTree/BlackboardComponent.h" // 添加这一行，修复UBlackboardComponent不完整类型错误

void UAnimInstance_Boss::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    APawn* Pawn = TryGetPawnOwner();
    if (!Pawn)
    {
        Speed = 0.0f;
        return;
    }
    Speed = Pawn->GetVelocity().Size2D();

	// 获取Boss的阶段信息
    ABoss_Berserker* Boss = Cast<ABoss_Berserker>(Pawn);
    if (Boss && Boss->GetBossAttributeSet())
    {
        bIsPhaseOne = (Boss->GetBossAttributeSet()->GetPhase() == 1.0f);
	}
}