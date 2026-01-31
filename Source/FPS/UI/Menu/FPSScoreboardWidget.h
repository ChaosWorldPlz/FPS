// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPS/Team/FPSTeamTypes.h"
#include "FPSScoreboardWidget.generated.h"

class AFPSGameState;

/**
 * FFPSPlayerScoreInfo
 * Snapshot of a single player's score data for UI display.
 */
USTRUCT(BlueprintType)
struct FFPSPlayerScoreInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Scoreboard")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Scoreboard")
	EFPSTeam Team = EFPSTeam::None;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Scoreboard")
	int32 Kills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Scoreboard")
	int32 Deaths = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Scoreboard")
	int32 Assists = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Scoreboard")
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Scoreboard")
	int32 Ping = 0;
};

/**
 * UFPSScoreboardWidget
 *
 * C++ base class for the scoreboard widget.
 * Provides data query interface; actual UI rendering is done in Lua.
 */
UCLASS(Blueprintable)
class FPS_API UFPSScoreboardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Get score info for all connected players */
	UFUNCTION(BlueprintCallable, Category = "FPS|Scoreboard")
	TArray<FFPSPlayerScoreInfo> GetAllPlayerStats() const;

	/** Get team score */
	UFUNCTION(BlueprintCallable, Category = "FPS|Scoreboard")
	int32 GetTeamScore(EFPSTeam Team) const;

	/** Get remaining match time */
	UFUNCTION(BlueprintCallable, Category = "FPS|Scoreboard")
	float GetMatchTimeRemaining() const;

	/** Get current match state */
	UFUNCTION(BlueprintCallable, Category = "FPS|Scoreboard")
	EFPSMatchState GetMatchState() const;

	/** Get formatted time remaining (MM:SS) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Scoreboard")
	FString GetFormattedTimeRemaining() const;

protected:
	/** Get the FPS game state */
	AFPSGameState* GetFPSGameState() const;
};
