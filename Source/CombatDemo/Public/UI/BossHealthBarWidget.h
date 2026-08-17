#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BossHealthBarWidget.generated.h"

class UProgressBar;
class ABoss_Berserker;

/**
 * Boss 血条（屏幕上方中央）：
 * - 每帧从 Boss 的 ASC 读取 Health/MaxHealth 更新进度条；
 * - 玩家与 Boss 距离 <= ShowDistance 时渐显，超出后渐隐（FadeSpeed 控制速度）。
 * 在编辑器中创建 Widget Blueprint 子类并放置同名 ProgressBar（BossHealthBar）。
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

private:
	TWeakObjectPtr<ABoss_Berserker> CachedBoss;
	float CurrentOpacity = 0.0f;
};
