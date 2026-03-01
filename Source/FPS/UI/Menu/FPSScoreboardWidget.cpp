// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSScoreboardWidget.h"
#include "FPS/Team/FPSGameState.h"
#include "FPS/Team/FPSPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"

TArray<FFPSPlayerScoreInfo> UFPSScoreboardWidget::GetAllPlayerStats() const
{
	TArray<FFPSPlayerScoreInfo> Results;

	AGameStateBase* GS = UGameplayStatics::GetGameState(this);
	if (!GS)
	{
		return Results;
	}

	for (APlayerState* PS : GS->PlayerArray)
	{
		AFPSPlayerState* FPSPS = Cast<AFPSPlayerState>(PS);
		if (!FPSPS)
		{
			continue;
		}

		FFPSPlayerScoreInfo Info;
		Info.PlayerName = FPSPS->GetPlayerName();
		Info.Team = FPSPS->GetTeam();
		Info.Kills = FPSPS->GetKills();
		Info.Deaths = FPSPS->GetDeaths();
		Info.Assists = FPSPS->GetAssists();
		Info.Score = FPSPS->GetMatchScore();
		Info.Ping = FMath::RoundToInt(FPSPS->GetPingInMilliseconds());

		Results.Add(Info);
	}

	// Sort by score descending
	Results.Sort([](const FFPSPlayerScoreInfo& A, const FFPSPlayerScoreInfo& B)
	{
		return A.Score > B.Score;
	});

	return Results;
}

int32 UFPSScoreboardWidget::GetTeamScore(EFPSTeam Team) const
{
	AFPSGameState* GS = GetFPSGameState();
	return GS ? GS->GetTeamScore(Team) : 0;
}

float UFPSScoreboardWidget::GetMatchTimeRemaining() const
{
	AFPSGameState* GS = GetFPSGameState();
	return GS ? GS->GetMatchTimeRemaining() : 0.0f;
}

EFPSMatchState UFPSScoreboardWidget::GetMatchState() const
{
	AFPSGameState* GS = GetFPSGameState();
	return GS ? GS->GetFPSMatchState() : EFPSMatchState::WaitingForPlayers;
}

FString UFPSScoreboardWidget::GetFormattedTimeRemaining() const
{
	AFPSGameState* GS = GetFPSGameState();
	return GS ? GS->GetFormattedTimeRemaining() : TEXT("00:00");
}

AFPSGameState* UFPSScoreboardWidget::GetFPSGameState() const
{
	return Cast<AFPSGameState>(UGameplayStatics::GetGameState(this));
}
