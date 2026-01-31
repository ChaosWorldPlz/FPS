// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPSMenuWidgetBase.h"
#include "FPS/UI/FPSMenuTypes.h"
#include "FPS/Level/FPSLevelFlowTypes.h"
#include "FPSLoadoutWidget.generated.h"

/**
 * UFPSLoadoutWidget
 *
 * Loadout/Equipment screen that serves two modes:
 * 1. Preparing Mode: Pre-raid equipment management with "Start Raid" button
 * 2. RaidResult Mode: Post-raid result display with "Exit" button
 *
 * Both modes share the same layout with character preview and stash/backpack UI.
 */
UCLASS()
class FPS_API UFPSLoadoutWidget : public UFPSMenuWidgetBase
{
	GENERATED_BODY()

public:
	//-------------------------------------------------------------------
	// Mode Management
	//-------------------------------------------------------------------

	/** Set the current mode (Preparing or RaidResult) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	void SetLoadoutMode(EFPSLoadoutMode Mode);

	/** Get current mode */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	EFPSLoadoutMode GetLoadoutMode() const { return CurrentMode; }

	/** Check if in preparing mode */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	bool IsPreparingMode() const { return CurrentMode == EFPSLoadoutMode::Preparing; }

	/** Check if in raid result mode */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	bool IsRaidResultMode() const { return CurrentMode == EFPSLoadoutMode::RaidResult; }

	//-------------------------------------------------------------------
	// Raid Result Mode
	//-------------------------------------------------------------------

	/** Set raid result data (for result mode) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	void SetRaidResult(const FFPSRaidResult& Result);

	/** Get cached raid result */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	FFPSRaidResult GetRaidResult() const { return CachedRaidResult; }

	/** Check if raid was successful */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	bool WasRaidSuccessful() const { return CachedRaidResult.bSuccess; }

	//-------------------------------------------------------------------
	// Statistics (for result mode display)
	//-------------------------------------------------------------------

	/** Get formatted survival time string */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	FString GetSurvivalTimeString() const;

	/** Get kill count */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	int32 GetKillCount() const { return CachedRaidResult.Kills; }

	/** Get experience gained */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	int32 GetExperienceGained() const { return CachedRaidResult.ExperienceGained; }

	//-------------------------------------------------------------------
	// Actions
	//-------------------------------------------------------------------

	/** Start the raid (Preparing mode only) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	void StartRaid();

	/** Exit to main menu (RaidResult mode or back button) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	void ExitToMainMenu();

	/** Go back to previous screen */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Loadout")
	void OnBackClicked();

	//-------------------------------------------------------------------
	// Events (for Blueprint/Lua binding)
	//-------------------------------------------------------------------

	/** Called when mode changes */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|Loadout")
	void OnLoadoutModeChanged(EFPSLoadoutMode NewMode);

	/** Called when raid result is set */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|Loadout")
	void OnRaidResultUpdated(const FFPSRaidResult& Result);

	/** Called to refresh inventory display */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|Loadout")
	void OnRefreshInventory();

protected:
	virtual void NativeOnMenuShown() override;

	/** Current mode */
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Menu|Loadout")
	EFPSLoadoutMode CurrentMode = EFPSLoadoutMode::Preparing;

	/** Cached raid result */
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Menu|Loadout")
	FFPSRaidResult CachedRaidResult;
};
