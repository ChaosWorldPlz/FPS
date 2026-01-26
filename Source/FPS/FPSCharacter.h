// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Logging/LogMacros.h"
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

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AFPSCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P;

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

public:
	AFPSCharacter();

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	/** Get the FPS-specific ability system component */
	UFUNCTION(BlueprintCallable, Category = "FPS|GAS")
	UFPSAbilitySystemComponent* GetFPSAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** Get the combat attribute set */
	UFUNCTION(BlueprintCallable, Category = "FPS|GAS")
	UFPSCombatAttributeSet* GetCombatAttributeSet() const { return CombatAttributeSet; }

	/** Check if the character is dead */
	UFUNCTION(BlueprintCallable, Category = "FPS|Combat")
	bool IsDead() const { return bDead; }

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

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
	// Weapon System
	//-------------------------------------------------------------------

	/** Currently equipped weapon */
	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
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
	bool bDead;

	/** Called when the character dies */
	UFUNCTION()
	void OnDeath(AActor* Killer);

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};
