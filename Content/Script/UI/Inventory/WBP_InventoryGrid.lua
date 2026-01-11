-- Content/Script/UI/Inventory/WBP_InventoryGrid.lua
-- 背包网格 Widget 的 Lua 逻辑

local WBP_InventoryGrid = UnLua.Class()

--[[
============================================================================
                              生命周期
============================================================================
--]]

function WBP_InventoryGrid:Construct()
    print("[WBP_InventoryGrid] WBP_InventoryGrid:Construct")

    -- 初始化拖拽状态
    self:InitDragState()
end

function WBP_InventoryGrid:Destruct()
    print("[WBP_InventoryGrid] WBP_InventoryGrid:Destruct")

    -- 清理拖拽状态
    self:CleanupDragState()
end

--[[
============================================================================
                            拖拽状态管理
============================================================================
--]]

-- 初始化拖拽状态（类似 C++ 的结构体）
function WBP_InventoryGrid:InitDragState()
    self.DragState = {
        -- 基础状态
        IsDragging = false,           -- 是否正在拖拽

        -- 被拖拽的物品信息
        DraggedItem = nil,            -- FInventoryItem 数据
        DraggedWidget = nil,          -- 原始 ItemIconWidget
        OriginalPosition = nil,       -- 原始网格位置 FIntPoint
        OriginalRotated = false,      -- 原始旋转状态

        -- 拖拽中的状态
        CurrentRotated = false,       -- 当前旋转状态（可能按R改变）
        CurrentGridPos = nil,         -- 当前鼠标所在的网格位置
        CanPlace = false,             -- 当前位置是否可以放置

        -- 预览相关
        PreviewWidget = nil,          -- 拖拽预览 Widget
        HighlightWidgets = {},        -- 高亮格子 Widget 数组
    }
end

-- 重置拖拽状态到初始值
function WBP_InventoryGrid:ResetDragState()
    self.DragState.IsDragging = false
    self.DragState.DraggedItem = nil
    self.DragState.DraggedWidget = nil
    self.DragState.OriginalPosition = nil
    self.DragState.OriginalRotated = false
    self.DragState.CurrentRotated = false
    self.DragState.CurrentGridPos = nil
    self.DragState.CanPlace = false
    -- PreviewWidget 和 HighlightWidgets 在 CleanupDragState 中处理
end

--[[
============================================================================
                              开始拖拽
============================================================================
--]]

-- 开始拖拽物品
-- @param ItemWidget: 被点击的 ItemIconWidget
function WBP_InventoryGrid:StartDrag(ItemWidget)
    -- 防止重复拖拽
    if self.DragState.IsDragging then
        print("[WBP_InventoryGrid.lua] Already dragging, ignore StartDrag")
        return false
    end

    -- 检查参数
    if not ItemWidget then
        print("[WBP_InventoryGrid] StartDrag: ItemWidget is nil")
        return false
    end

    -- 获取物品数据
    local ItemData = ItemWidget:GetItemData()
    if not ItemData then
        print("[WBP_InventoryGrid] StartDrag: ItemData is nil")
        return false
    end

    -- 记录拖拽状态
    self.DragState.IsDragging = true
    self.DragState.DraggedWidget = ItemWidget
    self.DragState.DraggedItem = ItemData
    self.DragState.OriginalPosition = self:GetWidgetGridPosition(ItemWidget)
    self.DragState.OriginalRotated = ItemWidget:IsRotated()
    self.DragState.CurrentRotated = ItemWidget:IsRotated()

    -- 设置拖拽预览（直接使用原物品）
    self:CreateDragPreview()

    -- 播放拾取音效
    self:PlaySound("PickUp")

    print(string.format("[WBP_InventoryGrid] StartDrag: %s at (%d, %d), Rotated: %s",
        tostring(ItemData.ItemDefID),
        self.DragState.OriginalPosition.X,
        self.DragState.OriginalPosition.Y,
        tostring(self.DragState.OriginalRotated)))

    return true
end

