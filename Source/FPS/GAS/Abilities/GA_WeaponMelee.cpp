// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_WeaponMelee.h"
#include "FPS/Weapon/FPSWeaponBase.h"
#include "FPS/GAS/FPSGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_WeaponMelee::UGA_WeaponMelee()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationPolicy = EFPSAbilityActivationPolicy::OnInputTriggered;

	// Set ability tags
	AbilityTags.AddTag(FFPSGameplayTags::Get().Ability_Weapon_Melee);

	// Block other actions during melee
	BlockAbilitiesWithTag.AddTag(FFPSGameplayTags::Get().Ability_Weapon_Fire);
	BlockAbilitiesWithTag.AddTag(FFPSGameplayTags::Get().Ability_Weapon_Reload);
}

void UGA_WeaponMelee::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// Perform melee attack
	PerformMeleeAttack();

	// Set timer to end ability
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			MeleeTimerHandle,
			FTimerDelegate::CreateLambda([this, Handle, ActorInfo, ActivationInfo]()
			{
				EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
			}),
			MeleeDuration,
			false
		);
	}
}

void UGA_WeaponMelee::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MeleeTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_WeaponMelee::PerformMeleeAttack()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	// Get attack origin and direction
	FVector StartLocation = AvatarActor->GetActorLocation();
	FVector ForwardVector = AvatarActor->GetActorForwardVector();

	// If we have a controller, use its rotation for more accurate aiming
	if (APawn* Pawn = Cast<APawn>(AvatarActor))
	{
		if (AController* Controller = Pawn->GetController())
		{
			ForwardVector = Controller->GetControlRotation().Vector();
		}
	}

	FVector EndLocation = StartLocation + (ForwardVector * MeleeRange);

	// Perform sphere sweep
	TArray<FHitResult> HitResults;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(AvatarActor);

	bool bHit = UKismetSystemLibrary::SphereTraceMulti(
		GetWorld(),
		StartLocation,
		EndLocation,
		MeleeRadius,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	if (!bHit)
	{
		return;
	}

	// Process hits
	TSet<AActor*> DamagedActors;
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || DamagedActors.Contains(HitActor))
		{
			continue;
		}
		DamagedActors.Add(HitActor);

		// Apply damage via GAS if target has ASC
		UAbilitySystemComponent* TargetASC = HitActor->FindComponentByClass<UAbilitySystemComponent>();
		if (TargetASC && SourceASC && MeleeDamageEffect)
		{
			FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
			ContextHandle.AddSourceObject(AvatarActor);
			ContextHandle.AddHitResult(Hit);

			FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(MeleeDamageEffect, GetAbilityLevel(), ContextHandle);
			if (SpecHandle.IsValid())
			{
				// Set damage magnitude
				SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("FPS.Effect.Damage")), MeleeDamage);
				TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}
