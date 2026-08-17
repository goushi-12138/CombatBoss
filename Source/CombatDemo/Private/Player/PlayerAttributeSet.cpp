#include "Player/PlayerAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemComponent.h"

UPlayerAttributeSet::UPlayerAttributeSet()
{
    InitMaxHealth(100.0f);
    InitHealth(100.0f);

    // 精力：最大 100，初始 100
    InitMaxStamina(100.0f);
    InitStamina(100.0f);
}

void UPlayerAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        // 【防御减伤】玩家举盾（Status.Blocking 标签）时只承受 30% 伤害。
        // 此时 GE 已把全额伤害扣进 Health，这里把多扣的 70% 返还 → 净效果 = 伤害 × 30%。
        const float AppliedDelta = Data.EvaluatedData.Magnitude;
        if (AppliedDelta < 0.0f)
        {
            if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
            {
                if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.Blocking"))))
                {
                    SetHealth(GetHealth() - AppliedDelta * 0.7f); // 返还 70%
                    UE_LOG(LogTemp, Log, TEXT("[PlayerAttr] Blocked! Damage %.1f -> %.1f (70%% reduced)"),
                        -AppliedDelta, -AppliedDelta * 0.3f);
                }
            }
        }

        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
    }
}