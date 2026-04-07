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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Menu|MainMenu")
	void OnNewGameClicked();
	virtual void OnNewGameClicked_Implementation();

	/** Continue game - goes to loadout */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Menu|MainMenu")
	void OnContinueClicked();
	virtual void OnContinueClicked_Implementation();

	/** Open settings */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Menu|MainMenu")
	void OnSettingsClicked();
	virtual void OnSettingsClicked_Implementation();

	/** Quit the game */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Menu|MainMenu")
	void OnQuitClicked();
	virtual void OnQuitClicked_Implementation();

	//-------------------------------------------------------------------
	// Events
	//-------------------------------------------------------------------

	/** Called to confirm quit (show confirmation dialog) */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|MainMenu")
	void OnQuitRequested();

protected:
	virtual void NativeOnMenuShown() override;
};
