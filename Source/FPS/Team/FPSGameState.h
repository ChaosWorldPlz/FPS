// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "FPSTeamTypes.h"
#include "FPSGameState.generated.h"

/**
 * AFPSGameState
 *
 * Replicated game state for PVP matches.
 * Tracks match state, team scores, and match timer.
 */
UCLASS()
class FPS_API AFPSGameState : public AGameState
{
	GENERATED_BODY()

public:
	AFPSGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//-------------------------------------------------------------------
	// Match State
	//-------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "FPS|Match")
	EFPSMatchState GetFPSMatchState() const { return FPSMatchState; }

	/** Set match state (server only) */
	void SetMatchState(EFPSMatchState NewState);

	//-------------------------------------------------------------------
	// Team Scores
	//-------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "FPS|Match")
	int32 GetTeamAScore() const { return TeamAScore; }

	UFUNCTION(BlueprintPure, Category = "FPS|Match")
	int32 GetTeamBScore() const { return TeamBScore; }

	/** Get score for a specific team */
	UFUNCTION(BlueprintPure, Category = "FPS|Match")
	int32 GetTeamScore(EFPSTeam Team) const;

	/** Add score for a team (server only) */
	void AddTeamScore(EFPSTeam Team, int32 Amount);

	/** Get the team that is currently winning */
	UFUNCTION(BlueprintPure, Category = "FPS|Match")
	EFPSTeam GetWinningTeam() const;

	//-------------------------------------------------------------------
	// Timer
	//-------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "FPS|Match")
	float GetMatchTimeRemaining() const { return MatchTimeRemaining; }

	/** Set time remaining (server only) */
	void SetMatchTimeRemaining(float Time);

	/** Get formatted time string (MM:SS) */
	UFUNCTION(BlueprintPure, Category = "FPS|Match")
	FString GetFormattedTimeRemaining() const;

	//-------------------------------------------------------------------
	// Configuration
	//-------------------------------------------------------------------

	/** Score needed to win */
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Match|Config")
	int32 ScoreToWin;

	/** Match time limit in seconds */
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Match|Config")
	int32 MatchTimeLimitSeconds;

	//-------------------------------------------------------------------
	// Delegates
	//-------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnMatchStateChanged OnMatchStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnTeamScoreChanged OnTeamScoreChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_FPSMatchState)
	EFPSMatchState FPSMatchState;

	UPROPERTY(ReplicatedUsing = OnRep_TeamAScore)
	int32 TeamAScore;

	UPROPERTY(ReplicatedUsing = OnRep_TeamBScore)
	int32 TeamBScore;

	UPROPERTY(Replicated)
	float MatchTimeRemaining;

	UFUNCTION()
	void OnRep_FPSMatchState();

	UFUNCTION()
	void OnRep_TeamAScore();

	UFUNCTION()
	void OnRep_TeamBScore();

private:
	EFPSMatchState PreviousFPSMatchState;
};
