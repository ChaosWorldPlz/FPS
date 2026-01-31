// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSMainMenuWidget.h"
#include "FPS/System/FPSMenuSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"

void UFPSMainMenuWidget::NativeOnMenuShown()
{
	// Called when main menu is shown
}

void UFPSMainMenuWidget::OnNewGameClicked()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->OpenMapSelect();
	}
}

void UFPSMainMenuWidget::OnContinueClicked()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->OpenLoadout();
	}
}

void UFPSMainMenuWidget::OnSettingsClicked()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->OpenSettings();
	}
}

void UFPSMainMenuWidget::OnQuitClicked()
{
	// Let Blueprint handle confirmation dialog
	OnQuitRequested();
}
