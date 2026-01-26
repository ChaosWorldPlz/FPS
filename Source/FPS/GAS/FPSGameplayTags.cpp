// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSGameplayTags.h"
#include "GameplayTagsManager.h"

FFPSGameplayTags FFPSGameplayTags::GameplayTags;

void FFPSGameplayTags::InitializeNativeTags()
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	GameplayTags.AddAllTags(Manager);

	// Notify when all tags are registered
	Manager.DoneAddingNativeTags();
}

void FFPSGameplayTags::AddAllTags(UGameplayTagsManager& Manager)
{
	// State Tags
	AddTag(State_Dead, "FPS.State.Dead", "Character is dead");
	AddTag(State_Stunned, "FPS.State.Stunned", "Character is stunned and cannot act");
	AddTag(State_Sprinting, "FPS.State.Sprinting", "Character is sprinting");
	AddTag(State_Aiming, "FPS.State.Aiming", "Character is aiming down sights");
	AddTag(State_Reloading, "FPS.State.Reloading", "Character is reloading weapon");

	// Ability Tags - Weapon
	AddTag(Ability_Weapon_Fire, "FPS.Ability.Weapon.Fire", "Weapon fire ability");
	AddTag(Ability_Weapon_Reload, "FPS.Ability.Weapon.Reload", "Weapon reload ability");
	AddTag(Ability_Weapon_Melee, "FPS.Ability.Weapon.Melee", "Melee attack ability");
	AddTag(Ability_Weapon_Aim, "FPS.Ability.Weapon.Aim", "Aim down sights ability");
	AddTag(Ability_Weapon_Equip, "FPS.Ability.Weapon.Equip", "Weapon equip ability");

	// Ability Tags - Item
	AddTag(Ability_Item_Use, "FPS.Ability.Item.Use", "Use item ability");
	AddTag(Ability_Item_Drop, "FPS.Ability.Item.Drop", "Drop item ability");

	// Ability Tags - Movement
	AddTag(Ability_Movement_Sprint, "FPS.Ability.Movement.Sprint", "Sprint ability");
	AddTag(Ability_Movement_Jump, "FPS.Ability.Movement.Jump", "Jump ability");
	AddTag(Ability_Movement_Crouch, "FPS.Ability.Movement.Crouch", "Crouch ability");

	// Effect Tags - Damage
	AddTag(Effect_Damage, "FPS.Effect.Damage", "Damage effect (parent)");
	AddTag(Effect_Damage_Physical, "FPS.Effect.Damage.Physical", "Physical damage");
	AddTag(Effect_Damage_Bleeding, "FPS.Effect.Damage.Bleeding", "Bleeding damage over time");

	// Effect Tags - Healing
	AddTag(Effect_Heal, "FPS.Effect.Heal", "Healing effect (parent)");
	AddTag(Effect_Heal_Instant, "FPS.Effect.Heal.Instant", "Instant healing");
	AddTag(Effect_Heal_OverTime, "FPS.Effect.Heal.OverTime", "Healing over time");

	// Effect Tags - Buff/Debuff
	AddTag(Effect_Buff, "FPS.Effect.Buff", "Buff effect (parent)");
	AddTag(Effect_Debuff, "FPS.Effect.Debuff", "Debuff effect (parent)");
	AddTag(Effect_Buff_SpeedBoost, "FPS.Effect.Buff.SpeedBoost", "Movement speed increase");
	AddTag(Effect_Debuff_Slow, "FPS.Effect.Debuff.Slow", "Movement speed decrease");

	// Event Tags
	AddTag(Event_Montage_End, "FPS.Event.Montage.End", "Montage playback ended");
	AddTag(Event_Montage_Cancelled, "FPS.Event.Montage.Cancelled", "Montage was cancelled");
	AddTag(Event_Combat_Hit, "FPS.Event.Combat.Hit", "Combat hit event");
	AddTag(Event_Combat_Death, "FPS.Event.Combat.Death", "Death event");
	AddTag(Event_Weapon_Fired, "FPS.Event.Weapon.Fired", "Weapon was fired");
	AddTag(Event_Weapon_Reloaded, "FPS.Event.Weapon.Reloaded", "Weapon was reloaded");

	// Input Tags
	AddTag(Input_Fire, "FPS.Input.Fire", "Fire input action");
	AddTag(Input_Reload, "FPS.Input.Reload", "Reload input action");
	AddTag(Input_Aim, "FPS.Input.Aim", "Aim input action");
	AddTag(Input_Melee, "FPS.Input.Melee", "Melee input action");
	AddTag(Input_Use, "FPS.Input.Use", "Use/Interact input action");
	AddTag(Input_Sprint, "FPS.Input.Sprint", "Sprint input action");
}

void FFPSGameplayTags::AddTag(FGameplayTag& OutTag, const ANSICHAR* TagName, const ANSICHAR* TagComment)
{
	OutTag = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName(TagName),
		FString(TEXT("(Native) ")) + FString(TagComment)
	);
}
