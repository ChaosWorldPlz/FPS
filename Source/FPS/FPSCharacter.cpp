// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSCharacter.h"
#include "Weapon/FPSProjectile.h"
#include "Animation/AnimInstance.h"
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
#include "Weapon/FPSWeaponSlotComponent.h"
#include "GAS/FPSRecoilComponent.h"
#include "Team/FPSPlayerState.h"
#include "FPSGameMode.h"
#include "GAS/FPSGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Armor/FPSArmorComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AFPSCharacter

AFPSCharacter::AFPSCharacter()
	: bAbilitiesInitialized(false)
	, WeaponSlotComp(nullptr)
	, bDead(false)
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// ASC and AttributeSet are now on the PlayerState
	AbilitySystemComponent = nullptr;
	CombatAttributeSet = nullptr;

	// Create the weapon slot component (manages 3 carry slots)
	WeaponSlotComp = CreateDefaultSubobject<UFPSWeaponSlotComponent>(TEXT("WeaponSlotComp"));

	// Create the recoil component (manages Pattern / Spread state)
	RecoilComponent = CreateDefaultSubobject<UFPSRecoilComponent>(TEXT("RecoilComponent"));

	// Create the armor component
	ArmorComponent = CreateDefaultSubobject<UFPSArmorComponent>(TEXT("ArmorComponent"));
}

void AFPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSCharacter, bDead);
	DOREPLIFETIME(AFPSCharacter, bIsSprinting);
}

UAbilitySystemComponent* AFPSCharacter::GetAbilitySystemComponent() const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent;
	}
	// Fallback to PlayerState if not cached yet
	AFPSPlayerState* PS = GetPlayerState<AFPSPlayerState>();
	if (PS)
	{
		return PS->GetAbilitySystemComponent();
	}
	return nullptr;
}

UFPSCombatAttributeSet* AFPSCharacter::GetCombatAttributeSet() const
{
	if (CombatAttributeSet)
	{
		return CombatAttributeSet;
	}
	AFPSPlayerState* PS = GetPlayerState<AFPSPlayerState>();
	if (PS)
	{
		return PS->GetCombatAttributeSet();
	}
	return nullptr;
}

UFPSAbilitySystemComponent* AFPSCharacter::GetFPSAbilitySystemComponent() const
{
	return Cast<UFPSAbilitySystemComponent>(GetAbilitySystemComponent());
}

//-------------------------------------------------------------------
// IGenericTeamAgentInterface
//-------------------------------------------------------------------

FGenericTeamId AFPSCharacter::GetGenericTeamId() const
{
	AFPSPlayerState* PS = GetFPSPlayerState();
	if (PS)
	{
		return FGenericTeamId(static_cast<uint8>(PS->GetTeam()));
	}
	return FGenericTeamId::NoTeam;
}

ETeamAttitude::Type AFPSCharacter::GetTeamAttitudeTowards(const AActor& Other) const
{
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);
	if (!OtherTeamAgent)
	{
		return ETeamAttitude::Neutral;
	}

	FGenericTeamId MyTeamId = GetGenericTeamId();
	FGenericTeamId OtherTeamId = OtherTeamAgent->GetGenericTeamId();

	if (MyTeamId == FGenericTeamId::NoTeam || OtherTeamId == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	if (MyTeamId == OtherTeamId)
	{
		return ETeamAttitude::Friendly;
	}

	return ETeamAttitude::Hostile;
}

//-------------------------------------------------------------------
// Team
//-------------------------------------------------------------------

EFPSTeam AFPSCharacter::GetTeam() const
{
	AFPSPlayerState* PS = GetFPSPlayerState();
	return PS ? PS->GetTeam() : EFPSTeam::None;
}

AFPSPlayerState* AFPSCharacter::GetFPSPlayerState() const
{
	return Cast<AFPSPlayerState>(GetPlayerState());
}

//-------------------------------------------------------------------
// Lifecycle
//-------------------------------------------------------------------


void AFPSCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 让子弹能打到 SkeletalMesh 并返回骨骼名。
	// 此时 Physics Asset 骨骼体已经创建完毕，SetCollisionResponseToChannel 能正确同步到每个 Body。
	// ECC_GameTraceChannel1 = 项目自定义 "Projectile" 通道（见 DefaultEngine.ini）
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
}

void AFPSCharacter::BeginPlay()
{
	Super::BeginPlay();


	// Cache base walk speed for sprint calculations
	if (GetCharacterMovement())
	{
		BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	}

	// 默认武器由游戏流程（Raid开局/装备配置）决定，不在此处自动生成
}

void AFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bDead && HasAuthority())
	{
		UpdateStamina(DeltaTime);
	}
}

void AFPSCharacter::Jump()
{
	if (!HasStamina(JumpStaminaCost))
	{
		return;
	}

	ConsumeStamina(JumpStaminaCost);
	Super::Jump();
}

void AFPSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Server: Initialize ability system with PlayerState as owner for proper replication
	InitializeAbilitySystem();
}

void AFPSCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Client: Initialize ability system when player state replicates
	InitializeAbilitySystem();
}

void AFPSCharacter::InitializeAbilitySystem()
{
	if (bAbilitiesInitialized)
	{
		return;
	}

	AFPSPlayerState* PS = GetPlayerState<AFPSPlayerState>();
	if (PS)
	{
		// Cache ASC and AttributeSet from PlayerState
		AbilitySystemComponent = Cast<UFPSAbilitySystemComponent>(PS->GetAbilitySystemComponent());
		CombatAttributeSet = PS->GetCombatAttributeSet();

		if (AbilitySystemComponent)
		{
			// PlayerState is Owner, Character is Avatar
			AbilitySystemComponent->InitAbilityActorInfo(PS, this);

			// Bind death delegate
			if (CombatAttributeSet)
			{
				CombatAttributeSet->OnDeath.AddDynamic(this, &AFPSCharacter::OnDeath);
			}

			// Only grant abilities on server
			if (HasAuthority())
			{
				GrantDefaultAbilities();
				ApplyDefaultEffects();
			}

			bAbilitiesInitialized = true;

			UE_LOG(LogTemplateCharacter, Log, TEXT("Ability System initialized from PlayerState for %s"), *GetName());
		}
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

void AFPSCharacter::ResetForRespawn()
{
	bDead = false;
	bAbilitiesInitialized = false;

	// Re-enable movement
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	// Re-enable input
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		EnableInput(PC);
	}

	// Re-initialize abilities and effects (restores health etc.)
	InitializeAbilitySystem();
}

//////////////////////////////////////////////////////////////////////////// Input

void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping (uses our override for stamina check)
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AFPSCharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Look);

		// Sprinting
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AFPSCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AFPSCharacter::StopSprint);
		}

		// Fire
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AFPSCharacter::HandleFire);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AFPSCharacter::HandleFireReleased);
		}

		// Aim (ADS)
		if (AimAction)
		{
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AFPSCharacter::HandleAimStart);
			EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AFPSCharacter::HandleAimStop);
		}

		// Crouch
		if (CrouchToggleAction)
		{
			EnhancedInputComponent->BindAction(CrouchToggleAction, ETriggerEvent::Started, this, &AFPSCharacter::HandleCrouchToggle);
		}

		// Reload
		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AFPSCharacter::HandleReload);
		}
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

AFPSWeaponBase* AFPSCharacter::GetCurrentWeapon() const
{
	return WeaponSlotComp ? WeaponSlotComp->GetActiveWeapon() : nullptr;
}

void AFPSCharacter::ServerSwitchWeaponSlot_Implementation(EFPSWeaponSlot Slot)
{
	if (WeaponSlotComp)
	{
		WeaponSlotComp->SwitchToSlot(Slot);
	}
}

void AFPSCharacter::ServerCycleWeapon_Implementation()
{
	if (WeaponSlotComp)
	{
		WeaponSlotComp->CycleToNextSlot();
	}
}

//-------------------------------------------------------------------
// Stamina System
//-------------------------------------------------------------------

void AFPSCharacter::StartSprint()
{
	if (bDead || bIsSprinting)
	{
		return;
	}

	if (!HasStamina(0.1f))
	{
		return;
	}

	bIsSprinting = true;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SprintSpeedMultiplier;
	}

	static const FGameplayTag SprintingTag = FGameplayTag::RequestGameplayTag(FName("FPS.State.Sprinting"));
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(SprintingTag);
	}
}

void AFPSCharacter::StopSprint()
{
	if (!bIsSprinting)
	{
		return;
	}

	bIsSprinting = false;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}

	static const FGameplayTag SprintingTag = FGameplayTag::RequestGameplayTag(FName("FPS.State.Sprinting"));
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(SprintingTag);
	}
}

bool AFPSCharacter::HasStamina(float Amount) const
{
	if (!CombatAttributeSet)
	{
		return false;
	}
	return CombatAttributeSet->GetStamina() >= Amount;
}

