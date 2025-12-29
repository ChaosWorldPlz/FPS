// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerInteractComponent.h"
#include "IInteractable.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"

UPlayerInteractComponent::UPlayerInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UPlayerInteractComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPlayerInteractComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                             FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Only perform tick detection if enabled
	if (!bEnableTickDetection)
	{
		return;
	}

	// Accumulate time and check if we should perform detection
	TimeSinceLastDetection += DeltaTime;
	if (TimeSinceLastDetection >= DetectionInterval)
	{
		PerformInteractTrace();
		TimeSinceLastDetection = 0.0f;
	}
}

bool UPlayerInteractComponent::PerformInteractTrace()
{
	FVector TraceStart, TraceEnd;
	if (!GetTraceStartAndEnd(TraceStart, TraceEnd))
	{
		// Failed to get valid trace parameters
		SetCurrentInteractable(nullptr);
		return false;
	}

	// Perform line trace
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner()); // Ignore the owner (player character)

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		TraceChannel,
		QueryParams
	);

	// Draw debug visualization if enabled
	if (bDrawDebugTrace)
	{
		FColor DebugColor = bHit ? FColor::Green : FColor::Red;
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, DebugColor, false, DetectionInterval);

		if (bHit)
		{
			DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 10.0f, 8, FColor::Yellow, false, DetectionInterval);
		}
	}

	// Check if we hit an interactable actor
	if (bHit && HitResult.GetActor())
	{
		AActor* HitActor = HitResult.GetActor();
		if (IsActorInteractable(HitActor))
		{
			SetCurrentInteractable(HitActor);
			return true;
		}
	}

	// No valid interactable found
	SetCurrentInteractable(nullptr);
	return false;
}

bool UPlayerInteractComponent::GetTraceStartAndEnd(FVector& OutStart, FVector& OutEnd) const
{
	switch (TraceMode)
	{
	case EInteractTraceMode::Camera:
		{
			// Get the owner pawn
			APawn* OwnerPawn = Cast<APawn>(GetOwner());
			if (!OwnerPawn)
			{
				UE_LOG(LogTemp, Warning, TEXT("PlayerInteractComponent: Owner is not a Pawn"));
				return false;
			}

			// Find camera component
			UCameraComponent* CameraComponent = OwnerPawn->FindComponentByClass<UCameraComponent>();
			if (!CameraComponent)
			{
				UE_LOG(LogTemp, Warning, TEXT("PlayerInteractComponent: No Camera component found on Pawn"));
				return false;
			}

			// Get camera location and forward direction
			OutStart = CameraComponent->GetComponentLocation();
			FVector CameraForward = CameraComponent->GetForwardVector();
			OutEnd = OutStart + (CameraForward * InteractDistance);
			return true;
		}

	case EInteractTraceMode::Line:
		{
			// Not yet implemented - placeholder
			UE_LOG(LogTemp, Warning, TEXT("PlayerInteractComponent: Line trace mode not yet implemented"));
			return false;
		}

	case EInteractTraceMode::Cone:
		{
			// Not yet implemented - placeholder
			UE_LOG(LogTemp, Warning, TEXT("PlayerInteractComponent: Cone trace mode not yet implemented"));
			return false;
		}

	default:
		return false;
	}
}

bool UPlayerInteractComponent::IsActorInteractable(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// Check if actor implements the IInteractable interface
	if (!Actor->Implements<UInteractable>())
	{
		return false;
	}

	// Call the interface's CanInteract method
	return IInteractable::Execute_CanInteract(Actor, GetOwner());
}

void UPlayerInteractComponent::SetCurrentInteractable(AActor* NewInteractable)
{
	// No change, skip
	if (CurrentInteractable == NewInteractable)
	{
		return;
	}

	// Unfocus old interactable
	if (CurrentInteractable)
	{
		if (CurrentInteractable->Implements<UInteractable>())
		{
			IInteractable::Execute_OnInteractUnfocus(CurrentInteractable, GetOwner());
		}
		OnInteractableLost.Broadcast(CurrentInteractable);
	}

	// Update current interactable
	CurrentInteractable = NewInteractable;

	// Focus new interactable
	if (CurrentInteractable)
	{
		if (CurrentInteractable->Implements<UInteractable>())
		{
			IInteractable::Execute_OnInteractFocus(CurrentInteractable, GetOwner());
		}
		OnInteractableFound.Broadcast(CurrentInteractable);
	}
}

void UPlayerInteractComponent::TryInteract()
{
	// If tick detection is disabled, perform a trace now
	if (!bEnableTickDetection)
	{
		PerformInteractTrace();
	}

	// Check if we have a valid interactable
	if (!CurrentInteractable)
	{
		return;
	}

	// Perform one more trace to get fresh hit result
	FVector TraceStart, TraceEnd;
	if (!GetTraceStartAndEnd(TraceStart, TraceEnd))
	{
		return;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		TraceChannel,
		QueryParams
	);

	if (bHit && HitResult.GetActor() == CurrentInteractable)
	{
		// Call the interface's OnInteract method
		if (CurrentInteractable->Implements<UInteractable>())
		{
			IInteractable::Execute_OnInteract(CurrentInteractable, GetOwner(), HitResult);
		}

		// Broadcast the event
		OnInteractTriggered.Broadcast(CurrentInteractable, HitResult);
	}
}

void UPlayerInteractComponent::UpdateInteractableDetection()
{
	PerformInteractTrace();
}

void UPlayerInteractComponent::SetCustomTraceStart(FVector Start, FVector Direction)
{
	CustomTraceStartPos = Start;
	CustomTraceDirection = Direction.GetSafeNormal();
}

void UPlayerInteractComponent::SetInteractDistance(float NewDistance)
{
	InteractDistance = FMath::Max(0.0f, NewDistance);
}

void UPlayerInteractComponent::SetDetectionInterval(float NewInterval)
{
	DetectionInterval = FMath::Max(0.0f, NewInterval);
}
