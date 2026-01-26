// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_WeaponFire.h"
#include "AbilitySystemComponent.h"
#include "FPS/Weapon/FPSWeaponBase.h"
#include "FPS/Weapon/FPSWeaponDataAsset.h"
#include "FPS/GAS/FPSGameplayTags.h"

UGA_WeaponFire::UGA_WeaponFire()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationPolicy = EFPSAbilityActivationPolicy::OnInputTriggered;

	// Set ability tags
	AbilityTags.AddTag(FFPSGameplayTags::Get().Ability_Weapon_Fire);

	// Block other weapon abilities while firing
	BlockAbilitiesWithTag.AddTag(FFPSGameplayTags::Get().Ability_Weapon_Reload);
}

void UGA_WeaponFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AFPSWeaponBase* Weapon = GetWeapon();
	if (!Weapon)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Fire once immediately
	FireWeapon();

	// Check for automatic fire mode
	UFPSWeaponDataAsset* WeaponData = Weapon->WeaponData;
	if (WeaponData && WeaponData->FireMode == EFPSWeaponFireMode::Auto)
	{
		// Start auto fire timer
		bAutoFiring = true;
		float FireDelay = WeaponData->GetTimeBetweenShots();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				AutoFireTimerHandle,
				this,
				&UGA_WeaponFire::AutoFireTick,
				FireDelay,
				true
			);
		}
	}
	else
	{
		// Single shot, end ability
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_WeaponFire::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear auto fire timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFireTimerHandle);
	}
	bAutoFiring = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_WeaponFire::InputReleased(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// Stop auto fire when input is released
	if (bAutoFiring)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}

	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
}

bool UGA_WeaponFire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	AFPSWeaponBase* Weapon = GetWeapon();
	return Weapon && Weapon->CanFire();
}

AFPSWeaponBase* UGA_WeaponFire::GetWeapon() const
{
	// Get weapon from the ability spec's source object
	if (CurrentSpecHandle.IsValid())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (!ASC)
		{
			return nullptr;
		}
		
		if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle))
		{
			return Cast<AFPSWeaponBase>(Spec->SourceObject.Get());
		}
	}
	return nullptr;
}

void UGA_WeaponFire::FireWeapon()
{
	AFPSWeaponBase* Weapon = GetWeapon();
	if (Weapon && Weapon->CanFire())
	{
		Weapon->Fire();
	}
	else if (bAutoFiring)
	{
		// Can't fire anymore (out of ammo), stop auto fire
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_WeaponFire::AutoFireTick()
{
	FireWeapon();
}
