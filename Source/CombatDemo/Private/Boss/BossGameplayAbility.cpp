#include "Boss/BossGameplayAbility.h"
#include "Boss/Boss_Berserker.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffect.h" // FGameplayModifierInfo / FScalableFloat（格挡减伤克隆GE用）
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h" // 解决 FOverlapResult 不完整类型报错
#include "Engine/EngineTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h" // LaunchCharacter 推开被格挡的玩家
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

			// ===== 攻击锥限制：AttackConeAngle < 360 时，只命中 Boss 前方该角度范围内的目标 =====
			// （左挥砍/右挥砍/二连砍/举砸 配 150 = 前方 ±75°；旋转连砍/跳劈保持 360 全向）
			if (AttackConeAngle > 0.0f && AttackConeAngle < 359.9f)
			{
				const FVector ToVictim = (Victim->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal2D();
				const FVector BossForward = Avatar->GetActorForwardVector().GetSafeNormal2D();
				const float Angle = FMath::RadiansToDegrees(
					FMath::Acos(FMath::Clamp(FVector::DotProduct(BossForward, ToVictim), -1.0f, 1.0f)));
				if (Angle > AttackConeAngle * 0.5f)
				{
					continue; // 目标在攻击锥外（Boss 身后），不造成伤害
				}
			}

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
						// ===== 盾牌格挡判定：玩家防御中 且 攻击来自玩家前方防御锥内 =====
						// 格挡成功：减伤70%（玩家侧属性集根据 Status.Blocking + 方向判定完成）
						//           + 不播受击 + 盾牌格挡音效 + 向后推开
						// 格挡无效（无防御/攻击来自背后）：全额伤害 + 正常播放受击动画
						// 注意：伤害GE这里按原样应用，减伤由玩家侧 PlayerAttributeSet 结算，
						// 不在此修改GE（避免依赖 UE5.7 中频繁变动的 GE 结构 API）。
						const bool bShieldBlocked = IsAttackBlockedByShield(Victim);

						FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponentFromActorInfo()->MakeEffectContext();
						GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectToTarget(
							DamageEffectClass.GetDefaultObject(), TargetASC, 1.0f, EffectContext);
						DamagedTargets.Add(Victim);

						if (bShieldBlocked)
						{
							// 格挡冲击：把玩家沿"背向Boss"方向推开（水平滑步一小段，表现冲击力）。
							// LaunchCharacter 给一个初速度，靠地面摩擦自然滑行停下。
							if (BlockPushSpeed > 0.0f)
							{
								if (ACharacter* PlayerChar = Cast<ACharacter>(Victim))
								{
									const FVector PushDir =
										(Victim->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal2D();
									PlayerChar->LaunchCharacter(PushDir * BlockPushSpeed, false, false);
								}
							}

							// 格挡成功：不播受击动画，发送盾牌被击事件（玩家侧播放格挡音效）
							FGameplayEventData BlockEvent;
							BlockEvent.Instigator = Avatar;
							BlockEvent.Target = Victim;
							TargetASC->HandleGameplayEvent(
								FGameplayTag::RequestGameplayTag(FName("Event.Player.BlockHit")), &BlockEvent);
						}
						else
						{
							// 无防御 或 攻击来自背后（格挡无效）：正常播放受击动画
							SendHitReactEvent(Victim);
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

bool UBossGameplayAbility::IsAttackBlockedByShield(AActor* Victim) const
{
	if (!Victim) return false;

	IAbilitySystemInterface* ASCI = Cast<IAbilitySystemInterface>(Victim);
	UAbilitySystemComponent* TargetASC = ASCI ? ASCI->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC) return false;

	// 玩家没举盾 → 无格挡
	if (!TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.Blocking"))))
		return false;

	// 防御锥 360（全向）→ 任意方向都算格挡
	if (ShieldDefenseConeAngle >= 359.9f)
		return true;

	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return false;

	// 格挡成功的语义：Boss 位于玩家"前方"（玩家面朝 Boss）。
	// 因此用"玩家 → Boss"方向 与 玩家 forward 的夹角判断：
	// 玩家正对 Boss → 两方向同向（Dot>0、夹角小）→ 格挡成功；
	// 玩家背对 Boss → 夹角大 → 格挡失败（来自身后的攻击防御无效）。
	const FVector ToBoss = (Avatar->GetActorLocation() - Victim->GetActorLocation()).GetSafeNormal2D();
	const FVector PlayerForward = Victim->GetActorForwardVector().GetSafeNormal2D();
	const float Angle = FMath::RadiansToDegrees(
		FMath::Acos(FMath::Clamp(FVector::DotProduct(ToBoss, PlayerForward), -1.0f, 1.0f)));

	return Angle <= ShieldDefenseConeAngle * 0.5f;
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

	// 无敌中不播放受击（伤害照常施加）。
	// 注意：防御（Status.Blocking）不再在这里跳过——格挡是否有效由调用方
	// ApplyDamageToTarget 通过 IsAttackBlockedByShield 判定：正面格挡 → 不调用本函数（改发盾牌音效）；
	// 背后攻击/无防御 → 调用本函数播放受击动画。
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
