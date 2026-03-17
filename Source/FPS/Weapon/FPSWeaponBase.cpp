// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSWeaponBase.h"
#include "FPSWeaponDataAsset.h"
#include "FPSWeaponAttachmentData.h"
#include "FPS/FPSCharacter.h"
#include "AbilitySystemComponent.h"
#include "FPS/GAS/FPSAbilitySystemComponent.h"
#include "FPS/GAS/FPSGameplayAbility.h"
#include "FPS/GAS/FPSCombatAttributeSet.h"
#include "AbilitySystemGlobals.h"
#include "FPS/Team/FPSPlayerState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GenericTeamAgentInterface.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "FPSProjectile.h"
#include "FPS/GAS/FPSRecoilComponent.h"

const FName AFPSWeaponBase::MuzzleSocketName = TEXT("Muzzle");

AFPSWeaponBase::AFPSWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// Root scene component so WeaponMesh can be freely transformed in Blueprint
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// Create weapon mesh
	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFPSWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSWeaponBase, InstalledAttachmentIDs);
}

void AFPSWeaponBase::BeginPlay()
{
	Super::BeginPlay();

	// Initialize ammo from weapon data
	if (WeaponData)
	{
		AmmoInfo = WeaponData->GetDefaultAmmoInfo();
		// Use effective magazine size (accounts for pre-installed attachments)
		AmmoInfo.MaxMagazine = GetEffectiveMagazineSize();
		AmmoInfo.CurrentMagazine = AmmoInfo.MaxMagazine;
		CurrentSpread = GetEffectiveSpread();
	}
}

void AFPSWeaponBase::OnRep_InstalledAttachments()
{
	// Rebuild CachedAttachmentData from InstalledAttachmentIDs
	// (data assets must be looked up via primary asset manager on clients)
	// For now clear the cache; GetAttachment() can lazily reload from ID if needed.
	CachedAttachmentData.Empty();
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
	SetOwner(NewOwner);
	SetInstigator(NewOwner);
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

	// Client: play effects locally (prediction) then RPC to server for authoritative hit detection
	if (OwningCharacter.IsValid() && OwningCharacter->IsLocallyControlled() && !HasAuthority())
	{
		FVector MuzzleLoc = GetMuzzleLocation();
		FVector FireDir = OwningCharacter->GetControlRotation().Vector();
		FHitResult PredictedHit = PerformLineTrace(MuzzleLoc, MuzzleLoc + FireDir * GetEffectiveRange());
		PlayFireEffectsLocally(MuzzleLoc, PredictedHit);
		ServerFire(MuzzleLoc, FireDir);
		return;
	}

	// Server or standalone: authoritative fire
	FVector MuzzleLoc = GetMuzzleLocation();

	if (WeaponData->bUseProjectile && WeaponData->ProjectileClass)
	{
		// 弹体模式：服务端 Spawn，自动 Replicate 到客户端
		FVector FireDir = GetFireDirectionWithSpread();
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner      = this;
		SpawnParams.Instigator = OwningCharacter.Get();
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FTransform SpawnTransform(FireDir.Rotation(), MuzzleLoc);
		if (AFPSProjectile* Proj = GetWorld()->SpawnActor<AFPSProjectile>(
			WeaponData->ProjectileClass, SpawnTransform, SpawnParams))
		{
			Proj->Launch(FireDir, WeaponData, OwningCharacter.Get());
		}

		PlayFireEffectsLocally(MuzzleLoc, FHitResult());
		MulticastFireEffects(MuzzleLoc, FHitResult());
	}
	else
	{
		// Hitscan 模式
		for (int32 i = 0; i < WeaponData->PelletsPerShot; i++)
		{
			FVector FireDir = GetFireDirectionWithSpread();
			FVector EndPoint = MuzzleLoc + (FireDir * GetEffectiveRange());

			FHitResult HitResult = PerformLineTrace(MuzzleLoc, EndPoint);

			if (HitResult.bBlockingHit)
			{
				ApplyDamage(HitResult);
			}

			PlayFireEffectsLocally(MuzzleLoc, HitResult);
			MulticastFireEffects(MuzzleLoc, HitResult);
		}
	}
}

//-------------------------------------------------------------------
// Network RPCs
//-------------------------------------------------------------------

bool AFPSWeaponBase::ServerFire_Validate(FVector MuzzleLocation, FVector FireDirection)
{
	// Basic validation: check muzzle position isn't too far from expected
	FVector ServerMuzzle = GetMuzzleLocation();
	float Distance = FVector::Dist(MuzzleLocation, ServerMuzzle);
	return Distance <= MaxMuzzlePositionError;
}

