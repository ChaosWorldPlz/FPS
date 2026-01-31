// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * FFPSGameplayTags
 *
 * Singleton containing native gameplay tags for the FPS project.
 * Tags are registered once and cached for efficient access.
 */
struct FPS_API FFPSGameplayTags
{
public:
	static const FFPSGameplayTags& Get() { return GameplayTags; }
	static void InitializeNativeTags();

	// State Tags
	FGameplayTag State_Dead;
	FGameplayTag State_Stunned;
	FGameplayTag State_Sprinting;
	FGameplayTag State_Aiming;
	FGameplayTag State_Reloading;

	// Ability Tags - Weapon
	FGameplayTag Ability_Weapon_Fire;
	FGameplayTag Ability_Weapon_Reload;
	FGameplayTag Ability_Weapon_Melee;
	FGameplayTag Ability_Weapon_Aim;
	FGameplayTag Ability_Weapon_Equip;

	// Ability Tags - Item
	FGameplayTag Ability_Item_Use;
	FGameplayTag Ability_Item_Drop;

	// Ability Tags - Movement
	FGameplayTag Ability_Movement_Sprint;
	FGameplayTag Ability_Movement_Jump;
	FGameplayTag Ability_Movement_Crouch;

	// Effect Tags - Damage
	FGameplayTag Effect_Damage;
	FGameplayTag Effect_Damage_Physical;
	FGameplayTag Effect_Damage_Bleeding;

	// Effect Tags - Healing
	FGameplayTag Effect_Heal;
	FGameplayTag Effect_Heal_Instant;
	FGameplayTag Effect_Heal_OverTime;

	// Effect Tags - Buff/Debuff
	FGameplayTag Effect_Buff;
	FGameplayTag Effect_Debuff;
	FGameplayTag Effect_Buff_SpeedBoost;
	FGameplayTag Effect_Debuff_Slow;

	// Event Tags
	FGameplayTag Event_Montage_End;
	FGameplayTag Event_Montage_Cancelled;
	FGameplayTag Event_Combat_Hit;
	FGameplayTag Event_Combat_Death;
	FGameplayTag Event_Weapon_Fired;
	FGameplayTag Event_Weapon_Reloaded;

	// Team Tags
	FGameplayTag Team_A;
	FGameplayTag Team_B;
	FGameplayTag Team_None;

	// Input Tags (for Enhanced Input binding)
	FGameplayTag Input_Fire;
	FGameplayTag Input_Reload;
	FGameplayTag Input_Aim;
	FGameplayTag Input_Melee;
	FGameplayTag Input_Use;
	FGameplayTag Input_Sprint;

protected:
	void AddAllTags(class UGameplayTagsManager& Manager);
	void AddTag(FGameplayTag& OutTag, const ANSICHAR* TagName, const ANSICHAR* TagComment);

private:
	static FFPSGameplayTags GameplayTags;
};
