// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_WeaponReload.h"
#include "AbilitySystemComponent.h"
#include "FPS/FPSCharacter.h"
#include "FPS/Weapon/FPSWeaponBase.h"
#include "FPS/Weapon/FPSWeaponDataAsset.h"
#include "FPS/GAS/FPSGameplayTags.h"

UGA_WeaponReload::UGA_WeaponReload()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationPolicy = EFPSAbilityActivationPolicy::OnInputTriggered;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Set ability tags — use RequestGameplayTag to avoid CDO-before-InitializeNativeTags timing issue
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("FPS.Ability.Weapon.Reload"), false));

	// Cancelled by firing
	CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("FPS.Ability.Weapon.Reload"), false));

	// Blocked while reloading
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("FPS.State.Reloading"), false));
}

void UGA_WeaponReload::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AFPSWeaponBase* Weapon = GetWeapon(Handle, ActorInfo);
	if (!Weapon || !Weapon->WeaponData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Start reload on weapon
	Weapon->Reload();

	// Use effective reload time (accounts for attachment modifiers)
	float ReloadTime = Weapon->GetEffectiveReloadTime();

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
	
	// Add reloading tag — 用 static const 避免依赖 FFPSGameplayTags 单例初始化时序
	static const FGameplayTag ReloadingTag = FGameplayTag::RequestGameplayTag(FName("FPS.State.Reloading"));
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(ReloadingTag);
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
	static const FGameplayTag ReloadingTag = FGameplayTag::RequestGameplayTag(FName("FPS.State.Reloading"));
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->RemoveLooseGameplayTag(ReloadingTag);
	}

	// If cancelled, cancel the weapon reload
	if (bWasCancelled)
	{
		AFPSWeaponBase* Weapon = GetWeapon(Handle, ActorInfo);
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

	AFPSWeaponBase* Weapon = GetWeapon(Handle, ActorInfo);
	return Weapon && Weapon->CanReload();
}

AFPSWeaponBase* UGA_WeaponReload::GetWeapon(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	FGameplayAbilitySpecHandle SpecHandle = Handle.IsValid() ? Handle : CurrentSpecHandle;
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : GetAbilitySystemComponentFromActorInfo();

	if (SpecHandle.IsValid() && ASC)
	{
		if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AFPSWeaponBase* Weapon = Cast<AFPSWeaponBase>(Spec->SourceObject.Get()))
			{
				return Weapon;
			}
		}
	}

	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : GetAvatarActorFromActorInfo();
	if (const AFPSCharacter* Char = Cast<const AFPSCharacter>(Avatar))
	{
		return Char->GetCurrentWeapon();
	}
	return nullptr;
}

void UGA_WeaponReload::OnReloadComplete()
{
	// Weapon handles its own finish reload through its timer
	// Just end the ability
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
