// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnLuaInterface.h"
#include "FPSRecoilComponent.generated.h"

class UFPSRecoilProfile;

/**
 * UFPSRecoilComponent
 *
 * 挂在 AFPSCharacter 上，管理后坐力运行时状态。
 * 武器开火时调用 OnShotFired()，取射击方向时调用 GetFireDirection()。
 *
 * 四个关键计算节点暴露为 BlueprintNativeEvent，Lua 可按武器特性重写。
 */
UCLASS(ClassGroup = "FPS", BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FPS_API UFPSRecoilComponent : public UActorComponent, public IUnLuaInterface
{
	GENERATED_BODY()

public:
	UFPSRecoilComponent();

	//-------------------------------------------------------------------
	// 对外接口（武器 / GA 调用）
	//-------------------------------------------------------------------

	/** 每发子弹调用一次，推进 Pattern 并累加 Spread */
	UFUNCTION(BlueprintCallable, Category = "Recoil")
	void OnShotFired(UFPSRecoilProfile* Profile);

	/**
	 * 取含后坐力偏移的最终射击方向。
	 * 在 GA_WeaponFire 执行 LineTrace / SpawnProjectile 前调用。
	 */
	UFUNCTION(BlueprintCallable, Category = "Recoil")
	FVector GetFireDirection(FVector BaseAimDirection);

	/** 换弹 / 切枪时强制 Pattern 归零 */
	UFUNCTION(BlueprintCallable, Category = "Recoil")
	void ForceResetPattern();

	//-------------------------------------------------------------------
	// 运行时状态（Lua 可读）
	//-------------------------------------------------------------------

	/** 当前 Pattern 数组位置 */
	UPROPERTY(BlueprintReadOnly, Category = "Recoil|State")
	int32 CurrentPatternIndex = 0;

	/** 当前散布半径（度） */
	UPROPERTY(BlueprintReadOnly, Category = "Recoil|State")
	float CurrentSpread = 0.0f;

	/** 本次连射总发数 */
	UPROPERTY(BlueprintReadOnly, Category = "Recoil|State")
	int32 TotalShotsFired = 0;

	/** 上次开枪时的 Spread 值（Lazy Evaluation 基准） */
	UPROPERTY(BlueprintReadOnly, Category = "Recoil|State")
	float SpreadAtLastShot = 0.0f;

	/** 上次开枪的时间戳 */
	UPROPERTY(BlueprintReadOnly, Category = "Recoil|State")
	float LastShotTime = -1.0f;


	//-------------------------------------------------------------------
	// Lua 热更接口（BlueprintNativeEvent）
	// C++ 提供合理默认实现，Lua 按需重写
	//-------------------------------------------------------------------

	/**
	 * 计算本发的散布半径。
	 * @param BaseSpread         Profile 中当前累计散布值
	 * @param PatternIndex       当前 Pattern 位置
	 * @param TimeSinceLastShot  距上次射击时间（秒）
	 * @param bIsAiming          是否处于 ADS 瞄准状态
	 * @return 最终散布角度（度）
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Recoil|Lua")
	float CalculateSpreadRadius(float BaseSpread, int32 PatternIndex,
		float TimeSinceLastShot, bool bIsAiming);

	/**
	 * 计算本发的 Pattern 偏移量（可完全替换数组查表逻辑）。
	 * @param RawPatternValue  数组中的原始值（Index 越界时为 ZeroVector）
	 * @param PatternIndex     当前索引
	 * @param TotalShotsFired  本次连射总发数
	 * @return FVector2D(Yaw偏移°, Pitch偏移°)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Recoil|Lua")
	FVector2D CalculatePatternOffset(FVector2D RawPatternValue,
		int32 PatternIndex, int32 TotalShotsFired);

	/**
	 * Pattern 走到末尾时，决定下一发使用的 Index。
	 * @param TotalShotsFired  本次连射总发数
	 * @param PatternLength    Pattern 数组长度
	 * @return 下一个 PatternIndex
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Recoil|Lua")
	int32 OnPatternFinished(int32 TotalShotsFired, int32 PatternLength);

	/**
	 * Timer 到期时调用，决定是否真正归零 Pattern。
	 * Lua 可加额外条件（如正在射击时阻止归零）。
	 * @param PatternResetTime  Profile 中配置的重置时间（秒），供参考
	 * @return true = 执行归零
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Recoil|Lua")
	bool ShouldResetPattern(float PatternResetTime);

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** 当前激活的 Profile（每次 OnShotFired 更新） */
	UPROPERTY()
	TObjectPtr<UFPSRecoilProfile> ActiveProfile;

	/** 按时间戳现算当前 Spread（Lazy Evaluation） */
	float ComputeCurrentSpread() const;

	/** 停火后重置 Pattern 的计时器 */
	FTimerHandle PatternResetTimerHandle;

	/** 计时器回调：归零 Pattern */
	void ResetPattern();

	/** 生成高斯随机偏移（Box-Muller） */
	static float GaussianRandom();

	/** 将角度偏移叠加到方向向量上 */
	static FVector ApplyAngularOffset(FVector Direction, float PitchDeg, float YawDeg);
};
