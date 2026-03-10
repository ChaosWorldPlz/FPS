// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSPlayerController.h"
#include "FPSCharacter.h"
#include "Weapon/FPSWeaponSlotComponent.h"
#include "Level/FPSWorldWeapon.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Engine/LocalPlayer.h"
#include "System/FPSMenuSubsystem.h"
#include "UI/FPSMenuTypes.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameModeBase.h"

AFPSPlayerController::AFPSPlayerController()
	: MainMenuMapName(TEXT("/Game/FirstPerson/Maps/Level_LoginMain"))
{
}

void AFPSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Get the enhanced input subsystem
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		// Add the mapping context so we get controls
		if (InputMappingContext)
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}

		// Add menu input mapping context with higher priority
		if (MenuInputMappingContext)
		{
			Subsystem->AddMappingContext(MenuInputMappingContext, 1);
		}
	}
}

void AFPSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	SetupInputBindings();
}

void AFPSPlayerController::SetupInputBindings()
{
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Pause menu
		if (PauseMenuAction)
		{
			EnhancedInputComponent->BindAction(PauseMenuAction, ETriggerEvent::Started, this, &AFPSPlayerController::HandlePauseMenuInput);
		}

		// Weapon slot switching
		if (SwitchToSlot1Action)
		{
			EnhancedInputComponent->BindAction(SwitchToSlot1Action, ETriggerEvent::Started, this, &AFPSPlayerController::HandleSwitchToSlot1);
		}
		if (SwitchToSlot2Action)
		{
			EnhancedInputComponent->BindAction(SwitchToSlot2Action, ETriggerEvent::Started, this, &AFPSPlayerController::HandleSwitchToSlot2);
		}
		if (SwitchToSlot3Action)
		{
			EnhancedInputComponent->BindAction(SwitchToSlot3Action, ETriggerEvent::Started, this, &AFPSPlayerController::HandleSwitchToSlot3);
		}
		if (CycleWeaponAction)
		{
			EnhancedInputComponent->BindAction(CycleWeaponAction, ETriggerEvent::Started, this, &AFPSPlayerController::HandleCycleWeapon);
		}

		// Interact
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AFPSPlayerController::HandleInteract);
		}
	}
}

//-------------------------------------------------------------------
// Weapon Slot Inputs
//-------------------------------------------------------------------

void AFPSPlayerController::HandleSwitchToSlot1()
{
	if (AFPSCharacter* FPSChar = Cast<AFPSCharacter>(GetPawn()))
	{
		FPSChar->ServerSwitchWeaponSlot(EFPSWeaponSlot::Primary1);
	}
}

void AFPSPlayerController::HandleSwitchToSlot2()
{
	if (AFPSCharacter* FPSChar = Cast<AFPSCharacter>(GetPawn()))
	{
		FPSChar->ServerSwitchWeaponSlot(EFPSWeaponSlot::Primary2);
	}
}

void AFPSPlayerController::HandleSwitchToSlot3()
{
	if (AFPSCharacter* FPSChar = Cast<AFPSCharacter>(GetPawn()))
	{
		FPSChar->ServerSwitchWeaponSlot(EFPSWeaponSlot::Pistol);
	}
}

void AFPSPlayerController::HandleCycleWeapon()
{
	if (AFPSCharacter* FPSChar = Cast<AFPSCharacter>(GetPawn()))
	{
		FPSChar->ServerCycleWeapon();
	}
}

//-------------------------------------------------------------------
// Interact Input
//-------------------------------------------------------------------

void AFPSPlayerController::HandleInteract()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	// Find overlapping AFPSWorldWeapon actors (proximity detected via sphere collision)
	TArray<AActor*> OverlappingActors;
	ControlledPawn->GetOverlappingActors(OverlappingActors, AFPSWorldWeapon::StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (AFPSWorldWeapon* WorldWeapon = Cast<AFPSWorldWeapon>(Actor))
		{
			if (WorldWeapon->CanInteract())
			{
				AFPSCharacter* FPSChar = Cast<AFPSCharacter>(ControlledPawn);
				WorldWeapon->ServerInteract(FPSChar);
				break; // Pick up one weapon at a time
			}
		}
	}
}

