// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSMenuWidgetBase.h"
#include "FPS/System/FPSMenuSubsystem.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UFPSMenuWidgetBase::UFPSMenuWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UFPSMenuWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	// Start hidden by default
	SetVisibility(ESlateVisibility::Collapsed);
	bIsVisible = false;
}

void UFPSMenuWidgetBase::NativeDestruct()
{
	// Clear any pending timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AnimationTimerHandle);
	}

	Super::NativeDestruct();
}

void UFPSMenuWidgetBase::ShowMenu()
{
	if (bIsVisible || bIsAnimating)
	{
		return;
	}

	bIsAnimating = true;

	// Make visible before animation
	SetVisibility(ESlateVisibility::Visible);

	// Notify start
	NativeOnMenuShowStarted();
	OnMenuShowStarted();

	// Play animation
	PlayShowAnimation();
}

void UFPSMenuWidgetBase::HideMenu()
{
	if (!bIsVisible || bIsAnimating)
	{
		return;
	}

	bIsAnimating = true;

	// Notify start
	NativeOnMenuHideStarted();
	OnMenuHideStarted();

	// Play animation
	PlayHideAnimation();
}

void UFPSMenuWidgetBase::GoBack()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->PopMenu();
	}
}

void UFPSMenuWidgetBase::CloseAllMenus()
{
	if (UFPSMenuSubsystem* MenuSubsystem = GetMenuSubsystem())
	{
		MenuSubsystem->PopAllMenus();
	}
}

void UFPSMenuWidgetBase::PlayShowAnimation_Implementation()
{
	// Default implementation: simple fade in using timer
	// Subclasses can override for custom animations

	if (UWorld* World = GetWorld())
	{
		// Set initial opacity
		SetRenderOpacity(0.0f);

		// Animate opacity over time
		float ElapsedTime = 0.0f;
		const float Duration = ShowAnimationDuration;

		World->GetTimerManager().SetTimer(
			AnimationTimerHandle,
			[this, Duration]()
			{
				if (!IsValid(this))
				{
					return;
				}

				float CurrentOpacity = GetRenderOpacity();
				float NewOpacity = FMath::Min(CurrentOpacity + (GetWorld()->GetDeltaSeconds() / Duration), 1.0f);
				SetRenderOpacity(NewOpacity);

				if (NewOpacity >= 1.0f)
				{
					OnShowAnimationComplete();
				}
			},
			0.016f, // ~60fps
			true
		);
	}
	else
	{
		// Fallback: instant show
		SetRenderOpacity(1.0f);
		OnShowAnimationComplete();
	}
}

void UFPSMenuWidgetBase::PlayHideAnimation_Implementation()
{
	// Default implementation: simple fade out using timer
	// Subclasses can override for custom animations

	if (UWorld* World = GetWorld())
	{
		const float Duration = HideAnimationDuration;

		World->GetTimerManager().SetTimer(
			AnimationTimerHandle,
			[this, Duration]()
			{
				if (!IsValid(this))
				{
					return;
				}

				float CurrentOpacity = GetRenderOpacity();
				float NewOpacity = FMath::Max(CurrentOpacity - (GetWorld()->GetDeltaSeconds() / Duration), 0.0f);
				SetRenderOpacity(NewOpacity);

				if (NewOpacity <= 0.0f)
				{
					OnHideAnimationComplete();
				}
			},
			0.016f, // ~60fps
			true
		);
	}
	else
	{
		// Fallback: instant hide
		SetRenderOpacity(0.0f);
		OnHideAnimationComplete();
	}
}

void UFPSMenuWidgetBase::OnShowAnimationComplete()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AnimationTimerHandle);
	}

	bIsAnimating = false;
	bIsVisible = true;

	// Ensure full opacity
	SetRenderOpacity(1.0f);

	// Notify complete
	NativeOnMenuShown();
	OnMenuShown();
}

void UFPSMenuWidgetBase::OnHideAnimationComplete()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AnimationTimerHandle);
	}

	bIsAnimating = false;
	bIsVisible = false;

	// Hide the widget
	SetVisibility(ESlateVisibility::Collapsed);
	SetRenderOpacity(1.0f); // Reset for next show

	// Notify complete
	NativeOnMenuHidden();
	OnMenuHidden();
}

UFPSMenuSubsystem* UFPSMenuWidgetBase::GetMenuSubsystem() const
{
	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this))
	{
		return GameInstance->GetSubsystem<UFPSMenuSubsystem>();
	}
	return nullptr;
}
