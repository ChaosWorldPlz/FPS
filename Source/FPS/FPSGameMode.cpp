// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSGameMode.h"
#include "FPSCharacter.h"
#include "FPSPlayerController.h"
#include "Team/FPSPlayerState.h"
#include "Team/FPSGameState.h"
#include "Level/FPSPlayerStart.h"
#include "System/FPSMenuSubsystem.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"

AFPSGameMode::AFPSGameMode()
	: Super()
	, RespawnDelay(5.0f)
	, KillScore(1)
	, CountdownDuration(5.0f)
	, MinPlayersToStart(2)
{
	// Set default classes
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	PlayerStateClass = AFPSPlayerState::StaticClass();
	GameStateClass = AFPSGameState::StaticClass();
	PlayerControllerClass = AFPSPlayerController::StaticClass();

	// Enable ticking for match timer
	PrimaryActorTick.bCanEverTick = true;

	// Use seamless travel
	bUseSeamlessTravel = false;
}

void AFPSGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

void AFPSGameMode::BeginPlay()
{
	Super::BeginPlay();

	SetFPSMatchState(EFPSMatchState::WaitingForPlayers);
}

void AFPSGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AFPSGameState* GS = GetFPSGameState();
	if (GS && GS->GetMatchState() == EFPSMatchState::InProgress)
	{
		UpdateMatchTimer(DeltaSeconds);
	}
}

void AFPSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	// Assign team
	AFPSPlayerState* PS = Cast<AFPSPlayerState>(NewPlayer->PlayerState);
	if (PS)
	{
		EFPSTeam AssignedTeam = ChooseTeamForPlayer(NewPlayer);
		PS->ServerSetTeam(AssignedTeam);

		UE_LOG(LogTemp, Log, TEXT("Player %s assigned to %s"),
			*PS->GetPlayerName(),
			*UFPSTeamStatics::GetTeamDisplayName(AssignedTeam).ToString());
	}
}

void AFPSGameMode::Logout(AController* Exiting)
{
	// Clear any pending respawn timer
	if (FTimerHandle* Handle = PendingRespawns.Find(Exiting))
	{
		GetWorldTimerManager().ClearTimer(*Handle);
		PendingRespawns.Remove(Exiting);
	}

	Super::Logout(Exiting);
}

EFPSTeam AFPSGameMode::ChooseTeamForPlayer(AController* Player)
{
	int32 TeamACount = GetTeamPlayerCount(EFPSTeam::TeamA);
	int32 TeamBCount = GetTeamPlayerCount(EFPSTeam::TeamB);

	// Auto-balance: assign to team with fewer players
	if (TeamACount <= TeamBCount)
	{
		return EFPSTeam::TeamA;
	}
	return EFPSTeam::TeamB;
}

int32 AFPSGameMode::GetTeamPlayerCount(EFPSTeam Team) const
{
	int32 Count = 0;
	if (GameState)
	{
		for (APlayerState* PS : GameState->PlayerArray)
		{
			AFPSPlayerState* FPSPS = Cast<AFPSPlayerState>(PS);
			if (FPSPS && FPSPS->GetTeam() == Team)
			{
				Count++;
			}
		}
	}
	return Count;
}

void AFPSGameMode::HandlePlayerDeath(AFPSPlayerState* Victim, AFPSPlayerState* Killer)
{
	if (!Victim)
	{
		return;
	}

	// Record death
	Victim->AddDeath();

	// Record kill and add team score
	if (Killer && Killer != Victim)
	{
		Killer->AddKill();
		Killer->AddScore(KillScore);

		AFPSGameState* GS = GetFPSGameState();
		if (GS)
		{
			GS->AddTeamScore(Killer->GetTeam(), KillScore);
		}

		// Notify clients for kill feed
		if (APlayerController* KillerPC = Cast<APlayerController>(Killer->GetOwner()))
		{
			// Kill feed can be handled by Lua via delegates
		}
	}

	// Check win condition
	CheckWinCondition();

	// Schedule respawn
	AController* VictimController = Cast<AController>(Victim->GetOwner());
	if (VictimController)
	{
		FTimerHandle RespawnHandle;
		FTimerDelegate RespawnDelegate;
		RespawnDelegate.BindUFunction(this, FName("RespawnPlayer"), VictimController);

		GetWorldTimerManager().SetTimer(
			RespawnHandle,
			[this, VictimController]()
			{
				RespawnPlayer(VictimController);
			},
			RespawnDelay,
			false
		);

		PendingRespawns.Add(VictimController, RespawnHandle);
	}
}

void AFPSGameMode::RespawnPlayer(AController* Controller)
{
	if (!Controller)
	{
		return;
	}

	PendingRespawns.Remove(Controller);

	// Don't respawn if match is over
	AFPSGameState* GS = GetFPSGameState();
	if (GS && GS->GetMatchState() == EFPSMatchState::GameOver)
	{
		return;
	}

	// Find team spawn point
	AActor* StartSpot = FindPlayerStart(Controller);
	if (StartSpot)
	{
		RestartPlayerAtPlayerStart(Controller, StartSpot);
	}
	else
	{
		RestartPlayer(Controller);
	}
}

