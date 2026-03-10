// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPSAttachmentTypes.generated.h"

/** Which physical slot on a weapon an attachment occupies */
UENUM(BlueprintType)
enum class EFPSAttachmentSlotType : uint8
{
	None      UMETA(DisplayName = "None"),
	Sight     UMETA(DisplayName = "瞄具"),
	Muzzle    UMETA(DisplayName = "枪口"),
	Handguard UMETA(DisplayName = "握把/导轨"),
	Magazine  UMETA(DisplayName = "弹匣"),
};

/** Character carry slot for a weapon (3 total) */
UENUM(BlueprintType)
enum class EFPSWeaponSlot : uint8
{
	None     = 0 UMETA(DisplayName = "None"),
	Primary1 = 1 UMETA(DisplayName = "主武器槽1"),
	Primary2 = 2 UMETA(DisplayName = "主武器槽2"),
	Pistol   = 3 UMETA(DisplayName = "手枪槽"),
};

/**
 * FFPSInstalledAttachment
 *
 * One entry in the replicated attachment list: which slot + which attachment ID.
 * Using a struct array instead of TMap because UE does not support replicated TMap.
 */
USTRUCT(BlueprintType)
struct FFPSInstalledAttachment
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Attachments")
	EFPSAttachmentSlotType Slot = EFPSAttachmentSlotType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon|Attachments")
	FName AttachmentID;
};

/**
 * FFPSAttachmentStatModifiers
 *
 * Additive stat modifiers applied by a weapon attachment.
 * All values are deltas added on top of the weapon's base stats.
 * Negative values reduce the stat (e.g. SpreadDelta < 0 = tighter grouping).
 */
USTRUCT(BlueprintType)
struct FFPSAttachmentStatModifiers
{
	GENERATED_BODY()

	/** Damage delta */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float DamageDelta = 0.f;

	/** Base spread delta in degrees (negative = tighter grouping) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float SpreadDelta = 0.f;

	/** Reload time delta in seconds (negative = faster) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float ReloadTimeDelta = 0.f;

	/** Magazine capacity delta in rounds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float MagazineSizeDelta = 0.f;

	/** Max effective range delta in Unreal units */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float RangeDelta = 0.f;
};
