// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSLevelFlowTypes.h"
#include "FPSExtractionZone.generated.h"

class UBoxComponent;
class UBillboardComponent;

/**
 * AFPSExtractionZone
 *
 * Extraction point where players can leave the raid.
 * Players must stay in the zone for a set duration to extract.
 */
UCLASS()
class FPS_API AFPSExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	AFPSExtractionZone();

	//-------------------------------------------------------------------
	// Components
	//-------------------------------------------------------------------

	/** Trigger volume for extraction */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* ExtractionVolume;

	//-------------------------------------------------------------------
	// Configuration
	//-------------------------------------------------------------------

	/** Display name for this extraction point */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
	FText ExtractionName;

	/** Time required to extract (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction", meta = (ClampMin = "0"))
	float ExtractionTime = 5.0f;

	/** Whether this extraction point is currently active */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
	bool bIsActive = true;

	/** Whether this extraction has requirements (keys, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction")
	bool bHasRequirements = false;

	/** Required item ID for extraction (if bHasRequirements is true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction", meta = (EditCondition = "bHasRequirements"))
	FName RequiredItemID;

	/** Whether extraction consumes the required item */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Extraction", meta = (EditCondition = "bHasRequirements"))
	bool bConsumeRequiredItem = true;

	//-------------------------------------------------------------------
	// State
	//-------------------------------------------------------------------

	/** Current extraction state */
	UPROPERTY(BlueprintReadOnly, Category = "Extraction|State")
	EFPSExtractionState ExtractionState = EFPSExtractionState::Available;

	/** Current extraction progress (0-1) */
	UPROPERTY(BlueprintReadOnly, Category = "Extraction|State")
	float CurrentProgress = 0.0f;

	/** Players currently in the zone */
	UPROPERTY(BlueprintReadOnly, Category = "Extraction|State")
	TArray<TObjectPtr<APawn>> PlayersInZone;
	// claude大人原意是 TArray<TWeakObjectPtr<APawn>> 但是蓝图不支持显示 弱类指针数组，退而求其次，使用硬引用指针，后面可能有问题todo

	//-------------------------------------------------------------------
	// Delegates
	//-------------------------------------------------------------------

	/** Called when extraction state changes */
	UPROPERTY(BlueprintAssignable, Category = "Extraction|Events")
	FOnRaidStateChanged OnExtractionStateChanged;

	/** Called when extraction progress updates */
	UPROPERTY(BlueprintAssignable, Category = "Extraction|Events")
	FOnExtractionProgress OnExtractionProgressUpdated;

	//-------------------------------------------------------------------
	// Interface
	//-------------------------------------------------------------------

	/** Activate this extraction zone */
	UFUNCTION(BlueprintCallable, Category = "Extraction")
	void ActivateZone();

	/** Deactivate this extraction zone */
	UFUNCTION(BlueprintCallable, Category = "Extraction")
	void DeactivateZone();

	/** Check if a player can extract here */
	UFUNCTION(BlueprintCallable, Category = "Extraction")
	bool CanPlayerExtract(APawn* Player) const;

	/** Get extraction progress (0-1) */
	UFUNCTION(BlueprintCallable, Category = "Extraction")
	float GetExtractionProgress() const { return CurrentProgress; }

	/** Reset extraction progress */
	UFUNCTION(BlueprintCallable, Category = "Extraction")
	void ResetProgress();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Called when an actor enters the zone */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Called when an actor leaves the zone */
	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/** Update extraction progress */
	void UpdateExtractionProgress(float DeltaTime);

	/** Complete extraction for all players in zone */
	void CompleteExtraction();

	/** Set extraction state */
	void SetExtractionState(EFPSExtractionState NewState);
};
