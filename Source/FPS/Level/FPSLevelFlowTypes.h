// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRaidStateChanged, EFPSRaidState, OldState, EFPSRaidState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaidTimeUpdated, float, TimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRaidCompleted, const FFPSRaidResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExtractionProgress, float, CurrentProgress, float, RequiredTime);
