// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSSettingsWidget.h"
#include "FPS/System/FPSMenuSubsystem.h"

void UFPSSettingsWidget::NativeOnMenuShown()
{
	LoadCurrentSettings();
	bSettingsModified = false;
}

void UFPSSettingsWidget::LoadCurrentSettings()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		EditingSettings = MenuSubsystem->GetGameSettings();
	}

	// TODO: Load key bindings from input system
}

//-------------------------------------------------------------------
// Graphics Settings
//-------------------------------------------------------------------

void UFPSSettingsWidget::SetGraphicsQuality(EFPSGraphicsQuality Quality)
{
	EditingSettings.GraphicsQuality = Quality;
	bSettingsModified = true;
}

EFPSGraphicsQuality UFPSSettingsWidget::GetGraphicsQuality() const
{
	return EditingSettings.GraphicsQuality;
}

void UFPSSettingsWidget::SetFieldOfView(float FOV)
{
	EditingSettings.FieldOfView = FMath::Clamp(FOV, 60.0f, 120.0f);
	bSettingsModified = true;
}

float UFPSSettingsWidget::GetFieldOfView() const
{
	return EditingSettings.FieldOfView;
}

//-------------------------------------------------------------------
// Audio Settings
//-------------------------------------------------------------------

void UFPSSettingsWidget::SetSFXVolume(float Volume)
{
	EditingSettings.SFXVolume = FMath::Clamp(Volume, 0.0f, 100.0f);
	bSettingsModified = true;
}

float UFPSSettingsWidget::GetSFXVolume() const
{
	return EditingSettings.SFXVolume;
}

void UFPSSettingsWidget::SetMusicVolume(float Volume)
{
	EditingSettings.MusicVolume = FMath::Clamp(Volume, 0.0f, 100.0f);
	bSettingsModified = true;
}

float UFPSSettingsWidget::GetMusicVolume() const
{
	return EditingSettings.MusicVolume;
}

//-------------------------------------------------------------------
// Control Settings
//-------------------------------------------------------------------

void UFPSSettingsWidget::SetMouseSensitivity(float Sensitivity)
{
	EditingSettings.MouseSensitivity = FMath::Clamp(Sensitivity, 0.1f, 3.0f);
	bSettingsModified = true;
}

float UFPSSettingsWidget::GetMouseSensitivity() const
{
	return EditingSettings.MouseSensitivity;
}

//-------------------------------------------------------------------
// Key Bindings
//-------------------------------------------------------------------

TArray<FFPSKeyBinding> UFPSSettingsWidget::GetKeyBindings() const
{
	return KeyBindings;
}

void UFPSSettingsWidget::SetKeyBinding(FName ActionName, FKey NewKey, bool bPrimary)
{
	for (FFPSKeyBinding& Binding : KeyBindings)
	{
		if (Binding.ActionName == ActionName)
		{
			if (bPrimary)
			{
				Binding.PrimaryKey = NewKey;
			}
			else
			{
				Binding.SecondaryKey = NewKey;
			}
			bSettingsModified = true;
			return;
		}
	}
}

void UFPSSettingsWidget::ResetKeyBindings()
{
	// TODO: Reset to default key bindings
	bSettingsModified = true;
}

//-------------------------------------------------------------------
// Apply / Reset
//-------------------------------------------------------------------

void UFPSSettingsWidget::ApplySettings()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->SetGameSettings(EditingSettings);
		MenuSubsystem->ApplySettings();
	}

	// TODO: Apply key bindings

	bSettingsModified = false;
	OnSettingsApplied();
}

void UFPSSettingsWidget::ResetToDefault()
{
	EditingSettings.ResetToDefault();
	ResetKeyBindings();
	bSettingsModified = true;
	OnSettingsReset();
}

bool UFPSSettingsWidget::HasUnsavedChanges() const
{
	return bSettingsModified;
}

void UFPSSettingsWidget::OnBackClicked_Implementation()
{
	GoBack();
}