void AFPSWeaponBase::ServerFire_Implementation(FVector MuzzleLocation, FVector FireDirection)
{
	if (!WeaponData || !CanFire())
	{
		return;
	}

	// Consume ammo on server
	if (WeaponData->bUsesAmmo)
	{
		AmmoInfo.ConsumeAmmo();
		OnAmmoChanged.Broadcast(AmmoInfo.CurrentMagazine, AmmoInfo.CurrentReserve);
	}

	// Set fire cooldown on server
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

	// Server performs hit scan with spread
	for (int32 i = 0; i < WeaponData->PelletsPerShot; i++)
	{
		// Apply spread to client-provided direction
		FVector SpreadDir = FireDirection;
		if (CurrentSpread > 0.0f)
		{
			float HalfSpreadRad = FMath::DegreesToRadians(CurrentSpread * 0.5f);
			float RandomAngle = FMath::FRand() * 2.0f * PI;
			float RandomRadius = FMath::FRand() * HalfSpreadRad;

			FVector Right = FVector::CrossProduct(SpreadDir, FVector::UpVector).GetSafeNormal();
			FVector Up = FVector::CrossProduct(Right, SpreadDir).GetSafeNormal();

			SpreadDir = SpreadDir.RotateAngleAxis(FMath::RadiansToDegrees(RandomRadius * FMath::Cos(RandomAngle)), Up);
			SpreadDir = SpreadDir.RotateAngleAxis(FMath::RadiansToDegrees(RandomRadius * FMath::Sin(RandomAngle)), Right);
			SpreadDir = SpreadDir.GetSafeNormal();
		}

		FVector EndPoint = MuzzleLocation + (SpreadDir * GetEffectiveRange());
		FHitResult HitResult = PerformLineTrace(MuzzleLocation, EndPoint);

		if (HitResult.bBlockingHit)
		{
			ApplyDamage(HitResult);
		}

		MulticastFireEffects(MuzzleLocation, HitResult);
	}

	IncreaseSpread();
}

void AFPSWeaponBase::MulticastFireEffects_Implementation(FVector MuzzleLocation, FHitResult HitResult)
{
	// Skip for the local player who already played effects
	if (OwningCharacter.IsValid() && OwningCharacter->IsLocallyControlled())
	{
		return;
	}

	PlayFireEffectsLocally(MuzzleLocation, HitResult);
}

void AFPSWeaponBase::PlayFireEffectsLocally(FVector MuzzleLocation, const FHitResult& HitResult)
{
	if (!WeaponData)
	{
		return;
	}

	// 1. 枪口火焰特效（优先 Attach 到 Muzzle 插槽，跟随武器移动）
	if (WeaponData->MuzzleFlashEffect.IsValid())
	{
		UParticleSystem* MuzzleVFX = WeaponData->MuzzleFlashEffect.LoadSynchronous();
		if (MuzzleVFX)
		{
			if (WeaponMesh && WeaponMesh->DoesSocketExist(MuzzleSocketName))
			{
				UGameplayStatics::SpawnEmitterAttached(
					MuzzleVFX,
					WeaponMesh,
					MuzzleSocketName,
					FVector::ZeroVector,
					FRotator::ZeroRotator,
					EAttachLocation::SnapToTarget
				);
			}
			else
			{
				UGameplayStatics::SpawnEmitterAtLocation(this, MuzzleVFX, MuzzleLocation, GetMuzzleRotation());
			}
		}
	}

	// 2. 弹道曳光线特效（从枪口到命中点/最远射程，通过 BeamEnd 参数传递终点）
	if (WeaponData->TracerEffect.IsValid())
	{
		UParticleSystem* TracerVFX = WeaponData->TracerEffect.LoadSynchronous();
		if (TracerVFX)
		{
			FVector TraceEnd = HitResult.bBlockingHit
				? FVector(HitResult.ImpactPoint)
				: (MuzzleLocation + GetMuzzleRotation().Vector() * GetEffectiveRange());

			UParticleSystemComponent* TracerComp = UGameplayStatics::SpawnEmitterAtLocation(
				this, TracerVFX, MuzzleLocation, (TraceEnd - MuzzleLocation).Rotation()
			);
			// Cascade 曳光粒子通常用 BeamEnd 参数指定光束终点
			if (TracerComp)
			{
				TracerComp->SetVectorParameter(FName("BeamEnd"), TraceEnd);
			}
		}
	}

	// 3. 命中点特效
	if (HitResult.bBlockingHit && WeaponData->ImpactEffect.IsValid())
	{
		UParticleSystem* ImpactVFX = WeaponData->ImpactEffect.LoadSynchronous();
		if (ImpactVFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				this, ImpactVFX, HitResult.ImpactPoint, HitResult.ImpactNormal.Rotation()
			);
		}
	}

	// 4. 开火音效
	if (WeaponData->FireSound.IsValid())
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponData->FireSound.LoadSynchronous(), MuzzleLocation);
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

	// Set reload timer (uses effective reload time including attachment modifiers)
	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle,
		this,
		&AFPSWeaponBase::FinishReload,
		GetEffectiveReloadTime(),
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
	return GetEffectiveMagazineSize();
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

