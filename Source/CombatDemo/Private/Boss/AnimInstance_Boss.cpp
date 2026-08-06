#include "Boss/AnimInstance_Boss.h"
#include "Boss/BossAIController.h"
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
        Direction = 0.0f;
        return;
    }
    // 从 AI 控制器的黑板中读取阶段
    if (ABossAIController* AIC = Cast<ABossAIController>(Pawn->GetController()))
    {
        if (UBlackboardComponent* BB = AIC->GetBlackboardComponent())
        {
            bIsPhaseOne = BB->GetValueAsBool(FName("IsPhaseOne"));
        }
    }
   
    // 1. Speed（水平速度）
    Speed = Pawn->GetVelocity().Size2D();
    
    // 2. Direction（旋转角度差值，用来驱动转身动画）
    float CurrentYaw = Pawn->GetActorRotation().Yaw;

    if (bHasPreviousYaw)
    {
        // 计算本帧 Yaw 变化（度）
        float DeltaYaw = FMath::UnwindDegrees(CurrentYaw - PreviousYaw);

        if (Speed < 30.0f) // 原地旋转
        {
            if (FMath::Abs(DeltaYaw) > 0.01f)
            {
                // 关键：必须放大 DeltaYaw，因为 AI 的平滑旋转每帧只有 1-3 度
                // 放大 100 倍后，3 度的旋转就能映射到 Direction = 180（触发 180° 转身动画）
                float TargetDirection = FMath::Clamp(DeltaYaw * 10.0f, -180.0f, 180.0f);
                Direction = FMath::FInterpTo(Direction, TargetDirection, DeltaSeconds, 15.0f);
            }
            else
            {
                Direction = FMath::FInterpTo(Direction, 0.0f, DeltaSeconds, 10.0f);
            }
        }
        else if (Speed >= 30.0f) // 移动中，使用移动方向计算
        {
            FVector VelocityDir = Pawn->GetVelocity().GetSafeNormal2D();
            FVector Forward = Pawn->GetActorForwardVector().GetSafeNormal2D();

            float Dot = FVector::DotProduct(Forward, VelocityDir);
            float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
            FVector Cross = FVector::CrossProduct(Forward, VelocityDir);

            float MoveDirection = (Cross.Z < 0) ? -Angle : Angle;
            Direction = FMath::FInterpTo(Direction, MoveDirection, DeltaSeconds, 10.0f);
        }
    }
    else
    {
        Direction = 0.0f;
    }

    PreviousYaw = CurrentYaw;
    bHasPreviousYaw = true;

    // 调试日志
    if (FMath::Abs(Direction) > 1.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Boss Anim | Speed: %.2f | Direction: %.2f"), Speed, Direction);
    }
    
}