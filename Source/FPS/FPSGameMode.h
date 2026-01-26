// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Level/FPSLevelFlowTypes.h"
#include "FPSGameMode.generated.h"

class AFPSExtractionZone;
class AFPSCharacter;

UCLASS(minimalapi)
class AFPSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFPSGameMode();

	//-------------------------------------------------------------------
	// Raid Configuration
	//-------------------------------------------------------------------

	/** Total raid time limit (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Raid")
	float RaidTimeLimit = 1200.0f; // 20 minutes

	/** Warning time before raid ends (seconds) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Raid")
	float RaidWarningTime = 300.0f; // 5 minutes

	/** Time to wait before starting raid after all players ready */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Raid")
	float RaidStartDelay = 5.0f;

	//-------------------------------------------------------------------
	// Raid State
	//-------------------------------------------------------------------

	/** Current raid state */
	UPROPERTY(BlueprintReadOnly, Category = "Raid|State")
	EFPSRaidState CurrentRaidState = EFPSRaidState::WaitingToStart;

	/** Time remaining in raid */
	UPROPERTY(BlueprintReadOnly, Category = "Raid|State")
	float RaidTimeRemaining = 0.0f;

	/** Raid result (set when raid ends) */
	UPROPERTY(BlueprintReadOnly, Category = "Raid|State")
	FFPSRaidResult RaidResult;

	//-------------------------------------------------------------------
	// Delegates
	//-------------------------------------------------------------------

	/** Called when raid state changes */
	UPROPERTY(BlueprintAssignable, Category = "Raid|Events")
	FOnRaidStateChanged OnRaidStateChanged;

	/** Called when raid time updates */
	UPROPERTY(BlueprintAssignable, Category = "Raid|Events")
	FOnRaidTimeUpdated OnRaidTimeUpdated;

	/** Called when raid completes */
	UPROPERTY(BlueprintAssignable, Category = "Raid|Events")
	FOnRaidCompleted OnRaidCompleted;

	//-------------------------------------------------------------------
	// Raid Interface
	//-------------------------------------------------------------------

	/** Start the raid */
	UFUNCTION(BlueprintCallable, Category = "Raid")
	void StartRaid();

	/** End the raid with a specific result */
	UFUNCTION(BlueprintCallable, Category = "Raid")
	void EndRaid(EFPSRaidState EndState);

	/** Called when a player successfully extracts */
	UFUNCTION(BlueprintCallable, Category = "Raid")
	void OnPlayerExtracted(AFPSExtractionZone* ExtractionZone);

	/** Called when a player dies */
	UFUNCTION(BlueprintCallable, Category = "Raid")
	void OnPlayerDied(AFPSCharacter* Player, AActor* Killer);

	/** Get formatted time remaining string (MM:SS) */
	UFUNCTION(BlueprintCallable, Category = "Raid")
	FString GetFormattedTimeRemaining() const;

	/** Check if in warning time */
	UFUNCTION(BlueprintCallable, Category = "Raid")
	bool IsInWarningTime() const { return RaidTimeRemaining <= RaidWarningTime; }

	//-------------------------------------------------------------------
	// Extraction Points
	//-------------------------------------------------------------------

	/** Get all extraction zones in the level */
	UFUNCTION(BlueprintCallable, Category = "Raid|Extraction")
	TArray<AFPSExtractionZone*> GetAllExtractionZones() const;

	/** Get active extraction zones */
	UFUNCTION(BlueprintCallable, Category = "Raid|Extraction")
	TArray<AFPSExtractionZone*> GetActiveExtractionZones() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Set raid state and broadcast */
	void SetRaidState(EFPSRaidState NewState);

	/** Update raid timer */
	void UpdateRaidTimer(float DeltaTime);

	/** Handle raid timeout */
	void OnRaidTimeout();

	/** Calculate and finalize raid result */
	void FinalizeRaidResult(bool bSuccess);

	/** Timer handle for raid start delay */
	FTimerHandle RaidStartTimerHandle;

	/** Cached extraction zones */
	UPROPERTY()
	TArray<AFPSExtractionZone*> CachedExtractionZones;

	/** Raid start time */
	float RaidStartTime = 0.0f;

	/** Has warning been triggered */
	bool bWarningTriggered = false;
};
