#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatusWidget.generated.h"

class UProgressBar;

/**
 * 玩家状态 HUD（屏幕左下方）：
 * - HealthBar  血条（上方）
 * - StaminaBar 精力条（下方，翻滚消耗/每秒恢复）
 * 每帧从玩家 ASC 读取 Health/MaxHealth、Stamina/MaxStamina 更新进度条。
 * 在编辑器中创建 Widget Blueprint 子类并放置两个同名 ProgressBar（BindWidget）。
 */
UCLASS()
class COMBATDEMO_API UPlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* StaminaBar;
};
