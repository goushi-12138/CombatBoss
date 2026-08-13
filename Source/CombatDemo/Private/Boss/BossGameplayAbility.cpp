#include "Boss/BossGameplayAbility.h"
#include "Boss/Boss_Berserker.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h" // 解决 FOverlapResult 不完整类型报错
#include "Engine/EngineTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "AbilitySystemBlueprintLibrary.h" // 添加头文件

UBossGameplayAbility::UBossGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

}

bool UBossGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
		return false;

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
		return false;

	// 距离检查
	if (MinDistance > 0.0f || MaxDistance > 0.0f)
	{
		APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		if (PlayerPawn)
		{
			float Dist = FVector::Dist(ActorInfo->AvatarActor->GetActorLocation(), PlayerPawn->GetActorLocation());
			if (Dist < MinDistance || (MaxDistance > 0 && Dist > MaxDistance))
				return false;
		}
	}

	return true;
}

void UBossGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (ABoss_Berserker* Boss = Cast<ABoss_Berserker>(GetAvatarActorFromActorInfo()))
	{
		Boss->bIsAttacking = true;
	}

	// 播放蒙太奇
	if (MontageToPlay)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontageToPlay);
		MontageTask->OnCompleted.AddDynamic(this, &UBossGameplayAbility::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UBossGameplayAbility::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UBossGameplayAbility::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UBossGameplayAbility::OnMontageInterrupted);
		MontageTask->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 等待来自动画通知的 AttackStart 和 AttackEnd 事件
	UAbilityTask_WaitGameplayEvent* WaitStart = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Event.Attack.Start")));
	WaitStart->EventReceived.AddDynamic(this, &UBossGameplayAbility::OnAttackStartEvent);
	WaitStart->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitEnd = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Event.Attack.End")));
	WaitEnd->EventReceived.AddDynamic(this, &UBossGameplayAbility::OnAttackEndEvent);
	WaitEnd->ReadyForActivation();
}

void UBossGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilityActivationInfo ActivationInfo,bool bReplicateEndAbility,bool bWasCancelled)
{
	// 【新增】清除攻击状态
	if (ABoss_Berserker* Boss = Cast<ABoss_Berserker>(GetAvatarActorFromActorInfo()))
	{
		Boss->bIsAttacking = false;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UBossGameplayAbility::OnAttackStartEvent(FGameplayEventData Payload)
{
	bIsAttacking = true;
	DamagedTargets.Empty();
	GetWorld()->GetTimerManager().SetTimer(AttackTraceTimer, this, &UBossGameplayAbility::ApplyDamageToTarget, 0.1f, true);
}

void UBossGameplayAbility::OnAttackEndEvent(FGameplayEventData Payload)
{
	bIsAttacking = false;
	GetWorld()->GetTimerManager().ClearTimer(AttackTraceTimer);
}

void UBossGameplayAbility::ApplyDamageToTarget()
{
	if (!bIsAttacking) return;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return;

	FVector Center = Avatar->GetActorLocation();
	float Radius = 300.0f;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Avatar);

	bool bHit = GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), Params);

	if (bHit)
	{
		for (const FOverlapResult& Result : Overlaps)
		{
			AActor* Victim = Result.GetActor();
			if (!Victim || Victim == Avatar) continue;
			if (DamagedTargets.Contains(Victim)) continue;

			IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Victim);
			if (ASCInterface)
			{
				UAbilitySystemComponent* TargetASC = ASCInterface->GetAbilitySystemComponent();
				if (TargetASC)
				{
					// 无敌检查
					if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.Dodging"))))
						continue;

					if (DamageEffectClass)
					{
						FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
						GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectToTarget(
							DamageEffectClass.GetDefaultObject(), TargetASC, 1.0f, EffectContext);
						DamagedTargets.Add(Victim);
					}
				}
			}
		}
	}
}

void UBossGameplayAbility::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UBossGameplayAbility::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UBossGameplayAbility::SetIgnorePlayerCollision(bool bIgnore)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!Avatar || !Player) return;

	if (UCapsuleComponent* BossCapsule = Avatar->FindComponentByClass<UCapsuleComponent>())
	{
		if (bIgnore)
		{
			// 双向忽略，确保不会踩头
			BossCapsule->MoveIgnoreActors.AddUnique(Player);
			if (UCapsuleComponent* PlayerCapsule = Player->GetCapsuleComponent())
			{
				BossCapsule->IgnoreComponentWhenMoving(PlayerCapsule, true);
			}
		}
		else
		{
			BossCapsule->MoveIgnoreActors.Remove(Player);
			if (UCapsuleComponent* PlayerCapsule = Player->GetCapsuleComponent())
			{
				BossCapsule->IgnoreComponentWhenMoving(PlayerCapsule, false);
			}
		}
	}
}