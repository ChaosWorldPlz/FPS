// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSAttributeSetBase.h"
#include "GameplayEffectExtension.h"

UFPSAttributeSetBase::UFPSAttributeSetBase()
{
}

AActor* UFPSAttributeSetBase::GetOwningActor() const
{
	return GetOwningAbilitySystemComponent() ? GetOwningAbilitySystemComponent()->GetOwner() : nullptr;
}

UAbilitySystemComponent* UFPSAttributeSetBase::GetOwningAbilitySystemComponent() const
{
	return Cast<UAbilitySystemComponent>(GetOuter());
}

void UFPSAttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	// Override in derived classes for attribute-specific clamping
}

void UFPSAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	// Override in derived classes for post-effect logic (e.g., death handling)
}

void UFPSAttributeSetBase::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue, float MinValue, float MaxValue) const
{
	NewValue = FMath::Clamp(NewValue, MinValue, MaxValue);
}

void UFPSAttributeSetBase::AdjustAttributeForMaxChange(const FGameplayAttribute& AffectedAttribute,
	const FGameplayAttribute& MaxAttribute,
	float NewMaxValue,
	const FGameplayAttribute& AffectedAttributeProperty) const
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const float CurrentMaxValue = ASC->GetNumericAttribute(MaxAttribute);
	if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && CurrentMaxValue > 0.0f)
	{
		// Calculate ratio and apply to affected attribute
		const float CurrentValue = ASC->GetNumericAttribute(AffectedAttribute);
		const float NewValue = (CurrentValue / CurrentMaxValue) * NewMaxValue;
		ASC->SetNumericAttributeBase(AffectedAttributeProperty, NewValue);
	}
}
