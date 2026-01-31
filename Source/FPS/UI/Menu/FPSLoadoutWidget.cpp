// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSLoadoutWidget.h"
#include "FPS/System/FPSMenuSubsystem.h"

void UFPSLoadoutWidget::NativeOnMenuShown()
{
	// Refresh inventory when shown
	OnRefreshInventory();
}

void UFPSLoadoutWidget::SetLoadoutMode(EFPSLoadoutMode Mode)
{
	if (CurrentMode != Mode)
	{
		CurrentMode = Mode;
		OnLoadoutModeChanged(Mode);
	}
}

void UFPSLoadoutWidget::SetRaidResult(const FFPSRaidResult& Result)
{
	CachedRaidResult = Result;
	OnRaidResultUpdated(Result);
}

FString UFPSLoadoutWidget::GetSurvivalTimeString() const
{
	int32 TotalSeconds = FMath::FloorToInt(CachedRaidResult.TimeSpent);
	int32 Minutes = TotalSeconds / 60;
	int32 Seconds = TotalSeconds % 60;
	return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

void UFPSLoadoutWidget::StartRaid()
{
	if (CurrentMode != EFPSLoadoutMode::Preparing)
	{
		return;
	}

	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->StartRaid();
	}
}

void UFPSLoadoutWidget::ExitToMainMenu()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->ExitToMainMenu();
	}
}

void UFPSLoadoutWidget::OnBackClicked()
{
	if (CurrentMode == EFPSLoadoutMode::RaidResult)
	{
		// In result mode, back button exits to main menu
		ExitToMainMenu();
	}
	else
	{
		// In preparing mode, go back to map select
		GoBack();
	}
}
