#include "UGCWireOverlay.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"

//=============================================================================
// 内部 Slate Widget — 实际绘制逻辑
//=============================================================================

struct FUGCWireData
{
    FVector2D Start;
    FVector2D End;
    FLinearColor Color;
    float Thickness;
};

class SUGCWireCanvas : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SUGCWireCanvas) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs) {}

    //--------------------------------------------------------------
    // 数据更新接口
    //--------------------------------------------------------------

    void BeginWireUpdate()
    {
        PendingWires.Reset();
    }

    void AddWire(FVector2D Start, FVector2D End, FLinearColor Color, float Thickness)
    {
        PendingWires.Add({ Start, End, Color, Thickness });
    }

    void EndWireUpdate()
    {
        CommittedWires = MoveTemp(PendingWires);
        Invalidate(EInvalidateWidgetReason::Paint);
    }

    void SetPendingWire(FVector2D Start, FVector2D End, bool bVisible)
    {
        bPendingVisible = bVisible;
        PendingStart    = Start;
        PendingEnd      = End;
        Invalidate(EInvalidateWidgetReason::Paint);
    }

    //--------------------------------------------------------------
    // Slate 接口
    //--------------------------------------------------------------

    virtual FVector2D ComputeDesiredSize(float) const override
    {
        return FVector2D::ZeroVector;
    }

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
        int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
    {
        for (const FUGCWireData& Wire : CommittedWires)
        {
            DrawBezier(AllottedGeometry, OutDrawElements, LayerId,
                Wire.Start, Wire.End, Wire.Color, Wire.Thickness);
        }

        if (bPendingVisible)
        {
            DrawBezier(AllottedGeometry, OutDrawElements, LayerId,
                PendingStart, PendingEnd,
                FLinearColor(1.f, 1.f, 1.f, 0.55f), 1.5f);
        }

        return LayerId;
    }

private:
    //--------------------------------------------------------------
    // 三次贝塞尔：水平切线，最小切线长 60px
    //--------------------------------------------------------------
    static void DrawBezier(const FGeometry& Geo,
        FSlateWindowElementList& OutElements, int32 LayerId,
        FVector2D P0, FVector2D P3,
        FLinearColor Color, float Thickness)
    {
        const float TanX = FMath::Max(FMath::Abs(P3.X - P0.X) * 0.5f, 60.f);
        const FVector2D P1 = P0 + FVector2D(TanX,  0.f);
        const FVector2D P2 = P3 + FVector2D(-TanX, 0.f);

        constexpr int32 N = 24;
        TArray<FVector2D> Points;
        Points.Reserve(N + 1);

        for (int32 i = 0; i <= N; ++i)
        {
            const float t  = static_cast<float>(i) / N;
            const float u  = 1.f - t;
            const float u2 = u  * u;
            const float u3 = u2 * u;
            const float t2 = t  * t;
            const float t3 = t2 * t;
            Points.Add(u3*P0 + 3.f*u2*t*P1 + 3.f*u*t2*P2 + t3*P3);
        }

        FSlateDrawElement::MakeLines(
            OutElements,
            LayerId,
            Geo.ToPaintGeometry(),
            Points,
            ESlateDrawEffect::None,
            Color,
            /*bAntialias=*/true,
            Thickness);
    }

    TArray<FUGCWireData> CommittedWires;
    TArray<FUGCWireData> PendingWires;

    bool      bPendingVisible = false;
    FVector2D PendingStart;
    FVector2D PendingEnd;
};

//=============================================================================
// UUGCWireOverlay — UMG 包装层
//=============================================================================

TSharedRef<SWidget> UUGCWireOverlay::RebuildWidget()
{
    WireCanvas = SNew(SUGCWireCanvas);
    return WireCanvas.ToSharedRef();
}

void UUGCWireOverlay::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    WireCanvas.Reset();
}

void UUGCWireOverlay::BeginWireUpdate()
{
    if (WireCanvas) WireCanvas->BeginWireUpdate();
}

void UUGCWireOverlay::AddWire(FVector2D Start, FVector2D End,
    float R, float G, float B, float A, float Thickness)
{
    if (WireCanvas)
        WireCanvas->AddWire(Start, End, FLinearColor(R, G, B, A), Thickness);
}

void UUGCWireOverlay::EndWireUpdate()
{
    if (WireCanvas) WireCanvas->EndWireUpdate();
}

void UUGCWireOverlay::SetPendingWire(FVector2D Start, FVector2D End, bool bVisible)
{
    if (WireCanvas) WireCanvas->SetPendingWire(Start, End, bVisible);
}

#if WITH_EDITOR
const FText UUGCWireOverlay::GetPaletteCategory()
{
    return FText::FromString(TEXT("UGC"));
}
#endif