-- 获取 ItemIconWidget 在网格中的位置
function WBP_InventoryGrid:GetWidgetGridPosition(ItemWidget)
    if not ItemWidget then
        return UE.FIntPoint(0, 0)
    end

    -- 从 CanvasPanel Slot 获取位置
    local Slot = ItemWidget.Slot
    if Slot then
        local Position = Slot:GetPosition()
        local TotalSlotSize = self.SlotSize + self.SlotPadding
        local GridX = math.floor(Position.X / TotalSlotSize)
        local GridY = math.floor(Position.Y / TotalSlotSize)
        return UE.FIntPoint(GridX, GridY)
    end

    return UE.FIntPoint(0, 0)
end

--[[
============================================================================
                            创建拖拽预览
============================================================================
--]]

function WBP_InventoryGrid:CreateDragPreview()
    -- 简化方案：不创建新的预览 Widget
    -- 直接让原物品跟随鼠标（通过修改 Slot 位置）

    -- 保存原始位置，用于取消时恢复
    local OriginalSlot = self.DragState.DraggedWidget.Slot
    if OriginalSlot then
        self.DragState.OriginalSlotPosition = OriginalSlot:GetPosition()
        self.DragState.OriginalPixelSize = OriginalSlot:GetSize()
    end

    -- 设置半透明效果
    self.DragState.DraggedWidget:SetRenderOpacity(0.7)

    -- 显示物品（之前被隐藏了）
    self.DragState.DraggedWidget:SetVisibility(UE.ESlateVisibility.HitTestInvisible)

    print("[WBP_InventoryGrid] CreateDragPreview: Using original widget as preview")
end

-- 更新预览位置（跟随鼠标）
function WBP_InventoryGrid:UpdateDragPreview(MousePosition)
    if not self.DragState.IsDragging then
        return
    end

    if not self.DragState.DraggedWidget then
        return
    end

    -- 计算鼠标所在的网格位置
    local GridPos = self:ScreenToGridPosition(MousePosition)
    self.DragState.CurrentGridPos = GridPos

    -- 将网格位置转换为像素位置，让物品吸附到网格
    local SnapPos = self:GridToLocalPosition(GridPos)

    -- 更新原物品的 Slot 位置
    local Slot = self.DragState.DraggedWidget.Slot
    if Slot then
        Slot:SetPosition(SnapPos)
    end

    -- 检查是否可以放置
    local CanPlace = self:CheckCanPlace(GridPos)
    self.DragState.CanPlace = CanPlace

    -- 更新高亮显示
    self:UpdateHighlight(GridPos, CanPlace)
end

-- 获取物品预览尺寸（像素）
function WBP_InventoryGrid:GetItemPreviewSize()
    local ItemSize = self:GetCurrentItemSize()
    local TotalSlotSize = self.SlotSize + self.SlotPadding

    return UE.FVector2D(
        ItemSize.X * TotalSlotSize,
        ItemSize.Y * TotalSlotSize
    )
end

-- 获取当前拖拽物品的尺寸（考虑旋转）
function WBP_InventoryGrid:GetCurrentItemSize()
    if not self.DragState.DraggedWidget then
        return UE.FIntPoint(1, 1)
    end

    -- 从 Widget 的 Slot 获取像素尺寸，然后转换为格子数
    local Slot = self.DragState.DraggedWidget.Slot
    if Slot then
        local PixelSize = Slot:GetSize()
        local TotalSlotSize = self.SlotSize + self.SlotPadding

        -- 像素尺寸 → 格子数（四舍五入）
        local GridSizeX = math.floor((PixelSize.X + self.SlotPadding) / TotalSlotSize + 0.5)
        local GridSizeY = math.floor((PixelSize.Y + self.SlotPadding) / TotalSlotSize + 0.5)

        -- 如果当前旋转状态和原始不同，需要交换宽高
        if self.DragState.CurrentRotated ~= self.DragState.OriginalRotated then
            GridSizeX, GridSizeY = GridSizeY, GridSizeX
        end

        return UE.FIntPoint(GridSizeX, GridSizeY)
    end

    return UE.FIntPoint(1, 1)
end

--[[
============================================================================
                            坐标转换
============================================================================
--]]

