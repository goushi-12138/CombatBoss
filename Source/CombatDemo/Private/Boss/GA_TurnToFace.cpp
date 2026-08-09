#include "Boss/GA_TurnToFace.h"
#include "Boss/Boss_Berserker.h"
#include "Boss/BossAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_TurnToFace::UGA_TurnToFace()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    // 设置标签
    FGameplayTagContainer NewTags;
    NewTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Boss.Turn")));
    SetAssetTags(NewTags);
}

void UGA_TurnToFace::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AActor* Avatar = GetAvatarActorFromActorInfo();
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Avatar || !Player)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 1. 标记攻击状态（防止行为树打断）
    if (ABoss_Berserker* Boss = Cast<ABoss_Berserker>(Avatar))
    {
        Boss->bIsAttacking = true;
    }

    // 2. 计算夹角
    FVector ToPlayer = (Player->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal2D();
    FVector MyForward = Avatar->GetActorForwardVector().GetSafeNormal2D();
    float Dot = FVector::DotProduct(MyForward, ToPlayer);
    float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
    FVector Cross = FVector::CrossProduct(MyForward, ToPlayer);
    float SignedAngle = (Cross.Z < 0) ? -Angle : Angle;
    const float AbsAngle = FMath::Abs(SignedAngle);
     
    // 新的角度阈值
    if (AbsAngle < 15.0f)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // 2. 获取当前阶段
    int32 CurrentPhase = 1;
    if (ABoss_Berserker* Boss = Cast<ABoss_Berserker>(Avatar))
    {
        if (UBossAttributeSet* AttrSet = Boss->GetBossAttributeSet())
        {
            CurrentPhase = FMath::RoundToInt(AttrSet->GetPhase());
        }
    }

    // 3. 根据阶段和角度选择对应的蒙太奇
    UAnimMontage* PlayMontage = nullptr;
    bool bIsPhase2 = (CurrentPhase == 2);

    if (AbsAngle < 60.0f) // 45° 动画
    {
        if (SignedAngle > 0)
            PlayMontage = bIsPhase2 ? TurnRight45_P2 : TurnRight45;
        else
            PlayMontage = bIsPhase2 ? TurnLeft45_P2 : TurnLeft45;
    }
    else if (AbsAngle < 110.0f) // 90° 动画
    {
        if (SignedAngle > 0)
            PlayMontage = bIsPhase2 ? TurnRight90_P2 : TurnRight90;
        else
            PlayMontage = bIsPhase2 ? TurnLeft90_P2 : TurnLeft90;
    }
    else if (AbsAngle < 150.0f) // 135° 动画
    {
        if (SignedAngle > 0)
            PlayMontage = bIsPhase2 ? TurnRight135_P2 : TurnRight135;
        else
            PlayMontage = bIsPhase2 ? TurnLeft135_P2 : TurnLeft135;
    }
    else // 180° 动画
    {
        if (SignedAngle > 0)
            PlayMontage = bIsPhase2 ? TurnRight180_P2 : TurnRight180;
        else
            PlayMontage = bIsPhase2 ? TurnLeft180_P2 : TurnLeft180;
    }

    if (!PlayMontage)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 4. 播放蒙太奇（带根运动）
    UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, PlayMontage, 1.0f, NAME_None, true); // true = 应用根运动
    MontageTask->OnCompleted.AddDynamic(this, &UGA_TurnToFace::OnTurnMontageCompleted);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_TurnToFace::OnTurnMontageCompleted);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_TurnToFace::OnTurnMontageInterrupted);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_TurnToFace::OnTurnMontageInterrupted);
    MontageTask->ReadyForActivation();
}

void UGA_TurnToFace::OnTurnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_TurnToFace::OnTurnMontageInterrupted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
