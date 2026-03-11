// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "FPSTeamTypes.h"
#include "FPSPlayerState.generated.h"

class UFPSAbilitySystemComponent;
class UFPSCombatAttributeSet;

/**
 * AFPSPlayerState
 *
 * Replicated player state for PVP matches.
 * Tracks team assignment, kills, deaths, score, and damage stats.
 * Serves as the central authority for the Gameplay Ability System.
 */
UCLASS()
class FPS_API AFPSPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AFPSPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	/** Get the Combat Attribute Set */
	UFUNCTION(BlueprintCallable, Category = "FPS|GAS")
	UFPSCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributeSet; }

	//-------------------------------------------------------------------
	// Team
	//-------------------------------------------------------------------

	/** Get current team */
	UFUNCTION(BlueprintPure, Category = "FPS|Team")
	EFPSTeam GetTeam() const { return Team; }

	/** Set team (server only) */
	void ServerSetTeam(EFPSTeam NewTeam);

	//-------------------------------------------------------------------
	// Stats
	//-------------------------------------------------------------------

	UFUNCTION(BlueprintPure, Category = "FPS|Stats")
	int32 GetKills() const { return Kills; }

	UFUNCTION(BlueprintPure, Category = "FPS|Stats")
	int32 GetDeaths() const { return Deaths; }

	UFUNCTION(BlueprintPure, Category = "FPS|Stats")
	int32 GetAssists() const { return Assists; }

	UFUNCTION(BlueprintPure, Category = "FPS|Stats")
	int32 GetMatchScore() const { return MatchScore; }

	UFUNCTION(BlueprintPure, Category = "FPS|Stats")
	float GetDamageDealt() const { return DamageDealt; }

	UFUNCTION(BlueprintPure, Category = "FPS|Stats")
	float GetDamageTaken() const { return DamageTaken; }

	/** Add a kill (server only) */
	void AddKill();

	/** Add a death (server only) */
	void AddDeath();

	/** Add an assist (server only) */
	void AddAssist();

	/** Add score (server only) */
	void AddScore(int32 Amount);

	/** Add damage dealt (server only) */
	void AddDamageDealt(float Amount);

	/** Add damage taken (server only) */
	void AddDamageTaken(float Amount);

	/** Reset all stats (server only) */
	void ResetStats();

	//-------------------------------------------------------------------
	// Delegates
	//-------------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnTeamChanged OnTeamChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnPlayerScoreChanged OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnKillsChanged OnKillsChanged;

protected:
	/** Ability System Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	UFPSAbilitySystemComponent* AbilitySystemComponent;

	/** Combat Attribute Set */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	UFPSCombatAttributeSet* CombatAttributeSet;

	UPROPERTY(ReplicatedUsing = OnRep_Team)
	EFPSTeam Team;

	UPROPERTY(ReplicatedUsing = OnRep_Kills)
	int32 Kills;

	UPROPERTY(Replicated)
	int32 Deaths;

	UPROPERTY(Replicated)
	int32 Assists;

	UPROPERTY(ReplicatedUsing = OnRep_MatchScore)
	int32 MatchScore;

	UPROPERTY(Replicated)
	float DamageDealt;

	UPROPERTY(Replicated)
	float DamageTaken;

	UFUNCTION()
	void OnRep_Team();

	UFUNCTION()
	void OnRep_Kills();

	UFUNCTION()
	void OnRep_MatchScore();
};
