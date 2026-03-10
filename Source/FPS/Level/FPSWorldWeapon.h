// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FPS/Weapon/FPSAttachmentTypes.h"
#include "FPSWorldWeapon.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class AFPSWeaponBase;
class UFPSWeaponAttachmentData;
class AFPSCharacter;

/**
 * AFPSWorldWeapon
 *
 * A weapon lying in the world that players can pick up.
 *
 * Interaction flow:
 *  1. Player walks near the weapon — sphere collision triggers OnOverlapBegin/End
 *     on the character's capsule, which caches this actor as "nearby interactable".
 *  2. Player presses E — PlayerController detects overlapping AFPSWorldWeapon and
 *     calls ServerInteract().
 *  3. ServerInteract() tries to put the weapon into an empty carry slot via
 *     UFPSWeaponSlotComponent::SetWeaponInSlot().
 *  4. If no slot is free, the weapon is placed in the inventory grid instead.
 *  5. On success, the actor destroys itself on the server (replicates to clients).
 */
UCLASS()
class FPS_API AFPSWorldWeapon : public AActor
{
	GENERATED_BODY()

public:
	AFPSWorldWeapon();

	//-------------------------------------------------------------------
	// Components
	//-------------------------------------------------------------------

	/** Visual representation of the weapon in the world */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* WeaponMesh;

	/** Proximity trigger — characters inside this sphere can interact */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USphereComponent* InteractionSphere;

	//-------------------------------------------------------------------
	// Configuration
	//-------------------------------------------------------------------

	/** Weapon actor class to spawn when picked up */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AFPSWeaponBase> WeaponActorClass;

	/** ItemDefID of this weapon in the item DataTable (for inventory item creation) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FName WeaponItemDefID;

	/** Pre-installed attachments (optional) spawned along with the weapon */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Attachments")
	TMap<EFPSAttachmentSlotType, UFPSWeaponAttachmentData*> PreInstalledAttachments;

	//-------------------------------------------------------------------
	// Interaction
	//-------------------------------------------------------------------

	/** Server RPC — called when a character interacts with this weapon */
	UFUNCTION(Server, Reliable)
	void ServerInteract(AFPSCharacter* Interactor);

	/** Can a character currently pick up this weapon? */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool CanInteract() const { return bAvailable; }

protected:
	virtual void BeginPlay() override;

	/** Whether the weapon is still available for pickup (false after first interaction) */
	bool bAvailable = true;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                    bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
