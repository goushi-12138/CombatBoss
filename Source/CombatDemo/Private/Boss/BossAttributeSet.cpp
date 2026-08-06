#include "Boss/BossAttributeSet.h"
#include "Boss/Boss_Berserker.h"
#include "GameplayEffectExtension.h"

UBossAttributeSet::UBossAttributeSet()
{
	MaxHealth = 1000.0f;
	Health = 1000.0f;
	Phase = 1.0f;
}

void UBossAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        const float NewHealth = GetHealth();
        const float MaxH = GetMaxHealth();
        const float CurrentPhase = GetPhase();

        // 血量归零检查
        if (NewHealth <= 0.0f)
        {
            SetHealth(0.0f);
            // 触发死亡事件（可选）
            return;
        }

        // 阶段切换：血量低于50%且仍在一阶段
        if (CurrentPhase == 1.0f && NewHealth <= MaxH * 0.5f)
        {
            SetPhase(2.0f);

            // 通知Boss执行阶段切换
            if (AActor* Owner = GetOwningActor())
            {
                if (ABoss_Berserker* Boss = Cast<ABoss_Berserker>(Owner))
                {
                    Boss->OnPhaseChanged();
                }
            }

            // 【新增】向自身发送阶段切换GameplayEvent，用于触发转阶段GA
            if (AActor* Owner = GetOwningActor())
            {
                if (UAbilitySystemComponent* ASC = Owner->FindComponentByClass<UAbilitySystemComponent>())
                {
                    FGameplayEventData EventData;
                    ASC->HandleGameplayEvent(FGameplayTag::RequestGameplayTag(FName("Event.Boss.PhaseTransition")), &EventData);
                }
            }
        }
    }
}