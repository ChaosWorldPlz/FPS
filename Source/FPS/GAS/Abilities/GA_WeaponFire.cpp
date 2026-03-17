// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_WeaponFire.h"
#include "AbilitySystemComponent.h"
#include "GameplayCueManager.h"
#include "Animation/AnimMontage.h"
#include "FPS/FPSCharacter.h"
#include "FPS/Weapon/FPSWeaponBase.h"
#include "FPS/Weapon/FPSWeaponDataAsset.h"
#include "FPS/GAS/FPSGameplayTags.h"
#include "FPS/GAS/FPSRecoilComponent.h"

UGA_WeaponFire::UGA_WeaponFire()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationPolicy = EFPSAbilityActivationPolicy::OnInputTriggered;

	// LocalPredicted: fires immediately on client, server validates via ServerFire RPC
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// Set ability tags — use RequestGameplayTag to avoid CDO-before-InitializeNativeTags timing issue
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("FPS.Ability.Weapon.Fire"), false));

	// Block other weapon abilities while firing
	BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(FName("FPS.Ability.Weapon.Reload"), false));
}

void UGA_WeaponFire::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AFPSWeaponBase* Weapon = GetWeapon(Handle, ActorInfo);
	if (!Weapon)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Fire once immediately
	FireWeapon();
	PlayFireMontage();

	// Check for automatic fire mode
	UFPSWeaponDataAsset* WeaponData = Weapon->WeaponData;
	if (WeaponData && WeaponData->FireMode == EFPSWeaponFireMode::Auto)
	{
		// Start auto fire timer
		bAutoFiring = true;
		float FireDelay = WeaponData->GetTimeBetweenShots();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				AutoFireTimerHandle,
				this,
				&UGA_WeaponFire::AutoFireTick,
				FireDelay,
				true
			);
		}
	}
	else
	{
		// Single shot, end ability
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_WeaponFire::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear auto fire timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFireTimerHandle);
	}
	bAutoFiring = false;

	StopFireMontage();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_WeaponFire::InputReleased(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// Stop auto fire when input is released
	if (bAutoFiring)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}

	Super::InputReleased(Handle, ActorInfo, ActivationInfo);
}

bool UGA_WeaponFire::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	AFPSWeaponBase* Weapon = GetWeapon(Handle, ActorInfo);
	if (!Weapon || !Weapon->CanFire())
	{
		return false;
	}

	return true;
}

AFPSWeaponBase* UGA_WeaponFire::GetWeapon(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	FGameplayAbilitySpecHandle SpecHandle = Handle.IsValid() ? Handle : CurrentSpecHandle;
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : GetAbilitySystemComponentFromActorInfo();

	if (SpecHandle.IsValid() && ASC)
	{
		if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(SpecHandle))
		{
			if (AFPSWeaponBase* Weapon = Cast<AFPSWeaponBase>(Spec->SourceObject.Get()))
			{
				return Weapon;
			}
		}
	}

	// Fallback：SourceObject 未设置时（如 DefaultAbilities 直接授予），取角色当前武器
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : GetAvatarActorFromActorInfo();
	if (const AFPSCharacter* Char = Cast<const AFPSCharacter>(Avatar))
	{
		return Char->GetCurrentWeapon();
	}
	return nullptr;
}

void UGA_WeaponFire::FireWeapon()
{
	AFPSWeaponBase* Weapon = GetWeapon(CurrentSpecHandle, CurrentActorInfo);
	if (Weapon && Weapon->CanFire())
	{
		// 通知 RecoilComponent 推进 Pattern / 累加 Spread
		if (AFPSCharacter* Char = Cast<AFPSCharacter>(GetAvatarActorFromActorInfo()))
		{
			if (Char->RecoilComponent && Weapon->WeaponData && Weapon->WeaponData->RecoilProfile)
			{
				Char->RecoilComponent->OnShotFired(Weapon->WeaponData->RecoilProfile);
			}
		}

		Weapon->Fire();

		// 触发 GameplayCue 播放开火音效和特效（仅本地客户端，避免重复）
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayCueParameters CueParams;
			CueParams.SourceObject = Weapon;
			CueParams.Location = Weapon->GetMuzzleLocation();
			CueParams.Normal = Weapon->GetMuzzleRotation().Vector();
			ASC->ExecuteGameplayCue(
				FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.Fire")),
				CueParams);
		}
	}
	else if (bAutoFiring && Weapon && Weapon->AmmoInfo.IsMagazineEmpty())
	{
		// Out of ammo, stop auto fire
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_WeaponFire::AutoFireTick()
{
	FireWeapon();
	PlayFireMontage();
}

float UGA_WeaponFire::PlayFireMontage()
{
	AFPSWeaponBase* Weapon = GetWeapon(CurrentSpecHandle, CurrentActorInfo);
	if (!Weapon || !Weapon->WeaponData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_WeaponFire] PlayFireMontage: Weapon=%s WeaponData=%s"),
			*GetNameSafe(Weapon), Weapon ? *GetNameSafe(Weapon->WeaponData) : TEXT("N/A"));
		return 0.0f;
	}

	UAnimMontage* Montage = Weapon->WeaponData->FireMontage.LoadSynchronous();
	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_WeaponFire] PlayFireMontage: FireMontage is NULL in DataAsset '%s'. Please assign it in the editor."),
			*GetNameSafe(Weapon->WeaponData));
		return 0.0f;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_WeaponFire] PlayFireMontage: AvatarActor is not a Character."));
		return 0.0f;
	}

	// 根据射速自动调整 Montage 播放速率，使动画恰好在下一发开枪前播完
	float PlayRate = 1.0f;
	if (Weapon->WeaponData->FireRate > 0.0f)
	{
		float MontageLength = Montage->GetPlayLength();
		float TimeBetweenShots = Weapon->WeaponData->GetTimeBetweenShots();
		if (MontageLength > 0.0f && TimeBetweenShots > 0.0f)
		{
			PlayRate = MontageLength / TimeBetweenShots;
		}
	}

	return Character->PlayAnimMontage(Montage, PlayRate);
}

void UGA_WeaponFire::StopFireMontage()
{
	AFPSWeaponBase* Weapon = GetWeapon(CurrentSpecHandle, CurrentActorInfo);
	if (!Weapon || !Weapon->WeaponData)
	{
		return;
	}

	UAnimMontage* Montage = Weapon->WeaponData->FireMontage.Get();
	if (!Montage)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character)
	{
		Character->StopAnimMontage(Montage);
	}
}
