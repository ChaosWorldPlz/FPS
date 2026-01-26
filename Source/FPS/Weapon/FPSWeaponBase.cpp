// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSWeaponBase.h"
#include "FPSWeaponDataAsset.h"
#include "FPS/FPSCharacter.h"
#include "AbilitySystemComponent.h"
#include "FPS/GAS/FPSAbilitySystemComponent.h"
#include "FPS/GAS/FPSCombatAttributeSet.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

const FName AFPSWeaponBase::MuzzleSocketName = TEXT("Muzzle");

AFPSWeaponBase::AFPSWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create weapon mesh
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	RootComponent = WeaponMesh;
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFPSWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	// Initialize ammo from weapon data
	if (WeaponData)
	{
		AmmoInfo = WeaponData->GetDefaultAmmoInfo();
		CurrentSpread = WeaponData->BaseSpread;
	}
}

void AFPSWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update spread recovery
	UpdateSpread(DeltaTime);
}

void AFPSWeaponBase::OnEquip(AFPSCharacter* NewOwner)
{
	if (!NewOwner)
	{
		return;
	}

	OwningCharacter = NewOwner;
	SetWeaponState(EFPSWeaponState::Equipping);

	// Grant abilities
	GrantAbilities();

	// Set equip timer
	if (WeaponData)
	{
		FTimerHandle EquipTimerHandle;
		GetWorldTimerManager().SetTimer(
			EquipTimerHandle,
			[this]() { SetWeaponState(EFPSWeaponState::Idle); },
			WeaponData->EquipTime,
			false
		);
	}
	else
	{
		SetWeaponState(EFPSWeaponState::Idle);
	}

	// Broadcast ammo state
	OnAmmoChanged.Broadcast(AmmoInfo.CurrentMagazine, AmmoInfo.CurrentReserve);
}

void AFPSWeaponBase::OnUnequip()
{
	SetWeaponState(EFPSWeaponState::Unequipping);

	// Remove abilities
	RemoveAbilities();

	// Clear timers
	GetWorldTimerManager().ClearTimer(FireCooldownTimerHandle);
	GetWorldTimerManager().ClearTimer(ReloadTimerHandle);

	OwningCharacter.Reset();
}

bool AFPSWeaponBase::TryFire()
{
	if (!CanFire())
	{
		// Play dry fire sound if magazine empty
		if (WeaponData && AmmoInfo.IsMagazineEmpty() && WeaponData->DryFireSound.IsValid())
		{
			UGameplayStatics::PlaySoundAtLocation(this, WeaponData->DryFireSound.LoadSynchronous(), GetActorLocation());
		}
		return false;
	}

	Fire();
	return true;
}

void AFPSWeaponBase::Fire()
{
	if (!WeaponData)
	{
		return;
	}

	// Consume ammo
	if (WeaponData->bUsesAmmo)
	{
		AmmoInfo.ConsumeAmmo();
		OnAmmoChanged.Broadcast(AmmoInfo.CurrentMagazine, AmmoInfo.CurrentReserve);
	}

	// Increase spread
	IncreaseSpread();

	// Set fire cooldown
	bCanFireAgain = false;
	float FireDelay = WeaponData->GetTimeBetweenShots();
	GetWorldTimerManager().SetTimer(
		FireCooldownTimerHandle,
		this,
		&AFPSWeaponBase::ResetFireCooldown,
		FireDelay,
		false
	);
	LastFireTime = GetWorld()->GetTimeSeconds();

	// Perform hit scan for each pellet
	for (int32 i = 0; i < WeaponData->PelletsPerShot; i++)
	{
		FVector MuzzleLoc = GetMuzzleLocation();
		FVector FireDir = GetFireDirectionWithSpread();
		FVector EndPoint = MuzzleLoc + (FireDir * WeaponData->MaxRange);

		FHitResult HitResult = PerformLineTrace(MuzzleLoc, EndPoint);

		if (HitResult.bBlockingHit)
		{
			ApplyDamage(HitResult);
		}
	}

	// Play fire sound
	if (WeaponData->FireSound.IsValid())
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponData->FireSound.LoadSynchronous(), GetMuzzleLocation());
	}
}

bool AFPSWeaponBase::TryReload()
{
	if (!CanReload())
	{
		return false;
	}

	Reload();
	return true;
}

