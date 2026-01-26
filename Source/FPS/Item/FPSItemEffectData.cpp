// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSItemEffectData.h"

UFPSItemEffectData::UFPSItemEffectData()
{
}

FPrimaryAssetId UFPSItemEffectData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ItemEffectData"), EffectID);
}