FVector AFPSWeaponBase::GetFireDirectionWithSpread_Implementation()
{
	FVector BaseDirection = OwningCharacter.IsValid()
		? OwningCharacter->GetControlRotation().Vector()
		: GetActorForwardVector();

	// 优先走 RecoilComponent（含 Pattern + Spread + Lua 重写）
	if (OwningCharacter.IsValid() && OwningCharacter->RecoilComponent)
	{
		return OwningCharacter->RecoilComponent->GetFireDirection(BaseDirection);
	}

	// Fallback：无 RecoilComponent 时保留旧随机圆锥逻辑
	if (CurrentSpread > 0.0f)
	{
		float HalfSpreadRad = FMath::DegreesToRadians(CurrentSpread * 0.5f);
		float RandomAngle   = FMath::FRand() * 2.0f * PI;
		float RandomRadius  = FMath::FRand() * HalfSpreadRad;

		FVector Right = FVector::CrossProduct(BaseDirection, FVector::UpVector).GetSafeNormal();
		FVector Up    = FVector::CrossProduct(Right, BaseDirection).GetSafeNormal();

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

	const float BaseSpread = GetEffectiveSpread();

	// Recover spread over time toward the (potentially attachment-adjusted) base
	if (CurrentSpread > BaseSpread)
	{
		CurrentSpread -= WeaponData->SpreadRecoveryRate * DeltaTime;
		CurrentSpread = FMath::Max(CurrentSpread, BaseSpread);
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

	// Notify Lua (or Blueprint) that a shot was fired — used to advance recoil pattern
	OnShotFired();
}

void AFPSWeaponBase::OnShotFired_Implementation()
{
	// Default C++ implementation: advance pattern index.
	// Lua overrides this to also track time for pattern reset.
	CurrentPatternIndex++;
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

bool AFPSWeaponBase::IsFriendlyTarget(const FHitResult& HitResult) const
{
	if (!HitResult.GetActor())
	{
		return false;
	}

	AFPSCharacter* OwnerChar = OwningCharacter.IsValid() ? OwningCharacter.Get() : nullptr;
	AFPSCharacter* TargetChar = Cast<AFPSCharacter>(HitResult.GetActor());

	if (OwnerChar && TargetChar)
	{
		ETeamAttitude::Type Attitude = OwnerChar->GetTeamAttitudeTowards(*TargetChar);
		return Attitude == ETeamAttitude::Friendly;
	}

	return false;
}

void AFPSWeaponBase::ApplyDamage(const FHitResult& HitResult)
{
	if (!WeaponData || !HitResult.GetActor())
	{
		return;
	}

	// Team filtering: don't damage friendlies
	if (IsFriendlyTarget(HitResult))
	{
		return;
	}

	// Calculate damage based on range (using attachment-adjusted base damage)
	float Distance = FVector::Dist(GetMuzzleLocation(), HitResult.ImpactPoint);
	// Scale effective damage by the same range falloff curve as the base weapon
	const float BaseDamageFraction = (WeaponData->BaseDamage > 0.f)
		? (WeaponData->GetDamageAtRange(Distance) / WeaponData->BaseDamage)
		: 1.f;
	float Damage = GetEffectiveDamage() * BaseDamageFraction;

	// Check for headshot
	bLastHitWasHeadshot = (HitResult.BoneName == TEXT("head"));
	if (bLastHitWasHeadshot)
	{
		Damage *= WeaponData->HeadshotMultiplier;
	}

	// Apply damage via GAS if target has ASC
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitResult.GetActor()))
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
	if (!HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	auto GrantOne = [&](TSubclassOf<UGameplayAbility> AbilityClass)
	{
		if (AbilityClass)
		{
			FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
			GrantedAbilityHandles.Add(ASC->GiveAbility(Spec));
		}
	};

	if (WeaponData)
	{
		GrantOne(WeaponData->FireAbilityClass);
		GrantOne(WeaponData->ReloadAbilityClass);
		GrantOne(WeaponData->MeleeAbilityClass);
	}

	for (const TSubclassOf<UFPSGameplayAbility>& AbilityClass : WeaponAbilities)
	{
		GrantOne(AbilityClass);
	}
}

void AFPSWeaponBase::RemoveAbilities()
{
	// Only the server can remove abilities
	if (!HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		GrantedAbilityHandles.Empty();
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

//-------------------------------------------------------------------
// Attachment System
//-------------------------------------------------------------------

bool AFPSWeaponBase::InstallAttachment(EFPSAttachmentSlotType Slot, UFPSWeaponAttachmentData* AttData)
{
	if (!AttData || Slot == EFPSAttachmentSlotType::None)
	{
		return false;
	}

	// Verify the slot is supported by this weapon
	if (!CanInstallAttachment(Slot))
	{
		return false;
	}

	// Verify the attachment is for the correct slot
	if (AttData->SlotType != Slot)
	{
		return false;
	}

	// Install (replaces any existing attachment in the slot silently)
	InstalledAttachmentIDs.RemoveAll([Slot](const FFPSInstalledAttachment& E){ return E.Slot == Slot; });
	FFPSInstalledAttachment Entry;
	Entry.Slot = Slot;
	Entry.AttachmentID = AttData->AttachmentID;
	InstalledAttachmentIDs.Add(Entry);
	CachedAttachmentData.Add(Slot, AttData);

	return true;
}

bool AFPSWeaponBase::RemoveAttachment(EFPSAttachmentSlotType Slot, UFPSWeaponAttachmentData*& OutData)
{
	OutData = nullptr;

	UFPSWeaponAttachmentData** Found = CachedAttachmentData.Find(Slot);
	if (!Found || !(*Found))
	{
		return false;
	}

	OutData = *Found;
	CachedAttachmentData.Remove(Slot);
	InstalledAttachmentIDs.RemoveAll([Slot](const FFPSInstalledAttachment& E){ return E.Slot == Slot; });
	return true;
}

bool AFPSWeaponBase::CanInstallAttachment(EFPSAttachmentSlotType Slot) const
{
	if (!WeaponData || Slot == EFPSAttachmentSlotType::None)
	{
		return false;
	}
	return WeaponData->SupportedAttachmentSlots.Contains(Slot);
}

UFPSWeaponAttachmentData* AFPSWeaponBase::GetAttachment(EFPSAttachmentSlotType Slot) const
{
	UFPSWeaponAttachmentData* const* Found = CachedAttachmentData.Find(Slot);
	return Found ? *Found : nullptr;
}

//-------------------------------------------------------------------
// Effective Stat Queries
//-------------------------------------------------------------------

float AFPSWeaponBase::GetEffectiveDamage() const
{
	if (!WeaponData)
	{
		return 0.f;
	}
	float Total = WeaponData->BaseDamage;
	for (const auto& Pair : CachedAttachmentData)
	{
		if (Pair.Value)
		{
			Total += Pair.Value->StatModifiers.DamageDelta;
		}
	}
	return FMath::Max(0.f, Total);
}

float AFPSWeaponBase::GetEffectiveSpread() const
{
	if (!WeaponData)
	{
		return 0.f;
	}
	float Total = WeaponData->BaseSpread;
	for (const auto& Pair : CachedAttachmentData)
	{
		if (Pair.Value)
		{
			Total += Pair.Value->StatModifiers.SpreadDelta;
		}
	}
	return FMath::Max(0.f, Total);
}

float AFPSWeaponBase::GetEffectiveReloadTime() const
{
	if (!WeaponData)
	{
		return 2.f;
	}
	float Total = WeaponData->ReloadTime;
	for (const auto& Pair : CachedAttachmentData)
	{
		if (Pair.Value)
		{
			Total += Pair.Value->StatModifiers.ReloadTimeDelta;
		}
	}
	return FMath::Max(0.1f, Total); // Minimum 0.1s to avoid timer issues
}

int32 AFPSWeaponBase::GetEffectiveMagazineSize() const
{
	if (!WeaponData)
	{
		return 1;
	}
	float Total = static_cast<float>(WeaponData->MagazineSize);
	for (const auto& Pair : CachedAttachmentData)
	{
		if (Pair.Value)
		{
			Total += Pair.Value->StatModifiers.MagazineSizeDelta;
		}
	}
	return FMath::Max(1, FMath::RoundToInt(Total));
}

float AFPSWeaponBase::GetEffectiveRange() const
{
	if (!WeaponData)
	{
		return 5000.f;
	}
	float Total = WeaponData->MaxRange;
	for (const auto& Pair : CachedAttachmentData)
	{
		if (Pair.Value)
		{
			Total += Pair.Value->StatModifiers.RangeDelta;
		}
	}
	return FMath::Max(100.f, Total);
}
