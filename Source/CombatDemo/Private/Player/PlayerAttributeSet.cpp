#include "Player/PlayerAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemComponent.h"
#include "Boss/Boss_Berserker.h"
#include "Kismet/GameplayStatics.h"

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
        // 【防御减伤】玩家举盾（Status.Blocking）且攻击来自玩家前方（Boss 在玩家前方 ±75° 内）
        // 时才只承受 30% 伤害；来自身后的攻击格挡无效 → 全额伤害。
        // 此时 GE 已把全额伤害扣进 Health，这里把多扣的 70% 返还 → 净效果 = 伤害 × 30%。
        const float AppliedDelta = Data.EvaluatedData.Magnitude;
        if (AppliedDelta < 0.0f)
        {
            if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
            {
                if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Status.Blocking")))
                    && IsAttackFromFront())
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

bool UPlayerAttributeSet::IsAttackFromFront() const
{
    AActor* Owner = GetOwningActor();
    if (!Owner)
    {
        return true; // 拿不到玩家时默认按正面格挡处理
    }

    // 全局查找 Boss（单机场景一个 Boss）
    ABoss_Berserker* Boss = Cast<ABoss_Berserker>(
        UGameplayStatics::GetActorOfClass(Owner->GetWorld(), ABoss_Berserker::StaticClass()));
    if (!Boss)
    {
        return true; // 找不到 Boss 时默认按正面格挡处理
    }

    // Boss 位于玩家前方（玩家面朝 Boss）才算正面格挡，与 Boss 侧 ShieldDefenseConeAngle=150 对应（±75°）
    const FVector ToBoss = (Boss->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal2D();
    const FVector PlayerForward = Owner->GetActorForwardVector().GetSafeNormal2D();
    const float Angle = FMath::RadiansToDegrees(
        FMath::Acos(FMath::Clamp(FVector::DotProduct(ToBoss, PlayerForward), -1.0f, 1.0f)));

    return Angle <= 75.0f;
}