AActor* AFPSGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Try to find a team-specific spawn point
	EFPSTeam PlayerTeam = EFPSTeam::None;
	if (Player)
	{
		AFPSPlayerState* PS = Cast<AFPSPlayerState>(Player->PlayerState);
		if (PS)
		{
			PlayerTeam = PS->GetTeam();
		}
	}

	// Collect matching spawn points
	TArray<AFPSPlayerStart*> TeamStarts;
	TArray<APlayerStart*> FallbackStarts;

	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* Start = *It;
		AFPSPlayerStart* TeamStart = Cast<AFPSPlayerStart>(Start);

		if (TeamStart && TeamStart->Team == PlayerTeam)
		{
			TeamStarts.Add(TeamStart);
		}
		else if (!TeamStart)
		{
			FallbackStarts.Add(Start);
		}
	}

	// Pick a random team start
	if (TeamStarts.Num() > 0)
	{
		return TeamStarts[FMath::RandRange(0, TeamStarts.Num() - 1)];
	}

	// Fallback to any start
	if (FallbackStarts.Num() > 0)
	{
		return FallbackStarts[FMath::RandRange(0, FallbackStarts.Num() - 1)];
	}

	// Ultimate fallback
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AFPSGameMode::CheckWinCondition()
{
	AFPSGameState* GS = GetFPSGameState();
	if (!GS || GS->GetMatchState() != EFPSMatchState::InProgress)
	{
		return;
	}

	// Check score limit
	if (GS->GetTeamAScore() >= GS->ScoreToWin)
	{
		EndMatch(EFPSTeam::TeamA);
	}
	else if (GS->GetTeamBScore() >= GS->ScoreToWin)
	{
		EndMatch(EFPSTeam::TeamB);
	}
}

void AFPSGameMode::EndMatch(EFPSTeam Winner)
{
	AFPSGameState* GS = GetFPSGameState();
	if (!GS)
	{
		return;
	}

	SetFPSMatchState(EFPSMatchState::GameOver);

	// Clear all pending respawns
	for (auto& Pair : PendingRespawns)
	{
		GetWorldTimerManager().ClearTimer(Pair.Value);
	}
	PendingRespawns.Empty();

	// Notify all player controllers
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AFPSPlayerController* PC = Cast<AFPSPlayerController>(*It);
		if (PC)
		{
			PC->ClientOnMatchStateChanged(EFPSMatchState::GameOver, Winner);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Match Over! Winner: %s (Score: A=%d, B=%d)"),
		*UFPSTeamStatics::GetTeamDisplayName(Winner).ToString(),
		GS->GetTeamAScore(),
		GS->GetTeamBScore());
}

void AFPSGameMode::HandleMatchIsWaitingToStart()
{
	Super::HandleMatchIsWaitingToStart();
	SetFPSMatchState(EFPSMatchState::WaitingForPlayers);
}

void AFPSGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	// Start countdown
	StartCountdown();
}

void AFPSGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
}

bool AFPSGameMode::ReadyToStartMatch_Implementation()
{
	// Need minimum players
	return NumPlayers >= MinPlayersToStart;
}

void AFPSGameMode::StartCountdown()
{
	SetFPSMatchState(EFPSMatchState::Countdown);

	AFPSGameState* GS = GetFPSGameState();
	if (GS)
	{
		GS->SetMatchTimeRemaining(CountdownDuration);
	}

	GetWorldTimerManager().SetTimer(
		CountdownTimerHandle,
		this,
		&AFPSGameMode::OnCountdownFinished,
		CountdownDuration,
		false
	);

	UE_LOG(LogTemp, Log, TEXT("Match countdown started: %.0f seconds"), CountdownDuration);
}

void AFPSGameMode::OnCountdownFinished()
{
	SetFPSMatchState(EFPSMatchState::InProgress);

	AFPSGameState* GS = GetFPSGameState();
	if (GS)
	{
		GS->SetMatchTimeRemaining(static_cast<float>(GS->MatchTimeLimitSeconds));
	}

	UE_LOG(LogTemp, Log, TEXT("Match started!"));
}

void AFPSGameMode::UpdateMatchTimer(float DeltaTime)
{
	AFPSGameState* GS = GetFPSGameState();
	if (!GS)
	{
		return;
	}

	float TimeRemaining = GS->GetMatchTimeRemaining() - DeltaTime;
	GS->SetMatchTimeRemaining(TimeRemaining);

	if (TimeRemaining <= 0.0f)
	{
		OnMatchTimeExpired();
	}
}

void AFPSGameMode::OnMatchTimeExpired()
{
	AFPSGameState* GS = GetFPSGameState();
	if (!GS)
	{
		return;
	}

	// Time expired - winner is team with higher score
	EFPSTeam Winner = GS->GetWinningTeam();
	EndMatch(Winner);
}

void AFPSGameMode::SetFPSMatchState(EFPSMatchState NewState)
{
	AFPSGameState* GS = GetFPSGameState();
	if (GS)
	{
		GS->SetMatchState(NewState);
	}
}

AFPSGameState* AFPSGameMode::GetFPSGameState() const
{
	return Cast<AFPSGameState>(GameState);
}

UFPSMenuSubsystem* AFPSGameMode::GetMenuSubsystem() const
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
	{
		return GI->GetSubsystem<UFPSMenuSubsystem>();
	}
	return nullptr;
}
