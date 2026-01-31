// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FPS/UI/FPSMenuTypes.h"
#include "FPS/Level/FPSLevelFlowTypes.h"
#include "FPSMenuSubsystem.generated.h"

class UFPSMenuWidgetBase;
class UFPSMainMenuWidget;
class UFPSPauseMenuWidget;
class UFPSSettingsWidget;
class UFPSMapSelectWidget;
class UFPSLoadoutWidget;
class UFPSMenuConfig;

/**
 * UFPSMenuSubsystem
 *
 * Game instance subsystem that manages all menu widgets.
 * Handles menu navigation, stack management, and input mode switching.
 */
UCLASS()
class FPS_API UFPSMenuSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//-------------------------------------------------------------------
	// Subsystem Lifecycle
	//-------------------------------------------------------------------

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	//-------------------------------------------------------------------
	// Menu Navigation
	//-------------------------------------------------------------------

	/** Open the main menu */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void OpenMainMenu();

	/** Open the pause menu (from gameplay) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void OpenPauseMenu();

	/** Open the settings menu */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void OpenSettings();

	/** Open the map selection menu */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void OpenMapSelect();

	/** Open the loadout screen (preparing mode) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void OpenLoadout();

	/** Open the raid result screen (uses loadout in result mode) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void OpenRaidResult(const FFPSRaidResult& Result);

	//-------------------------------------------------------------------
	// Menu Stack Management
	//-------------------------------------------------------------------

	/** Push a menu onto the stack */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void PushMenu(UFPSMenuWidgetBase* Menu);

	/** Pop the top menu from the stack */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void PopMenu();

	/** Pop all menus and return to game */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void PopAllMenus();

	/** Get the current top menu */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	UFPSMenuWidgetBase* GetCurrentMenu() const;

	/** Get the current menu state */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	EFPSMenuState GetCurrentMenuState() const { return CurrentState; }

	/** Check if any menu is open */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	bool IsInMenu() const { return MenuStack.Num() > 0; }

	/** Check if in main menu (not in gameplay) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	bool IsInMainMenu() const { return CurrentState == EFPSMenuState::MainMenu; }

	//-------------------------------------------------------------------
	// Settings Management
	//-------------------------------------------------------------------

	/** Get current game settings */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	FFPSGameSettings GetGameSettings() const { return GameSettings; }

	/** Set game settings (does not apply) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void SetGameSettings(const FFPSGameSettings& NewSettings);

	/** Apply current settings to the game */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void ApplySettings();

	/** Reset settings to defaults */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void ResetSettingsToDefault();

	/** Save settings to disk */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void SaveSettings();

	/** Load settings from disk */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void LoadSettings();

	//-------------------------------------------------------------------
	// Map Management
	//-------------------------------------------------------------------

	/** Get all available maps */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	TArray<FFPSMapInfo> GetAvailableMaps() const;

	/** Get info for a specific map */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	FFPSMapInfo GetMapInfo(FName MapId) const;

	/** Set the selected map for next raid */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void SetSelectedMap(FName MapId);

	/** Get the selected map */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	FName GetSelectedMap() const { return SelectedMapId; }

	//-------------------------------------------------------------------
	// Raid Flow
	//-------------------------------------------------------------------

	/** Start a new raid with current loadout */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void StartRaid();

	/** Exit to main menu */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void ExitToMainMenu();

	/** Cache the raid result for display */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void SetRaidResult(const FFPSRaidResult& Result);

	/** Get cached raid result */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	FFPSRaidResult GetCachedRaidResult() const { return CachedRaidResult; }

	//-------------------------------------------------------------------
	// Events
	//-------------------------------------------------------------------

	/** Called when menu state changes */
	UPROPERTY(BlueprintAssignable, Category = "FPS|Menu|Events")
	FOnMenuStateChanged OnMenuStateChanged;

	/** Called when settings are applied */
	UPROPERTY(BlueprintAssignable, Category = "FPS|Menu|Events")
	FOnSettingsApplied OnSettingsApplied;

protected:
	//-------------------------------------------------------------------
	// Internal Helpers
	//-------------------------------------------------------------------

	/** Create a menu widget of the specified class */
	template<typename T>
	T* CreateMenuWidget(TSubclassOf<T> WidgetClass);

	/** Set input mode for UI */
	void SetUIInputMode(bool bUIOnly);

	/** Set the current menu state */
	void SetMenuState(EFPSMenuState NewState);

	/** Apply graphics settings */
	void ApplyGraphicsSettings();

	/** Apply audio settings */
	void ApplyAudioSettings();

	//-------------------------------------------------------------------
	// Menu Configuration
	//-------------------------------------------------------------------

	/** Menu config data asset path (set in Project Settings or here) */
	UPROPERTY(EditDefaultsOnly, Category = "FPS|Menu|Config")
	TSoftObjectPtr<UFPSMenuConfig> MenuConfigAsset;

	/** Load menu config and cache widget classes */
	void LoadMenuConfig();

	//-------------------------------------------------------------------
	// Widget Classes (loaded from MenuConfig)
	//-------------------------------------------------------------------

	UPROPERTY()
	TSubclassOf<UFPSMainMenuWidget> MainMenuWidgetClass;

	UPROPERTY()
	TSubclassOf<UFPSPauseMenuWidget> PauseMenuWidgetClass;

	UPROPERTY()
	TSubclassOf<UFPSSettingsWidget> SettingsWidgetClass;

	UPROPERTY()
	TSubclassOf<UFPSMapSelectWidget> MapSelectWidgetClass;

	UPROPERTY()
	TSubclassOf<UFPSLoadoutWidget> LoadoutWidgetClass;

	//-------------------------------------------------------------------
	// State
	//-------------------------------------------------------------------

	/** Current menu state */
	UPROPERTY()
	EFPSMenuState CurrentState = EFPSMenuState::None;

	/** Menu stack for navigation */
	UPROPERTY()
	TArray<UFPSMenuWidgetBase*> MenuStack;

	/** Current game settings */
	UPROPERTY()
	FFPSGameSettings GameSettings;

	/** Selected map for next raid */
	UPROPERTY()
	FName SelectedMapId;

	/** Cached raid result */
	UPROPERTY()
	FFPSRaidResult CachedRaidResult;

	/** Available maps (loaded from config) */
	UPROPERTY()
	TArray<FFPSMapInfo> AvailableMaps;

	//-------------------------------------------------------------------
	// Widget Instances
	//-------------------------------------------------------------------

	UPROPERTY()
	TObjectPtr<UFPSMainMenuWidget> MainMenuWidget;

	UPROPERTY()
	TObjectPtr<UFPSPauseMenuWidget> PauseMenuWidget;

	UPROPERTY()
	TObjectPtr<UFPSSettingsWidget> SettingsWidget;

	UPROPERTY()
	TObjectPtr<UFPSMapSelectWidget> MapSelectWidget;

	UPROPERTY()
	TObjectPtr<UFPSLoadoutWidget> LoadoutWidget;
};
