// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSCrosshairWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"

void UFPSCrosshairWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

int32 UFPSCrosshairWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// Let parent paint first
	int32 RetLayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements,
		LayerId, InWidgetStyle, bParentEnabled);

	// Calculate center of widget
	FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	FVector2D Center = LocalSize * 0.5f;

	// Get spread in pixels
	float SpreadPixels = GetSpreadInPixels();
	float Gap = BaseGap + SpreadPixels;

	// Draw based on style
	if (CrosshairStyle == EFPSCrosshairStyle::Dynamic || CrosshairStyle == EFPSCrosshairStyle::Default)
	{
		// Draw four lines
		TArray<FVector2D> Points;

		// Top line
		Points.Empty();
		Points.Add(FVector2D(Center.X, Center.Y - Gap));
		Points.Add(FVector2D(Center.X, Center.Y - Gap - LineLength));
		FSlateDrawElement::MakeLines(OutDrawElements, RetLayerId, AllottedGeometry.ToPaintGeometry(),
			Points, ESlateDrawEffect::None, CrosshairColor, true, LineThickness);

		// Bottom line
		Points.Empty();
		Points.Add(FVector2D(Center.X, Center.Y + Gap));
		Points.Add(FVector2D(Center.X, Center.Y + Gap + LineLength));
		FSlateDrawElement::MakeLines(OutDrawElements, RetLayerId, AllottedGeometry.ToPaintGeometry(),
			Points, ESlateDrawEffect::None, CrosshairColor, true, LineThickness);

		// Left line
		Points.Empty();
		Points.Add(FVector2D(Center.X - Gap, Center.Y));
		Points.Add(FVector2D(Center.X - Gap - LineLength, Center.Y));
		FSlateDrawElement::MakeLines(OutDrawElements, RetLayerId, AllottedGeometry.ToPaintGeometry(),
			Points, ESlateDrawEffect::None, CrosshairColor, true, LineThickness);

		// Right line
		Points.Empty();
		Points.Add(FVector2D(Center.X + Gap, Center.Y));
		Points.Add(FVector2D(Center.X + Gap + LineLength, Center.Y));
		FSlateDrawElement::MakeLines(OutDrawElements, RetLayerId, AllottedGeometry.ToPaintGeometry(),
			Points, ESlateDrawEffect::None, CrosshairColor, true, LineThickness);
	}

	// Draw center dot
	if (bShowCenterDot || CrosshairStyle == EFPSCrosshairStyle::Dot)
	{
		// Draw a small box as the dot
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			RetLayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2D(Center.X - DotRadius, Center.Y - DotRadius),
				FVector2D(DotRadius * 2, DotRadius * 2)
			),
			FCoreStyle::Get().GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			CrosshairColor
		);
	}

	return RetLayerId;
}

void UFPSCrosshairWidget::SetSpread(float SpreadAngle)
{
	CurrentSpread = SpreadAngle;
	float SpreadPixels = AngleToPixels(SpreadAngle);
	CachedSpreadPixels = SpreadPixels;

	OnSpreadChanged(SpreadAngle, SpreadPixels);
}

void UFPSCrosshairWidget::SetCrosshairColor(FLinearColor NewColor)
{
	CrosshairColor = NewColor;
	OnColorChanged(NewColor);
}

void UFPSCrosshairWidget::SetCrosshairStyle(EFPSCrosshairStyle NewStyle)
{
	CrosshairStyle = NewStyle;
	OnStyleChanged(NewStyle);
}

void UFPSCrosshairWidget::ShowHitMarker(bool bKill)
{
	bLastHitWasKill = bKill;
	HitMarkerTimer = HitMarkerDuration;

	OnHitMarker(bKill);
}

float UFPSCrosshairWidget::GetSpreadInPixels() const
{
	return AngleToPixels(CurrentSpread);
}

float UFPSCrosshairWidget::AngleToPixels(float Angle) const
{
	// Get viewport size
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			int32 ViewportSizeX, ViewportSizeY;
			PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

			// Assume 90 degree FOV for calculation
			// pixels = tan(angle) * (viewport_width / 2) / tan(fov/2)
			float FOV = 90.0f;
			if (PC->PlayerCameraManager)
			{
				FOV = PC->PlayerCameraManager->GetFOVAngle();
			}

			float HalfFOVRad = FMath::DegreesToRadians(FOV * 0.5f);
			float AngleRad = FMath::DegreesToRadians(Angle);

			float Pixels = FMath::Tan(AngleRad) * (ViewportSizeX * 0.5f) / FMath::Tan(HalfFOVRad);
			return Pixels;
		}
	}

	// Fallback calculation
	return Angle * 10.0f;
}
