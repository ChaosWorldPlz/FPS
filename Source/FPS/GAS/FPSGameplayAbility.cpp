// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSGameplayAbility.h"
#include "FPSAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"

UFPSGameplayAbility::UFPSGameplayAbility()
{
	// Default values
	ActivationPolicy = EFPSAbilityActivationPolicy::OnInputTriggered;

	// Default instancing policy - instanced per actor for state retention
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Net execution policy for single player
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;

	// Net security policy
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;
}

UFPSAbilitySystemComponent* UFPSGameplayAbility::GetFPSAbilitySystemComponentFromActorInfo() const
{
	return Cast<UFPSAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

void UFPSGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	// Auto-activate abilities with OnGranted policy
	if (ActivationPolicy == EFPSAbilityActivationPolicy::OnGranted)
	{
		if (ActorInfo && !Spec.IsActive())
		{
			ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
		}
	}
}

void UFPSGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnRemoveAbility(ActorInfo, Spec);
}

bool UFPSGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return true;
}

void UFPSGameplayAbility::InputPressed(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);
}

void UFPSGameplayAbility::InputReleased(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputReleased(Handle, ActorInfo, ActivationInfo);

	// End ability when input is released for WhileInputHeld policy
	if (ActivationPolicy == EFPSAbilityActivationPolicy::WhileInputHeld)
	{
		CancelAbility(Handle, ActorInfo, ActivationInfo, true);
	}
}

FActiveGameplayEffectHandle UFPSGameplayAbility::ApplyEffectToOwner(TSubclassOf<UGameplayEffect> EffectClass)
{
	if (!EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());

	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EffectClass, GetAbilityLevel(), ContextHandle);
	if (SpecHandle.IsValid())
	{
		return ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	return FActiveGameplayEffectHandle();
}

FActiveGameplayEffectHandle UFPSGameplayAbility::ApplyEffectSpecToOwner(const FGameplayEffectSpecHandle& SpecHandle)
{
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return FActiveGameplayEffectHandle();
	}

	return ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UFPSGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
}

void UFPSGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

bool UFPSGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}
