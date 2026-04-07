// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FPSLevelFlowTypes.generated.h"

/**
 * EFPSRaidState
 *
 * Current state of the raid/level.
 */
UENUM(BlueprintType)
enum class EFPSRaidState : uint8
{
	WaitingToStart	UMETA(DisplayName = "Waiting to Start"),
	InProgress		UMETA(DisplayName = "In Progress"),
	Extracting		UMETA(DisplayName = "Extracting"),
	Extracted		UMETA(DisplayName = "Extracted"),
	Failed			UMETA(DisplayName = "Failed"),
	TimedOut		UMETA(DisplayName = "Timed Out")
};

/**
 * EFPSExtractionState
 *
 * State of an extraction zone.
 */
UENUM(BlueprintType)
enum class EFPSExtractionState : uint8
{
	Inactive	UMETA(DisplayName = "Inactive"),
	Available	UMETA(DisplayName = "Available"),
	InProgress	UMETA(DisplayName = "In Progress"),
	Complete	UMETA(DisplayName = "Complete")
};

/**
 * FFPSRaidResult
 *
 * Result data for a completed raid.
 */
USTRUCT(BlueprintType)
struct FFPSRaidResult
{
	GENERATED_BODY()

	/** Whether the raid was successful */
	UPROPERTY(BlueprintReadOnly)
	bool bSuccess = false;

	/** Final raid state */
	UPROPERTY(BlueprintReadOnly)
	EFPSRaidState FinalState = EFPSRaidState::Failed;

	/** Time spent in raid (seconds) */
	UPROPERTY(BlueprintReadOnly)
	float TimeSpent = 0.0f;

	/** Time remaining when extracted (0 if failed) */
	UPROPERTY(BlueprintReadOnly)
	float TimeRemaining = 0.0f;

	/** Damage dealt */
	UPROPERTY(BlueprintReadOnly)
	float DamageDealt = 0.0f;

	/** Damage taken */
	UPROPERTY(BlueprintReadOnly)
	float DamageTaken = 0.0f;

	/** Kills */
	UPROPERTY(BlueprintReadOnly)
	int32 Kills = 0;

	/** Experience gained */
	UPROPERTY(BlueprintReadOnly)
	int32 ExperienceGained = 0;
};

/**
 * FFPSMapInfoRow
 *
 * DataTable 行结构，描述一张可进入的地图。
 * 在编辑器里创建 DataTable（行类型选此结构），Lua 通过 ItemDataManager 同样的方式查询。
 */
USTRUCT(BlueprintType)
struct FFPSMapInfoRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 地图显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FText DisplayName;

	/** 地图描述 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FText Description;

	/** UE 关卡路径，如 /Game/Maps/Map_Factory */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FSoftObjectPath LevelPath;

	/** 缩略图 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TSoftObjectPtr<UTexture2D> PreviewImage;

	/** 最大玩家数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	int32 MaxPlayers = 10;

	/** 地图难度（1~5） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map", meta = (ClampMin = 1, ClampMax = 5))
	int32 Difficulty = 1;

	/** 预计时长（分钟） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	int32 DurationMinutes = 30;

	/** 是否在选图界面显示 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	bool bEnabled = true;
};

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRaidStateChanged, EFPSRaidState, OldState, EFPSRaidState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaidTimeUpdated, float, TimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaidCompleted, const FFPSRaidResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExtractionProgress, float, CurrentProgress, float, RequiredTime);
