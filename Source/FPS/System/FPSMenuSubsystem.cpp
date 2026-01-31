// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSMenuSubsystem.h"
#include "FPS/UI/Menu/FPSMenuWidgetBase.h"
#include "FPS/UI/Menu/FPSMainMenuWidget.h"
#include "FPS/UI/Menu/FPSPauseMenuWidget.h"
#include "FPS/UI/Menu/FPSSettingsWidget.h"
#include "FPS/UI/Menu/FPSMapSelectWidget.h"
#include "FPS/UI/Menu/FPSLoadoutWidget.h"
#include "FPS/UI/FPSMenuConfig.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/StreamableManager.h"

void UFPSMenuSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Load menu config
	LoadMenuConfig();

	// Load settings from disk
	LoadSettings();

	// Initialize default maps (TODO: load from DataTable)
	FFPSMapInfo DefaultMap;
	DefaultMap.MapId = FName("Factory");
	DefaultMap.DisplayName = FText::FromString(TEXT("工厂"));
	DefaultMap.Description = FText::FromString(TEXT("废弃的工业区，充满危险和战利品。"));
	DefaultMap.Difficulty = 3;
	DefaultMap.DurationMinutes = 20;
	DefaultMap.MaxPlayers = 4;
	AvailableMaps.Add(DefaultMap);
}

