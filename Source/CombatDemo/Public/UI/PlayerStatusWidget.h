#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerStatusWidget.generated.h"

class UProgressBar;

/**
 * 玩家状态 HUD（屏幕左下方）：
 * - HealthBar  红色血条（上层，受伤立即减少）
 * - DelayBar   白色延迟血条（下层，受伤后延迟0.3s平滑追赶到红条）——白条跟扣效果
 * - StaminaBar 精力条（下方，翻滚消耗/每秒恢复）
 * 每帧从玩家 ASC 读取 Health/MaxHealth、Stamina/MaxStamina 更新。
 * 在编辑器中创建 Widget Blueprint 子类并放置同名 ProgressBar（BindWidget）。
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

	UPROPERTY(meta = (BindWidget))
	UProgressBar* DelayBar;

	// ===== 延迟白条参数（蓝图 Class Defaults 可调）=====
	/** 受伤后白条静止时长（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "DelayedBar")
	float WhiteBarDelay = 0.3f;

	/** 白条追赶红条的插值速度（FInterpTo，越大越快） */
	UPROPERTY(EditDefaultsOnly, Category = "DelayedBar")
	float WhiteBarInterpSpeed = 2.3f;

	// ===== 延迟白条状态（跨帧保留）=====
	float WhiteBarPercent = 1.0f;
	float LastRedPercent = 1.0f;
	float DelayRemaining = 0.0f;
};