-- 屏幕坐标 -> 网格坐标
function WBP_InventoryGrid:ScreenToGridPosition(ScreenPos)
    if not self.GridCanvas then
        return UE.FIntPoint(0, 0)
    end

    -- 获取 GridCanvas 的 Geometry
    local Geometry = self.GridCanvas:GetCachedGeometry()

    -- 转换为本地坐标
    local LocalPos = UE.USlateBlueprintLibrary.AbsoluteToLocal(Geometry, ScreenPos)

    -- 计算网格坐标
    local TotalSlotSize = self.SlotSize + self.SlotPadding
    local GridX = math.floor(LocalPos.X / TotalSlotSize)
    local GridY = math.floor(LocalPos.Y / TotalSlotSize)

    return UE.FIntPoint(GridX, GridY)
end

-- 网格坐标 -> 本地像素坐标
function WBP_InventoryGrid:GridToLocalPosition(GridPos)
    local TotalSlotSize = self.SlotSize + self.SlotPadding
    return UE.FVector2D(
        GridPos.X * TotalSlotSize,
        GridPos.Y * TotalSlotSize
    )
end

--[[
============================================================================
                            放置验证
============================================================================
--]]

-- 检查当前位置是否可以放置
function WBP_InventoryGrid:CheckCanPlace(GridPos)
    if not self.InventoryComponent then
        return false
    end

    if not self.DragState.DraggedItem then
        return false
    end

    -- 调用 C++ 的 CanPlaceItem（第4个参数用于忽略自己）
    local CanPlace = self.InventoryComponent:CanPlaceItem(
        self.DragState.DraggedItem.ItemDefID,
        GridPos,
        self.DragState.CurrentRotated,
        self.DragState.DraggedItem.InstanceID  -- 忽略自己
    )

    return CanPlace
end

--[[
============================================================================
                              结束拖拽
============================================================================
--]]

-- 结束拖拽（放置物品）
function WBP_InventoryGrid:EndDrag()
    if not self.DragState.IsDragging then
        return
    end

    local TargetPos = self.DragState.CurrentGridPos
    local CanPlace = self.DragState.CanPlace

    if CanPlace and TargetPos then
        -- 可以放置：调用 C++ MoveItem
        local Success = self.InventoryComponent:MoveItem(
            self.DragState.DraggedItem.InstanceID,
            TargetPos,
            self.DragState.CurrentRotated
        )

        if Success then
            print(string.format("[WBP_InventoryGrid] EndDrag: Moved to (%d, %d)", TargetPos.X, TargetPos.Y))
            self:PlaySound("PutDown")
            -- 成功移动后，清除原始位置，防止 CleanupDragState 恢复原位
            self.DragState.OriginalSlotPosition = nil
        else
            print("[WBP_InventoryGrid] EndDrag: MoveItem failed, this shouldn't happen")
            self:PlaySound("Error")
        end
    else
        -- 不能放置：回到原位
        print("[WBP_InventoryGrid] EndDrag: Cannot place, returning to original position")
        self:PlaySound("Error")
    end

    -- 清理拖拽状态
    self:CleanupDragState()
end

-- 取消拖拽（ESC 或右键）
function WBP_InventoryGrid:CancelDrag()
    if not self.DragState.IsDragging then
        return
    end

    print("[WBP_InventoryGrid] CancelDrag: Returning to original position")

    -- 清理拖拽状态（物品会回到原位，因为我们没有调用 MoveItem）
    self:CleanupDragState()
end

-- 清理拖拽状态
function WBP_InventoryGrid:CleanupDragState()
    -- 恢复原物品的状态
    if self.DragState.DraggedWidget then
        -- 恢复透明度
        self.DragState.DraggedWidget:SetRenderOpacity(1.0)
        -- 恢复可见性（可点击）
        self.DragState.DraggedWidget:SetVisibility(UE.ESlateVisibility.Visible)

        -- 如果没有成功移动，恢复原始位置
        if self.DragState.OriginalSlotPosition then
            local Slot = self.DragState.DraggedWidget.Slot
            if Slot then
                Slot:SetPosition(self.DragState.OriginalSlotPosition)
            end
        end
    end

    -- 清除高亮
    self:ClearHighlight()

    -- 重置状态
    self:ResetDragState()

    print("[WBP_InventoryGrid] CleanupDragState: Done")
end

--[[
============================================================================
                              R键旋转
============================================================================
--]]

