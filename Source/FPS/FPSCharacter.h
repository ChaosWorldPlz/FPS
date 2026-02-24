// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayTagContainer.h"
#include "Logging/LogMacros.h"
#include "Team/FPSTeamTypes.h"
#include "FPSCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
class UFPSAbilitySystemComponent;
class UFPSAttributeSetBase;
class UFPSCombatAttributeSet;
class UGameplayEffect;
class UGameplayAbility;
class AFPSWeaponBase;
class AFPSPlayerState;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AFPSCharacter : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P;

	/** Third person mesh (seen by others) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh3P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Ability System Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	UFPSAbilitySystemComponent* AbilitySystemComponent;

	/** Combat Attribute Set */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	UFPSCombatAttributeSet* CombatAttributeSet;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

public:
	AFPSCharacter();

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	//~ Begin IGenericTeamAgentInterface
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~ End IGenericTeamAgentInterface

	/** Get the FPS-specific ability system component */
	UFUNCTION(BlueprintCallable, Category = "FPS|GAS")
	UFPSAbilitySystemComponent* GetFPSAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** Get the combat attribute set */
	UFUNCTION(BlueprintCallable, Category = "FPS|GAS")
	UFPSCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributeSet; }

	/** Check if the character is dead */
	UFUNCTION(BlueprintCallable, Category = "FPS|Combat")
	bool IsDead() const { return bDead; }

	/** Get the team of this character */
	UFUNCTION(BlueprintPure, Category = "FPS|Team")
	EFPSTeam GetTeam() const;

	/** Get the FPS player state */
	UFUNCTION(BlueprintPure, Category = "FPS|Team")
	AFPSPlayerState* GetFPSPlayerState() const;

	/** Reset character for respawn (restore health, re-enable input) */
	void ResetForRespawn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Jump() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Initialize the ability system for this character */
	void InitializeAbilitySystem();

	/** Grant default abilities to the character */
	void GrantDefaultAbilities();

	/** Apply default effects (like initial attribute values) */
	void ApplyDefaultEffects();

public:
	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	/** Default abilities to grant on spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	/** Default effects to apply on spawn (e.g., initial attribute values) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Effects")
	TArray<TSubclassOf<UGameplayEffect>> DefaultEffects;

	//-------------------------------------------------------------------
	// Stamina System
	//-------------------------------------------------------------------

	/** Stamina drain rate while sprinting (per second) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Stamina")
	float SprintStaminaDrainRate = 20.0f;

	/** Stamina cost per jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Stamina")
	float JumpStaminaCost = 15.0f;

	/** Stamina cost per melee attack */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Stamina")
	float MeleeStaminaCost = 20.0f;

	/** Delay before stamina starts regenerating after last consumption (seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Stamina")
	float StaminaRegenDelay = 1.0f;

	/** Movement speed multiplier while sprinting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Stamina")
	float SprintSpeedMultiplier = 1.5f;

	/** Check if currently sprinting */
	UFUNCTION(BlueprintPure, Category = "FPS|Stamina")
	bool IsSprinting() const { return bIsSprinting; }

	/** Check if character has enough stamina */
	UFUNCTION(BlueprintPure, Category = "FPS|Stamina")
	bool HasStamina(float Amount) const;

	/** Consume stamina. Returns true if enough stamina was available. */
	UFUNCTION(BlueprintCallable, Category = "FPS|Stamina")
	bool ConsumeStamina(float Amount);

	//-------------------------------------------------------------------
	// Weapon System
	//-------------------------------------------------------------------

	/** Currently equipped weapon */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Weapon")
	AFPSWeaponBase* CurrentWeapon;

	/** Equip a weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void EquipWeapon(AFPSWeaponBase* NewWeapon);

	/** Unequip current weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UnequipWeapon();

	/** Get the currently equipped weapon */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AFPSWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	/** Flag to track if abilities have been initialized */
	bool bAbilitiesInitialized;

	/** Flag to track if character is dead */
	UPROPERTY(Replicated)
	bool bDead;

	/** Whether the player is currently sprinting */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "FPS|Stamina")
	bool bIsSprinting = false;

	/** Cached base walk speed (from CharacterMovementComponent) */
	float BaseWalkSpeed = 0.0f;

	/** Time of last stamina consumption (for regen delay) */
	float LastStaminaConsumeTime = 0.0f;

	/** Start sprinting */
	void StartSprint();

	/** Stop sprinting */
	void StopSprint();

	/** Tick-based stamina drain and regen */
	void UpdateStamina(float DeltaTime);

	/** Called when the character dies */
	UFUNCTION()
	void OnDeath(AActor* Killer);

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns Mesh3P subobject **/
	USkeletalMeshComponent* GetMesh3P() const { return Mesh3P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};
