// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSGameState.h"
#include "Net/UnrealNetwork.h"

AFPSGameState::AFPSGameState()
	: FPSMatchState(EFPSMatchState::WaitingForPlayers)
	, TeamAScore(0)
	, TeamBScore(0)
	, MatchTimeRemaining(0.0f)
	, ScoreToWin(30)
	, MatchTimeLimitSeconds(600)
	, PreviousFPSMatchState(EFPSMatchState::WaitingForPlayers)
{
}

void AFPSGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSGameState, FPSMatchState);
	DOREPLIFETIME(AFPSGameState, TeamAScore);
	DOREPLIFETIME(AFPSGameState, TeamBScore);
	DOREPLIFETIME(AFPSGameState, MatchTimeRemaining);
	DOREPLIFETIME(AFPSGameState, ScoreToWin);
	DOREPLIFETIME(AFPSGameState, MatchTimeLimitSeconds);
}

void AFPSGameState::SetMatchState(EFPSMatchState NewState)
{
	if (!HasAuthority())
	{
		return;
	}

	if (FPSMatchState != NewState)
	{
		EFPSMatchState OldState = FPSMatchState;
		FPSMatchState = NewState;
		OnMatchStateChanged.Broadcast(OldState, NewState);
	}
}

int32 AFPSGameState::GetTeamScore(EFPSTeam Team) const
{
	switch (Team)
	{
	case EFPSTeam::TeamA:
		return TeamAScore;
	case EFPSTeam::TeamB:
		return TeamBScore;
	default:
		return 0;
	}
}

void AFPSGameState::AddTeamScore(EFPSTeam Team, int32 Amount)
{
	if (!HasAuthority())
	{
		return;
	}

	switch (Team)
	{
	case EFPSTeam::TeamA:
		TeamAScore += Amount;
		OnTeamScoreChanged.Broadcast(Team, TeamAScore);
		break;
	case EFPSTeam::TeamB:
		TeamBScore += Amount;
		OnTeamScoreChanged.Broadcast(Team, TeamBScore);
		break;
	default:
		break;
	}
}

EFPSTeam AFPSGameState::GetWinningTeam() const
{
	if (TeamAScore > TeamBScore)
	{
		return EFPSTeam::TeamA;
	}
	else if (TeamBScore > TeamAScore)
	{
		return EFPSTeam::TeamB;
	}
	return EFPSTeam::None; // Tie
}

void AFPSGameState::SetMatchTimeRemaining(float Time)
{
	if (HasAuthority())
	{
		MatchTimeRemaining = FMath::Max(0.0f, Time);
	}
}

FString AFPSGameState::GetFormattedTimeRemaining() const
{
	int32 TotalSeconds = FMath::CeilToInt(MatchTimeRemaining);
	int32 Minutes = TotalSeconds / 60;
	int32 Seconds = TotalSeconds % 60;
	return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

void AFPSGameState::OnRep_FPSMatchState()
{
	OnMatchStateChanged.Broadcast(PreviousFPSMatchState, FPSMatchState);
	PreviousFPSMatchState = FPSMatchState;
}

void AFPSGameState::OnRep_TeamAScore()
{
	OnTeamScoreChanged.Broadcast(EFPSTeam::TeamA, TeamAScore);
}

void AFPSGameState::OnRep_TeamBScore()
{
	OnTeamScoreChanged.Broadcast(EFPSTeam::TeamB, TeamBScore);
}
