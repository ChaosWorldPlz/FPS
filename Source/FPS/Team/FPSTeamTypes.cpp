// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSTeamTypes.h"

bool UFPSTeamStatics::AreEnemies(EFPSTeam A, EFPSTeam B)
{
	if (A == EFPSTeam::None || B == EFPSTeam::None)
	{
		return false;
	}
	return A != B;
}

bool UFPSTeamStatics::AreFriendly(EFPSTeam A, EFPSTeam B)
{
	if (A == EFPSTeam::None || B == EFPSTeam::None)
	{
		return false;
	}
	return A == B;
}

FText UFPSTeamStatics::GetTeamDisplayName(EFPSTeam Team)
{
	switch (Team)
	{
	case EFPSTeam::TeamA:
		return FText::FromString(TEXT("Red Team"));
	case EFPSTeam::TeamB:
		return FText::FromString(TEXT("Blue Team"));
	default:
		return FText::FromString(TEXT("No Team"));
	}
}

FLinearColor UFPSTeamStatics::GetTeamColor(EFPSTeam Team)
{
	switch (Team)
	{
	case EFPSTeam::TeamA:
		return FLinearColor(0.9f, 0.2f, 0.2f, 1.0f); // Red
	case EFPSTeam::TeamB:
		return FLinearColor(0.2f, 0.4f, 0.9f, 1.0f); // Blue
	default:
		return FLinearColor::White;
	}
}

EFPSTeam UFPSTeamStatics::GetOppositeTeam(EFPSTeam Team)
{
	switch (Team)
	{
	case EFPSTeam::TeamA:
		return EFPSTeam::TeamB;
	case EFPSTeam::TeamB:
		return EFPSTeam::TeamA;
	default:
		return EFPSTeam::None;
	}
}