void AFPSPlayerController::HandlePauseMenuInput()
{
	TogglePauseMenu();
}

void AFPSPlayerController::TogglePauseMenu()
{
	UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem();
	if (!MenuSubsystem)
	{
		return;
	}

	if (MenuSubsystem->IsInMenu())
	{
		// If in pause menu, close it
		if (MenuSubsystem->GetCurrentMenuState() == EFPSMenuState::PauseMenu)
		{
			MenuSubsystem->PopAllMenus();
		}
		// If in settings (from pause), go back
		else if (MenuSubsystem->GetCurrentMenuState() == EFPSMenuState::Settings)
		{
			MenuSubsystem->PopMenu();
		}
		// If in main menu, don't do anything (can't close main menu with ESC)
	}
	else
	{
		// Open pause menu
		MenuSubsystem->OpenPauseMenu();
	}
}

bool AFPSPlayerController::IsInMenu() const
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		return MenuSubsystem->IsInMenu();
	}
	return false;
}

UFPSMenuSubsystem* AFPSPlayerController::GetMenuSubsystem() const
{
	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this))
	{
		return GameInstance->GetSubsystem<UFPSMenuSubsystem>();
	}
	return nullptr;
}

//-------------------------------------------------------------------
// Session Management
//-------------------------------------------------------------------

void AFPSPlayerController::HostGame(const FString& MapName, int32 MaxPlayers)
{
	if (!HasAuthority() && !IsLocalController())
	{
		return;
	}

	// Build travel URL with listen option
	FString TravelURL = MapName + TEXT("?listen") + FString::Printf(TEXT("?MaxPlayers=%d"), MaxPlayers);

	UE_LOG(LogTemp, Log, TEXT("Hosting game: %s"), *TravelURL);

	// Server travel to the map
	GetWorld()->ServerTravel(TravelURL);
}

void AFPSPlayerController::JoinGame(const FString& Address)
{
	if (!IsLocalController())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Joining game at: %s"), *Address);

	// Direct connect to IP:Port
	ClientTravel(Address, TRAVEL_Absolute);
}

void AFPSPlayerController::LeaveGame()
{
	if (!IsLocalController())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Leaving game, returning to main menu"));

	// If we're the server, destroy the session
	if (HasAuthority())
	{
		if (AGameModeBase* GM = GetWorld()->GetAuthGameMode())
		{
			GM->ReturnToMainMenuHost();
			return;
		}
	}

	// Client: travel back to main menu
	ClientTravel(MainMenuMapName, TRAVEL_Absolute);
}

//-------------------------------------------------------------------
// Client RPCs
//-------------------------------------------------------------------

void AFPSPlayerController::ClientShowKillFeed_Implementation(FFPSKillFeedInfo KillFeedInfo)
{
	UE_LOG(LogTemp, Log, TEXT("Kill Feed: %s eliminated %s (Weapon: %d, Headshot: %s)"),
		*KillFeedInfo.KillerName, *KillFeedInfo.VictimName,
		static_cast<int32>(KillFeedInfo.WeaponType),
		KillFeedInfo.bHeadshot ? TEXT("Yes") : TEXT("No"));

	// Broadcast to Lua/Blueprint listeners
	OnKillFeedReceived.Broadcast(KillFeedInfo);
}

void AFPSPlayerController::ClientOnMatchStateChanged_Implementation(EFPSMatchState NewState, EFPSTeam Winner)
{
	// This is a hook point for Lua/Blueprint to react to match state changes
	UE_LOG(LogTemp, Log, TEXT("Match State Changed: %d, Winner: %d"), static_cast<int32>(NewState), static_cast<int32>(Winner));
}
