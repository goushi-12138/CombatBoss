// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemy/CombatPatrolEnemy.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"

ACombatPatrolEnemy::ACombatPatrolEnemy()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ACombatPatrolEnemy::BeginPlay()
{
    Super::BeginPlay();
    if (PatrolPoints.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("ACombatPatrolEnemy %s has no patrol points set!"), *GetName());
    }
    Super::BeginPlay();
    // 游戏开始时缓存，或者延迟到第一次攻击时再缓存
    CacheAttackSections();
}

FVector ACombatPatrolEnemy::GetInitialPatrolPointLocation() const
{
    if (PatrolPoints.Num() > 0 && IsValid(PatrolPoints[0]))
    {
        return PatrolPoints[0]->GetActorLocation();
    }
    return GetActorLocation();
}

FVector ACombatPatrolEnemy::GetNextPatrolPointLocation()
{
    if (PatrolPoints.Num() == 0)
        return GetActorLocation();

    CurrentPatrolIndex = (CurrentPatrolIndex + 1) % PatrolPoints.Num();
    AActor* NextPoint = PatrolPoints[CurrentPatrolIndex];
    if (IsValid(NextPoint))
    {
        return NextPoint->GetActorLocation();
    }
    return GetActorLocation();
}

bool ACombatPatrolEnemy::CanAttack() const
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    return (CurrentTime - LastAttackTime) >= AttackCooldown;

}

void ACombatPatrolEnemy::Attack()
{
    if (!CanAttack())
        return;

    PlayRandomAttackMontage();
    LastAttackTime = GetWorld()->GetTimeSeconds();
}

void ACombatPatrolEnemy::CacheAttackSections()
{
    CachedAttackSections.Empty();
    if (!AttackMontage)
    {
        return;
    }

    // 方式2：如果 GetSectionNames 不可用（极老版本），可以手动遍历 CompositeSections
    for (const FCompositeSection& Section : AttackMontage->CompositeSections)
     {
         CachedAttackSections.Add(Section.SectionName);
     }

    bAttackSectionsCached = true;
}

void ACombatPatrolEnemy::PlayRandomAttackMontage()
{
    if (!AttackMontage)
    {
        return;
    }

    USkeletalMeshComponent* CharacterMesh = GetMesh();
    if (!CharacterMesh)
    {
        return;
    }

    UAnimInstance* AnimInstance = CharacterMesh->GetAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    // 确保已缓存（如果还没缓存，这里强制缓存一次）
    if (!bAttackSectionsCached)
    {
        CacheAttackSections();
    }

    if (CachedAttackSections.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Attack montage has no sections!"));
        return;
    }

    // 随机选一个 Section
    const int32 RandomIndex = FMath::RandRange(0, CachedAttackSections.Num() - 1);
    const FName SectionName = CachedAttackSections[RandomIndex];

    // 播放蒙太奇并立刻跳到随机 Section
    AnimInstance->Montage_Play(AttackMontage);
    AnimInstance->Montage_JumpToSection(SectionName, AttackMontage);

    // 调试信息（可选）
    UE_LOG(LogTemp, Log, TEXT("Playing attack section: %s"), *SectionName.ToString());
}
