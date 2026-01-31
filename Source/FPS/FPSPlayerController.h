// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Team/FPSTeamTypes.h"
#include "FPSPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UFPSMenuSubsystem;

/**
 * AFPSPlayerController
 *
 * Player controller with menu system integration and session management.
 */
UCLASS()
class FPS_API AFPSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFPSPlayerController();

	//-------------------------------------------------------------------
	// Menu Control
	//-------------------------------------------------------------------

	/** Toggle pause menu (ESC key) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	void TogglePauseMenu();

	/** Check if any menu is currently open */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	bool IsInMenu() const;

	/** Get the menu subsystem */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	UFPSMenuSubsystem* GetMenuSubsystem() const;

	//-------------------------------------------------------------------
	// Session Management (for Lua to call)
	//-------------------------------------------------------------------

	/** Host a game: create session and server travel */
	UFUNCTION(BlueprintCallable, Category = "FPS|Session")
	void HostGame(const FString& MapName, int32 MaxPlayers);

	/** Join a game by IP address (LAN direct connect) */
	UFUNCTION(BlueprintCallable, Category = "FPS|Session")
	void JoinGame(const FString& Address);

	/** Leave the current game and return to main menu */
	UFUNCTION(BlueprintCallable, Category = "FPS|Session")
	void LeaveGame();

	//-------------------------------------------------------------------
	// Client RPCs
	//-------------------------------------------------------------------

	/** Show kill feed on client */
	UFUNCTION(Client, Reliable)
	void ClientShowKillFeed(const FString& KillerName, const FString& VictimName);

	/** Notify client of match state change */
	UFUNCTION(Client, Reliable)
	void ClientOnMatchStateChanged(EFPSMatchState NewState, EFPSTeam Winner);

protected:
	//-------------------------------------------------------------------
	// Input
	//-------------------------------------------------------------------

	/** Input Mapping Context to be used for player input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* InputMappingContext;

	/** Input Mapping Context for menu navigation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* MenuInputMappingContext;

	/** Input action for pause/menu toggle */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* PauseMenuAction;

	/** Setup input bindings */
	void SetupInputBindings();

	/** Handle pause menu input */
	void HandlePauseMenuInput();

	//-------------------------------------------------------------------
	// Lifecycle
	//-------------------------------------------------------------------

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Map to travel to for main menu */
	UPROPERTY(EditDefaultsOnly, Category = "FPS|Session")
	FString MainMenuMapName;
};
