// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPSMenuWidgetBase.h"
#include "FPS/UI/FPSMenuTypes.h"
#include "FPS/Level/FPSLevelFlowTypes.h"
#include "FPSMapSelectWidget.generated.h"

/**
 * UFPSMapSelectWidget
 *
 * Map selection screen with graphical map display and live camera preview.
 * Shows map info (difficulty, duration, player count) and allows selection.
 */
UCLASS()
class FPS_API UFPSMapSelectWidget : public UFPSMenuWidgetBase
{
	GENERATED_BODY()

public:
	//-------------------------------------------------------------------
	// Map Selection
	//-------------------------------------------------------------------

	/** Select a map by ID */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	void SelectMap(FName MapId);

	/** Get currently selected map ID */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	FName GetSelectedMapId() const { return SelectedMapId; }

	/** Get info for selected map */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	FFPSMapInfo GetSelectedMapInfo() const;

	/** Get all available maps */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	TArray<FFPSMapInfo> GetAvailableMaps() const;

	/** Get map info by ID */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	FFPSMapInfo GetMapInfo(FName MapId) const;

	//-------------------------------------------------------------------
	// DataTable Access (for Lua — GetDataTableRow is not callable in UnLua)
	//-------------------------------------------------------------------

	/** 设置地图 DataTable 路径，由 Lua 在 BeginPlay 传入 */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	void SetMapDataTable(UDataTable* InTable) { MapDataTable = InTable; }

	/** 获取所有行名 */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	TArray<FName> GetMapRowNames() const;

	/** 以下逐字段读取，避免 UnLua 对含 UObject 的结构体崩溃 */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	FText GetMapDisplayName(FName RowName) const;

	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	FText GetMapDescription(FName RowName) const;

	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	FSoftObjectPath GetMapLevelPath(FName RowName) const;

	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	int32 GetMapDifficulty(FName RowName) const;

	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	int32 GetMapDurationMinutes(FName RowName) const;

	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	int32 GetMapMaxPlayers(FName RowName) const;

	UFUNCTION(BlueprintCallable, Category = "FPS|Menu|MapSelect")
	bool IsMapEnabled(FName RowName) const;

	//-------------------------------------------------------------------
	// Actions
	//-------------------------------------------------------------------

	/** Confirm selection and proceed to loadout */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Menu|MapSelect")
	void ConfirmSelection();
	virtual void ConfirmSelection_Implementation();

	/** Go back to main menu */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Menu|MapSelect")
	void OnBackClicked();
	virtual void OnBackClicked_Implementation();

	//-------------------------------------------------------------------
	// Events
	//-------------------------------------------------------------------

	/** Called when a map is selected */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|MapSelect")
	void OnMapSelected(const FFPSMapInfo& MapInfo);

	/** Called to update camera preview for selected map */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|MapSelect")
	void OnUpdateMapPreview(FVector CameraLocation, FRotator CameraRotation);

protected:
	virtual void NativeOnMenuShown() override;

	/** Currently selected map */
	UPROPERTY()
	FName SelectedMapId;

	/** Map DataTable（由 Lua 通过 UObject.Load + SetMapDataTable 传入）*/
	UPROPERTY()
	UDataTable* MapDataTable = nullptr;
};
