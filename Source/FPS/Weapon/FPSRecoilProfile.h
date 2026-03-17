// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FPSRecoilProfile.generated.h"

/**
 * UFPSRecoilProfile
 *
 * 纯数据资产，描述一把武器的后坐力表现。
 * 挂在 UFPSWeaponDataAsset::RecoilProfile 字段上。
 * 运行时由 UFPSRecoilComponent 消费。
 */
UCLASS(BlueprintType)
class FPS_API UFPSRecoilProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	//-------------------------------------------------------------------
	// Pattern（弹道图案）
	//-------------------------------------------------------------------

	/**
	 * 弹道偏移序列，每个元素对应一发子弹。
	 * X = Yaw 偏移（度，正值向右）
	 * Y = Pitch 偏移（度，正值向上）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil|Pattern")
	TArray<FVector2D> RecoilPattern;

	/** 停止射击多久后 PatternIndex 归零（秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil|Pattern",
		meta = (ClampMin = "0.05"))
	float PatternResetTime = 0.4f;

	//-------------------------------------------------------------------
	// Spread（离散分布）
	//-------------------------------------------------------------------

	/** 静止时的底噪散布（度） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil|Spread",
		meta = (ClampMin = "0"))
	float BaseSpread = 0.3f;

	/** 每发增加的散布（度） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil|Spread",
		meta = (ClampMin = "0"))
	float SpreadIncreasePerShot = 0.5f;

	/** 散布上限（度） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil|Spread",
		meta = (ClampMin = "0"))
	float MaxSpread = 5.0f;

	/** 停止射击后每秒散布恢复量（度/秒） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil|Spread",
		meta = (ClampMin = "0"))
	float SpreadRecoveryRate = 8.0f;

	/**
	 * true  = 高斯分布（中心密集、边缘稀疏，更真实）
	 * false = 均匀圆锥
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil|Spread")
	bool bUseGaussianSpread = true;
};
