// Copyright Epic Games, Inc. All Rights Reserved.

#include "GA_UseItem.h"
#include "FPS/Item/FPSItemEffectData.h"
#include "FPS/GAS/FPSGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

UGA_UseItem::UGA_UseItem()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ActivationPolicy = EFPSAbilityActivationPolicy::OnInputTriggered;

	// Set ability tags
	AbilityTags.AddTag(FFPSGameplayTags::Get().Ability_Item_Use);
}

void UGA_UseItem::SetItemEffectData(UFPSItemEffectData* InEffectData)
{
	ItemEffectData = InEffectData;
}

void UGA_UseItem::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ItemEffectData)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Play use start sound
	if (ItemEffectData->UseStartSound.IsValid())
	{
		AActor* AvatarActor = GetAvatarActorFromActorInfo();
		if (AvatarActor)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				ItemEffectData->UseStartSound.LoadSynchronous(),
				AvatarActor->GetActorLocation()
			);
		}
	}

	// Play use animation
	PlayUseAnimation();

	// Instant use or delayed
	if (ItemEffectData->IsInstantUse())
	{
		OnUseComplete();
	}
	else
	{
		// Set timer for use completion
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				UseTimerHandle,
				this,
				&UGA_UseItem::OnUseComplete,
				ItemEffectData->UseTime,
				false
			);
		}
	}
}

void UGA_UseItem::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear use timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UseTimerHandle);
	}

	// Play cancel sound if cancelled
	if (bWasCancelled && ItemEffectData && ItemEffectData->UseCancelSound.IsValid())
	{
		AActor* AvatarActor = GetAvatarActorFromActorInfo();
		if (AvatarActor)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				ItemEffectData->UseCancelSound.LoadSynchronous(),
				AvatarActor->GetActorLocation()
			);
		}
	}

	// Stop animation if playing
	if (CurrentMontage)
	{
		if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Stop(0.2f, CurrentMontage);
			}
		}
		CurrentMontage = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_UseItem::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	if (!ItemEffectData)
	{
		return false;
	}

	// Check required tags
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		// Check required tags
		if (ItemEffectData->RequiredTags.Num() > 0)
		{
			if (!ASC->HasAllMatchingGameplayTags(ItemEffectData->RequiredTags))
			{
				return false;
			}
		}

		// Check blocked tags
		if (ItemEffectData->BlockedTags.Num() > 0)
		{
			if (ASC->HasAnyMatchingGameplayTags(ItemEffectData->BlockedTags))
			{
				return false;
			}
		}
	}

	return true;
}

void UGA_UseItem::ApplyItemEffects()
{
	if (!ItemEffectData)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	for (const FFPSItemEffectEntry& Entry : ItemEffectData->Effects)
	{
		switch (Entry.EffectType)
		{
		case EFPSItemEffectType::InstantEffect:
		case EFPSItemEffectType::DurationEffect:
			if (Entry.GameplayEffectClass)
			{
				FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
				ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());

				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
					Entry.GameplayEffectClass,
					Entry.Level,
					ContextHandle
				);

				if (SpecHandle.IsValid())
				{
					// Apply magnitude if set
					if (Entry.Magnitude != 1.0f)
					{
						// Set magnitude via SetByCallerTag if available
						for (const FGameplayTag& Tag : Entry.EffectTags)
						{
							SpecHandle.Data->SetSetByCallerMagnitude(Tag, Entry.Magnitude);
						}
					}

					ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				}
			}
			break;

		case EFPSItemEffectType::GrantAbility:
			if (Entry.AbilityClass)
			{
				FGameplayAbilitySpec AbilitySpec(Entry.AbilityClass, Entry.Level, INDEX_NONE, GetAvatarActorFromActorInfo());
				ASC->GiveAbility(AbilitySpec);
			}
			break;

		default:
			break;
		}
	}
}

void UGA_UseItem::OnUseComplete()
{
	// Apply effects
	ApplyItemEffects();

	// Play complete sound
	if (ItemEffectData && ItemEffectData->UseCompleteSound.IsValid())
	{
		AActor* AvatarActor = GetAvatarActorFromActorInfo();
		if (AvatarActor)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				ItemEffectData->UseCompleteSound.LoadSynchronous(),
				AvatarActor->GetActorLocation()
			);
		}
	}

	// End ability
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_UseItem::PlayUseAnimation()
{
	if (!ItemEffectData || !ItemEffectData->UseMontage.IsValid())
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	CurrentMontage = ItemEffectData->UseMontage.LoadSynchronous();
	if (CurrentMontage)
	{
		AnimInstance->Montage_Play(CurrentMontage);
	}
}
