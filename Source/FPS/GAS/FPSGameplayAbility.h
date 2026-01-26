// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "FPSGameplayAbility.generated.h"

class UFPSAbilitySystemComponent;

/**
 * EFPSAbilityActivationPolicy
 *
 * Defines how an ability can be activated.
 */
UENUM(BlueprintType)
enum class EFPSAbilityActivationPolicy : uint8
{
	// Ability is activated when the input is triggered
	OnInputTriggered,

	// Ability is activated while the input is held
	WhileInputHeld,

	// Ability is automatically activated when granted
	OnGranted
};

/**
 * UFPSGameplayAbility
 *
 * Base class for all gameplay abilities in the FPS project.
 * Provides convenient accessor methods and common functionality.
 */
UCLASS()
class FPS_API UFPSGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UFPSGameplayAbility();

	/** How this ability is activated */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Ability")
	EFPSAbilityActivationPolicy ActivationPolicy;

	/** Get the FPS-specific ability system component */
	UFUNCTION(BlueprintCallable, Category = "FPS|Ability")
	UFPSAbilitySystemComponent* GetFPSAbilitySystemComponentFromActorInfo() const;

	/** Get the avatar actor as a specific type */
	template<class T>
	T* GetAvatarActorAs() const
	{
		return Cast<T>(GetAvatarActorFromActorInfo());
	}

	/** Get the owning actor as a specific type */
	template<class T>
	T* GetOwningActorAs() const
	{
		return Cast<T>(GetOwningActorFromActorInfo());
	}

protected:
	/** Called when the ability is granted to an actor */
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	/** Called when the ability is removed from an actor */
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

	/** Check if the ability can be activated */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	/** Called when ability input is pressed */
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	/** Called when ability input is released */
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	/** Apply a gameplay effect to the ability owner using this ability's level */
	UFUNCTION(BlueprintCallable, Category = "FPS|Ability")
	FActiveGameplayEffectHandle ApplyEffectToOwner(TSubclassOf<UGameplayEffect> EffectClass);

	/** Apply a gameplay effect spec to the ability owner */
	UFUNCTION(BlueprintCallable, Category = "FPS|Ability")
	FActiveGameplayEffectHandle ApplyEffectSpecToOwner(const FGameplayEffectSpecHandle& SpecHandle);

	/** Apply cooldown effect (override for custom cooldown logic) */
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** Apply cost effect (override for custom cost logic) */
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** Check if the ability cost can be paid */
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
};
