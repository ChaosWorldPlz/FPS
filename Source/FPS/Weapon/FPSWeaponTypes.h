// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPSWeaponTypes.generated.h"

/**
 * EFPSWeaponType
 *
 * Defines the type of weapon for gameplay categorization.
 */
UENUM(BlueprintType)
enum class EFPSWeaponType : uint8
{
	None		UMETA(DisplayName = "None"),
	Rifle		UMETA(DisplayName = "Rifle"),
	Pistol		UMETA(DisplayName = "Pistol"),
	Shotgun		UMETA(DisplayName = "Shotgun"),
	SMG			UMETA(DisplayName = "SMG"),
	Sniper		UMETA(DisplayName = "Sniper"),
	Melee		UMETA(DisplayName = "Melee"),
	Throwable	UMETA(DisplayName = "Throwable")
};

/**
 * EFPSWeaponFireMode
 *
 * Defines the firing mode of a weapon.
 */
UENUM(BlueprintType)
enum class EFPSWeaponFireMode : uint8
{
	Single		UMETA(DisplayName = "Single Shot"),
	Burst		UMETA(DisplayName = "Burst Fire"),
	Auto		UMETA(DisplayName = "Full Auto")
};

/**
 * EFPSWeaponState
 *
 * Current state of the weapon.
 */
UENUM(BlueprintType)
enum class EFPSWeaponState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Firing		UMETA(DisplayName = "Firing"),
	Reloading	UMETA(DisplayName = "Reloading"),
	Equipping	UMETA(DisplayName = "Equipping"),
	Unequipping	UMETA(DisplayName = "Unequipping")
};

/**
 * FWeaponAmmoInfo
 *
 * Struct containing ammo information for a weapon.
 */
USTRUCT(BlueprintType)
struct FWeaponAmmoInfo
{
	GENERATED_BODY()

	/** Current ammo in magazine */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int32 CurrentMagazine = 0;

	/** Maximum ammo per magazine */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int32 MaxMagazine = 30;

	/** Current reserve ammo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int32 CurrentReserve = 0;

	/** Maximum reserve ammo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int32 MaxReserve = 120;

	/** Check if magazine is empty */
	bool IsMagazineEmpty() const { return CurrentMagazine <= 0; }

	/** Check if magazine is full */
	bool IsMagazineFull() const { return CurrentMagazine >= MaxMagazine; }

	/** Check if reserve is empty */
	bool IsReserveEmpty() const { return CurrentReserve <= 0; }

	/** Check if can reload (has reserve and magazine not full) */
	bool CanReload() const { return !IsMagazineFull() && !IsReserveEmpty(); }

	/** Consume one round from magazine, returns true if successful */
	bool ConsumeAmmo()
	{
		if (CurrentMagazine > 0)
		{
			CurrentMagazine--;
			return true;
		}
		return false;
	}

	/** Reload magazine from reserve */
	void Reload()
	{
		int32 AmmoNeeded = MaxMagazine - CurrentMagazine;
		int32 AmmoToTransfer = FMath::Min(AmmoNeeded, CurrentReserve);
		CurrentMagazine += AmmoToTransfer;
		CurrentReserve -= AmmoToTransfer;
	}

	/** Add ammo to reserve */
	int32 AddReserve(int32 Amount)
	{
		int32 SpaceAvailable = MaxReserve - CurrentReserve;
		int32 AmmoToAdd = FMath::Min(Amount, SpaceAvailable);
		CurrentReserve += AmmoToAdd;
		return Amount - AmmoToAdd; // Return leftover
	}
};
