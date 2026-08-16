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

void UBossGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
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

	// 连续伤害（旋转挥砍）：使用更长的判定间隔；普通攻击保持 0.1s
	const float TickInterval = bContinuousDamage ? RepeatDamageInterval : 0.1f;
	GetWorld()->GetTimerManager().SetTimer(AttackTraceTimer, this, &UBossGameplayAbility::ApplyDamageToTarget, TickInterval, true);
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

						// ===== 新增：命中玩家后发送受击事件（播放受击蒙太奇）=====
						SendHitReactEvent(Victim);

						// 【盾牌格挡音效】玩家防御中（Status.Blocking）：
						// SendHitReactEvent 内部会跳过受击动画，这里改发盾牌被击事件，
						// 由玩家 GA_PlayerBlock 监听并在盾牌组件位置播放格挡音效。
						if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.Blocking"))))
						{
							FGameplayEventData BlockEvent;
							BlockEvent.Instigator = Avatar;
							BlockEvent.Target = Victim;
							TargetASC->HandleGameplayEvent(
								FGameplayTag::RequestGameplayTag(FName("Event.Player.BlockHit")), &BlockEvent);
						}

						// 连续伤害（旋转挥砍）：命中后立即从已伤害列表移除，
						// 使下一次伤害判定（RepeatDamageInterval 后）可再次命中同一目标 → 每tick受伤+受击
						if (bContinuousDamage)
						{
							DamagedTargets.Remove(Victim);
						}
						// ===== 新增结束 =====
					}
				}
			}
		}
	}
}

void UBossGameplayAbility::SendHitReactEvent(AActor* Victim)
{
	if (!Victim) return;

	IAbilitySystemInterface* ASCI = Cast<IAbilitySystemInterface>(Victim);
	UAbilitySystemComponent* TargetASC = ASCI ? ASCI->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossGA] SendHitReactEvent: Victim has no ASC! Victim=%s"), *GetNameSafe(Victim));
		return;
	}

	// 防御中/无敌中不播放受击（伤害照常施加，防御减伤逻辑可后续自行扩展）
	if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.Blocking"))))
		return;
	if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.Dodging"))))
		return;

	const FGameplayTag DirectionTag = GetHitReactDirectionTag(Victim);
	FGameplayEventData EventData;
	EventData.Instigator = GetAvatarActorFromActorInfo();
	EventData.Target = Victim;
	EventData.EventTag = DirectionTag;
	TargetASC->HandleGameplayEvent(DirectionTag, &EventData);

	UE_LOG(LogTemp, Log, TEXT("[BossGA] SendHitReactEvent -> Direction=%s, Victim=%s"),
		*DirectionTag.ToString(), *GetNameSafe(Victim));
}

FGameplayTag UBossGameplayAbility::GetHitReactDirectionTag(AActor* Victim) const
{
	// 1. 优先使用蓝图中配置的固定方向
	switch (HitReactDirection)
	{
	case EBossHitReactDirection::Front:
		return FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Front"));
	case EBossHitReactDirection::Back:
		return FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Back"));
	case EBossHitReactDirection::Left:
		return FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Left"));
	case EBossHitReactDirection::Right:
		return FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Right"));
	default:
		break;
	}

	// 2. FromBoss：根据 Boss→玩家 方向相对玩家朝向动态计算
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !Victim)
		return FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Front"));

	const FVector AttackDir = (Victim->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal2D();
	const FVector PlayerForward = Victim->GetActorForwardVector().GetSafeNormal2D();
	const FVector PlayerRight = Victim->GetActorRightVector().GetSafeNormal2D();

	const float DotF = FVector::DotProduct(AttackDir, PlayerForward);
	if (DotF > 0.3f)
		return FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Front"));  // 攻击来自前方 → 向后倒
	if (DotF < -0.3f)
		return FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Back"));   // 攻击来自后方 → 向前倒

	const float DotR = FVector::DotProduct(AttackDir, PlayerRight);
	return DotR > 0.0f
		? FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Left"))         // 攻击来自右方 → 向左倒
		: FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Right"));       // 攻击来自左方 → 向右倒
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
