// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPSMenuTypes.generated.h"

/**
 * EFPSGraphicsQuality
 *
 * Graphics quality presets.
 */
UENUM(BlueprintType)
enum class EFPSGraphicsQuality : uint8
{
	Low		UMETA(DisplayName = "低"),
	Medium	UMETA(DisplayName = "中"),
	High	UMETA(DisplayName = "高"),
	Ultra	UMETA(DisplayName = "极致")
};

/**
 * EFPSMenuState
 *
 * Current menu state.
 */
UENUM(BlueprintType)
enum class EFPSMenuState : uint8
{
	None		UMETA(DisplayName = "None"),
	MainMenu	UMETA(DisplayName = "Main Menu"),
	PauseMenu	UMETA(DisplayName = "Pause Menu"),
	Settings	UMETA(DisplayName = "Settings"),
	MapSelect	UMETA(DisplayName = "Map Select"),
	Loadout		UMETA(DisplayName = "Loadout")
};

/**
 * EFPSLoadoutMode
 *
 * Mode of the loadout screen (preparing for raid or showing raid result).
 */
UENUM(BlueprintType)
enum class EFPSLoadoutMode : uint8
{
	Preparing	UMETA(DisplayName = "出战准备"),
	RaidResult	UMETA(DisplayName = "战局结算")
};

/**
 * FFPSMapInfo
 *
 * Information about a playable map.
 */
USTRUCT(BlueprintType)
struct FFPSMapInfo
{
	GENERATED_BODY()

	/** Map unique identifier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FName MapId;

	/** Display name for UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FText DisplayName;

	/** Map description */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	FText Description;

	/** Soft reference to the map asset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	TSoftObjectPtr<UWorld> MapAsset;

	/** Difficulty rating (1-5) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map", meta = (ClampMin = "1", ClampMax = "5"))
	int32 Difficulty = 1;

	/** Raid duration in minutes */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	int32 DurationMinutes = 20;

	/** Maximum player count */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Map")
	int32 MaxPlayers = 4;

	/** Preview camera location (for map preview) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview")
	FVector PreviewCameraLocation = FVector::ZeroVector;

	/** Preview camera rotation (isometric angle) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview")
	FRotator PreviewCameraRotation = FRotator(-45.0f, 0.0f, 0.0f);

	/** Preview thumbnail texture */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preview")
	TSoftObjectPtr<UTexture2D> PreviewThumbnail;
};

/**
 * FFPSGameSettings
 *
 * Player game settings (graphics, audio, controls).
 */
USTRUCT(BlueprintType)
struct FFPSGameSettings
{
	GENERATED_BODY()

	// Graphics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphics")
	EFPSGraphicsQuality GraphicsQuality = EFPSGraphicsQuality::High;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Graphics", meta = (ClampMin = "60", ClampMax = "120"))
	float FieldOfView = 90.0f;

	// Audio
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0", ClampMax = "100"))
	float SFXVolume = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0", ClampMax = "100"))
	float MusicVolume = 60.0f;

	// Controls
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Controls", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float MouseSensitivity = 1.0f;

	/** Reset to default values */
	void ResetToDefault()
	{
		GraphicsQuality = EFPSGraphicsQuality::High;
		FieldOfView = 90.0f;
		SFXVolume = 80.0f;
		MusicVolume = 60.0f;
		MouseSensitivity = 1.0f;
	}
};

/**
 * FFPSKeyBinding
 *
 * A single key binding entry.
 */
USTRUCT(BlueprintType)
struct FFPSKeyBinding
{
	GENERATED_BODY()

	/** Action name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KeyBinding")
	FName ActionName;

	/** Display name for UI */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KeyBinding")
	FText DisplayName;

	/** Primary key */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KeyBinding")
	FKey PrimaryKey;

	/** Secondary key (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KeyBinding")
	FKey SecondaryKey;
};

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMenuStateChanged, EFPSMenuState, OldState, EFPSMenuState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadoutModeChanged, EFPSLoadoutMode, NewMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsApplied);
