// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSPlayerState.h"
#include "Net/UnrealNetwork.h"

AFPSPlayerState::AFPSPlayerState()
	: Team(EFPSTeam::None)
	, Kills(0)
	, Deaths(0)
	, Assists(0)
	, MatchScore(0)
	, DamageDealt(0.0f)
	, DamageTaken(0.0f)
{
}

void AFPSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSPlayerState, Team);
	DOREPLIFETIME(AFPSPlayerState, Kills);
	DOREPLIFETIME(AFPSPlayerState, Deaths);
	DOREPLIFETIME(AFPSPlayerState, Assists);
	DOREPLIFETIME(AFPSPlayerState, MatchScore);
	DOREPLIFETIME(AFPSPlayerState, DamageDealt);
	DOREPLIFETIME(AFPSPlayerState, DamageTaken);
}

void AFPSPlayerState::ServerSetTeam(EFPSTeam NewTeam)
{
	if (!HasAuthority())
	{
		return;
	}

	if (Team != NewTeam)
	{
		Team = NewTeam;
		OnTeamChanged.Broadcast(NewTeam);
	}
}

void AFPSPlayerState::AddKill()
{
	if (!HasAuthority())
	{
		return;
	}
	Kills++;
	OnKillsChanged.Broadcast(Kills);
}

void AFPSPlayerState::AddDeath()
{
	if (!HasAuthority())
	{
		return;
	}
	Deaths++;
}

void AFPSPlayerState::AddAssist()
{
	if (!HasAuthority())
	{
		return;
	}
	Assists++;
}

void AFPSPlayerState::AddScore(int32 Amount)
{
	if (!HasAuthority())
	{
		return;
	}
	int32 OldScore = MatchScore;
	MatchScore += Amount;
	OnScoreChanged.Broadcast(OldScore, MatchScore);
}

void AFPSPlayerState::AddDamageDealt(float Amount)
{
	if (!HasAuthority())
	{
		return;
	}
	DamageDealt += Amount;
}

void AFPSPlayerState::AddDamageTaken(float Amount)
{
	if (!HasAuthority())
	{
		return;
	}
	DamageTaken += Amount;
}

void AFPSPlayerState::ResetStats()
{
	if (!HasAuthority())
	{
		return;
	}
	Kills = 0;
	Deaths = 0;
	Assists = 0;
	MatchScore = 0;
	DamageDealt = 0.0f;
	DamageTaken = 0.0f;
}

void AFPSPlayerState::OnRep_Team()
{
	OnTeamChanged.Broadcast(Team);
}

void AFPSPlayerState::OnRep_Kills()
{
	OnKillsChanged.Broadcast(Kills);
}

void AFPSPlayerState::OnRep_MatchScore()
{
	OnScoreChanged.Broadcast(MatchScore - 1, MatchScore); // approximate old value
}