function WBP_InventoryGrid:OnRotatePressed()
    if not self.DragState.IsDragging then
        return
    end

    -- 切换旋转状态
    self.DragState.CurrentRotated = not self.DragState.CurrentRotated

    -- 更新原物品的旋转显示
    if self.DragState.DraggedWidget then
        self.DragState.DraggedWidget:SetItemData(
            self.DragState.DraggedItem,
            self.DragState.CurrentRotated
        )

        -- 更新物品尺寸（宽高互换）
        local Slot = self.DragState.DraggedWidget.Slot
        if Slot and self.DragState.OriginalPixelSize then
            local OldSize = self.DragState.OriginalPixelSize
            -- 旋转后宽高互换
            if self.DragState.CurrentRotated ~= self.DragState.OriginalRotated then
                Slot:SetSize(UE.FVector2D(OldSize.Y, OldSize.X))
            else
                Slot:SetSize(UE.FVector2D(OldSize.X, OldSize.Y))
            end
        end
    end

    -- 重新检查是否可以放置（因为尺寸变了）
    if self.DragState.CurrentGridPos then
        local CanPlace = self:CheckCanPlace(self.DragState.CurrentGridPos)
        self.DragState.CanPlace = CanPlace
        self:UpdateHighlight(self.DragState.CurrentGridPos, CanPlace)
    end

    -- 播放旋转音效
    self:PlaySound("Rotate")

    print(string.format("[WBP_InventoryGrid] OnRotatePressed: Rotated = %s", tostring(self.DragState.CurrentRotated)))
end

--[[
============================================================================
                              高亮显示
============================================================================
--]]

-- 更新高亮显示
-- 注意：需要在蓝图 WBP_InventoryGrid 中创建一个名为 HighlightImage 的 Image 组件
-- 并将其添加到 GridCanvas 中
function WBP_InventoryGrid:UpdateHighlight(GridPos, CanPlace)
    -- 检查是否有高亮 Image（需要在蓝图中创建）
    if not self.HighlightImage then
        -- 如果没有预先创建的 HighlightImage，跳过高亮显示
        return
    end

    -- 获取物品尺寸
    local ItemSize = self:GetCurrentItemSize()
    local TotalSlotSize = self.SlotSize + self.SlotPadding

    -- 计算高亮区域的位置和尺寸
    local PosX = GridPos.X * TotalSlotSize
    local PosY = GridPos.Y * TotalSlotSize
    local Width = ItemSize.X * self.SlotSize + (ItemSize.X - 1) * self.SlotPadding
    local Height = ItemSize.Y * self.SlotSize + (ItemSize.Y - 1) * self.SlotPadding

    -- 设置位置和尺寸
    local Slot = self.HighlightImage.Slot
    if Slot then
        Slot:SetPosition(UE.FVector2D(PosX, PosY))
        Slot:SetSize(UE.FVector2D(Width, Height))
    end

    -- 设置颜色
    local Color
    if CanPlace then
        Color = UE.FLinearColor(0, 1, 0, 0.4)  -- 绿色半透明
    else
        Color = UE.FLinearColor(1, 0, 0, 0.4)  -- 红色半透明
    end
    self.HighlightImage:SetColorAndOpacity(Color)

    -- 显示高亮
    self.HighlightImage:SetVisibility(UE.ESlateVisibility.HitTestInvisible)
end

-- 清除高亮
function WBP_InventoryGrid:ClearHighlight()
    if self.HighlightImage then
        self.HighlightImage:SetVisibility(UE.ESlateVisibility.Collapsed)
    end
end

--[[
============================================================================
                              鼠标事件
============================================================================
--]]

-- 鼠标按下事件（在蓝图中绑定）
function WBP_InventoryGrid:OnMouseButtonDown(MyGeometry, MouseEvent)
    local bIsLeftButton = UE.UKismetInputLibrary.PointerEvent_IsMouseButtonDown(
        MouseEvent, UE.EKeys.LeftMouseButton
    )
    local bIsRightButton = UE.UKismetInputLibrary.PointerEvent_IsMouseButtonDown(
        MouseEvent, UE.EKeys.RightMouseButton
    )

    if bIsRightButton and self.DragState.IsDragging then
        -- 右键取消拖拽
        self:CancelDrag()
        return UE.UWidgetBlueprintLibrary.Handled()
    end

    return UE.UWidgetBlueprintLibrary.Unhandled()
