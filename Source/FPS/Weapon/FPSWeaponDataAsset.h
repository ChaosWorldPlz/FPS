// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FPSWeaponTypes.h"
#include "FPSAttachmentTypes.h"
#include "GameplayTagContainer.h"
#include "FPSWeaponDataAsset.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UAnimMontage;
class USoundBase;
class UParticleSystem;
class USkeletalMesh;
class UStaticMesh;

/**
 * UFPSWeaponDataAsset
 *
 * Data asset containing all configuration for a weapon type.
 * This allows weapons to be data-driven and easily configured in the editor.
 */
UCLASS(BlueprintType)
class FPS_API UFPSWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UFPSWeaponDataAsset();

	//-------------------------------------------------------------------
	// Basic Info
	//-------------------------------------------------------------------

	/** Unique identifier for this weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Info")
	FName WeaponID;

	/** Display name */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Info")
	FText WeaponName;

	/** Description */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Info")
	FText Description;

	/** Weapon type */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Info")
	EFPSWeaponType WeaponType = EFPSWeaponType::Rifle;

	/** Fire mode */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Info")
	EFPSWeaponFireMode FireMode = EFPSWeaponFireMode::Auto;

	/** Gameplay tag identifying this weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Info")
	FGameplayTag WeaponTag;

	//-------------------------------------------------------------------
	// Combat Stats
	//-------------------------------------------------------------------

	/** Base damage per shot */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "0"))
	float BaseDamage = 25.0f;

	/** Shots per minute (fire rate) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "1"))
	float FireRate = 600.0f;

	/** Maximum effective range */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "0"))
	float MaxRange = 5000.0f;

	/** Damage falloff start range */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "0"))
	float FalloffStartRange = 2000.0f;

	/** Minimum damage multiplier at max range */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "0", ClampMax = "1"))
	float MinDamageMultiplier = 0.5f;

	/** Base spread (in degrees) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "0"))
	float BaseSpread = 1.0f;

	/** Spread increase per shot */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "0"))
	float SpreadIncreasePerShot = 0.5f;

	/** Maximum spread */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "0"))
	float MaxSpread = 5.0f;

	/** Spread recovery rate (per second) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "0"))
	float SpreadRecoveryRate = 10.0f;

	/** Headshot damage multiplier */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "1"))
	float HeadshotMultiplier = 2.0f;

	/** Pellets per shot (for shotguns) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Combat", meta = (ClampMin = "1"))
	int32 PelletsPerShot = 1;

	//-------------------------------------------------------------------
	// Ammo
	//-------------------------------------------------------------------

	/** Whether this weapon uses ammo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	bool bUsesAmmo = true;

	/** Magazine capacity */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo", meta = (EditCondition = "bUsesAmmo", ClampMin = "1"))
	int32 MagazineSize = 30;

	/** Maximum reserve ammo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo", meta = (EditCondition = "bUsesAmmo", ClampMin = "0"))
	int32 MaxReserveAmmo = 120;

	/** Starting reserve ammo */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo", meta = (EditCondition = "bUsesAmmo", ClampMin = "0"))
	int32 StartingReserveAmmo = 90;

	/** Reload time in seconds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo", meta = (EditCondition = "bUsesAmmo", ClampMin = "0"))
	float ReloadTime = 2.0f;

	//-------------------------------------------------------------------
	// Timing
	//-------------------------------------------------------------------

	/** Time to equip weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Timing", meta = (ClampMin = "0"))
	float EquipTime = 0.5f;

	/** Time to unequip weapon */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Timing", meta = (ClampMin = "0"))
	float UnequipTime = 0.3f;

	//-------------------------------------------------------------------
	// Visual Assets
	//-------------------------------------------------------------------

	/** First person weapon mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visuals")
	TSoftObjectPtr<USkeletalMesh> FirstPersonMesh;

	/** Third person weapon mesh */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visuals")
	TSoftObjectPtr<USkeletalMesh> ThirdPersonMesh;

	/** Muzzle flash effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visuals")
	TSoftObjectPtr<UParticleSystem> MuzzleFlashEffect;

	/** Impact effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visuals")
	TSoftObjectPtr<UParticleSystem> ImpactEffect;

	/** Tracer effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Visuals")
	TSoftObjectPtr<UParticleSystem> TracerEffect;

	//-------------------------------------------------------------------
	// Audio Assets
	//-------------------------------------------------------------------

	/** Fire sound */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TSoftObjectPtr<USoundBase> FireSound;

	/** Empty fire (dry) sound */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TSoftObjectPtr<USoundBase> DryFireSound;

	/** Reload start sound */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TSoftObjectPtr<USoundBase> ReloadStartSound;

	/** Reload finish sound */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TSoftObjectPtr<USoundBase> ReloadFinishSound;

	/** Equip sound */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Audio")
	TSoftObjectPtr<USoundBase> EquipSound;

	//-------------------------------------------------------------------
	// Animations
	//-------------------------------------------------------------------

	/** Fire animation montage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	TSoftObjectPtr<UAnimMontage> FireMontage;

	/** Reload animation montage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	TSoftObjectPtr<UAnimMontage> ReloadMontage;

	/** Equip animation montage */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	TSoftObjectPtr<UAnimMontage> EquipMontage;

	/** Melee animation montage (for melee weapons) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	TSoftObjectPtr<UAnimMontage> MeleeMontage;

	//-------------------------------------------------------------------
	// GAS Integration
	//-------------------------------------------------------------------

	/** 开火能力类（支持 Blueprint 子类，装备时由武器 GrantAbilities 授予） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|GAS")
	TSubclassOf<UGameplayAbility> FireAbilityClass;

	/** 换弹能力类 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|GAS")
	TSubclassOf<UGameplayAbility> ReloadAbilityClass;

	/** 近战能力类 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|GAS")
	TSubclassOf<UGameplayAbility> MeleeAbilityClass;

	/** 伤害效果类 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	//-------------------------------------------------------------------
	// Utility Functions
	//-------------------------------------------------------------------

	/** Get time between shots in seconds */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	float GetTimeBetweenShots() const { return 60.0f / FireRate; }

	/** Get damage at a given range */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	float GetDamageAtRange(float Range) const;

	/** Get default ammo info */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FWeaponAmmoInfo GetDefaultAmmoInfo() const;

	// UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	//-------------------------------------------------------------------
	// Attachments
	//-------------------------------------------------------------------

	/** Attachment slot types this weapon supports (checked by InstallAttachment) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Attachments")
	TSet<EFPSAttachmentSlotType> SupportedAttachmentSlots;

	/** True for pistols — restricts this weapon to the Pistol carry slot */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Info")
	bool bIsPistol = false;
};
