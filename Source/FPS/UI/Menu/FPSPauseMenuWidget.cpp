// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSPauseMenuWidget.h"
#include "FPS/System/FPSMenuSubsystem.h"

void UFPSPauseMenuWidget::NativeOnMenuShown()
{
	// Game is paused when pause menu is shown
}

void UFPSPauseMenuWidget::NativeOnMenuHidden()
{
	// Game is unpaused when pause menu is hidden
}

void UFPSPauseMenuWidget::OnResumeClicked()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->PopAllMenus();
	}
}

void UFPSPauseMenuWidget::OnSettingsClicked()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->OpenSettings();
	}
}

void UFPSPauseMenuWidget::OnQuitToMenuClicked()
{
	// Let Blueprint handle confirmation dialog
	OnQuitToMenuRequested();
}
