// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPSMenuWidgetBase.h"
#include "FPSMainMenuWidget.generated.h"

/**
 * UFPSMainMenuWidget
 *
 * Main menu widget with 3D scene background.
 * Entry point for the game: New Game, Continue, Settings, Quit.
 */
UCLASS()
class FPS_API UFPSMainMenuWidget : public UFPSMenuWidgetBase
{
	GENERATED_BODY()

public:
	//-------------------------------------------------------------------
	// Button Actions
	//-------------------------------------------------------------------

	/** Start a new game - goes to map select */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MainMenu")
	void OnNewGameClicked();

	/** Continue game - goes to loadout */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MainMenu")
	void OnContinueClicked();

	/** Open settings */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MainMenu")
	void OnSettingsClicked();

	/** Quit the game */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MainMenu")
	void OnQuitClicked();

	//-------------------------------------------------------------------
	// Events
	//-------------------------------------------------------------------

	/** Called to confirm quit (show confirmation dialog) */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|MainMenu")
	void OnQuitRequested();

protected:
	virtual void NativeOnMenuShown() override;
};
