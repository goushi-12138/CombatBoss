#pragma once

#include "CoreMinimal.h"

// ===== 延迟血条（白条跟扣）更新辅助：供玩家/Boss 血条 Widget 复用 =====
namespace DelayedBarHelper
{
	/**
	 * 每帧调用，返回新的白条百分比（0~1）。
	 * 参数：
	 * - InOutWhitePercent    ：白条当前值（成员变量，跨帧保留）
	 * - InOutLastRedPercent  ：上一帧红条值（用于检测"新受伤"以重置延迟）
	 * - InOutDelayRemaining  ：受伤后白条静止的延迟剩余时间（跨帧保留）
	 * - RedPercent           ：本帧红条值（真实血量百分比）
	 * - Delay                ：受伤后白条静止时长（秒，默认0.3）
	 * - InterpSpeed          ：白条追赶红条的插值速度（FInterpTo，越大越快，默认2.3）
	 */
	inline float Tick(float& InOutWhitePercent, float& InOutLastRedPercent, float& InOutDelayRemaining,
		float RedPercent, float DeltaTime, float Delay, float InterpSpeed)
	{
		// 检测"新受伤"（红条比上一帧下降）→ 重置延迟计时（白条先静止 Delay 秒）
		if (RedPercent < InOutLastRedPercent - 0.0001f)
		{
			InOutDelayRemaining = FMath::Max(InOutDelayRemaining, Delay);
		}
		InOutLastRedPercent = RedPercent;

		if (InOutWhitePercent > RedPercent)
		{
			// 受伤：白条在前，延迟结束后平滑追赶到红条
			if (InOutDelayRemaining > 0.0f)
			{
				InOutDelayRemaining -= DeltaTime;
			}
			else if (InOutWhitePercent - RedPercent <= 0.001f)
			{
				InOutWhitePercent = RedPercent; // 已追到目标，停止更新
			}
			else
			{
				InOutWhitePercent = FMath::FInterpTo(InOutWhitePercent, RedPercent, DeltaTime, InterpSpeed);
			}
		}
		else if (InOutWhitePercent < RedPercent)
		{
			// 回血/治疗：白条立即同步，不越过红条
			InOutWhitePercent = RedPercent;
		}

		return InOutWhitePercent;
	}
}
