// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPSAttributeSetBase.h"
#include "FPSCombatAttributeSet.generated.h"

// Forward declarations
class AFPSCharacter;

/** Record of damage dealt by an instigator, used for assist tracking */
USTRUCT()
struct FDamageRecord
{
	GENERATED_BODY()

	/** Who dealt the damage */
	TWeakObjectPtr<AActor> Instigator;

	/** How much actual health damage was dealt (after armor) */
	float DamageAmount = 0.0f;

	/** World time when the damage occurred */
	double Timestamp = 0.0;
};

// Delegate for attribute changes
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAttributeChangedDelegate, float, OldValue, float, NewValue, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDeathDelegate, AActor*, Killer);

/**
 * UFPSCombatAttributeSet
 *
 * Attribute set containing combat-related attributes:
 * - Health/MaxHealth
 * - Armor/MaxArmor
 * - Stamina/MaxStamina
 * - MovementSpeed
 */
UCLASS()
class FPS_API UFPSCombatAttributeSet : public UFPSAttributeSetBase
{
	GENERATED_BODY()

public:
	UFPSCombatAttributeSet();

	// Attribute replication (for future multiplayer support)
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//-------------------------------------------------------------------
	// Health
	//-------------------------------------------------------------------
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Attributes|Health", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UFPSCombatAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Attributes|Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UFPSCombatAttributeSet, MaxHealth)

	//-------------------------------------------------------------------
	// Armor
	//-------------------------------------------------------------------
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Attributes|Armor", ReplicatedUsing = OnRep_Armor)
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(UFPSCombatAttributeSet, Armor)

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Attributes|Armor", ReplicatedUsing = OnRep_MaxArmor)
	FGameplayAttributeData MaxArmor;
	ATTRIBUTE_ACCESSORS(UFPSCombatAttributeSet, MaxArmor)

	/** Armor damage reduction percentage (0.0 - 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Attributes|Armor", ReplicatedUsing = OnRep_ArmorReduction)
	FGameplayAttributeData ArmorReduction;
	ATTRIBUTE_ACCESSORS(UFPSCombatAttributeSet, ArmorReduction)

	//-------------------------------------------------------------------
	// Stamina
	//-------------------------------------------------------------------
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Attributes|Stamina", ReplicatedUsing = OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UFPSCombatAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "FPS|Attributes|Stamina", ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UFPSCombatAttributeSet, MaxStamina)

	/** Stamina regeneration rate per second */
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Attributes|Stamina", ReplicatedUsing = OnRep_StaminaRegenRate)
	FGameplayAttributeData StaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UFPSCombatAttributeSet, StaminaRegenRate)

	//-------------------------------------------------------------------
	// Movement
	//-------------------------------------------------------------------
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Attributes|Movement", ReplicatedUsing = OnRep_MovementSpeed)
	FGameplayAttributeData MovementSpeed;
	ATTRIBUTE_ACCESSORS(UFPSCombatAttributeSet, MovementSpeed)

	//-------------------------------------------------------------------
	// Meta Attributes (not replicated, used for calculations)
	//-------------------------------------------------------------------

	/** Incoming damage - applied and cleared in PostGameplayEffectExecute */
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UFPSCombatAttributeSet, IncomingDamage)

	/** Incoming heal - applied and cleared in PostGameplayEffectExecute */
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Attributes|Meta")
	FGameplayAttributeData IncomingHeal;
	ATTRIBUTE_ACCESSORS(UFPSCombatAttributeSet, IncomingHeal)

	//-------------------------------------------------------------------
	// Delegates
	//-------------------------------------------------------------------

	/** Broadcast when health changes */
	UPROPERTY(BlueprintAssignable, Category = "FPS|Attributes|Events")
	FOnAttributeChangedDelegate OnHealthChanged;

	/** Broadcast when armor changes */
	UPROPERTY(BlueprintAssignable, Category = "FPS|Attributes|Events")
	FOnAttributeChangedDelegate OnArmorChanged;

	/** Broadcast when stamina changes */
	UPROPERTY(BlueprintAssignable, Category = "FPS|Attributes|Events")
	FOnAttributeChangedDelegate OnStaminaChanged;

	/** Broadcast when character dies */
	UPROPERTY(BlueprintAssignable, Category = "FPS|Attributes|Events")
	FOnDeathDelegate OnDeath;

protected:
	//-------------------------------------------------------------------
	// Attribute callbacks
	//-------------------------------------------------------------------

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	//-------------------------------------------------------------------
	// Replication callbacks
	//-------------------------------------------------------------------

	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Armor(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxArmor(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_ArmorReduction(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_Stamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MovementSpeed(const FGameplayAttributeData& OldValue);

public:
	//-------------------------------------------------------------------
	// Assist Tracking
	//-------------------------------------------------------------------

	/**
	 * Get all actors who contributed damage within a time window.
	 * @param TimeWindow How far back to look (in seconds)
	 * @param ExcludeActor Actor to exclude (typically the killer)
	 * @return Map of Instigator -> TotalDamage
	 */
	TMap<AActor*, float> GetRecentDamageContributors(float TimeWindow, AActor* ExcludeActor = nullptr) const;

	/** Clear all damage records (call after death processing) */
	void ClearDamageRecords();

private:
	/** Handle damage calculation with armor absorption */
	void HandleDamage(const FGameplayEffectModCallbackData& Data);

	/** Handle healing */
	void HandleHeal(const FGameplayEffectModCallbackData& Data);

	/** Check and trigger death */
	void CheckDeath(AActor* Instigator);

	/** Flag to prevent multiple death triggers */
	bool bDead;

	/** Recent damage records for assist tracking (server only, not replicated) */
	TArray<FDamageRecord> DamageRecords;
};
