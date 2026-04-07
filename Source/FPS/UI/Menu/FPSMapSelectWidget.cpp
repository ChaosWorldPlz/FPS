// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSMapSelectWidget.h"
#include "FPS/System/FPSMenuSubsystem.h"
#include "FPS/Level/FPSLevelFlowTypes.h"
#include "Engine/DataTable.h"

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

void UFPSMapSelectWidget::ConfirmSelection_Implementation()
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

void UFPSMapSelectWidget::OnBackClicked_Implementation()
{
	GoBack();
}

TArray<FName> UFPSMapSelectWidget::GetMapRowNames() const
{
	if (!MapDataTable) return TArray<FName>();
	return MapDataTable->GetRowNames();
}

static FFPSMapInfoRow* FindMapRow(const UDataTable* Table, FName RowName)
{
	if (!Table) return nullptr;
	return Table->FindRow<FFPSMapInfoRow>(RowName, TEXT("MapSelect"));
}

FText UFPSMapSelectWidget::GetMapDisplayName(FName RowName) const
{
	if (auto* Row = FindMapRow(MapDataTable, RowName)) return Row->DisplayName;
	return FText::GetEmpty();
}

FText UFPSMapSelectWidget::GetMapDescription(FName RowName) const
{
	if (auto* Row = FindMapRow(MapDataTable, RowName)) return Row->Description;
	return FText::GetEmpty();
}

FSoftObjectPath UFPSMapSelectWidget::GetMapLevelPath(FName RowName) const
{
	if (auto* Row = FindMapRow(MapDataTable, RowName)) return Row->LevelPath;
	return FSoftObjectPath();
}

int32 UFPSMapSelectWidget::GetMapDifficulty(FName RowName) const
{
	if (auto* Row = FindMapRow(MapDataTable, RowName)) return Row->Difficulty;
	return 0;
}

int32 UFPSMapSelectWidget::GetMapDurationMinutes(FName RowName) const
{
	if (auto* Row = FindMapRow(MapDataTable, RowName)) return Row->DurationMinutes;
	return 0;
}

int32 UFPSMapSelectWidget::GetMapMaxPlayers(FName RowName) const
{
	if (auto* Row = FindMapRow(MapDataTable, RowName)) return Row->MaxPlayers;
	return 0;
}

bool UFPSMapSelectWidget::IsMapEnabled(FName RowName) const
{
	if (auto* Row = FindMapRow(MapDataTable, RowName)) return Row->bEnabled;
	return false;
}
