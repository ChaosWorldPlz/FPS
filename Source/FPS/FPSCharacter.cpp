// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSCharacter.h"
#include "FPSProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/FPSAbilitySystemComponent.h"
#include "GAS/FPSGameplayAbility.h"
#include "GAS/FPSCombatAttributeSet.h"
#include "Weapon/FPSWeaponBase.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AFPSCharacter

AFPSCharacter::AFPSCharacter()
	: bAbilitiesInitialized(false)
	, bDead(false)
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create a CameraComponent
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	// Create the Ability System Component
	AbilitySystemComponent = CreateDefaultSubobject<UFPSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));

	// Create the Combat Attribute Set (automatically registered with ASC)
	CombatAttributeSet = CreateDefaultSubobject<UFPSCombatAttributeSet>(TEXT("CombatAttributeSet"));
}

UAbilitySystemComponent* AFPSCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AFPSCharacter::BeginPlay()
{
	// Call the base class
	Super::BeginPlay();
}

void AFPSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Initialize the ability system when possessed by a controller
	InitializeAbilitySystem();
}

void AFPSCharacter::InitializeAbilitySystem()
{
	if (AbilitySystemComponent && !bAbilitiesInitialized)
	{
		// Initialize ability actor info - for single player, owner and avatar are both this character
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// Bind death delegate
		if (CombatAttributeSet)
		{
			CombatAttributeSet->OnDeath.AddDynamic(this, &AFPSCharacter::OnDeath);
		}

		// Grant default abilities
		GrantDefaultAbilities();

		// Apply default effects
		ApplyDefaultEffects();

		bAbilitiesInitialized = true;

		UE_LOG(LogTemplateCharacter, Log, TEXT("Ability System initialized for %s"), *GetName());
	}
}

void AFPSCharacter::GrantDefaultAbilities()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GrantAbility(AbilityClass, 1);
		}
	}
}

void AFPSCharacter::ApplyDefaultEffects()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : DefaultEffects)
	{
		if (EffectClass)
		{
			AbilitySystemComponent->ApplyEffectToSelf(EffectClass, 1.0f);
		}
	}
}

//////////////////////////////////////////////////////////////////////////// Input

void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AFPSCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AFPSCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AFPSCharacter::EquipWeapon(AFPSWeaponBase* NewWeapon)
{
	if (!NewWeapon || NewWeapon == CurrentWeapon)
	{
		return;
	}

	// Unequip current weapon
	UnequipWeapon();

	// Equip new weapon
	CurrentWeapon = NewWeapon;
	CurrentWeapon->OnEquip(this);

	// Attach weapon to character
	if (Mesh1P)
	{
		CurrentWeapon->AttachToComponent(Mesh1P, FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("GripPoint"));
	}

	UE_LOG(LogTemplateCharacter, Log, TEXT("%s equipped weapon: %s"), *GetName(), *NewWeapon->GetName());
}

void AFPSCharacter::UnequipWeapon()
{
	if (!CurrentWeapon)
	{
		return;
	}

	CurrentWeapon->OnUnequip();
	CurrentWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	CurrentWeapon = nullptr;
}

void AFPSCharacter::OnDeath(AActor* Killer)
{
	if (bDead)
	{
		return;
	}

	bDead = true;

	UE_LOG(LogTemplateCharacter, Log, TEXT("%s has died. Killer: %s"),
		*GetName(),
		Killer ? *Killer->GetName() : TEXT("Unknown"));

	// Disable movement
	{
		if (GetCharacterMovement())
		{
			/*GetCharacterMovement()->DisableMovement();*/
			UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
			MovementComponent->DisableMovement();
		}
	}

	// Disable input
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
}