end

-- 鼠标移动事件
function WBP_InventoryGrid:OnMouseMove(MyGeometry, MouseEvent)
    if self.DragState.IsDragging then
        local ScreenPos = UE.UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(MouseEvent)
        self:UpdateDragPreview(ScreenPos)
        return UE.UWidgetBlueprintLibrary.Handled()
    end

    return UE.UWidgetBlueprintLibrary.Unhandled()
end

-- 鼠标松开事件
function WBP_InventoryGrid:OnMouseButtonUp(MyGeometry, MouseEvent)
    local bIsLeftButton = UE.UKismetInputLibrary.PointerEvent_IsMouseButtonDown(
        MouseEvent, UE.EKeys.LeftMouseButton
    )

    if self.DragState.IsDragging then
        self:EndDrag()
        return UE.UWidgetBlueprintLibrary.Handled()
    end

    return UE.UWidgetBlueprintLibrary.Unhandled()
end

-- 键盘事件（需要 Widget 设置 IsFocusable = true）
function WBP_InventoryGrid:OnKeyDown(MyGeometry, InKeyEvent)
    local Key = UE.UKismetInputLibrary.GetKey(InKeyEvent)

    -- R 键旋转
    if Key == UE.EKeys.R then
        self:OnRotatePressed()
        return UE.UWidgetBlueprintLibrary.Handled()
    end

    -- ESC 取消拖拽
    if Key == UE.EKeys.Escape then
        self:CancelDrag()
        return UE.UWidgetBlueprintLibrary.Handled()
    end

    return UE.UWidgetBlueprintLibrary.Unhandled()
end

--[[
============================================================================
                              音效
============================================================================
--]]

function WBP_InventoryGrid:PlaySound(SoundName)
    -- TODO: 实现音效播放
    -- 需要在蓝图中配置 SoundMap 或使用 DataTable
    print(string.format("[WBP_InventoryGrid] PlaySound: %s", SoundName))
end

--[[
============================================================================
                              右键菜单
============================================================================
--]]

-- 显示右键菜单
-- @param ItemWidget: 被右键点击的 ItemIconWidget
function WBP_InventoryGrid:ShowContextMenu(ItemWidget)
    if not ItemWidget then
        print("[WBP_InventoryGrid] ShowContextMenu: ItemWidget is nil")
        return
    end

    -- 如果正在拖拽，不显示菜单
    if self.DragState.IsDragging then
        print("[WBP_InventoryGrid] ShowContextMenu: Currently dragging, ignore")
        return
    end

    -- 先隐藏已有的菜单
    self:HideContextMenu()

    -- 创建菜单（如果还没有）
    if not self.ContextMenuWidget then
        -- 需要在蓝图中设置 ContextMenuWidgetClass
        if not self.ContextMenuWidgetClass then
            print("[WBP_InventoryGrid] ShowContextMenu: ContextMenuWidgetClass is not set!")
            return
        end

        self.ContextMenuWidget = UE.UWidgetBlueprintLibrary.Create(
            self,
            self.ContextMenuWidgetClass,
            nil
        )

        if self.ContextMenuWidget then
            -- 添加到 Viewport 以确保显示在最上层
            self.ContextMenuWidget:AddToViewport(100)
        end
    end

    if not self.ContextMenuWidget then
        print("[WBP_InventoryGrid] ShowContextMenu: Failed to create ContextMenu")
        return
    end

    -- 获取鼠标位置作为菜单位置
    local PlayerController = UE.UGameplayStatics.GetPlayerController(self, 0)
    local MouseX, MouseY = 0, 0
    if PlayerController then
        local bSuccess
        bSuccess, MouseX, MouseY = PlayerController:GetMousePosition(MouseX, MouseY)
    end
    local ScreenPos = UE.FVector2D(MouseX, MouseY)

    -- 设置菜单位置（使用 SetPositionInViewport）
    self.ContextMenuWidget:SetPositionInViewport(ScreenPos, false)

    -- 显示菜单
    self.ContextMenuWidget:ShowAtPosition(ScreenPos, ItemWidget, self)

    print(string.format("[WBP_InventoryGrid] ShowContextMenu at (%.0f, %.0f)", MouseX, MouseY))
