#include "UI/PlayerStatusWidget.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Player/PlayerAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "Components/ProgressBar.h"

void UPlayerStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn) return;

	IAbilitySystemInterface* ASCI = Cast<IAbilitySystemInterface>(Pawn);
	UAbilitySystemComponent* ASC = ASCI ? ASCI->GetAbilitySystemComponent() : nullptr;
	if (!ASC) return;

	// 血条
	const float Health = ASC->GetNumericAttribute(UPlayerAttributeSet::GetHealthAttribute());
	const float MaxHealth = ASC->GetNumericAttribute(UPlayerAttributeSet::GetMaxHealthAttribute());
	if (HealthBar && MaxHealth > 0.0f)
	{
		HealthBar->SetPercent(FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f));
	}

	// 精力条
	const float Stamina = ASC->GetNumericAttribute(UPlayerAttributeSet::GetStaminaAttribute());
	const float MaxStamina = ASC->GetNumericAttribute(UPlayerAttributeSet::GetMaxStaminaAttribute());
	if (StaminaBar && MaxStamina > 0.0f)
	{
		StaminaBar->SetPercent(FMath::Clamp(Stamina / MaxStamina, 0.0f, 1.0f));
	}
}
