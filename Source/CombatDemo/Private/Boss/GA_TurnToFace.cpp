#include "Boss/GA_TurnToFace.h"
#include "Boss/Boss_Berserker.h"
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

    // 3. 选择蒙太奇
    UAnimMontage* PlayMontage = nullptr;
    const float AbsAngle = FMath::Abs(SignedAngle);

    if (AbsAngle < 30.0f)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }
    else if (AbsAngle < 120.0f)
    {
        PlayMontage = (SignedAngle > 0) ? TurnRight90 : TurnLeft90;
    }
    else
    {
        PlayMontage = (SignedAngle > 0) ? TurnRight180 : TurnLeft180;
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

// EndAbility 在基类中已有，会自动清除 bIsAttacking