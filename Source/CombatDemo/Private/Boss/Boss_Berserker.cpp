#include "Boss/Boss_Berserker.h"
#include "Boss/BossAttributeSet.h"
#include "Boss/BossAIController.h"
#include "Boss/BossGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Conditions\StateTreeGameplayTagConditions.h>

ABoss_Berserker::ABoss_Berserker()
{
	PrimaryActorTick.bCanEverTick = true;

	// 创建 GAS 组件
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// 创建柱子组件，先不设置网格体，在编辑器中配置
	PillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarMesh"));
	PillarMesh->SetupAttachment(GetMesh());

    // 禁止角色自动朝向移动方向，因为我们要手动控制朝向
    GetCharacterMovement()->bOrientRotationToMovement = false;

    // 确保旋转只绕Z轴
    GetCharacterMovement()->bUseControllerDesiredRotation = false;
}

void ABoss_Berserker::BeginPlay()
{
    Super::BeginPlay();

    // 初始化 ASC
    AbilitySystemComponent->InitAbilityActorInfo(this, this);

    // 创建并注册属性集
    BossAttributeSet = const_cast<UBossAttributeSet*>(AbilitySystemComponent->GetSet<UBossAttributeSet>());
    if (!BossAttributeSet)
    {
        BossAttributeSet = NewObject<UBossAttributeSet>(this);
        AbilitySystemComponent->AddAttributeSetSubobject(BossAttributeSet);
    }

    // 初始化默认值
    AbilitySystemComponent->SetNumericAttributeBase(UBossAttributeSet::GetMaxHealthAttribute(), 1000.0f);
    AbilitySystemComponent->SetNumericAttributeBase(UBossAttributeSet::GetHealthAttribute(), 1000.0f);
    AbilitySystemComponent->SetNumericAttributeBase(UBossAttributeSet::GetPhaseAttribute(), 1.0f);

    // 将柱子附加到右手 Socket
    PillarMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("hand_r_Socket"));

    // 【新增】设置初始移动速度
    GetCharacterMovement()->MaxWalkSpeed = Phase1WalkSpeed;

    
    // 【新增】通知AI控制器初始阶段
    if (ABossAIController* AIC = Cast<ABossAIController>(GetController()))
    {
        AIC->SetPhase(true);
    }

    // 赋予初始技能
    if (AbilitySystemComponent)
    {
        for (TSubclassOf<UBossGameplayAbility> AbilityClass : InitialAbilities)
        {
            if (AbilityClass)
            {
                AbilitySystemComponent->GiveAbility(
                    FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this)
                );
            }
        }
    }
}

UAbilitySystemComponent* ABoss_Berserker::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

UBossAttributeSet* ABoss_Berserker::GetBossAttributeSet() const
{
    return BossAttributeSet;
}

void ABoss_Berserker::OnPhaseChanged()
{
    // 防止重复触发
    if (bPhaseTransitioned) return;
    bPhaseTransitioned = true;

    HandlePhaseTransition();
}

void ABoss_Berserker::HandlePhaseTransition()
{
    SetMovementSpeedForPhase(2);

    // 更新黑板阶段
    if (ABossAIController* AIC = Cast<ABossAIController>(GetController()))
    {
        AIC->SetPhase(false);
    }
    // 【强制激活转阶段 GA】
    if (AbilitySystemComponent)
    {
        FGameplayTagContainer TagContainer; // 修复：定义 TagContainer
        TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("Event.Boss.PhaseTransition")));
        AbilitySystemComponent->TryActivateAbilitiesByTag(TagContainer);
    }
}

void ABoss_Berserker::SetMovementSpeedForPhase(int32 NewPhase)
{
    if (NewPhase == 1)
    {
        GetCharacterMovement()->MaxWalkSpeed = Phase1WalkSpeed;
    }
    else
    {
        GetCharacterMovement()->MaxWalkSpeed = Phase2RunSpeed;
    }
}

void ABoss_Berserker::OnDeath()
{
    if (bIsDead) return;
    bIsDead = true;

    if (AbilitySystemComponent)
    {
        FGameplayTagContainer DeathTag;
        DeathTag.AddTag(FGameplayTag::RequestGameplayTag(FName("Event.Boss.Death")));
        AbilitySystemComponent->TryActivateAbilitiesByTag(DeathTag);
    }
}