void UFPSMenuSubsystem::LoadMenuConfig()
{
	if (!MenuConfigAsset.IsNull())
	{
		UFPSMenuConfig* Config = MenuConfigAsset.LoadSynchronous();
		if (Config)
		{
			MainMenuWidgetClass = Config->MainMenuWidgetClass;
			PauseMenuWidgetClass = Config->PauseMenuWidgetClass;
			SettingsWidgetClass = Config->SettingsWidgetClass;
			MapSelectWidgetClass = Config->MapSelectWidgetClass;
			LoadoutWidgetClass = Config->LoadoutWidgetClass;

			UE_LOG(LogTemp, Log, TEXT("FPSMenuSubsystem: Menu config loaded successfully"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FPSMenuSubsystem: No MenuConfigAsset set! Configure in Project Settings or Blueprint."));
	}
}

void UFPSMenuSubsystem::Deinitialize()
{
	// Save settings before shutdown
	SaveSettings();

	// Clean up widgets
	MenuStack.Empty();
	MainMenuWidget = nullptr;
	PauseMenuWidget = nullptr;
	SettingsWidget = nullptr;
	MapSelectWidget = nullptr;
	LoadoutWidget = nullptr;

	Super::Deinitialize();
}

//-------------------------------------------------------------------
// Menu Navigation
//-------------------------------------------------------------------

void UFPSMenuSubsystem::OpenMainMenu()
{
	PopAllMenus();

	if (!MainMenuWidget && MainMenuWidgetClass)
	{
		MainMenuWidget = CreateWidget<UFPSMainMenuWidget>(GetGameInstance(), MainMenuWidgetClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport(100);
		}
	}

	if (MainMenuWidget)
	{
		PushMenu(MainMenuWidget);
		SetMenuState(EFPSMenuState::MainMenu);
	}
}

void UFPSMenuSubsystem::OpenPauseMenu()
{
	if (!PauseMenuWidget && PauseMenuWidgetClass)
	{
		PauseMenuWidget = CreateWidget<UFPSPauseMenuWidget>(GetGameInstance(), PauseMenuWidgetClass);
		if (PauseMenuWidget)
		{
			PauseMenuWidget->AddToViewport(100);
		}
	}

	if (PauseMenuWidget)
	{
		// Pause the game
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetGameInstance()->GetWorld(), 0))
		{
			PC->SetPause(true);
		}

		PushMenu(PauseMenuWidget);
		SetMenuState(EFPSMenuState::PauseMenu);
	}
}

void UFPSMenuSubsystem::OpenSettings()
{
	if (!SettingsWidget && SettingsWidgetClass)
	{
		SettingsWidget = CreateWidget<UFPSSettingsWidget>(GetGameInstance(), SettingsWidgetClass);
		if (SettingsWidget)
		{
			SettingsWidget->AddToViewport(101);
		}
	}

	if (SettingsWidget)
	{
		PushMenu(SettingsWidget);
		SetMenuState(EFPSMenuState::Settings);
	}
}

void UFPSMenuSubsystem::OpenMapSelect()
{
	if (!MapSelectWidget && MapSelectWidgetClass)
	{
		MapSelectWidget = CreateWidget<UFPSMapSelectWidget>(GetGameInstance(), MapSelectWidgetClass);
		if (MapSelectWidget)
		{
			MapSelectWidget->AddToViewport(100);
		}
	}

	if (MapSelectWidget)
	{
		PushMenu(MapSelectWidget);
		SetMenuState(EFPSMenuState::MapSelect);
	}
}

void UFPSMenuSubsystem::OpenLoadout()
{
	if (!LoadoutWidget && LoadoutWidgetClass)
	{
		LoadoutWidget = CreateWidget<UFPSLoadoutWidget>(GetGameInstance(), LoadoutWidgetClass);
		if (LoadoutWidget)
		{
			LoadoutWidget->AddToViewport(100);
		}
	}

	if (LoadoutWidget)
	{
		LoadoutWidget->SetLoadoutMode(EFPSLoadoutMode::Preparing);
		PushMenu(LoadoutWidget);
		SetMenuState(EFPSMenuState::Loadout);
	}
}

void UFPSMenuSubsystem::OpenRaidResult(const FFPSRaidResult& Result)
{
	CachedRaidResult = Result;

	if (!LoadoutWidget && LoadoutWidgetClass)
	{
		LoadoutWidget = CreateWidget<UFPSLoadoutWidget>(GetGameInstance(), LoadoutWidgetClass);
		if (LoadoutWidget)
		{
			LoadoutWidget->AddToViewport(100);
		}
	}

	if (LoadoutWidget)
	{
		LoadoutWidget->SetLoadoutMode(EFPSLoadoutMode::RaidResult);
		LoadoutWidget->SetRaidResult(Result);
		PushMenu(LoadoutWidget);
		SetMenuState(EFPSMenuState::Loadout);
	}
}

//-------------------------------------------------------------------
// Menu Stack Management
//-------------------------------------------------------------------

void UFPSMenuSubsystem::PushMenu(UFPSMenuWidgetBase* Menu)
{
	if (!Menu)
	{
		return;
	}

	// Hide current top menu
	if (UFPSMenuWidgetBase* CurrentTop = GetCurrentMenu())
	{
		CurrentTop->HideMenu();
	}

	// Add new menu to stack
	MenuStack.Add(Menu);
	Menu->ShowMenu();

	// Set UI input mode
	SetUIInputMode(true);
}

void UFPSMenuSubsystem::PopMenu()
{
	if (MenuStack.Num() == 0)
	{
		return;
	}

	// Hide and remove top menu
	UFPSMenuWidgetBase* TopMenu = MenuStack.Pop();
	if (TopMenu)
	{
		TopMenu->HideMenu();
	}

	// Show previous menu or return to game
	if (UFPSMenuWidgetBase* NewTop = GetCurrentMenu())
	{
		NewTop->ShowMenu();

		// Update state based on which menu is now on top
		if (NewTop == MainMenuWidget)
		{
			SetMenuState(EFPSMenuState::MainMenu);
		}
		else if (NewTop == PauseMenuWidget)
		{
			SetMenuState(EFPSMenuState::PauseMenu);
		}
		else if (NewTop == SettingsWidget)
		{
			SetMenuState(EFPSMenuState::Settings);
		}
		else if (NewTop == MapSelectWidget)
		{
			SetMenuState(EFPSMenuState::MapSelect);
		}
		else if (NewTop == LoadoutWidget)
		{
			SetMenuState(EFPSMenuState::Loadout);
		}
	}
	else
	{
		// No more menus, return to game
		SetUIInputMode(false);
		SetMenuState(EFPSMenuState::None);

		// Unpause if we were in pause menu
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetGameInstance()->GetWorld(), 0))
		{
			PC->SetPause(false);
		}
	}
}

void UFPSMenuSubsystem::PopAllMenus()
{
	while (MenuStack.Num() > 0)
	{
		UFPSMenuWidgetBase* Menu = MenuStack.Pop();
		if (Menu)
		{
			Menu->HideMenu();
		}
	}

	SetUIInputMode(false);
	SetMenuState(EFPSMenuState::None);

	// Unpause
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetGameInstance()->GetWorld(), 0))
	{
		PC->SetPause(false);
	}
}

UFPSMenuWidgetBase* UFPSMenuSubsystem::GetCurrentMenu() const
{
	return MenuStack.Num() > 0 ? MenuStack.Last() : nullptr;
}

