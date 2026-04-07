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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Menu|Pause")
	void OnResumeClicked();
	virtual void OnResumeClicked_Implementation();

	/** Open settings */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Menu|Pause")
	void OnSettingsClicked();
	virtual void OnSettingsClicked_Implementation();

	/** Quit to main menu */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Menu|Pause")
	void OnQuitToMenuClicked();
	virtual void OnQuitToMenuClicked_Implementation();

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
