// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FPSMenuConfig.generated.h"

class UFPSMainMenuWidget;
class UFPSPauseMenuWidget;
class UFPSSettingsWidget;
class UFPSMapSelectWidget;
class UFPSLoadoutWidget;

/**
 * UFPSMenuConfig
 *
 * Data asset for configuring menu widget classes.
 * Create an instance in the editor and configure widget references.
 */
UCLASS(BlueprintType)
class FPS_API UFPSMenuConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Main menu widget class */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widgets")
	TSubclassOf<UFPSMainMenuWidget> MainMenuWidgetClass;

	/** Pause menu widget class */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widgets")
	TSubclassOf<UFPSPauseMenuWidget> PauseMenuWidgetClass;

	/** Settings widget class */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widgets")
	TSubclassOf<UFPSSettingsWidget> SettingsWidgetClass;

	/** Map select widget class */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widgets")
	TSubclassOf<UFPSMapSelectWidget> MapSelectWidgetClass;

	/** Loadout widget class */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Widgets")
	TSubclassOf<UFPSLoadoutWidget> LoadoutWidgetClass;
};
