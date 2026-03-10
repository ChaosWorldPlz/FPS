// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSWeaponAttachmentData.h"

FPrimaryAssetId UFPSWeaponAttachmentData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("AttachmentData"), AttachmentID);
}