end

-- 隐藏右键菜单
function WBP_InventoryGrid:HideContextMenu()
    if self.ContextMenuWidget then
        self.ContextMenuWidget:Hide()
    end
end

--[[
============================================================================
                              Tooltip
============================================================================
--]]

-- 显示 Tooltip
-- @param ItemWidget: 鼠标悬停的 ItemIconWidget
function WBP_InventoryGrid:ShowTooltip(ItemWidget)
    if not ItemWidget then
        return
    end

    -- 如果正在拖拽，不显示 Tooltip
    if self.DragState.IsDragging then
        return
    end

    -- 创建 Tooltip（如果还没有）
    if not self.TooltipWidget then
        -- 需要在蓝图中设置 TooltipWidgetClass
        if not self.TooltipWidgetClass then
            print("[WBP_InventoryGrid] ShowTooltip: TooltipWidgetClass is not set!")
            return
        end

        self.TooltipWidget = UE.UWidgetBlueprintLibrary.Create(
            self,
            self.TooltipWidgetClass,
            nil
        )

        if self.TooltipWidget then
            -- 添加到 Viewport
            self.TooltipWidget:AddToViewport(99)
        end
    end

    if not self.TooltipWidget then
        return
    end

    -- 设置物品数据
    self.TooltipWidget:SetItemData(ItemWidget)

    -- 更新位置
    self:UpdateTooltipPosition()

    -- 显示 Tooltip
    self.TooltipWidget:SetVisibility(UE.ESlateVisibility.HitTestInvisible)

    self.CurrentHoveredItem = ItemWidget
end

-- 隐藏 Tooltip
function WBP_InventoryGrid:HideTooltip()
    if self.TooltipWidget then
        self.TooltipWidget:SetVisibility(UE.ESlateVisibility.Collapsed)
    end
    self.CurrentHoveredItem = nil
end

-- 更新 Tooltip 位置（跟随鼠标）
function WBP_InventoryGrid:UpdateTooltipPosition()
    if not self.TooltipWidget then
        return
    end

    -- 获取鼠标位置
    local PlayerController = UE.UGameplayStatics.GetPlayerController(self, 0)
    local MouseX, MouseY = 0, 0
    if PlayerController then
        local bSuccess
        bSuccess, MouseX, MouseY = PlayerController:GetMousePosition(MouseX, MouseY)
    end

    -- 偏移一点，避免遮挡鼠标
    local OffsetX = 15
    local OffsetY = 15

    -- 使用 SetPositionInViewport（因为 Tooltip 是添加到 Viewport 的）
    self.TooltipWidget:SetPositionInViewport(UE.FVector2D(MouseX + OffsetX, MouseY + OffsetY), false)
end

--[[
============================================================================
                           重写 RefreshInventory
============================================================================
--]]

function WBP_InventoryGrid:RefreshInventory()
    -- 先调用 C++ 父类方法
    self.Overridden.RefreshInventory(self)

    -- 动态调整背景尺寸
    self:AdjustGridSize()
end

--[[
============================================================================
                            尺寸调整
============================================================================
--]]

function WBP_InventoryGrid:AdjustGridSize()
    if not self.InventoryComponent then
        print("[WBP_InventoryGrid] AdjustGridSize: InventoryComponent is nil")
        return
    end

    local GridSize = self.InventoryComponent:GetGridSize()
    local TotalSlotSize = self.SlotSize + self.SlotPadding

    local Width = GridSize.X * TotalSlotSize
    local Height = GridSize.Y * TotalSlotSize

    -- 设置 BackgroundImage 尺寸
    if self.BackgroundImage then
        local BGSlot = self.BackgroundImage.Slot
        if BGSlot then
            BGSlot:SetSize(UE.FVector2D(Width, Height))
        end
    end

    -- 设置 GridCanvas 尺寸
    if self.GridCanvas then
        local CanvasSlot = self.GridCanvas.Slot
        if CanvasSlot then
            CanvasSlot:SetSize(UE.FVector2D(Width, Height))
        end
    end

    print(string.format("[WBP_InventoryGrid] Grid size adjusted to: %d x %d (GridSize: %d x %d)",
        Width, Height, GridSize.X, GridSize.Y))
end

return WBP_InventoryGrid
