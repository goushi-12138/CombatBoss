// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "CombatPatrolEnemy.generated.h"

class UAnimMontage;

UCLASS(Blueprintable)
class COMBATDEMO_API ACombatPatrolEnemy : public ACharacter
{
    GENERATED_BODY()

public:
    ACombatPatrolEnemy();

    /** 巡逻点列表（在编辑器中设置） */
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "AI|Patrol")
    TArray<AActor*> PatrolPoints;

    /** 追击距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
    float ChaseRange = 800.0f;

    /** 攻击距离 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
    float AttackRange = 200.0f;

    /** 攻击冷却时间（秒） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Combat")
    float AttackCooldown = 1.5f;

    /** 获取初始巡逻点位置（列表第一个） */
    UFUNCTION(BlueprintCallable, Category = "AI|Patrol")
    FVector GetInitialPatrolPointLocation() const;

    /** 获取下一个巡逻点位置（循环） */
    UFUNCTION(BlueprintCallable, Category = "AI|Patrol")
    FVector GetNextPatrolPointLocation();

    /** 检查是否可以攻击（冷却已过） */
    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    bool CanAttack() const;

    /** 执行攻击：随机播放一个蒙太奇，并重置冷却计时 */
    UFUNCTION(BlueprintCallable, Category = "AI|Combat")
    void Attack();

protected:
    virtual void BeginPlay() override;

private:
    /** 当前巡逻点索引 */
    int32 CurrentPatrolIndex = 0;

    /** 上次攻击时间 */
    float LastAttackTime = -1000.0f;

protected:
    // 攻击蒙太奇资源（只有一个）
    UPROPERTY(EditAnywhere, Category = "Combat")
    UAnimMontage* AttackMontage;

    // 缓存从蒙太奇读出的所有 Section 名称
    TArray<FName> CachedAttackSections;

    // 是否已经缓存过
    bool bAttackSectionsCached = false;

    void CacheAttackSections();

public:
    void PlayRandomAttackMontage();
};