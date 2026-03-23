// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSWeaponDataAsset.h"

UFPSWeaponDataAsset::UFPSWeaponDataAsset()
{
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