void AFPSWeaponBase::Reload()
{
	if (!WeaponData)
	{
		return;
	}

	SetWeaponState(EFPSWeaponState::Reloading);

	// Play reload sound
	if (WeaponData->ReloadStartSound.IsValid())
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponData->ReloadStartSound.LoadSynchronous(), GetActorLocation());
	}

	// Set reload timer
	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&AFPSWeaponBase::FinishReload,
		WeaponData->ReloadTime,
		false
	);
}

void AFPSWeaponBase::FinishReload()
{
	if (CurrentState != EFPSWeaponState::Reloading)
	{
		return;
	}

	// Refill magazine
	AmmoInfo.Reload();
	OnAmmoChanged.Broadcast(AmmoInfo.CurrentMagazine, AmmoInfo.CurrentReserve);

	// Play finish sound
	if (WeaponData && WeaponData->ReloadFinishSound.IsValid())
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponData->ReloadFinishSound.LoadSynchronous(), GetActorLocation());
	}

	SetWeaponState(EFPSWeaponState::Idle);
}

void AFPSWeaponBase::CancelReload()
{
	if (CurrentState == EFPSWeaponState::Reloading)
	{
		GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
		SetWeaponState(EFPSWeaponState::Idle);
	}
}

void AFPSWeaponBase::MeleeAttack()
{
	// Base implementation - override in subclasses for melee weapons
}

bool AFPSWeaponBase::CanFire() const
{
	if (!bCanFireAgain)
	{
		return false;
	}

	if (CurrentState != EFPSWeaponState::Idle && CurrentState != EFPSWeaponState::Firing)
	{
		return false;
	}

	if (WeaponData && WeaponData->bUsesAmmo && AmmoInfo.IsMagazineEmpty())
	{
		return false;
	}

	return true;
}

bool AFPSWeaponBase::CanReload() const
{
	if (CurrentState != EFPSWeaponState::Idle)
	{
		return false;
	}

	if (!WeaponData || !WeaponData->bUsesAmmo)
	{
		return false;
	}

	return AmmoInfo.CanReload();
}

int32 AFPSWeaponBase::AddAmmo(int32 Amount)
{
	int32 Leftover = AmmoInfo.AddReserve(Amount);
	OnAmmoChanged.Broadcast(AmmoInfo.CurrentMagazine, AmmoInfo.CurrentReserve);
	return Leftover;
}

int32 AFPSWeaponBase::GetMagazineCapacity() const
{
	return WeaponData ? WeaponData->MagazineSize : 0;
}

FVector AFPSWeaponBase::GetMuzzleLocation() const
{
	if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponMesh->GetSocketLocation(MuzzleSocketName);
	}

	// Fallback to actor location
	return GetActorLocation();
}

FRotator AFPSWeaponBase::GetMuzzleRotation() const
{
	if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
	{
		return WeaponMesh->GetSocketRotation(MuzzleSocketName);
	}

	// Fallback to owner's control rotation
	if (OwningCharacter.IsValid())
	{
		return OwningCharacter->GetControlRotation();
	}

	return GetActorRotation();
}

FVector AFPSWeaponBase::GetFireDirectionWithSpread() const
{
	FVector BaseDirection = FVector::ZeroVector;

	// Get base direction from owner's aim
	if (OwningCharacter.IsValid())
	{
		BaseDirection = OwningCharacter->GetControlRotation().Vector();
	}
	else
	{
		BaseDirection = GetActorForwardVector();
	}

	// Apply spread
	if (CurrentSpread > 0.0f)
	{
		float HalfSpreadRad = FMath::DegreesToRadians(CurrentSpread * 0.5f);
		float RandomAngle = FMath::FRand() * 2.0f * PI;
		float RandomRadius = FMath::FRand() * HalfSpreadRad;

		FVector Right = FVector::CrossProduct(BaseDirection, FVector::UpVector).GetSafeNormal();
		FVector Up = FVector::CrossProduct(Right, BaseDirection).GetSafeNormal();

		BaseDirection = BaseDirection.RotateAngleAxis(FMath::RadiansToDegrees(RandomRadius * FMath::Cos(RandomAngle)), Up);
		BaseDirection = BaseDirection.RotateAngleAxis(FMath::RadiansToDegrees(RandomRadius * FMath::Sin(RandomAngle)), Right);
	}

	return BaseDirection.GetSafeNormal();
}

