// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPS/GAS/FPSGameplayAbility.h"
#include "GA_WeaponFire.generated.h"

class AFPSWeaponBase;

/**
 * UGA_WeaponFire
 *
 * Gameplay ability for firing a weapon.
 * Supports both single shot and automatic fire modes.
 */
UCLASS()
class FPS_API UGA_WeaponFire : public UFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_WeaponFire();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

protected:
	/** Get the weapon from the source object */
	AFPSWeaponBase* GetWeapon() const;

	/** Fire the weapon once */
	void FireWeapon();

	/** Timer callback for automatic fire */
	void AutoFireTick();

	/** Whether automatic fire is active */
	bool bAutoFiring = false;

	/** Timer handle for auto fire */
	FTimerHandle AutoFireTimerHandle;
};
