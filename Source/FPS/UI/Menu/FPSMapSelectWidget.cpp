// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSMapSelectWidget.h"
#include "FPS/System/FPSMenuSubsystem.h"

void UFPSMapSelectWidget::NativeOnMenuShown()
{
	// Select first map by default if none selected
	if (SelectedMapId.IsNone())
	{
		TArray<FFPSMapInfo> Maps = GetAvailableMaps();
		if (Maps.Num() > 0)
		{
			SelectMap(Maps[0].MapId);
		}
	}
}

void UFPSMapSelectWidget::SelectMap(FName MapId)
{
	SelectedMapId = MapId;

	FFPSMapInfo MapInfo = GetMapInfo(MapId);
	if (!MapInfo.MapId.IsNone())
	{
		// Notify Blueprint
		OnMapSelected(MapInfo);

		// Update camera preview
		OnUpdateMapPreview(MapInfo.PreviewCameraLocation, MapInfo.PreviewCameraRotation);
	}
}

FFPSMapInfo UFPSMapSelectWidget::GetSelectedMapInfo() const
{
	return GetMapInfo(SelectedMapId);
}

TArray<FFPSMapInfo> UFPSMapSelectWidget::GetAvailableMaps() const
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		return MenuSubsystem->GetAvailableMaps();
	}
	return TArray<FFPSMapInfo>();
}

FFPSMapInfo UFPSMapSelectWidget::GetMapInfo(FName MapId) const
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		return MenuSubsystem->GetMapInfo(MapId);
	}
	return FFPSMapInfo();
}

void UFPSMapSelectWidget::ConfirmSelection()
{
	if (SelectedMapId.IsNone())
	{
		return;
	}

	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->SetSelectedMap(SelectedMapId);
		MenuSubsystem->OpenLoadout();
	}
}

void UFPSMapSelectWidget::OnBackClicked()
{
	GoBack();
}
