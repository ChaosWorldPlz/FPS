// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPSCrosshairWidget.generated.h"

/**
 * EFPSCrosshairStyle
 *
 * Different crosshair visual styles.
 */
UENUM(BlueprintType)
enum class EFPSCrosshairStyle : uint8
{
	Default		UMETA(DisplayName = "Default"),
	Dot			UMETA(DisplayName = "Dot Only"),
	Circle		UMETA(DisplayName = "Circle"),
	Dynamic		UMETA(DisplayName = "Dynamic Lines"),
	Custom		UMETA(DisplayName = "Custom")
};

/**
 * UFPSCrosshairWidget
 *
 * Dynamic crosshair widget that responds to weapon spread.
 */
UCLASS()
class FPS_API UFPSCrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Set the crosshair spread (in degrees) */
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void SetSpread(float SpreadAngle);

	/** Set the crosshair color */
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void SetCrosshairColor(FLinearColor NewColor);

	/** Set the crosshair style */
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void SetCrosshairStyle(EFPSCrosshairStyle NewStyle);

	/** Show hit marker effect */
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	void ShowHitMarker(bool bKill = false);

	/** Get current spread in screen pixels */
	UFUNCTION(BlueprintCallable, Category = "Crosshair")
	float GetSpreadInPixels() const;

	//-------------------------------------------------------------------
	// Events
	//-------------------------------------------------------------------

	/** Called when spread changes */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crosshair|Events")
	void OnSpreadChanged(float NewSpread, float SpreadPixels);

	/** Called when color changes */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crosshair|Events")
	void OnColorChanged(FLinearColor NewColor);

	/** Called when style changes */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crosshair|Events")
	void OnStyleChanged(EFPSCrosshairStyle NewStyle);

	/** Called to show hit marker */
	UFUNCTION(BlueprintImplementableEvent, Category = "Crosshair|Events")
	void OnHitMarker(bool bKill);

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	/** Current crosshair spread angle (degrees) */
	UPROPERTY(BlueprintReadOnly, Category = "Crosshair")
	float CurrentSpread = 0.0f;

	/** Current crosshair color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	FLinearColor CrosshairColor = FLinearColor::White;

	/** Current crosshair style */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	EFPSCrosshairStyle CrosshairStyle = EFPSCrosshairStyle::Dynamic;

	/** Base crosshair gap (no spread) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair|Config")
	float BaseGap = 5.0f;

	/** Line thickness */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair|Config")
	float LineThickness = 2.0f;

	/** Line length */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair|Config")
	float LineLength = 10.0f;

	/** Dot radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair|Config")
	float DotRadius = 2.0f;

	/** Whether to show center dot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair|Config")
	bool bShowCenterDot = true;

	/** Hit marker duration */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair|Config")
	float HitMarkerDuration = 0.2f;

	/** Hit marker size */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair|Config")
	float HitMarkerSize = 8.0f;

	/** Kill marker color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair|Config")
	FLinearColor KillMarkerColor = FLinearColor::Red;

	/** Hit marker color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair|Config")
	FLinearColor HitMarkerColor = FLinearColor::White;

private:
	/** Convert angle to screen pixels based on FOV */
	float AngleToPixels(float Angle) const;

	/** Current spread in pixels (cached) */
	mutable float CachedSpreadPixels = 0.0f;

	/** Hit marker timer */
	float HitMarkerTimer = 0.0f;

	/** Was last hit a kill */
	bool bLastHitWasKill = false;
};
