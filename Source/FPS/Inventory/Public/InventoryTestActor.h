// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryGridComponent.h"
#include "GameFramework/Actor.h"
#include "InventoryTestActor.generated.h"

UCLASS()
class FPS_API AInventoryTestActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AInventoryTestActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UInventoryGridComponent> GridComponent;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	void RunTests();
};
