// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "PlayerInteractComponent.generated.h"

/**
 * Trace mode for interaction detection
 */
UENUM(BlueprintType)
enum class EInteractTraceMode : uint8
{
	/** Trace from the camera component (default for FPS) */
	Camera UMETA(DisplayName = "Camera"),

	/** Trace from an actor point along an axis (not yet implemented) */
	Line UMETA(DisplayName = "Line (WIP)"),

	/** Cone/wide angle trace like AI perception (not yet implemented) */
	Cone UMETA(DisplayName = "Cone (WIP)")
};

/**
 * Delegate fired when a new interactable object is found
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableFound, AActor*, InteractableActor);

/**
 * Delegate fired when the current interactable object is lost
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractableLost, AActor*, InteractableActor);

/**
 * Delegate fired when interaction is triggered
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractTriggered, AActor*, InteractableActor, FHitResult, HitResult);

/**
 * Component that handles player interaction detection and triggering
 * Supports shooting rays to detect interactable objects (implementing IInteractable interface)
 * Designed to be flexible: detection and triggering logic stays in C++, while actual interaction
 * behavior is delegated to the interactable objects and can be overridden in Blueprint/Lua
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAMEPLAY_API UPlayerInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerInteractComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// ========== Configuration Properties (Blueprint/Lua configurable) ==========

	/** Maximum distance for interaction traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Trace", meta=(ClampMin="0.0"))
	float InteractDistance = 300.0f;

	/** Trace mode to use for interaction detection */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Trace")
	EInteractTraceMode TraceMode = EInteractTraceMode::Camera;

	/** Collision channel to use for interaction traces */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Enable continuous tick-based detection (if false, only detects when TryInteract is called) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Detection")
	bool bEnableTickDetection = true;

	/** Interval between tick detections in seconds (helps reduce performance cost) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Detection",
	          meta=(ClampMin="0.0", EditCondition="bEnableTickDetection"))
	float DetectionInterval = 0.1f;

	/** Draw debug lines for interaction traces (for development) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact|Debug")
	bool bDrawDebugTrace = false;

	// ========== Events/Delegates ==========

	/** Called when a new interactable object is detected */
	UPROPERTY(BlueprintAssignable, Category = "Interact|Events")
	FOnInteractableFound OnInteractableFound;

	/** Called when the current interactable object is no longer in range/focus */
	UPROPERTY(BlueprintAssignable, Category = "Interact|Events")
	FOnInteractableLost OnInteractableLost;

	/** Called when interaction is triggered */
	UPROPERTY(BlueprintAssignable, Category = "Interact|Events")
	FOnInteractTriggered OnInteractTriggered;

	// ========== Public Interface ==========

	/**
	 * Attempt to interact with the currently focused interactable object
	 * This should be called by input events (e.g., when player presses "E" key)
	 */
	UFUNCTION(BlueprintCallable, Category = "Interact")
	void TryInteract();

	/**
	 * Get the currently focused interactable actor (if any)
	 * @return The current interactable actor, or nullptr if none
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interact")
	AActor* GetCurrentInteractable() const { return CurrentInteractable; }

	/**
	 * Force an immediate interaction trace update (useful when tick detection is disabled)
	 */
	UFUNCTION(BlueprintCallable, Category = "Interact")
	void UpdateInteractableDetection();

	/**
	 * Set custom trace start position (for TraceMode = Custom, future use)
	 * @param Start - World position to start the trace from
	 * @param Direction - Direction to trace in
	 */
	UFUNCTION(BlueprintCallable, Category = "Interact")
	void SetCustomTraceStart(FVector Start, FVector Direction);

	// ========== Blueprint/Lua Accessible Setters (optional convenience) ==========

	UFUNCTION(BlueprintCallable, Category = "Interact")
	void SetInteractDistance(float NewDistance);

	UFUNCTION(BlueprintCallable, Category = "Interact")
	void SetDetectionInterval(float NewInterval);

protected:
	/**
	 * Perform the interaction trace and update current interactable
	 * @return True if a valid interactable was found
	 */
	bool PerformInteractTrace();

	/**
	 * Check if an actor implements the interactable interface and can be interacted with
	 * @param Actor - The actor to check
	 * @return True if the actor is interactable
	 */
	bool IsActorInteractable(AActor* Actor) const;

	/**
	 * Update the current interactable actor (handles focus/unfocus events)
	 * @param NewInteractable - The new interactable actor (can be nullptr)
	 */
	void SetCurrentInteractable(AActor* NewInteractable);

	/**
	 * Get the trace start and end positions based on current trace mode
	 * @param OutStart - Output start position
	 * @param OutEnd - Output end position
	 * @return True if trace parameters are valid
	 */
	bool GetTraceStartAndEnd(FVector& OutStart, FVector& OutEnd) const;

private:
	/** Currently focused interactable actor */
	UPROPERTY()
	TObjectPtr<AActor> CurrentInteractable = nullptr;

	/** Time accumulator for tick-based detection interval */
	float TimeSinceLastDetection = 0.0f;

	/** Custom trace start (for future TraceMode extensions) */
	FVector CustomTraceStartPos = FVector::ZeroVector;

	/** Custom trace direction (for future TraceMode extensions) */
	FVector CustomTraceDirection = FVector::ForwardVector;
};
