#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossHealthBarWidget.generated.h"

class UProgressBar;
class ABoss_Berserker;

/**
 * Boss 血条（屏幕上方中央）：
 * - BossHealthBar 红色血条（上层，受伤立即减少）
 * - DelayBar      白色延迟血条（下层，受伤后延迟0.3s平滑追赶到红条）——白条跟扣效果
 * - 玩家与 Boss 距离 <= ShowDistance 时渐显，超出后渐隐（FadeSpeed 控制速度）。
 * 在编辑器中创建 Widget Blueprint 子类并放置同名 ProgressBar（BossHealthBar / DelayBar）。
 */
UCLASS()
class COMBATDEMO_API UBossHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 显示距离：玩家与Boss距离小于等于该值时显示血条（默认3600） */
	UPROPERTY(EditDefaultsOnly, Category = "BossBar")
	float ShowDistance = 3600.0f;

	/** 渐隐/渐显速度（每秒透明度变化量） */
	UPROPERTY(EditDefaultsOnly, Category = "BossBar")
	float FadeSpeed = 4.0f;

protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* BossHealthBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* DelayBar;

	// ===== 延迟白条参数（蓝图 Class Defaults 可调）=====
	/** 受伤后白条静止时长（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "DelayedBar")
	float WhiteBarDelay = 0.3f;

	/** 白条追赶红条的插值速度（FInterpTo，越大越快） */
	UPROPERTY(EditDefaultsOnly, Category = "DelayedBar")
	float WhiteBarInterpSpeed = 2.3f;

private:
	TWeakObjectPtr<ABoss_Berserker> CachedBoss;
	float CurrentOpacity = 0.0f;

	// ===== 延迟白条状态（跨帧保留）=====
	float WhiteBarPercent = 1.0f;
	float LastRedPercent = 1.0f;
	float DelayRemaining = 0.0f;
};
