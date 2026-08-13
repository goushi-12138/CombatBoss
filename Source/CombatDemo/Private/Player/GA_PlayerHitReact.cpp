#include "Player/GA_PlayerHitReact.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"

UGA_PlayerHitReact::UGA_PlayerHitReact()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 能力自身标签：用于"受击事件触发前先取消自己 → 重新播放新方向蒙太奇"
	// （GAS 在 TriggerAbilityFromGameplayEvent 时，会先取消 CancelAbilitiesWithTag 中
	//   匹配的所有正在运行的能力，包括本能力自身）
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.HitReact")));
	SetAssetTags(TagContainer);

	// 受击时打断玩家当前连招
	CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.Combo")));

	// 受击时先取消正在播放的受击GA自身 → 旋转挥砍连续命中时可每次重播最新方向
	CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Player.HitReact")));

	// 注册 6 个受击触发事件（AbilityTrigger）
	const FName EventNames[] = {
		FName("Event.Player.Hit.Front"),
		FName("Event.Player.Hit.Back"),
		FName("Event.Player.Hit.Left"),
		FName("Event.Player.Hit.Right"),
		FName("Event.Player.Hit.DownFront"),
		FName("Event.Player.Hit.DownBack")
	};
	for (const FName& EventName : EventNames)
	{
		FAbilityTriggerData Trigger;
		Trigger.TriggerTag = FGameplayTag::RequestGameplayTag(EventName);
		Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(Trigger);
	}
}

void UGA_PlayerHitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 每次激活都重置防重入标记
	bIsEnding = false;

	// 根据触发事件标签选择蒙太奇
	const FGameplayTag EventTag = TriggerEventData ? TriggerEventData->EventTag : FGameplayTag();
	UAnimMontage* MontageToPlay = SelectMontage(EventTag);
	if (!MontageToPlay)
	{
		// 检查是否有注册触发事件（AbilityTriggers），帮助定位"玩家蓝图配置的是C++类而非蓝图子类"问题
		const bool bHasRegisteredTrigger = AbilityTriggers.ContainsByPredicate(
			[&EventTag](const FAbilityTriggerData& Data) { return Data.TriggerTag == EventTag; });
		UE_LOG(LogTemp, Warning, TEXT("[HitReact] ActivateAbility: SelectMontage returned NULL! EventTag=%s, bHasRegisteredTrigger=%d, bIsActive=%d"),
			*EventTag.ToString(), bHasRegisteredTrigger, IsActive());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[HitReact] Activated. EventTag=%s, Montage=%s, bIsActive=%d"),
		*EventTag.ToString(), *GetNameSafe(MontageToPlay), IsActive());

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HitReact] ActivateAbility: Avatar is not ACharacter! Avatar=%s"),
			*GetNameSafe(GetAvatarActorFromActorInfo()));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 播放受击蒙太奇
	// 注意：根运动由蒙太奇资产控制（需求：所有受击蒙太奇不开启 EnableRootMotion）。
	// bStopMovement=true 仅在播放前停止角色当前位移，播放期间不锁定玩家输入。
	ActiveMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay, 1.0f, NAME_None, true);
	ActiveMontageTask->OnCompleted.AddDynamic(this, &UGA_PlayerHitReact::OnMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UGA_PlayerHitReact::OnMontageCompleted);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UGA_PlayerHitReact::OnMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UGA_PlayerHitReact::OnMontageInterrupted);
	ActiveMontageTask->ReadyForActivation();
}

void UGA_PlayerHitReact::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_PlayerHitReact::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_PlayerHitReact::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// ===== 防重入保护 =====
	// 场景1：蒙太奇播完 → OnCompleted → EndAbility；EndAbility 内 EndTask 可能再次触发
	//        OnCancelled → OnMontageInterrupted → EndAbility（此处拦截）
	// 场景2：外部取消（新受击事件先 Cancel 自身）→ EndAbility → EndTask 触发
	//        OnInterrupted → EndAbility（此处拦截）
	// 保证蒙太奇结束后只调用一次真正的 EndAbility，避免互相调用造成堆栈溢出。
	if (bIsEnding)
		return;
	bIsEnding = true;

	// 清理蒙太奇任务（若仍在播放则停止并触发中断回调，但已被 bIsEnding 拦截）
	if (ActiveMontageTask && ActiveMontageTask->IsActive())
	{
		ActiveMontageTask->EndTask();
	}
	ActiveMontageTask = nullptr;

	UGameplayAbility::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UAnimMontage* UGA_PlayerHitReact::SelectMontage(const FGameplayTag& EventTag) const
{
	if (EventTag == FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Front")))
		return HitFront;
	if (EventTag == FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Back")))
		return HitBack;
	if (EventTag == FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Left")))
		return HitLeft;
	if (EventTag == FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.Right")))
		return HitRight;
	if (EventTag == FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.DownFront")))
		return DownFront;
	if (EventTag == FGameplayTag::RequestGameplayTag(FName("Event.Player.Hit.DownBack")))
		return DownBack;
	return nullptr;
}
