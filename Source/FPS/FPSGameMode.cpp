// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSGameMode.h"
#include "FPSCharacter.h"
#include "Level/FPSExtractionZone.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AFPSGameMode::AFPSGameMode()
	: Super()
{
	// Set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// Enable ticking
	PrimaryActorTick.bCanEverTick = true;
}

void AFPSGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Cache extraction zones
	CachedExtractionZones.Empty();
	for (TActorIterator<AFPSExtractionZone> It(GetWorld()); It; ++It)
	{
		CachedExtractionZones.Add(*It);
	}

	// Initialize raid state
	RaidTimeRemaining = RaidTimeLimit;
	SetRaidState(EFPSRaidState::WaitingToStart);

	// Auto-start raid after delay (for testing)
	if (RaidStartDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			RaidStartTimerHandle,
			this,
			&AFPSGameMode::StartRaid,
			RaidStartDelay,
			false
		);
	}
	else
	{
		StartRaid();
	}
}

void AFPSGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CurrentRaidState == EFPSRaidState::InProgress)
	{
		UpdateRaidTimer(DeltaSeconds);
	}
}

void AFPSGameMode::StartRaid()
{
	RaidStartTime = GetWorld()->GetTimeSeconds();
	RaidTimeRemaining = RaidTimeLimit;
	bWarningTriggered = false;

	SetRaidState(EFPSRaidState::InProgress);

	UE_LOG(LogTemp, Log, TEXT("Raid started! Time limit: %.0f seconds"), RaidTimeLimit);
}

void AFPSGameMode::EndRaid(EFPSRaidState EndState)
{
	if (CurrentRaidState == EFPSRaidState::Extracted ||
		CurrentRaidState == EFPSRaidState::Failed ||
		CurrentRaidState == EFPSRaidState::TimedOut)
	{
		return; // Already ended
	}

	bool bSuccess = (EndState == EFPSRaidState::Extracted);
	FinalizeRaidResult(bSuccess);

	SetRaidState(EndState);

	OnRaidCompleted.Broadcast(RaidResult);

	UE_LOG(LogTemp, Log, TEXT("Raid ended! Success: %s, Time spent: %.0f seconds"),
		bSuccess ? TEXT("Yes") : TEXT("No"),
		RaidResult.TimeSpent);
}

void AFPSGameMode::OnPlayerExtracted(AFPSExtractionZone* ExtractionZone)
{
	UE_LOG(LogTemp, Log, TEXT("Player extracted via %s"),
		ExtractionZone ? *ExtractionZone->ExtractionName.ToString() : TEXT("Unknown"));

	EndRaid(EFPSRaidState::Extracted);
}

void AFPSGameMode::OnPlayerDied(AFPSCharacter* Player, AActor* Killer)
{
	UE_LOG(LogTemp, Log, TEXT("Player %s died. Killer: %s"),
		Player ? *Player->GetName() : TEXT("Unknown"),
		Killer ? *Killer->GetName() : TEXT("Unknown"));

	EndRaid(EFPSRaidState::Failed);
}

FString AFPSGameMode::GetFormattedTimeRemaining() const
{
	int32 TotalSeconds = FMath::CeilToInt(RaidTimeRemaining);
	int32 Minutes = TotalSeconds / 60;
	int32 Seconds = TotalSeconds % 60;

	return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

TArray<AFPSExtractionZone*> AFPSGameMode::GetAllExtractionZones() const
{
	return CachedExtractionZones;
}

TArray<AFPSExtractionZone*> AFPSGameMode::GetActiveExtractionZones() const
{
	TArray<AFPSExtractionZone*> ActiveZones;

	for (AFPSExtractionZone* Zone : CachedExtractionZones)
	{
		if (Zone && Zone->bIsActive)
		{
			ActiveZones.Add(Zone);
		}
	}

	return ActiveZones;
}

void AFPSGameMode::SetRaidState(EFPSRaidState NewState)
{
	if (CurrentRaidState != NewState)
	{
		EFPSRaidState OldState = CurrentRaidState;
		CurrentRaidState = NewState;
		OnRaidStateChanged.Broadcast(OldState, NewState);
	}
}

void AFPSGameMode::UpdateRaidTimer(float DeltaTime)
{
	RaidTimeRemaining -= DeltaTime;
	OnRaidTimeUpdated.Broadcast(RaidTimeRemaining);

	// Check warning threshold
	if (!bWarningTriggered && RaidTimeRemaining <= RaidWarningTime)
	{
		bWarningTriggered = true;
		UE_LOG(LogTemp, Warning, TEXT("Raid time warning! %.0f seconds remaining"), RaidTimeRemaining);
	}

	// Check timeout
	if (RaidTimeRemaining <= 0.0f)
	{
		RaidTimeRemaining = 0.0f;
		OnRaidTimeout();
	}
}

void AFPSGameMode::OnRaidTimeout()
{
	UE_LOG(LogTemp, Warning, TEXT("Raid timed out!"));
	EndRaid(EFPSRaidState::TimedOut);
}

void AFPSGameMode::FinalizeRaidResult(bool bSuccess)
{
	RaidResult.bSuccess = bSuccess;
	RaidResult.FinalState = bSuccess ? EFPSRaidState::Extracted : CurrentRaidState;
	RaidResult.TimeSpent = RaidTimeLimit - RaidTimeRemaining;
	RaidResult.TimeRemaining = RaidTimeRemaining;

	// TODO: Calculate damage dealt, taken, kills, experience from player stats
	RaidResult.DamageDealt = 0.0f;
	RaidResult.DamageTaken = 0.0f;
	RaidResult.Kills = 0;
	RaidResult.ExperienceGained = bSuccess ? 100 : 0;
}
