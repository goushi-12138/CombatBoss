// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "CombatPatrolAIController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;

UCLASS(Blueprintable)
class COMBATDEMO_API ACombatPatrolAIController : public AAIController
{
    GENERATED_BODY()

public:
    ACombatPatrolAIController();

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

    virtual void Tick(float DeltaTime) override;

    /** 行为树资产 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
    UBehaviorTree* BehaviorTree;

    /** 黑板组件（自动创建） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UBlackboardComponent* BlackboardComp;

    /** 感知组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAIPerceptionComponent* PerceptionComp;

    /** 视觉配置 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Sight")
    UAISenseConfig_Sight* SightConfig;

    /** 黑板键名 */
    static const FName TargetKey;
    static const FName TargetLocationKey;
    static const FName TargetDistanceKey;
    static const FName IsInvestigatingKey;
    static const FName LastKnownLocationKey;
private:
    /** 感知更新回调 */
    UFUNCTION()
    void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

    /** 更新目标距离到黑板 */
    void UpdateTargetDistance();

    /** 设置感知配置 */
    void SetupPerceptionSystem();

    /** 丢失目标后的缓冲计时器 
    FTimerHandle LossTimerHandle;
*/
public:

    /** 切换到追击速度（跑步） */
    void SetChaseSpeed();

    /** 切换到巡逻速度（走路） */
    void SetPatrolSpeed();

    /** 确认目标丢失（由 LossTimer 触发） */
    void ConfirmTargetLost();
protected:
    /** 当前是否在追击状态 */
    bool bIsChasing = false;

    /** 巡逻速度（走路） */
    UPROPERTY(EditAnywhere, Category = "AI|Movement")
    float PatrolSpeed = 200.0f;

    /** 追击速度（跑步） */
    UPROPERTY(EditAnywhere, Category = "AI|Movement")
    float ChaseSpeed = 600.0f;


    /** 缓冲时间（秒），在这段时间内短暂丢失不会立即判丢 
    UPROPERTY(EditAnywhere, Category = "AI|Perception", meta = (ClampMin = "0.5", ClampMax = "5.0"))
    float LossBufferTime = 1.5f;
*/

    // 调查总时长定时器
    FTimerHandle InvestigationTimerHandle;

    // 调查持续时间
    UPROPERTY(EditAnywhere, Category = "AI|Investigation")
    float InvestigationDuration = 15.0f;

    // 结束调查
    void EndInvestigation();
};