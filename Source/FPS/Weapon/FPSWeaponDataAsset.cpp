// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSWeaponDataAsset.h"

UFPSWeaponDataAsset::UFPSWeaponDataAsset()
{
}

float UFPSWeaponDataAsset::GetDamageAtRange(float Range) const
{
	if (Range <= FalloffStartRange)
	{
		return BaseDamage;
	}

	if (Range >= MaxRange)
	{
		return BaseDamage * MinDamageMultiplier;
	}

	// Linear falloff between start and max range
	float FalloffRange = MaxRange - FalloffStartRange;
	float RangeInFalloff = Range - FalloffStartRange;
	float FalloffPercent = RangeInFalloff / FalloffRange;

	float DamageMultiplier = FMath::Lerp(1.0f, MinDamageMultiplier, FalloffPercent);
	return BaseDamage * DamageMultiplier;
}

FWeaponAmmoInfo UFPSWeaponDataAsset::GetDefaultAmmoInfo() const
{
	FWeaponAmmoInfo AmmoInfo;
	AmmoInfo.CurrentMagazine = MagazineSize;
	AmmoInfo.MaxMagazine = MagazineSize;
	AmmoInfo.CurrentReserve = StartingReserveAmmo;
	AmmoInfo.MaxReserve = MaxReserveAmmo;
	return AmmoInfo;
}

FPrimaryAssetId UFPSWeaponDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("WeaponData"), WeaponID);
}
