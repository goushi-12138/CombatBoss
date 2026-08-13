#include "Boss/GA_LeapWithLand.h"
#include "Boss/Boss_Berserker.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"

UGA_LeapWithLand::UGA_LeapWithLand()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_LeapWithLand::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    ACharacter* Boss = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Boss)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.LeapAttack")));
    }

    // 跳跃开始时，允许角色脱离地面
    if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    }

    // 标记攻击状态
    if (ABoss_Berserker* Berserker = Cast<ABoss_Berserker>(Boss))
        Berserker->bIsAttacking = true;

    // 起跳时忽略玩家碰撞，防止踩头
    SetIgnorePlayerCollision(true);
    bCollisionIgnored = true;
    bHasLanded = false;

    // 播放跳跃蒙太奇（带根运动）
    if (LeapMontage)
    {
        UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, LeapMontage, 1.0f, NAME_None, true); // true = 应用根运动
        MontageTask->OnCompleted.AddDynamic(this, &UGA_LeapWithLand::OnMontageFinished);
        MontageTask->OnBlendOut.AddDynamic(this, &UGA_LeapWithLand::OnMontageFinished);
        MontageTask->OnInterrupted.AddDynamic(this, &UGA_LeapWithLand::OnMontageInterrupted);
        MontageTask->OnCancelled.AddDynamic(this, &UGA_LeapWithLand::OnMontageInterrupted);
        MontageTask->ReadyForActivation();
    }
    else
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // 等待落地事件（动画通知）
    UAbilityTask_WaitGameplayEvent* WaitLand = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
        this, FGameplayTag::RequestGameplayTag(FName("Event.Boss.Land")), nullptr, true, true);
    WaitLand->EventReceived.AddDynamic(this, &UGA_LeapWithLand::OnLandEvent);
    WaitLand->ReadyForActivation();
}

void UGA_LeapWithLand::OnLandEvent(FGameplayEventData EventData)
{
    if (bHasLanded) return; // 防止重复触发
    bHasLanded = true;

    // 落地时执行范围伤害与击倒
    ExecuteLandAOE();
}

void UGA_LeapWithLand::ExecuteLandAOE()
{
    AActor* Avatar = GetAvatarActorFromActorInfo();
    if (!Avatar) return;

    // 以Boss当前位置为落点中心
    FVector LandCenter = Avatar->GetActorLocation();

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(Avatar);

    bool bHit = GetWorld()->OverlapMultiByObjectType(
        Overlaps,
        LandCenter,
        FQuat::Identity,
        FCollisionObjectQueryParams(ECC_Pawn),
        FCollisionShape::MakeSphere(LandRadius),
        Params
    );

    if (bHit)
    {
        UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
        for (const FOverlapResult& Result : Overlaps)
        {
            AActor* Victim = Result.GetActor();
            if (!Victim) continue;

            IAbilitySystemInterface* ASCI = Cast<IAbilitySystemInterface>(Victim);
            if (ASCI)
            {
                UAbilitySystemComponent* TargetASC = ASCI->GetAbilitySystemComponent();
                if (TargetASC)
                {
                    // 直接伤害
                    if (DirectDamageEffect)
                    {
                        FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
                        SourceASC->ApplyGameplayEffectToTarget(DirectDamageEffect.GetDefaultObject(), TargetASC, 1.0f, Ctx);
                    }
                    // 击倒
                    if (StunEffect)
                    {
                        FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
                        SourceASC->ApplyGameplayEffectToTarget(StunEffect.GetDefaultObject(), TargetASC, 1.0f, Ctx);
                    }
                }
            }
        }
    }
}

void UGA_LeapWithLand::OnMontageFinished()
{
    // 蒙太奇正常播完，恢复碰撞并结束技能
    if (bCollisionIgnored)
    {
        SetIgnorePlayerCollision(false);
        bCollisionIgnored = false;
    }
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_LeapWithLand::OnMontageInterrupted()
{
    // 被打断时也要恢复碰撞
    if (bCollisionIgnored)
    {
        SetIgnorePlayerCollision(false);
        bCollisionIgnored = false;
    }
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_LeapWithLand::EndAbility(const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 安全清理
    if (bCollisionIgnored)
    {
        SetIgnorePlayerCollision(false);
        bCollisionIgnored = false;
    }

    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.LeapAttack")));
    }

    // 跳跃结束时，恢复行走模式
    if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
    {
        Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    }

    if (ABoss_Berserker* Berserker = Cast<ABoss_Berserker>(GetAvatarActorFromActorInfo()))
        Berserker->bIsAttacking = false;

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}