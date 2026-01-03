-- Content/Script/UI/Inventory/WBP_InventoryGrid.lua
-- 背包网格 Widget 的 Lua 逻辑

local WBP_InventoryGrid = UnLua.Class()

-- ==================== 生命周期 ====================

function WBP_InventoryGrid:Construct()
    print("[Lua] WBP_InventoryGrid:Construct")
end

function WBP_InventoryGrid:Destruct()
    print("[Lua] WBP_InventoryGrid:Destruct")
end

-- ==================== 绑定背包后调用 ====================

-- 重写 RefreshInventory，在C++刷新后重新调整尺寸
function WBP_InventoryGrid:RefreshInventory()
    -- 先调用 C++ 父类方法
    self.Overridden.RefreshInventory(self)
    
    --动态调整背景尺寸
    self:AdjustGridSize()
end


-- ==================== 自定义方法 ====================

-- 动态调整背包网格尺寸
function WBP_InventoryGrid:AdjustGridSize()
    if not self.InventoryComponent then
        print("[Lua] AdjustGridSize: InventoryComponent is nil")
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

    print(string.format("[Lua] Grid size adjusted to: %d x %d (GridSize: %d x %d)",
        Width, Height, GridSize.X, GridSize.Y))
end

return WBP_InventoryGrid
