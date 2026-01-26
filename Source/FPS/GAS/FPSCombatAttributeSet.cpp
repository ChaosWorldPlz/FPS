// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSCombatAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"

UFPSCombatAttributeSet::UFPSCombatAttributeSet()
	: bDead(false)
{
	// Default values
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitArmor(0.0f);
	InitMaxArmor(100.0f);
	InitArmorReduction(0.5f); // 50% damage reduction when armor absorbs
	InitStamina(100.0f);
	InitMaxStamina(100.0f);
	InitStaminaRegenRate(10.0f); // 10 stamina per second
	InitMovementSpeed(600.0f); // Default walk speed
	InitIncomingDamage(0.0f);
	InitIncomingHeal(0.0f);
}

void UFPSCombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UFPSCombatAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSCombatAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSCombatAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSCombatAttributeSet, MaxArmor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSCombatAttributeSet, ArmorReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSCombatAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSCombatAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSCombatAttributeSet, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UFPSCombatAttributeSet, MovementSpeed, COND_None, REPNOTIFY_Always);
}

void UFPSCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Clamp Health
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	// Clamp MaxHealth
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	// Clamp Armor
	else if (Attribute == GetArmorAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxArmor());
	}
	// Clamp MaxArmor
	else if (Attribute == GetMaxArmorAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	// Clamp ArmorReduction (0 - 1)
	else if (Attribute == GetArmorReductionAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
	}
	// Clamp Stamina
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}
	// Clamp MaxStamina
	else if (Attribute == GetMaxStaminaAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.0f);
	}
	// Clamp MovementSpeed
	else if (Attribute == GetMovementSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.0f);
	}
}

void UFPSCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// Handle incoming damage
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		HandleDamage(Data);
	}
	// Handle incoming heal
	else if (Data.EvaluatedData.Attribute == GetIncomingHealAttribute())
	{
		HandleHeal(Data);
	}
}

void UFPSCombatAttributeSet::HandleDamage(const FGameplayEffectModCallbackData& Data)
{
	if (bDead)
	{
		return;
	}

	float DamageAmount = GetIncomingDamage();
	SetIncomingDamage(0.0f);

	if (DamageAmount <= 0.0f)
	{
		return;
	}

	// Get the instigator
	AActor* Instigator = nullptr;
	if (Data.EffectSpec.GetContext().GetEffectCauser())
	{
		Instigator = Data.EffectSpec.GetContext().GetEffectCauser();
	}

	// Calculate armor absorption
	float RemainingDamage = DamageAmount;
	float CurrentArmor = GetArmor();

	if (CurrentArmor > 0.0f)
	{
		// Armor absorbs damage with reduction
		float ArmorDamage = FMath::Min(CurrentArmor, RemainingDamage);
		float AbsorbedDamage = ArmorDamage * GetArmorReduction();
		RemainingDamage -= AbsorbedDamage;

		// Reduce armor
		float OldArmor = CurrentArmor;
		float NewArmor = FMath::Max(0.0f, CurrentArmor - ArmorDamage);
		SetArmor(NewArmor);

		OnArmorChanged.Broadcast(OldArmor, NewArmor, Instigator);
	}

	// Apply remaining damage to health
	if (RemainingDamage > 0.0f)
	{
		float OldHealth = GetHealth();
		float NewHealth = FMath::Max(0.0f, OldHealth - RemainingDamage);
		SetHealth(NewHealth);

		OnHealthChanged.Broadcast(OldHealth, NewHealth, Instigator);

		// Check for death
		CheckDeath(Instigator);
	}
}

void UFPSCombatAttributeSet::HandleHeal(const FGameplayEffectModCallbackData& Data)
{
	if (bDead)
	{
		return;
	}

	float HealAmount = GetIncomingHeal();
	SetIncomingHeal(0.0f);

	if (HealAmount <= 0.0f)
	{
		return;
	}

	// Get the instigator
	AActor* Instigator = nullptr;
	if (Data.EffectSpec.GetContext().GetEffectCauser())
	{
		Instigator = Data.EffectSpec.GetContext().GetEffectCauser();
	}

	// Apply heal
	float OldHealth = GetHealth();
	float NewHealth = FMath::Min(GetMaxHealth(), OldHealth + HealAmount);
	SetHealth(NewHealth);

	OnHealthChanged.Broadcast(OldHealth, NewHealth, Instigator);
}

void UFPSCombatAttributeSet::CheckDeath(AActor* Instigator)
{
	if (bDead)
	{
		return;
	}

	if (GetHealth() <= 0.0f)
	{
		bDead = true;
		OnDeath.Broadcast(Instigator);
	}
}

//-------------------------------------------------------------------
// Replication callbacks
//-------------------------------------------------------------------

void UFPSCombatAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSCombatAttributeSet, Health, OldValue);
}

void UFPSCombatAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSCombatAttributeSet, MaxHealth, OldValue);
}

void UFPSCombatAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSCombatAttributeSet, Armor, OldValue);
}

void UFPSCombatAttributeSet::OnRep_MaxArmor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSCombatAttributeSet, MaxArmor, OldValue);
}

void UFPSCombatAttributeSet::OnRep_ArmorReduction(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSCombatAttributeSet, ArmorReduction, OldValue);
}

void UFPSCombatAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSCombatAttributeSet, Stamina, OldValue);
}

void UFPSCombatAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSCombatAttributeSet, MaxStamina, OldValue);
}

void UFPSCombatAttributeSet::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSCombatAttributeSet, StaminaRegenRate, OldValue);
}

void UFPSCombatAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UFPSCombatAttributeSet, MovementSpeed, OldValue);
}
