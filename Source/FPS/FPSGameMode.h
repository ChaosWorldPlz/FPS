// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Team/FPSTeamTypes.h"
#include "FPSGameMode.generated.h"

class AFPSPlayerState;
class AFPSGameState;
class AFPSCharacter;
class AFPSExtractionZone;
class UFPSMenuSubsystem;

/**
 * AFPSGameMode
 *
 * PVP game mode with team assignment, respawning, scoring, and win conditions.
 * Server-authoritative: all game logic runs on the server.
 */
UCLASS(minimalapi)
class AFPSGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AFPSGameMode();

	//-------------------------------------------------------------------
	// Match Configuration
	//-------------------------------------------------------------------

	/** Delay before respawning after death */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPS|Match")
	float RespawnDelay;

	/** Score awarded per kill */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPS|Match")
	int32 KillScore;

	/** Countdown duration before match starts */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPS|Match")
	float CountdownDuration;

	/** Minimum players to start match */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPS|Match")
	int32 MinPlayersToStart;

	/** Time window for assist eligibility (seconds before death) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPS|Match")
	float AssistTimeWindow = 10.0f;

	/** Minimum damage threshold for assist (fraction of MaxHealth, 0-1) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "FPS|Match")
	float AssistDamageThreshold = 0.2f;

	//-------------------------------------------------------------------
	// Team Assignment
	//-------------------------------------------------------------------

	/** Choose team for a new player (auto-balance) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Team")
	EFPSTeam ChooseTeamForPlayer(AController* Player);

	/** Get number of players on a team */
	UFUNCTION(BlueprintPure, Category = "FPS|Team")
	int32 GetTeamPlayerCount(EFPSTeam Team) const;

	//-------------------------------------------------------------------
	// Kill / Death / Score
	//-------------------------------------------------------------------

	/** Handle a player death - update stats, check win, schedule respawn */
	UFUNCTION(BlueprintCallable, Category = "FPS|Match")
	void HandlePlayerDeath(AFPSPlayerState* Victim, AFPSPlayerState* Killer);

	/** Respawn a player at their team's spawn point */
	UFUNCTION(BlueprintCallable, Category = "FPS|Match")
	void RespawnPlayer(AController* Controller);

	//-------------------------------------------------------------------
	// Win Condition
	//-------------------------------------------------------------------

	/** Check if a team has won */
	void CheckWinCondition();

	/** End the match with a winner */
	void EndMatch(EFPSTeam Winner);

	//-------------------------------------------------------------------
	// Utility
	//-------------------------------------------------------------------

	/** Get typed game state */
	UFUNCTION(BlueprintPure, Category = "FPS|Match")
	AFPSGameState* GetFPSGameState() const;

	/** Called when players successfully extract from a zone */
	UFUNCTION(BlueprintCallable, Category = "FPS|Match")
	void OnPlayerExtracted(AFPSExtractionZone* ExtractionZone);

	/** Get the menu subsystem */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	UFPSMenuSubsystem* GetMenuSubsystem() const;

protected:
	//-------------------------------------------------------------------
	// AGameMode Overrides
	//-------------------------------------------------------------------

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void HandleMatchIsWaitingToStart() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;
	virtual bool ReadyToStartMatch_Implementation() override;

	/** Start the countdown phase */
	void StartCountdown();

	/** Called when countdown finishes */
	void OnCountdownFinished();

	/** Update match timer */
	void UpdateMatchTimer(float DeltaTime);

	/** Handle match time expiry */
	void OnMatchTimeExpired();

	/** Set match state on our game state */
	void SetFPSMatchState(EFPSMatchState NewState);

	/** Timer handles */
	FTimerHandle CountdownTimerHandle;

	/** Pending respawn timers */
	TMap<AController*, FTimerHandle> PendingRespawns;
};
