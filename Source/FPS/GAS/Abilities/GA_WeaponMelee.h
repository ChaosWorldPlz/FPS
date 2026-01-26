// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPS/GAS/FPSGameplayAbility.h"
#include "GA_WeaponMelee.generated.h"

class AFPSWeaponBase;

/**
 * UGA_WeaponMelee
 *
 * Gameplay ability for melee attacks.
 * Can be used with any weapon for quick melee, or as primary attack for melee weapons.
 */
UCLASS()
class FPS_API UGA_WeaponMelee : public UFPSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_WeaponMelee();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	/** Base melee damage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	float MeleeDamage = 50.0f;

	/** Melee range */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	float MeleeRange = 150.0f;

	/** Melee attack radius (for sweep) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	float MeleeRadius = 30.0f;

	/** Duration of melee attack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	float MeleeDuration = 0.5f;

	/** Damage effect class */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Melee")
	TSubclassOf<UGameplayEffect> MeleeDamageEffect;

	/** Perform the melee attack */
	void PerformMeleeAttack();

	/** Timer handle for melee duration */
	FTimerHandle MeleeTimerHandle;
};
