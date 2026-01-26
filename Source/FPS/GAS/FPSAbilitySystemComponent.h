// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "FPSAbilitySystemComponent.generated.h"

/**
 * UFPSAbilitySystemComponent
 *
 * Custom AbilitySystemComponent for the FPS project.
 * Provides convenient wrapper functions for common GAS operations.
 */
UCLASS()
class FPS_API UFPSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UFPSAbilitySystemComponent();

	/** Initialize the ASC with owner info */
	void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor);

	/** Grant an ability and return its handle */
	UFUNCTION(BlueprintCallable, Category = "FPS|Abilities")
	FGameplayAbilitySpecHandle GrantAbility(TSubclassOf<UGameplayAbility> AbilityClass, int32 Level = 1, int32 InputID = -1);

	/** Grant multiple abilities at once */
	UFUNCTION(BlueprintCallable, Category = "FPS|Abilities")
	void GrantAbilities(const TArray<TSubclassOf<UGameplayAbility>>& AbilityClasses, int32 Level = 1);

	/** Remove an ability by class */
	UFUNCTION(BlueprintCallable, Category = "FPS|Abilities")
	void RemoveAbility(TSubclassOf<UGameplayAbility> AbilityClass);

	/** Try to activate ability by class */
	UFUNCTION(BlueprintCallable, Category = "FPS|Abilities")
	bool TryActivateAbilityByClassBP(TSubclassOf<UGameplayAbility> AbilityClass);

	/** Apply a GameplayEffect to self */
	UFUNCTION(BlueprintCallable, Category = "FPS|Effects")
	FActiveGameplayEffectHandle ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass, float Level = 1.0f);

	/** Apply a GameplayEffect spec to self */
	FActiveGameplayEffectHandle ApplyEffectSpecToSelf(const FGameplayEffectSpec& Spec);

	/** Check if an ability with the given tag is active */
	UFUNCTION(BlueprintCallable, Category = "FPS|Abilities")
	bool IsAbilityActiveWithTag(FGameplayTag AbilityTag) const;

	/** Get the current value of an attribute */
	UFUNCTION(BlueprintCallable, Category = "FPS|Attributes")
	float GetAttributeValue(FGameplayAttribute Attribute) const;

	/** Set the base value of an attribute */
	UFUNCTION(BlueprintCallable, Category = "FPS|Attributes")
	void SetAttributeBaseValue(FGameplayAttribute Attribute, float NewValue);

protected:
	/** Called when the component is initialized */
	virtual void BeginPlay() override;

	/** Cached owner actor */
	UPROPERTY()
	TWeakObjectPtr<AActor> CachedOwnerActor;

	/** Cached avatar actor */
	UPROPERTY()
	TWeakObjectPtr<AActor> CachedAvatarActor;
};
