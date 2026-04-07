// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Team/FPSTeamTypes.h"
#include "Weapon/FPSWeaponTypes.h"
#include "Weapon/FPSAttachmentTypes.h"
#include "FPSPlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UFPSMenuSubsystem;

/** Information about a kill for the kill feed UI */
USTRUCT(BlueprintType)
struct FFPSKillFeedInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "KillFeed")
	FString KillerName;

	UPROPERTY(BlueprintReadOnly, Category = "KillFeed")
	FString VictimName;

	UPROPERTY(BlueprintReadOnly, Category = "KillFeed")
	EFPSTeam KillerTeam = EFPSTeam::None;

	UPROPERTY(BlueprintReadOnly, Category = "KillFeed")
	EFPSTeam VictimTeam = EFPSTeam::None;

	UPROPERTY(BlueprintReadOnly, Category = "KillFeed")
	EFPSWeaponType WeaponType = EFPSWeaponType::None;

	UPROPERTY(BlueprintReadOnly, Category = "KillFeed")
	bool bHeadshot = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnKillFeedReceived, const FFPSKillFeedInfo&, KillFeedInfo);

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

	/** 切换到 UI+游戏 输入模式（显示鼠标） */
	UFUNCTION(BlueprintCallable, Category = "FPS|Input")
	void SetInputModeGameAndUI();

	/** 切换到纯游戏输入模式（隐藏鼠标） */
	UFUNCTION(BlueprintCallable, Category = "FPS|Input")
	void SetInputModeGameOnly();

	/** Toggle pause menu (ESC key)，Lua 可通过 BlueprintNativeEvent 覆盖 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Menu")
	void TogglePauseMenu();
	virtual void TogglePauseMenu_Implementation();

	/** Toggle inventory (TAB key)，Lua 可通过 BlueprintNativeEvent 覆盖 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "FPS|Menu")
	void ToggleInventory();
	virtual void ToggleInventory_Implementation();

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
	void ClientShowKillFeed(FFPSKillFeedInfo KillFeedInfo);

	/** Delegate for Lua/Blueprint to bind kill feed UI */
	UPROPERTY(BlueprintAssignable, Category = "FPS|Events")
	FOnKillFeedReceived OnKillFeedReceived;

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

	/** Weapon slot 1 (Primary1) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Weapon")
	UInputAction* SwitchToSlot1Action;

	/** Weapon slot 2 (Primary2) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Weapon")
	UInputAction* SwitchToSlot2Action;

	/** Weapon slot 3 (Pistol) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Weapon")
	UInputAction* SwitchToSlot3Action;

	/** Cycle weapon (Q / mouse wheel) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Weapon")
	UInputAction* CycleWeaponAction;

	/** Interact (E key — pick up world weapons, open doors, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Weapon")
	UInputAction* InteractAction;

	/** Inventory toggle (TAB key) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InventoryAction;

	/** Setup input bindings */
	void SetupInputBindings();

	/** Handle pause menu input */
	void HandlePauseMenuInput();

	/** Handle inventory toggle input */
	void HandleInventoryInput();

	/** Handle weapon slot switch inputs */
	void HandleSwitchToSlot1();
	void HandleSwitchToSlot2();
	void HandleSwitchToSlot3();
	void HandleCycleWeapon();

	/** Handle interact input (pick up nearby AFPSWorldWeapon) */
	void HandleInteract();

	//-------------------------------------------------------------------
	// Lifecycle
	//-------------------------------------------------------------------

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Map to travel to for main menu */
	UPROPERTY(EditDefaultsOnly, Category = "FPS|Session")
	FString MainMenuMapName;
};
