// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "FPS/Team/FPSTeamTypes.h"
#include "FPSPlayerStart.generated.h"

/**
 * AFPSPlayerStart
 *
 * A player start with team assignment.
 * Place in the level and assign a team to control spawn locations per team.
 */
UCLASS()
class FPS_API AFPSPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	AFPSPlayerStart(const FObjectInitializer& ObjectInitializer);

	/** Which team spawns here */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS|Team")
	EFPSTeam Team;
};
