#include "Boss/GA_TurnToFace.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_TurnToFace::UGA_TurnToFace()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 这个技能由事件触发，或者直接通过Tag激活
	// 设置默认的 AssetTags
	FGameplayTagContainer DefaultTags;
	DefaultTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Boss.Turn")));
	SetAssetTags(DefaultTags);
}

void UGA_TurnToFace::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	UE_LOG(LogTemp, Warning, TEXT("GA_TurnToFace: ActivateAbility called")); // 1. 函数是否被调用
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Error, TEXT("GA_TurnToFace: CommitAbility FAILED!")); // 2. CommitAbility 是否成功
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

	// 1. 计算朝向与玩家方向的夹角
	FVector ToPlayer = (Player->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal2D();
	FVector MyForward = Avatar->GetActorForwardVector().GetSafeNormal2D();

	float Dot = FVector::DotProduct(MyForward, ToPlayer);
	float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
	FVector Cross = FVector::CrossProduct(MyForward, ToPlayer);
	// 带符号的角度：正值为右，负值为左
	float SignedAngle = (Cross.Z < 0) ? -Angle : Angle;

	// 2. 根据夹角选择蒙太奇
	UAnimMontage* MontageToPlayLocal = nullptr;
	const float AbsAngle = FMath::Abs(SignedAngle);

	if (AbsAngle < 30.0f)
	{
		// 几乎已经面对，不需要转身，直接结束
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	else if (AbsAngle < 120.0f)
	{
		// 90度转身
		MontageToPlayLocal = (SignedAngle > 0) ? TurnRight90 : TurnLeft90;
	}
	else
	{
		// 180度转身
		MontageToPlayLocal = (SignedAngle > 0) ? TurnRight180 : TurnLeft180;
	}

	if (!MontageToPlayLocal)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 3. 播放蒙太奇，等待完成
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlayLocal);
	MontageTask->OnCompleted.AddDynamic(this, &UGA_TurnToFace::OnTurnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_TurnToFace::OnTurnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_TurnToFace::OnTurnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_TurnToFace::OnTurnMontageInterrupted);
	MontageTask->ReadyForActivation();

	// 标记 Boss 正在转身，防止攻击行为同时触发（可选）
	// 可在 Boss 类中设置 bIsTurning = true
}

void UGA_TurnToFace::OnTurnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_TurnToFace::OnTurnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}