#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UGCWireOverlay.generated.h"

class SUGCWireCanvas;

/**
 * UGCWireOverlay
 * 蓝图编辑器连线覆盖层，用 Slate OnPaint 绘制贝塞尔曲线。
 * 放置在 CanvasPanel 上方的 Overlay 槽，Visibility = HitTestInvisible。
 *
 * 使用方式（Lua 每帧调用）：
 *   self.w_wire_overlay:BeginWireUpdate()
 *   self.w_wire_overlay:AddWire(sx, sy, ex, ey, r, g, b, a, thickness)
 *   self.w_wire_overlay:EndWireUpdate()
 *   self.w_wire_overlay:SetPendingWire(sx, sy, ex, ey, bVisible)
 */
UCLASS()
class FPS_API UUGCWireOverlay : public UWidget
{
    GENERATED_BODY()

public:
    /** 清除上一帧所有连线，开始新一轮收集 */
    UFUNCTION(BlueprintCallable, Category = "UGC|Wire")
    void BeginWireUpdate();

    /** 添加一条贝塞尔连线（坐标为画布本地坐标） */
    UFUNCTION(BlueprintCallable, Category = "UGC|Wire")
    void AddWire(FVector2D Start, FVector2D End,
        float R, float G, float B, float A, float Thickness);

    /** 提交本轮连线，触发重绘 */
    UFUNCTION(BlueprintCallable, Category = "UGC|Wire")
    void EndWireUpdate();

    /** 设置拖拽中的"待连"临时线（每帧更新鼠标位置） */
    UFUNCTION(BlueprintCallable, Category = "UGC|Wire")
    void SetPendingWire(FVector2D Start, FVector2D End, bool bVisible);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
    virtual const FText GetPaletteCategory() override;
#endif

private:
    TSharedPtr<SUGCWireCanvas> WireCanvas;
};
