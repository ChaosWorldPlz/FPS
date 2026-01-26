// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPS/GAS/FPSGameplayAbility.h"
#include "GA_UseItem.generated.h"

class UFPSItemEffectData;

/**
 * UGA_UseItem
 *
 * Gameplay ability for using inventory items.
 * Handles use time, cancellation, and effect application.
 */
UCLASS()
class FPS_API UGA_UseItem : public UFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_UseItem();

	/** Set the item effect data to use */
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetItemEffectData(UFPSItemEffectData* InEffectData);

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	/** The item effect data to apply */
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	UFPSItemEffectData* ItemEffectData;

	/** Apply the item effects */
	void ApplyItemEffects();

	/** Called when use timer completes */
	void OnUseComplete();

	/** Play use animation if available */
	void PlayUseAnimation();

	/** Timer handle for use time */
	FTimerHandle UseTimerHandle;
	
};
