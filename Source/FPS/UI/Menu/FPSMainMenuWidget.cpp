// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSMainMenuWidget.h"
#include "FPS/System/FPSMenuSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"

void UFPSMainMenuWidget::NativeOnMenuShown()
{
	// Called when main menu is shown
}

void UFPSMainMenuWidget::OnNewGameClicked_Implementation()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->OpenMapSelect();
	}
}

void UFPSMainMenuWidget::OnContinueClicked_Implementation()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->OpenLoadout();
	}
}

void UFPSMainMenuWidget::OnSettingsClicked_Implementation()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->OpenSettings();
	}
}

void UFPSMainMenuWidget::OnQuitClicked_Implementation()
{
	OnQuitRequested();
}
