// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FPSTeamTypes.generated.h"

/**
 * EFPSTeam
 * Team assignment for PVP matches.
 */
UENUM(BlueprintType)
enum class EFPSTeam : uint8
{
	None = 0	UMETA(DisplayName = "None"),
	TeamA = 1	UMETA(DisplayName = "Team A (Red)"),
	TeamB = 2	UMETA(DisplayName = "Team B (Blue)"),
};

/**
 * EFPSMatchState
 * State of the current PVP match.
 */
UENUM(BlueprintType)
enum class EFPSMatchState : uint8
{
	WaitingForPlayers	UMETA(DisplayName = "Waiting For Players"),
	Countdown			UMETA(DisplayName = "Countdown"),
	InProgress			UMETA(DisplayName = "In Progress"),
	RoundEnd			UMETA(DisplayName = "Round End"),
	GameOver			UMETA(DisplayName = "Game Over"),
};

/**
 * UFPSTeamStatics
 * Utility functions for team operations.
 */
UCLASS()
class FPS_API UFPSTeamStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Check if two teams are enemies */
	UFUNCTION(BlueprintPure, Category = "FPS|Team")
	static bool AreEnemies(EFPSTeam A, EFPSTeam B);

	/** Check if two teams are friendly */
	UFUNCTION(BlueprintPure, Category = "FPS|Team")
	static bool AreFriendly(EFPSTeam A, EFPSTeam B);

	/** Get display name for a team */
	UFUNCTION(BlueprintPure, Category = "FPS|Team")
	static FText GetTeamDisplayName(EFPSTeam Team);

	/** Get color for a team */
	UFUNCTION(BlueprintPure, Category = "FPS|Team")
	static FLinearColor GetTeamColor(EFPSTeam Team);

	/** Get the opposite team */
	UFUNCTION(BlueprintPure, Category = "FPS|Team")
	static EFPSTeam GetOppositeTeam(EFPSTeam Team);
};

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamChanged, EFPSTeam, NewTeam);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchStateChanged, EFPSMatchState, OldState, EFPSMatchState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamScoreChanged, EFPSTeam, Team, int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerScoreChanged, int32, OldScore, int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKillsChanged, int32, NewKills);
