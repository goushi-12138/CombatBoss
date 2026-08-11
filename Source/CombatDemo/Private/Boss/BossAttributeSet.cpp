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
        if (bIsDead) return;

        const float NewHealth = GetHealth();
        const float MaxH = GetMaxHealth();
        const float CurrentPhase = GetPhase();

        if (NewHealth <= 0.0f)
        {
            bIsDead = true;
            SetHealth(0.0f);

            if (AActor* Owner = GetOwningActor())
            {
                if (ABoss_Berserker* Boss = Cast<ABoss_Berserker>(Owner))
                {
                    Boss->OnDeath();
                }
            }
            return;
        }

        // ½×¶ÎÇÐ»»
        if (CurrentPhase == 1.0f && NewHealth <= MaxH * 0.5f)
        {
            SetPhase(2.0f);

            if (AActor* Owner = GetOwningActor())
            {
                if (ABoss_Berserker* Boss = Cast<ABoss_Berserker>(Owner))
                {
                    Boss->OnPhaseChanged();
                }
            }

            if (AActor* Owner = GetOwningActor())
            {
                if (UAbilitySystemComponent* ASC = Owner->FindComponentByClass<UAbilitySystemComponent>())
                {
                    FGameplayEventData EventData;
                    ASC->HandleGameplayEvent(
                        FGameplayTag::RequestGameplayTag(FName("Event.Boss.PhaseTransition")), &EventData);
                }
            }
        }
    }
}