UAbilitySystemComponent* AFPSWeaponBase::GetOwnerASC() const
{
	if (OwningCharacter.IsValid())
	{
		return OwningCharacter->GetAbilitySystemComponent();
	}
	return nullptr;
}

void AFPSWeaponBase::SetWeaponState(EFPSWeaponState NewState)
{
	if (CurrentState != NewState)
	{
		CurrentState = NewState;
		OnWeaponStateChanged.Broadcast(NewState);
	}
}

void AFPSWeaponBase::UpdateSpread(float DeltaTime)
{
	if (!WeaponData)
	{
		return;
	}

	// Recover spread over time
	if (CurrentSpread > WeaponData->BaseSpread)
	{
		CurrentSpread -= WeaponData->SpreadRecoveryRate * DeltaTime;
		CurrentSpread = FMath::Max(CurrentSpread, WeaponData->BaseSpread);
	}
}

void AFPSWeaponBase::IncreaseSpread()
{
	if (!WeaponData)
	{
		return;
	}

	CurrentSpread += WeaponData->SpreadIncreasePerShot;
	CurrentSpread = FMath::Min(CurrentSpread, WeaponData->MaxSpread);
}

FHitResult AFPSWeaponBase::PerformLineTrace(const FVector& Start, const FVector& End) const
{
	FHitResult HitResult;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (OwningCharacter.IsValid())
	{
		QueryParams.AddIgnoredActor(OwningCharacter.Get());
	}
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnPhysicalMaterial = true;

	GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	return HitResult;
}

void AFPSWeaponBase::ApplyDamage(const FHitResult& HitResult)
{
	if (!WeaponData || !HitResult.GetActor())
	{
		return;
	}

	// Calculate damage based on range
	float Distance = FVector::Dist(GetMuzzleLocation(), HitResult.ImpactPoint);
	float Damage = WeaponData->GetDamageAtRange(Distance);

	// Check for headshot
	if (HitResult.BoneName == TEXT("head"))
	{
		Damage *= WeaponData->HeadshotMultiplier;
	}

	// Apply damage via GAS if target has ASC
	if (UAbilitySystemComponent* TargetASC = HitResult.GetActor()->FindComponentByClass<UAbilitySystemComponent>())
	{
		// Create damage effect
		if (WeaponData->DamageEffectClass)
		{
			UAbilitySystemComponent* SourceASC = GetOwnerASC();
			if (SourceASC)
			{
				FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
				ContextHandle.AddSourceObject(this);
				ContextHandle.AddHitResult(HitResult);

				FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(WeaponData->DamageEffectClass, 1.0f, ContextHandle);
				if (SpecHandle.IsValid())
				{
					// Set damage magnitude
					SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("FPS.Effect.Damage")), Damage);
					TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				}
			}
		}
	}
	else
	{
		// Fallback to standard UE damage system
		UGameplayStatics::ApplyPointDamage(
			HitResult.GetActor(),
			Damage,
			GetFireDirectionWithSpread(),
			HitResult,
			OwningCharacter.IsValid() ? OwningCharacter->GetController() : nullptr,
			this,
			nullptr
		);
	}
}

void AFPSWeaponBase::GrantAbilities()
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC || !WeaponData)
	{
		return;
	}

	// Grant fire ability
	if (WeaponData->FireAbilityClass)
	{
		FGameplayAbilitySpec AbilitySpec(WeaponData->FireAbilityClass, 1, INDEX_NONE, this);
		FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(AbilitySpec);
		GrantedAbilityHandles.Add(Handle);
	}

	// Grant reload ability
	if (WeaponData->ReloadAbilityClass)
	{
		FGameplayAbilitySpec AbilitySpec(WeaponData->ReloadAbilityClass, 1, INDEX_NONE, this);
		FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(AbilitySpec);
		GrantedAbilityHandles.Add(Handle);
	}

	// Grant melee ability
	if (WeaponData->MeleeAbilityClass)
	{
		FGameplayAbilitySpec AbilitySpec(WeaponData->MeleeAbilityClass, 1, INDEX_NONE, this);
		FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(AbilitySpec);
		GrantedAbilityHandles.Add(Handle);
	}
}

void AFPSWeaponBase::RemoveAbilities()
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
	{
		ASC->ClearAbility(Handle);
	}
	GrantedAbilityHandles.Empty();
}

void AFPSWeaponBase::ResetFireCooldown()
{
	bCanFireAgain = true;
}
