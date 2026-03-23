// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayTagContainer.h"
#include "Logging/LogMacros.h"
#include "Team/FPSTeamTypes.h"
#include "Weapon/FPSAttachmentTypes.h"
#include "FPSCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
class UFPSAbilitySystemComponent;
class UFPSAttributeSetBase;
class UFPSCombatAttributeSet;
class UFPSWeaponSlotComponent;
class UFPSRecoilComponent;
class UGameplayEffect;
class UGameplayAbility;
class AFPSWeaponBase;
class AFPSPlayerState;
class UFPSArmorComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AFPSCharacter : public ACharacter, public IAbilitySystemInterface, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

	/** Ability System Component (Cached from PlayerState) */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "GAS", meta = (AllowPrivateAccess = "true"))
	UFPSAbilitySystemComponent* AbilitySystemComponent;

	/** Combat Attribute Set (Cached from PlayerState) */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "GAS", meta = (AllowPrivateAccess = "true"))
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

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* FireAction;

	/** Aim (ADS) Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* AimAction;

	/** Crouch Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* CrouchToggleAction;

	/** Reload Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* ReloadAction;

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
	UFPSAbilitySystemComponent* GetFPSAbilitySystemComponent() const;

	/** Get the combat attribute set */
	UFUNCTION(BlueprintCallable, Category = "FPS|GAS")
	UFPSCombatAttributeSet* GetCombatAttributeSet() const;

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

	/** Manages the three weapon carry slots (Primary1, Primary2, Pistol) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UFPSWeaponSlotComponent* WeaponSlotComp;

	/** 后坐力组件：管理 Pattern / Spread 运行时状态 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UFPSRecoilComponent* RecoilComponent;

	/** 是否处于 ADS 瞄准状态（供 RecoilComponent 查询） */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	bool bIsAiming = false;

	/** 出生时自动装备的武器类（最多3个，按 Primary1/Primary2/Pistol 顺序） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TArray<TSubclassOf<AFPSWeaponBase>> DefaultWeaponClasses;

	/** Get the currently equipped weapon (forwards to WeaponSlotComp) */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	AFPSWeaponBase* GetCurrentWeapon() const;

	/** Server RPC: switch to a specific carry slot */
	UFUNCTION(Server, Reliable)
	void ServerSwitchWeaponSlot(EFPSWeaponSlot Slot);

	/** Server RPC: cycle to next occupied slot */
	UFUNCTION(Server, Reliable)
	void ServerCycleWeapon();

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Fire pressed: activates the weapon's fire GA */
	void HandleFire();

	/** Fire released: cancels auto-fire GA */
	void HandleFireReleased();

	/** Aim start/stop: adds/removes FPS.State.Aiming GameplayTag */
	void HandleAimStart();
	void HandleAimStop();

	/** Crouch toggle */
	void HandleCrouchToggle();

	/** Reload: forwards to current weapon's TryReload() */
	void HandleReload();

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
};
