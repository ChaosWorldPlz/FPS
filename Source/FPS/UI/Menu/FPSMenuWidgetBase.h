// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSMenuWidgetBase.generated.h"

class UFPSMenuSubsystem;

/**
 * UFPSMenuWidgetBase
 *
 * Base class for all menu widgets in the FPS game.
 * Provides common functionality for showing/hiding with animations.
 */
UCLASS(Abstract)
class FPS_API UFPSMenuWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFPSMenuWidgetBase(const FObjectInitializer& ObjectInitializer);

	//-------------------------------------------------------------------
	// Visibility Control
	//-------------------------------------------------------------------

	/** Show the menu with optional animation */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	virtual void ShowMenu();

	/** Hide the menu with optional animation */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	virtual void HideMenu();

	/** Check if menu is currently visible */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	bool IsMenuVisible() const { return bIsVisible; }

	//-------------------------------------------------------------------
	// Navigation
	//-------------------------------------------------------------------

	/** Go back to previous menu */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	virtual void GoBack();

	/** Close all menus and return to game */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	virtual void CloseAllMenus();

	//-------------------------------------------------------------------
	// Events (for Blueprint/Lua binding)
	//-------------------------------------------------------------------

	/** Called when menu is shown (after animation completes) */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|Events")
	void OnMenuShown();

	/** Called when menu is hidden (after animation completes) */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|Events")
	void OnMenuHidden();

	/** Called when menu is about to be shown (before animation starts) */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|Events")
	void OnMenuShowStarted();

	/** Called when menu is about to be hidden (before animation starts) */
	UFUNCTION(BlueprintImplementableEvent, Category = "FPS|Menu|Events")
	void OnMenuHideStarted();

	/** Native callback before showing - override in C++ subclasses */
	virtual void NativeOnMenuShowStarted() {}

	/** Native callback after showing - override in C++ subclasses */
	virtual void NativeOnMenuShown() {}

	/** Native callback before hiding - override in C++ subclasses */
	virtual void NativeOnMenuHideStarted() {}

	/** Native callback after hiding - override in C++ subclasses */
	virtual void NativeOnMenuHidden() {}

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	//-------------------------------------------------------------------
	// Animation
	//-------------------------------------------------------------------

	/** Duration of show animation in seconds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Menu|Animation")
	float ShowAnimationDuration = 0.25f;

	/** Duration of hide animation in seconds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FPS|Menu|Animation")
	float HideAnimationDuration = 0.2f;

	/** Play show animation - override for custom animations */
	UFUNCTION(BlueprintNativeEvent, Category = "FPS|Menu|Animation")
	void PlayShowAnimation();

	/** Play hide animation - override for custom animations */
	UFUNCTION(BlueprintNativeEvent, Category = "FPS|Menu|Animation")
	void PlayHideAnimation();

	/** Called when show animation completes */
	void OnShowAnimationComplete();

	/** Called when hide animation completes */
	void OnHideAnimationComplete();

	//-------------------------------------------------------------------
	// State
	//-------------------------------------------------------------------

	/** Current visibility state */
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Menu")
	bool bIsVisible = false;

	/** Is animation currently playing */
	UPROPERTY(BlueprintReadOnly, Category = "FPS|Menu")
	bool bIsAnimating = false;

	/** Timer handle for animation */
	FTimerHandle AnimationTimerHandle;

	//-------------------------------------------------------------------
	// Subsystem Access
	//-------------------------------------------------------------------

	/** Get the menu subsystem */
	UFUNCTION(BlueprintCallable, Category = "FPS|Menu")
	UFPSMenuSubsystem* GetMenuSubsystem() const;
};