//-------------------------------------------------------------------
// Settings Management
//-------------------------------------------------------------------

void UFPSMenuSubsystem::SetGameSettings(const FFPSGameSettings& NewSettings)
{
	GameSettings = NewSettings;
}

void UFPSMenuSubsystem::ApplySettings()
{
	ApplyGraphicsSettings();
	ApplyAudioSettings();

	SaveSettings();

	OnSettingsApplied.Broadcast();
}

void UFPSMenuSubsystem::ResetSettingsToDefault()
{
	GameSettings.ResetToDefault();
}

void UFPSMenuSubsystem::SaveSettings()
{
	// TODO: Implement save using SaveGame or config file
	// For now, use GameUserSettings for graphics
	if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
	{
		UserSettings->SaveSettings();
	}
}

void UFPSMenuSubsystem::LoadSettings()
{
	// TODO: Implement load using SaveGame or config file
	// For now, use defaults
	GameSettings.ResetToDefault();
}

void UFPSMenuSubsystem::ApplyGraphicsSettings()
{
	if (UGameUserSettings* UserSettings = GEngine->GetGameUserSettings())
	{
		int32 QualityLevel = 0;
		switch (GameSettings.GraphicsQuality)
		{
		case EFPSGraphicsQuality::Low:
			QualityLevel = 0;
			break;
		case EFPSGraphicsQuality::Medium:
			QualityLevel = 1;
			break;
		case EFPSGraphicsQuality::High:
			QualityLevel = 2;
			break;
		case EFPSGraphicsQuality::Ultra:
			QualityLevel = 3;
			break;
		}

		UserSettings->SetOverallScalabilityLevel(QualityLevel);
		UserSettings->ApplySettings(false);
	}

	// Apply FOV to player camera
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetGameInstance()->GetWorld(), 0))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->SetFOV(GameSettings.FieldOfView);
		}
	}
}

void UFPSMenuSubsystem::ApplyAudioSettings()
{
	// TODO: Apply audio settings using Sound Mix
	// This requires setting up Sound Classes and Sound Mix in the project
}

//-------------------------------------------------------------------
// Map Management
//-------------------------------------------------------------------

TArray<FFPSMapInfo> UFPSMenuSubsystem::GetAvailableMaps() const
{
	return AvailableMaps;
}

FFPSMapInfo UFPSMenuSubsystem::GetMapInfo(FName MapId) const
{
	for (const FFPSMapInfo& Map : AvailableMaps)
	{
		if (Map.MapId == MapId)
		{
			return Map;
		}
	}
	return FFPSMapInfo();
}

void UFPSMenuSubsystem::SetSelectedMap(FName MapId)
{
	SelectedMapId = MapId;
}

//-------------------------------------------------------------------
// Raid Flow
//-------------------------------------------------------------------

void UFPSMenuSubsystem::StartRaid()
{
	if (SelectedMapId.IsNone())
	{
		return;
	}

	FFPSMapInfo MapInfo = GetMapInfo(SelectedMapId);
	if (!MapInfo.MapAsset.IsNull())
	{
		PopAllMenus();

		// Load the map
		UGameplayStatics::OpenLevelBySoftObjectPtr(GetGameInstance()->GetWorld(), MapInfo.MapAsset);
	}
}

void UFPSMenuSubsystem::ExitToMainMenu()
{
	PopAllMenus();

	// Load main menu level (or just show main menu if already in menu level)
	// TODO: Configure main menu level name
	// UGameplayStatics::OpenLevel(GetGameInstance()->GetWorld(), FName("MainMenuLevel"));

	OpenMainMenu();
}

void UFPSMenuSubsystem::SetRaidResult(const FFPSRaidResult& Result)
{
	CachedRaidResult = Result;
}

//-------------------------------------------------------------------
// Internal Helpers
//-------------------------------------------------------------------

void UFPSMenuSubsystem::SetUIInputMode(bool bUIOnly)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetGameInstance()->GetWorld(), 0);
	if (!PC)
	{
		return;
	}

	if (bUIOnly)
	{
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(true);
	}
	else
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->SetShowMouseCursor(false);
	}
}

void UFPSMenuSubsystem::SetMenuState(EFPSMenuState NewState)
{
	if (CurrentState != NewState)
	{
		EFPSMenuState OldState = CurrentState;
		CurrentState = NewState;
		OnMenuStateChanged.Broadcast(OldState, NewState);
	}
}
