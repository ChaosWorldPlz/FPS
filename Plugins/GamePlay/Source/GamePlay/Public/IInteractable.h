// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IInteractable.generated.h"

/**
 * Interface for objects that can be interacted with
 * Implement this interface on Actors that should respond to player interaction
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class IInteractable
{
	GENERATED_BODY()

public:
	/**
	 * Called when the player interacts with this object
	 * @param Interactor - The actor performing the interaction (usually the player character)
	 * @param HitResult - The hit result from the interaction trace
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteract(AActor* Interactor, const FHitResult& HitResult);

	/**
	 * Called when this object becomes the focus of interaction (player looking at it)
	 * @param Interactor - The actor focusing on this object
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteractFocus(AActor* Interactor);

	/**
	 * Called when this object loses interaction focus (player looks away)
	 * @param Interactor - The actor that was focusing on this object
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteractUnfocus(AActor* Interactor);

	/**
	 * Check if this object can currently be interacted with
	 * @param Interactor - The actor attempting to interact
	 * @return True if interaction is allowed, false otherwise
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;
};
