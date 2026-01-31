// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPSMenuWidgetBase.h"
#include "FPS/UI/FPSMenuTypes.h"
#include "FPSSettingsWidget.generated.h"

/**
 * UFPSSettingsWidget
 *
 * Settings menu with tabs for Graphics, Audio, Controls, Key Bindings, and About.
 * Can be embedded in Main Menu or Pause Menu.
 */
UCLASS()
class FPS_API UFPSSettingsWidget : public UFPSMenuWidgetBase
{
	GENERATED_BODY()

public:
	//-------------------------------------------------------------------
	// Graphics Settings
	//-------------------------------------------------------------------

	/** Set graphics quality preset */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	void SetGraphicsQuality(EFPSGraphicsQuality Quality);

	/** Get current graphics quality */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	EFPSGraphicsQuality GetGraphicsQuality() const;

	/** Set field of view */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	void SetFieldOfView(float FOV);

	/** Get current field of view */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	float GetFieldOfView() const;

	//-------------------------------------------------------------------
	// Audio Settings
	//-------------------------------------------------------------------

	/** Set SFX volume (0-100) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	void SetSFXVolume(float Volume);

	/** Get current SFX volume */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	float GetSFXVolume() const;

	/** Set music volume (0-100) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	void SetMusicVolume(float Volume);

	/** Get current music volume */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	float GetMusicVolume() const;

	//-------------------------------------------------------------------
	// Control Settings
	//-------------------------------------------------------------------

	/** Set mouse sensitivity */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	void SetMouseSensitivity(float Sensitivity);

	/** Get current mouse sensitivity */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	float GetMouseSensitivity() const;

	//-------------------------------------------------------------------
	// Key Bindings
	//-------------------------------------------------------------------

	/** Get all key bindings */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	TArray<FFPSKeyBinding> GetKeyBindings() const;

	/** Set a key binding */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	void SetKeyBinding(FName ActionName, FKey NewKey, bool bPrimary = true);

	/** Reset key bindings to default */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	void ResetKeyBindings();

	//-------------------------------------------------------------------
	// Apply / Reset
	//-------------------------------------------------------------------

	/** Apply all settings */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	void ApplySettings();

	/** Reset all settings to default */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	void ResetToDefault();

	/** Check if there are unsaved changes */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Settings")
	bool HasUnsavedChanges() const;

	//-------------------------------------------------------------------
	// Events
	//-------------------------------------------------------------------

	/** Called when settings are applied */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|Settings")
	void OnSettingsApplied();

	/** Called when settings are reset */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|Settings")
	void OnSettingsReset();

protected:
	virtual void NativeOnMenuShown() override;

	/** Load current settings from subsystem */
	void LoadCurrentSettings();

	/** Cached settings being edited */
	UPROPERTY()
	FFPSGameSettings EditingSettings;

	/** Key bindings */
	UPROPERTY()
	TArray<FFPSKeyBinding> KeyBindings;

	/** Whether settings have been modified */
	bool bSettingsModified = false;
};
