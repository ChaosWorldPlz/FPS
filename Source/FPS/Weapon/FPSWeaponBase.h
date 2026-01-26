// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPSWeaponTypes.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "FPSWeaponBase.generated.h"

class UFPSWeaponDataAsset;
class USkeletalMeshComponent;
class AFPSCharacter;
class UAbilitySystemComponent;
class UGameplayAbility;
struct FGameplayAbilitySpecHandle;

// Delegate for ammo changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChangedDelegate, int32, CurrentMagazine, int32, CurrentReserve);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponStateChangedDelegate, EFPSWeaponState, NewState);

/**
 * AFPSWeaponBase
 *
 * Base class for all weapons in the FPS project.
 * Handles weapon state, ammo, and integrates with GAS for abilities.
 */
UCLASS(Abstract, Blueprintable)
class FPS_API AFPSWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AFPSWeaponBase();

	//-------------------------------------------------------------------
	// Components
	//-------------------------------------------------------------------

	/** Weapon mesh component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USkeletalMeshComponent* WeaponMesh;

	//-------------------------------------------------------------------
	// Configuration
	//-------------------------------------------------------------------

	/** Weapon data asset */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UFPSWeaponDataAsset* WeaponData;

	//-------------------------------------------------------------------
	// State
	//-------------------------------------------------------------------

	/** Current weapon state */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	EFPSWeaponState CurrentState = EFPSWeaponState::Idle;

	/** Current ammo info */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	FWeaponAmmoInfo AmmoInfo;

	/** Current spread value */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	float CurrentSpread = 0.0f;

	/** Owning character */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon|State")
	TWeakObjectPtr<AFPSCharacter> OwningCharacter;

	//-------------------------------------------------------------------
	// Delegates
	//-------------------------------------------------------------------

	/** Broadcast when ammo changes */
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnAmmoChangedDelegate OnAmmoChanged;

	/** Broadcast when weapon state changes */
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnWeaponStateChangedDelegate OnWeaponStateChanged;

	//-------------------------------------------------------------------
	// Weapon Interface
	//-------------------------------------------------------------------

	/** Called when weapon is equipped */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void OnEquip(AFPSCharacter* NewOwner);

	/** Called when weapon is unequipped */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void OnUnequip();

	/** Try to fire the weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool TryFire();

	/** Actually perform the fire action */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Fire();

	/** Try to reload the weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual bool TryReload();

	/** Actually perform the reload */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void Reload();

	/** Finish reload (called when reload animation completes) */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void FinishReload();

	/** Cancel reload */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void CancelReload();

	/** Melee attack */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	virtual void MeleeAttack();

	//-------------------------------------------------------------------
	// State Queries
	//-------------------------------------------------------------------

	/** Check if weapon can fire */
	UFUNCTION(BlueprintCallable, Category = "Weapon|State")
	virtual bool CanFire() const;

	/** Check if weapon can reload */
	UFUNCTION(BlueprintCallable, Category = "Weapon|State")
	virtual bool CanReload() const;

	/** Check if weapon is idle */
	UFUNCTION(BlueprintCallable, Category = "Weapon|State")
	bool IsIdle() const { return CurrentState == EFPSWeaponState::Idle; }

	/** Check if weapon is reloading */
	UFUNCTION(BlueprintCallable, Category = "Weapon|State")
	bool IsReloading() const { return CurrentState == EFPSWeaponState::Reloading; }

	/** Check if weapon is firing */
	UFUNCTION(BlueprintCallable, Category = "Weapon|State")
	bool IsFiring() const { return CurrentState == EFPSWeaponState::Firing; }

	//-------------------------------------------------------------------
	// Ammo Management
	//-------------------------------------------------------------------

	/** Add ammo to reserve */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo")
	int32 AddAmmo(int32 Amount);

	/** Get current magazine count */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo")
	int32 GetCurrentMagazine() const { return AmmoInfo.CurrentMagazine; }

	/** Get current reserve count */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo")
	int32 GetCurrentReserve() const { return AmmoInfo.CurrentReserve; }

	/** Get magazine capacity */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo")
	int32 GetMagazineCapacity() const;

	//-------------------------------------------------------------------
	// Utility
	//-------------------------------------------------------------------

	/** Get the muzzle location */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FVector GetMuzzleLocation() const;

	/** Get the muzzle rotation */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FRotator GetMuzzleRotation() const;

	/** Get fire direction with spread applied */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FVector GetFireDirectionWithSpread() const;

	/** Get the owner's ability system component */
	UFUNCTION(BlueprintCallable, Category = "Weapon|GAS")
	UAbilitySystemComponent* GetOwnerASC() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	/** Set weapon state and broadcast change */
	void SetWeaponState(EFPSWeaponState NewState);

	/** Update spread based on time */
	void UpdateSpread(float DeltaTime);

	/** Increase spread after firing */
	void IncreaseSpread();

	/** Perform line trace for hit detection */
	FHitResult PerformLineTrace(const FVector& Start, const FVector& End) const;

	/** Apply damage to hit target */
	void ApplyDamage(const FHitResult& HitResult);

	//-------------------------------------------------------------------
	// GAS Integration
	//-------------------------------------------------------------------

	/** Granted ability handles */
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

	/** Grant weapon abilities to owner */
	virtual void GrantAbilities();

	/** Remove weapon abilities from owner */
	virtual void RemoveAbilities();

	//-------------------------------------------------------------------
	// Timers
	//-------------------------------------------------------------------

	/** Timer handle for fire cooldown */
	FTimerHandle FireCooldownTimerHandle;

	/** Timer handle for reload */
	FTimerHandle ReloadTimerHandle;

	/** Last fire time */
	float LastFireTime = 0.0f;

	/** Can fire again (cooldown check) */
	bool bCanFireAgain = true;

	/** Reset fire cooldown */
	void ResetFireCooldown();

private:
	/** Name of muzzle socket */
	static const FName MuzzleSocketName;
};
