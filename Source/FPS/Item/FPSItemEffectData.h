// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "FPSItemEffectData.generated.h"

class UGameplayEffect;
class UGameplayAbility;
class UAnimMontage;
class USoundBase;

/**
 * EFPSItemEffectType
 *
 * Types of item effects.
 */
UENUM(BlueprintType)
enum class EFPSItemEffectType : uint8
{
	None			UMETA(DisplayName = "None"),
	InstantEffect	UMETA(DisplayName = "Instant Effect"),
	DurationEffect	UMETA(DisplayName = "Duration Effect"),
	GrantAbility	UMETA(DisplayName = "Grant Ability"),
	Custom			UMETA(DisplayName = "Custom")
};

/**
 * FFPSItemEffectEntry
 *
 * A single effect that can be applied by an item.
 */
USTRUCT(BlueprintType)
struct FFPSItemEffectEntry
{
	GENERATED_BODY()

	/** Type of effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EFPSItemEffectType EffectType = EFPSItemEffectType::None;

	/** Gameplay effect to apply */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "EffectType == EFPSItemEffectType::InstantEffect || EffectType == EFPSItemEffectType::DurationEffect"))
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	/** Ability to grant (temporary or permanent) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "EffectType == EFPSItemEffectType::GrantAbility"))
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** Effect magnitude (for scalable effects) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float Magnitude = 1.0f;

	/** Effect level */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Level = 1;

	/** Tags to apply with the effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTagContainer EffectTags;
};

/**
 * UFPSItemEffectData
 *
 * Data asset containing item usage effects and configuration.
 */
UCLASS(BlueprintType)
class FPS_API UFPSItemEffectData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFPSItemEffectData();

	//-------------------------------------------------------------------
	// Basic Info
	//-------------------------------------------------------------------

	/** Unique identifier for this effect data */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Info")
	FName EffectID;

	/** Display name for the use action */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Info")
	FText UseActionName;

	/** Description of the effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Info")
	FText EffectDescription;

	//-------------------------------------------------------------------
	// Effects
	//-------------------------------------------------------------------

	/** Effects to apply when item is used */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Effects")
	TArray<FFPSItemEffectEntry> Effects;

	/** Whether effects can stack with themselves */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Effects")
	bool bStackable = false;

	/** Maximum stack count */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Effects", meta = (EditCondition = "bStackable", ClampMin = "1"))
	int32 MaxStacks = 1;

	//-------------------------------------------------------------------
	// Usage Configuration
	//-------------------------------------------------------------------

	/** Time required to use the item (0 for instant) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Usage", meta = (ClampMin = "0"))
	float UseTime = 0.0f;

	/** Can the use action be cancelled */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Usage")
	bool bCanCancel = true;

	/** Cooldown after using */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Usage", meta = (ClampMin = "0"))
	float Cooldown = 0.0f;

	/** Whether the item is consumed on use */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Usage")
	bool bConsumeOnUse = true;

	/** Amount consumed per use */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Usage", meta = (EditCondition = "bConsumeOnUse", ClampMin = "1"))
	int32 ConsumeAmount = 1;

	/** Whether movement is blocked during use */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Usage")
	bool bBlockMovement = false;

	/** Required tags to use item */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Usage")
	FGameplayTagContainer RequiredTags;

	/** Blocked tags (cannot use if any of these are present) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Usage")
	FGameplayTagContainer BlockedTags;

	//-------------------------------------------------------------------
	// Visual/Audio
	//-------------------------------------------------------------------

	/** Animation to play during use */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Visual")
	TSoftObjectPtr<UAnimMontage> UseMontage;

	/** Sound to play on use start */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Audio")
	TSoftObjectPtr<USoundBase> UseStartSound;

	/** Sound to play on use complete */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Audio")
	TSoftObjectPtr<USoundBase> UseCompleteSound;

	/** Sound to play on use cancel */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Audio")
	TSoftObjectPtr<USoundBase> UseCancelSound;

	//-------------------------------------------------------------------
	// Utility
	//-------------------------------------------------------------------

	/** Check if item can be used instantly */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool IsInstantUse() const { return UseTime <= 0.0f; }

	// UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
