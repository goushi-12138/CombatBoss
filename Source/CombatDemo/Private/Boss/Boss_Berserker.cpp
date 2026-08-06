#include "Boss/Boss_Berserker.h"
#include "Boss/BossAttributeSet.h"
#include "Boss/BossAIController.h"
#include "Boss/BossGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

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

    // 【新增】绑定属性变化回调
    BindAttributeChangeCallbacks();

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

void ABoss_Berserker::BindAttributeChangeCallbacks()
{
	if (AbilitySystemComponent && BossAttributeSet)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBossAttributeSet::GetHealthAttribute()).AddUObject(this, &ABoss_Berserker::HealthChanged);
	}
}

void ABoss_Berserker::HealthChanged(const FOnAttributeChangeData& Data)
{
	// 属性集里的 PostGameplayEffectExecute 已经处理了阶段切换，这里可以做一些 UI 或动画反馈
}

void ABoss_Berserker::HandlePhaseTransition()
{
    // 切换移动速度
    SetMovementSpeedForPhase(2);

    // 隐藏柱子（不销毁，留作碎片效果）
    if (PillarMesh)
    {
        PillarMesh->SetVisibility(false);
    }

    // 通知 AI 控制器更新黑板
    if (ABossAIController* AIC = Cast<ABossAIController>(GetController()))
    {
        AIC->SetPhase(false); // IsPhaseOne = false
    }

    // 可选：播放怒吼蒙太奇（如果不用GA处理的话）
    // 建议通过GA_PhaseTransition来处理动画
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
