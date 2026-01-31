// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPSMenuWidgetBase.h"
#include "FPSPauseMenuWidget.generated.h"

/**
 * UFPSPauseMenuWidget
 *
 * Pause menu shown during gameplay.
 * Pauses the game and provides Resume, Settings, Quit to Menu options.
 */
UCLASS()
class FPS_API UFPSPauseMenuWidget : public UFPSMenuWidgetBase
{
	GENERATED_BODY()

public:
	//-------------------------------------------------------------------
	// Button Actions
	//-------------------------------------------------------------------

	/** Resume the game */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Pause")
	void OnResumeClicked();

	/** Open settings */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Pause")
	void OnSettingsClicked();

	/** Quit to main menu */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|Pause")
	void OnQuitToMenuClicked();

	//-------------------------------------------------------------------
	// Events
	//-------------------------------------------------------------------

	/** Called to confirm quit to menu (show confirmation dialog) */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|Pause")
	void OnQuitToMenuRequested();

protected:
	virtual void NativeOnMenuShown() override;
	virtual void NativeOnMenuHidden() override;
};
