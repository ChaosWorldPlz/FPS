// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPS/GAS/FPSGameplayAbility.h"
#include "GA_WeaponReload.generated.h"

class AFPSWeaponBase;

/**
 * UGA_WeaponReload
 *
 * Gameplay ability for reloading a weapon.
 */
UCLASS()
class FPS_API UGA_WeaponReload : public UFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_WeaponReload();

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
	/** Get the weapon from the source object */
	AFPSWeaponBase* GetWeapon(const FGameplayAbilitySpecHandle Handle = FGameplayAbilitySpecHandle(), const FGameplayAbilityActorInfo* ActorInfo = nullptr) const;

	/** Called when reload completes */
	void OnReloadComplete();

	/** Timer handle for reload */
	FTimerHandle ReloadTimerHandle;
};
