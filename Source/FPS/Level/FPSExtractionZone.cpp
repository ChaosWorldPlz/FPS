// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSExtractionZone.h"
#include "Components/BoxComponent.h"
#include "FPS/FPSCharacter.h"
#include "FPS/FPSGameMode.h"
#include "Kismet/GameplayStatics.h"

AFPSExtractionZone::AFPSExtractionZone()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create extraction volume
	ExtractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ExtractionVolume"));
	RootComponent = ExtractionVolume;
	ExtractionVolume->SetBoxExtent(FVector(200.0f, 200.0f, 100.0f));
	ExtractionVolume->SetCollisionProfileName(TEXT("Trigger"));
	ExtractionVolume->SetGenerateOverlapEvents(true);
}

void AFPSExtractionZone::BeginPlay()
{
	Super::BeginPlay();

	// Bind overlap events
	ExtractionVolume->OnComponentBeginOverlap.AddDynamic(this, &AFPSExtractionZone::OnOverlapBegin);
	ExtractionVolume->OnComponentEndOverlap.AddDynamic(this, &AFPSExtractionZone::OnOverlapEnd);

	// Set initial state
	ExtractionState = bIsActive ? EFPSExtractionState::Available : EFPSExtractionState::Inactive;
}

void AFPSExtractionZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ExtractionState == EFPSExtractionState::InProgress)
	{
		UpdateExtractionProgress(DeltaTime);
	}
}

void AFPSExtractionZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	if (!CanPlayerExtract(Pawn))
	{
		return;
	}

	PlayersInZone.Add(Pawn);

	// Start extraction if zone is available
	if (ExtractionState == EFPSExtractionState::Available && PlayersInZone.Num() > 0)
	{
		SetExtractionState(EFPSExtractionState::InProgress);
	}
}

void AFPSExtractionZone::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	PlayersInZone.Remove(Pawn);

	// Reset if no players left
	if (PlayersInZone.Num() == 0 && ExtractionState == EFPSExtractionState::InProgress)
	{
		ResetProgress();
		SetExtractionState(EFPSExtractionState::Available);
	}
}

void AFPSExtractionZone::ActivateZone()
{
	bIsActive = true;
	if (ExtractionState == EFPSExtractionState::Inactive)
	{
		SetExtractionState(EFPSExtractionState::Available);
	}
}

void AFPSExtractionZone::DeactivateZone()
{
	bIsActive = false;
	ResetProgress();
	SetExtractionState(EFPSExtractionState::Inactive);
}

bool AFPSExtractionZone::CanPlayerExtract(APawn* Player) const
{
	if (!Player || !bIsActive)
	{
		return false;
	}

	if (ExtractionState == EFPSExtractionState::Inactive || ExtractionState == EFPSExtractionState::Complete)
	{
		return false;
	}

	// Check requirements
	if (bHasRequirements && RequiredItemID != NAME_None)
	{
		// TODO: Check player inventory for required item
		// For now, return true
	}

	return true;
}

void AFPSExtractionZone::ResetProgress()
{
	CurrentProgress = 0.0f;
	OnExtractionProgressUpdated.Broadcast(0.0f, ExtractionTime);
}

void AFPSExtractionZone::UpdateExtractionProgress(float DeltaTime)
{
	// Clean up invalid references
	PlayersInZone.RemoveAll([](const TWeakObjectPtr<APawn>& Pawn) {
		return !Pawn.IsValid();
	});

	if (PlayersInZone.Num() == 0)
	{
		return;
	}

	// Update progress
	CurrentProgress += DeltaTime / ExtractionTime;
	OnExtractionProgressUpdated.Broadcast(CurrentProgress, ExtractionTime);

	// Check completion
	if (CurrentProgress >= 1.0f)
	{
		CompleteExtraction();
	}
}

void AFPSExtractionZone::CompleteExtraction()
{
	SetExtractionState(EFPSExtractionState::Complete);

	// Notify game mode
	if (AFPSGameMode* GameMode = Cast<AFPSGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->OnPlayerExtracted(this);
	}
}

void AFPSExtractionZone::SetExtractionState(EFPSExtractionState NewState)
{
	if (ExtractionState != NewState)
	{
		EFPSRaidState OldRaidState = static_cast<EFPSRaidState>(static_cast<uint8>(ExtractionState));
		ExtractionState = NewState;
		EFPSRaidState NewRaidState = static_cast<EFPSRaidState>(static_cast<uint8>(NewState));
		OnExtractionStateChanged.Broadcast(OldRaidState, NewRaidState);
	}
}
