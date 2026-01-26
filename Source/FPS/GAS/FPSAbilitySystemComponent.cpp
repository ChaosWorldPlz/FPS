// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSAbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UFPSAbilitySystemComponent::UFPSAbilitySystemComponent()
{
	// Default settings for single-player FPS
	ReplicationMode = EGameplayEffectReplicationMode::Full;
}

void UFPSAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	CachedOwnerActor = InOwnerActor;
	CachedAvatarActor = InAvatarActor;
}

void UFPSAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
}

FGameplayAbilitySpecHandle UFPSAbilitySystemComponent::GrantAbility(TSubclassOf<UGameplayAbility> AbilityClass, int32 Level, int32 InputID)
{
	if (!AbilityClass)
	{
		return FGameplayAbilitySpecHandle();
	}

	FGameplayAbilitySpec AbilitySpec(AbilityClass, Level, InputID, GetOwner());
	return GiveAbility(AbilitySpec);
}

void UFPSAbilitySystemComponent::GrantAbilities(const TArray<TSubclassOf<UGameplayAbility>>& AbilityClasses, int32 Level)
{
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : AbilityClasses)
	{
		GrantAbility(AbilityClass, Level);
	}
}

void UFPSAbilitySystemComponent::RemoveAbility(TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!AbilityClass)
	{
		return;
	}

	FGameplayAbilitySpec* Spec = FindAbilitySpecFromClass(AbilityClass);
	if (Spec)
	{
		ClearAbility(Spec->Handle);
	}
}

bool UFPSAbilitySystemComponent::TryActivateAbilityByClassBP(TSubclassOf<UGameplayAbility> AbilityClass)
{
	return TryActivateAbilityByClass(AbilityClass);
}

FActiveGameplayEffectHandle UFPSAbilitySystemComponent::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass, float Level)
{
	if (!EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle ContextHandle = MakeEffectContext();
	ContextHandle.AddSourceObject(GetOwner());

	FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(EffectClass, Level, ContextHandle);
	if (SpecHandle.IsValid())
	{
		return ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	return FActiveGameplayEffectHandle();
}

FActiveGameplayEffectHandle UFPSAbilitySystemComponent::ApplyEffectSpecToSelf(const FGameplayEffectSpec& Spec)
{
	return ApplyGameplayEffectSpecToSelf(Spec);
}

bool UFPSAbilitySystemComponent::IsAbilityActiveWithTag(FGameplayTag AbilityTag) const
{
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(AbilityTag);

	TArray<FGameplayAbilitySpec*> MatchingAbilities;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(TagContainer, MatchingAbilities);

	for (const FGameplayAbilitySpec* Spec : MatchingAbilities)
	{
		if (Spec && Spec->IsActive())
		{
			return true;
		}
	}

	return false;
}

float UFPSAbilitySystemComponent::GetAttributeValue(FGameplayAttribute Attribute) const
{
	if (!Attribute.IsValid())
	{
		return 0.0f;
	}

	/*float Value = 0.0f;
	GetGameplayAttributeValue(Attribute,true);
	return Value;*/
	return GetNumericAttributeBase(Attribute);
}

void UFPSAbilitySystemComponent::SetAttributeBaseValue(FGameplayAttribute Attribute, float NewValue)
{
	if (Attribute.IsValid())
	{
		SetNumericAttributeBase(Attribute, NewValue);
	}
}
