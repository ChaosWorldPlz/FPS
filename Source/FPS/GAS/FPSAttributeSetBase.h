// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "FPSAttributeSetBase.generated.h"

// Macros for attribute accessors
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * UFPSAttributeSetBase
 *
 * Base class for all attribute sets in the FPS project.
 * Provides common functionality and helper methods.
 */
UCLASS()
class FPS_API UFPSAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()

public:
	UFPSAttributeSetBase();

	/** Get the owning actor of this attribute set */
	UFUNCTION(BlueprintCallable, Category = "FPS|Attributes")
	AActor* GetOwningActor() const;

	/** Get the owning ability system component */
	UFUNCTION(BlueprintCallable, Category = "FPS|Attributes")
	UAbilitySystemComponent* GetOwningAbilitySystemComponent() const;

	/** Called before an attribute is modified. Use this to clamp values. */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	/** Called after an attribute is modified via a GameplayEffect. */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

protected:
	/** Clamp a value between min and max */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue, float MinValue, float MaxValue) const;

	/** Adjust attribute to not exceed maximum attribute value */
	void AdjustAttributeForMaxChange(const FGameplayAttribute& AffectedAttribute,
		const FGameplayAttribute& MaxAttribute,
		float NewMaxValue,
		const FGameplayAttribute& AffectedAttributeProperty) const;
};
