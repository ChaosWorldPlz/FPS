// Fill out your copyright notice in the Description page of Project Settings.

#include "InteractableTestActor.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AInteractableTestActor::AInteractableTestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);

	// Load cube mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		CubeMesh = CubeMeshAsset.Object;
		MeshComponent->SetStaticMesh(CubeMesh);
	}

	// Load sphere mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		SphereMesh = SphereMeshAsset.Object;
	}

	// Set initial scale
	MeshComponent->SetRelativeScale3D(FVector(DefaultScale));

	// Set collision
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
}

void AInteractableTestActor::BeginPlay()
{
	Super::BeginPlay();

	// Ensure we start as a cube
	SwitchToCube();
	SetMeshScale(DefaultScale);
}

void AInteractableTestActor::OnInteract_Implementation(AActor* Interactor, const FHitResult& HitResult)
{
	// Switch to sphere when interacted
	SwitchToSphere();
	bHasBeenInteracted = true;

	// Log for debugging
	UE_LOG(LogTemp, Log, TEXT("InteractableTestActor: Interacted by %s - Changed to SPHERE!"),
	       *Interactor->GetName());

	// Call blueprint event
	OnInteractedEvent(Interactor);
}

void AInteractableTestActor::OnInteractFocus_Implementation(AActor* Interactor)
{
	bIsFocused = true;

	// Scale up when focused (but only if not interacted yet)
	if (!bHasBeenInteracted)
	{
		SetMeshScale(FocusedScale);
	}

	// Log for debugging
	UE_LOG(LogTemp, Log, TEXT("InteractableTestActor: Focused by %s - Scaled UP!"), *Interactor->GetName());

	// Call blueprint event
	OnFocusedEvent(Interactor);
}

void AInteractableTestActor::OnInteractUnfocus_Implementation(AActor* Interactor)
{
	bIsFocused = false;

	// Return to default scale when unfocused (but only if not interacted)
	if (!bHasBeenInteracted)
	{
		SetMeshScale(DefaultScale);
	}

	// Log for debugging
	UE_LOG(LogTemp, Log, TEXT("InteractableTestActor: Unfocused by %s - Scaled DOWN!"), *Interactor->GetName());

	// Call blueprint event
	OnUnfocusedEvent(Interactor);
}

bool AInteractableTestActor::CanInteract_Implementation(AActor* Interactor) const
{
	return bIsInteractable;
}

void AInteractableTestActor::SwitchToSphere()
{
	if (MeshComponent && SphereMesh)
	{
		MeshComponent->SetStaticMesh(SphereMesh);
		UE_LOG(LogTemp, Log, TEXT("InteractableTestActor: Mesh switched to SPHERE"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("InteractableTestActor: Failed to switch to sphere - mesh not found"));
	}
}

void AInteractableTestActor::SwitchToCube()
{
	if (MeshComponent && CubeMesh)
	{
		MeshComponent->SetStaticMesh(CubeMesh);
		UE_LOG(LogTemp, Log, TEXT("InteractableTestActor: Mesh switched to CUBE"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("InteractableTestActor: Failed to switch to cube - mesh not found"));
	}
}

void AInteractableTestActor::SetMeshScale(float Scale)
{
	if (MeshComponent)
	{
		MeshComponent->SetRelativeScale3D(FVector(Scale));
	}
}
