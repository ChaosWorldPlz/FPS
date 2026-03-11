// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_WeaponFire.h"
#include "AbilitySystemComponent.h"
#include "GameplayCueManager.h"
#include "FPS/Weapon/FPSWeaponBase.h"
#include "FPS/Weapon/FPSWeaponDataAsset.h"
#include "FPS/GAS/FPSGameplayTags.h"

UGA_WeaponFire::UGA_WeaponFire()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationPolicy = EFPSAbilityActivationPolicy::OnInputTriggered;

	// LocalPredicted: fires immediately on client, server validates via ServerFire RPC
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

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

	AFPSWeaponBase* Weapon = GetWeapon(Handle, ActorInfo);
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
		UE_LOG(LogTemp, Warning, TEXT("[GA_WeaponFire] CanActivateAbility failed at Super."));
		return false;
	}

	AFPSWeaponBase* Weapon = GetWeapon(Handle, ActorInfo);
	if (!Weapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_WeaponFire] CanActivateAbility failed: Weapon is null"));
		return false;
	}
	
	if (!Weapon->CanFire())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_WeaponFire] CanActivateAbility failed: Weapon->CanFire() returned false. State: %d, Ammo: %d"), (int)Weapon->CurrentState, Weapon->AmmoInfo.CurrentMagazine);
		return false;
	}

	return true;
}

AFPSWeaponBase* UGA_WeaponFire::GetWeapon(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	FGameplayAbilitySpecHandle SpecHandle = Handle.IsValid() ? Handle : CurrentSpecHandle;
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : GetAbilitySystemComponentFromActorInfo();

	if (SpecHandle.IsValid() && ASC)
	{
		if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(SpecHandle))
		{
			return Cast<AFPSWeaponBase>(Spec->SourceObject.Get());
		}
	}
	return nullptr;
}

void UGA_WeaponFire::FireWeapon()
{
	AFPSWeaponBase* Weapon = GetWeapon(CurrentSpecHandle, CurrentActorInfo);
	if (Weapon && Weapon->CanFire())
	{
		Weapon->Fire();

		// 触发 GameplayCue 播放开火音效和特效（仅本地客户端，避免重复）
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayCueParameters CueParams;
			CueParams.SourceObject = Weapon;
			CueParams.Location = Weapon->GetMuzzleLocation();
			CueParams.Normal = Weapon->GetMuzzleRotation().Vector();
			ASC->ExecuteGameplayCue(
				FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.Fire")),
				CueParams);
		}
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
