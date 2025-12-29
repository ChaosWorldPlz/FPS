// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IInteractable.h"
#include "InteractableTestActor.generated.h"

/**
 * Test actor for demonstrating the interaction system
 * - Changes color when focused (player looks at it)
 * - Changes to red when interacted with
 */
UCLASS()
class GAMEPLAY_API AInteractableTestActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AInteractableTestActor();

protected:
	virtual void BeginPlay() override;

public:
	// ========== Components ==========

	/** Visual mesh component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UStaticMeshComponent> MeshComponent;

	// ========== Configuration ==========

	/** Scale when focused (player looking at it) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Visual")
	float FocusedScale = 1.2f;

	/** Default scale */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Visual")
	float DefaultScale = 1.0f;

	/** Can this actor be interacted with? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact")
	bool bIsInteractable = true;

	// ========== IInteractable Interface Implementation ==========

	virtual void OnInteract_Implementation(AActor* Interactor, const FHitResult& HitResult) override;
	virtual void OnInteractFocus_Implementation(AActor* Interactor) override;
	virtual void OnInteractUnfocus_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	// ========== Blueprint Events (for testing/extending in BP) ==========

	UFUNCTION(BlueprintImplementableEvent, Category = "Interact")
	void OnInteractedEvent(AActor* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interact")
	void OnFocusedEvent(AActor* Interactor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Interact")
	void OnUnfocusedEvent(AActor* Interactor);

protected:
	/** Switch mesh to sphere */
	void SwitchToSphere();

	/** Switch mesh to cube */
	void SwitchToCube();

	/** Set mesh scale */
	void SetMeshScale(float Scale);

private:
	/** Cube mesh reference */
	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	/** Sphere mesh reference */
	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

	/** Is currently focused? */
	bool bIsFocused = false;

	/** Has been interacted with? */
	bool bHasBeenInteracted = false;
};