bool AFPSCharacter::ConsumeStamina(float Amount)
{
	if (!HasStamina(Amount))
	{
		return false;
	}

	if (AbilitySystemComponent && CombatAttributeSet)
	{
		float NewStamina = FMath::Max(0.0f, CombatAttributeSet->GetStamina() - Amount);
		AbilitySystemComponent->SetNumericAttributeBase(
			UFPSCombatAttributeSet::GetStaminaAttribute(), NewStamina);
	}

	LastStaminaConsumeTime = GetWorld()->GetTimeSeconds();
	return true;
}

void AFPSCharacter::UpdateStamina(float DeltaTime)
{
	if (!CombatAttributeSet || !AbilitySystemComponent)
	{
		return;
	}

	// Sprint drain
	if (bIsSprinting)
	{
		float DrainAmount = SprintStaminaDrainRate * DeltaTime;
		if (!ConsumeStamina(DrainAmount))
		{
			StopSprint();
		}
		return; // No regen while sprinting
	}

	// Regen: only after delay since last consumption
	float CurrentStamina = CombatAttributeSet->GetStamina();
	float MaxStamina = CombatAttributeSet->GetMaxStamina();

	if (CurrentStamina < MaxStamina)
	{
		float TimeSinceLastConsume = GetWorld()->GetTimeSeconds() - LastStaminaConsumeTime;
		if (TimeSinceLastConsume >= StaminaRegenDelay)
		{
			float RegenRate = CombatAttributeSet->GetStaminaRegenRate();
			float NewStamina = FMath::Min(MaxStamina, CurrentStamina + RegenRate * DeltaTime);
			AbilitySystemComponent->SetNumericAttributeBase(
				UFPSCombatAttributeSet::GetStaminaAttribute(), NewStamina);
		}
	}
}

//-------------------------------------------------------------------
// Combat Input Handlers
//-------------------------------------------------------------------

void AFPSCharacter::HandleFire()
{
	if (bDead || !AbilitySystemComponent)
	{
		return;
	}

	// Use RequestGameplayTag directly to avoid FFPSGameplayTags singleton initialization timing issues
	static const FGameplayTag FireTag = FGameplayTag::RequestGameplayTag(FName("FPS.Ability.Weapon.Fire"));
	bool bSuccess = AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(FireTag));
}

void AFPSCharacter::HandleFireReleased()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	static const FGameplayTag FireTag = FGameplayTag::RequestGameplayTag(FName("FPS.Ability.Weapon.Fire"));
	FGameplayTagContainer FireTags(FireTag);
	AbilitySystemComponent->CancelAbilities(&FireTags);
}

void AFPSCharacter::HandleAimStart()
{
	if (bDead || !AbilitySystemComponent)
	{
		return;
	}

	bIsAiming = true;
	static const FGameplayTag AimingTag = FGameplayTag::RequestGameplayTag(FName("FPS.State.Aiming"));
	AbilitySystemComponent->AddLooseGameplayTag(AimingTag);
}

void AFPSCharacter::HandleAimStop()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	bIsAiming = false;
	static const FGameplayTag AimingTag = FGameplayTag::RequestGameplayTag(FName("FPS.State.Aiming"));
	AbilitySystemComponent->RemoveLooseGameplayTag(AimingTag);
}

void AFPSCharacter::HandleCrouchToggle()
{
	if (bDead)
	{
		return;
	}

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void AFPSCharacter::HandleReload()
{
	if (bDead || !AbilitySystemComponent)
	{
		return;
	}

	static const FGameplayTag ReloadTag = FGameplayTag::RequestGameplayTag(FName("FPS.Ability.Weapon.Reload"));
	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(ReloadTag));
}

void AFPSCharacter::OnDeath(AActor* Killer)
{
	if (bDead)
	{
		return;
	}

	bDead = true;

	// Stop sprinting on death
	StopSprint();

	UE_LOG(LogTemplateCharacter, Log, TEXT("%s has died. Killer: %s"),
		*GetName(),
		Killer ? *Killer->GetName() : TEXT("Unknown"));

	// Disable movement
	if (GetCharacterMovement())
	{
		UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
		MovementComponent->DisableMovement();
	}

	// Disable input
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}

	// Notify GameMode on server
	if (HasAuthority())
	{
		AFPSPlayerState* VictimPS = GetFPSPlayerState();

		// Find killer's player state
		AFPSPlayerState* KillerPS = nullptr;
		if (AFPSCharacter* KillerChar = Cast<AFPSCharacter>(Killer))
		{
			KillerPS = KillerChar->GetFPSPlayerState();
		}

		if (AFPSGameMode* GM = GetWorld()->GetAuthGameMode<AFPSGameMode>())
		{
			GM->HandlePlayerDeath(VictimPS, KillerPS);
		}
	}
}
