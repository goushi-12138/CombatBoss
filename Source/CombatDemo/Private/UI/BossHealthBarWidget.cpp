#include "UI/BossHealthBarWidget.h"
#include "UI/DelayedBarHelper.h"
#include "Boss/Boss_Berserker.h"
#include "Boss/BossAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Components/ProgressBar.h"

void UBossHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APawn* PlayerPawn = GetOwningPlayerPawn();
	if (!PlayerPawn) return;

	// 查找 Boss（缓存，防止每帧查找）
	if (!CachedBoss.IsValid())
	{
		CachedBoss = Cast<ABoss_Berserker>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ABoss_Berserker::StaticClass()));
	}
	ABoss_Berserker* Boss = CachedBoss.Get();
	if (!Boss) return;

	// 更新血量（红条立即更新）+ 延迟白条（受伤后延迟追赶到红条）
	if (UAbilitySystemComponent* BossASC = Boss->GetAbilitySystemComponent())
	{
		const float Health = BossASC->GetNumericAttribute(UBossAttributeSet::GetHealthAttribute());
		const float MaxHealth = BossASC->GetNumericAttribute(UBossAttributeSet::GetMaxHealthAttribute());
		if (MaxHealth > 0.0f)
		{
			const float RedPercent = FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f);
			if (BossHealthBar)
			{
				BossHealthBar->SetPercent(RedPercent);
			}

			if (DelayBar)
			{
				const float WhitePercent = DelayedBarHelper::Tick(
					WhiteBarPercent, LastRedPercent, DelayRemaining,
					RedPercent, InDeltaTime, WhiteBarDelay, WhiteBarInterpSpeed);
				DelayBar->SetPercent(WhitePercent);
			}
		}
	}

	// 距离显隐：<= ShowDistance 渐显，超出渐隐
	const float Dist = FVector::Dist(PlayerPawn->GetActorLocation(), Boss->GetActorLocation());
	const float TargetOpacity = (Dist <= ShowDistance) ? 1.0f : 0.0f;
	CurrentOpacity = FMath::FInterpTo(CurrentOpacity, TargetOpacity, InDeltaTime, FadeSpeed);
	SetRenderOpacity(CurrentOpacity);
}
