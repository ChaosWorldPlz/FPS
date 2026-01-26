// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_WeaponReload.h"
#include "AbilitySystemComponent.h"
#include "FPS/Weapon/FPSWeaponBase.h"
#include "FPS/Weapon/FPSWeaponDataAsset.h"
#include "FPS/GAS/FPSGameplayTags.h"

UGA_WeaponReload::UGA_WeaponReload()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationPolicy = EFPSAbilityActivationPolicy::OnInputTriggered;

	// Set ability tags
	AbilityTags.AddTag(FFPSGameplayTags::Get().Ability_Weapon_Reload);

	// Cancelled by firing
	CancelAbilitiesWithTag.AddTag(FFPSGameplayTags::Get().Ability_Weapon_Reload);

	// Blocked while reloading
	ActivationBlockedTags.AddTag(FFPSGameplayTags::Get().State_Reloading);
}

void UGA_WeaponReload::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AFPSWeaponBase* Weapon = GetWeapon();
	if (!Weapon || !Weapon->WeaponData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Start reload on weapon
	Weapon->Reload();

	// Set timer for reload completion
	float ReloadTime = Weapon->WeaponData->ReloadTime;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ReloadTimerHandle,
			this,
			&UGA_WeaponReload::OnReloadComplete,
			ReloadTime,
			false
		);
	}
	
	// Add reloading tag
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(FFPSGameplayTags::Get().State_Reloading);
	}
}

void UGA_WeaponReload::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}

	// Remove reloading tag
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(FFPSGameplayTags::Get().State_Reloading);
	}

	// If cancelled, cancel the weapon reload
	if (bWasCancelled)
	{
		AFPSWeaponBase* Weapon = GetWeapon();
		if (Weapon)
		{
			Weapon->CancelReload();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_WeaponReload::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	return Weapon && Weapon->CanReload();
}

AFPSWeaponBase* UGA_WeaponReload::GetWeapon() const
{
	// Get weapon from the ability spec's source object
	if (CurrentSpecHandle.IsValid())
	{
		if (const FGameplayAbilitySpec* Spec = GetAbilitySystemComponentFromActorInfo()->FindAbilitySpecFromHandle(CurrentSpecHandle))
		{
			return Cast<AFPSWeaponBase>(Spec->SourceObject.Get());
		}
	}
	return nullptr;
}

void UGA_WeaponReload::OnReloadComplete()
{
	// Weapon handles its own finish reload through its timer
	// Just end the ability
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
