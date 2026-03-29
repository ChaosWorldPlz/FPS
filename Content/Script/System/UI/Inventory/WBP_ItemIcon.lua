-- Content/Script/UI/Inventory/WBP_ItemIcon.lua
-- 物品图标 Widget 的 Lua 逻辑

local WBP_ItemIcon = UnLua.Class()

--[[
============================================================================
                              生命周期
============================================================================
--]]

function WBP_ItemIcon:Construct()
    print("[WBP_ItemIcon] WBP_ItemIcon:Construct")
end

function WBP_ItemIcon:Destruct()
    print("[WBP_ItemIcon] WBP_ItemIcon:Destruct")
end

--[[
============================================================================
                              C++ 回调
============================================================================
--]]

-- C++ SetItemData 之后会调用这个蓝图事件
function WBP_ItemIcon:OnItemDataSet()
    print(string.format("[WBP_ItemIcon] Item set: %s, Stack: %d",
        tostring(self.ItemData.ItemDefID),
        self.ItemData.StackCount))
end

--[[
============================================================================
                              鼠标事件
============================================================================
--]]

-- 鼠标按下事件（需要在蓝图中设置 Image_Icon 的 OnMouseButtonDown）
-- 或者直接重写 Widget 的 OnMouseButtonDown
function WBP_ItemIcon:OnMouseButtonDown(MyGeometry, MouseEvent)
    -- 判断是左键还是右键
    local bIsLeftButton = UE.UKismetInputLibrary.PointerEvent_IsMouseButtonDown(
        MouseEvent, UE.EKeys.LeftMouseButton
    )
    local bIsRightButton = UE.UKismetInputLibrary.PointerEvent_IsMouseButtonDown(
        MouseEvent, UE.EKeys.RightMouseButton
    )

    -- 获取父级 InventoryGridWidget
    local GridWidget = self:FindParentInventoryGrid()

    if not GridWidget then
        print("[WBP_ItemIcon] WBP_ItemIcon: Cannot find parent InventoryGrid!")
        return UE.UWidgetBlueprintLibrary.Unhandled()
    end

    if bIsLeftButton then
        -- 左键：开始拖拽
        print("[WBP_ItemIcon] WBP_ItemIcon: Left click, start drag")
        GridWidget:StartDrag(self)
        return UE.UWidgetBlueprintLibrary.Handled()

    elseif bIsRightButton then
        -- 右键：显示右键菜单
        print("[WBP_ItemIcon] WBP_ItemIcon: Right click, show context menu")
        GridWidget:ShowContextMenu(self)
        return UE.UWidgetBlueprintLibrary.Handled()
    end

    return UE.UWidgetBlueprintLibrary.Unhandled()
end

--[[
============================================================================
                              鼠标悬停事件
============================================================================
--]]

-- 鼠标进入时显示 Tooltip
function WBP_ItemIcon:OnMouseEnter(MyGeometry, MouseEvent)
    print("[WBP_ItemIcon] OnMouseEnter")

    local GridWidget = self:FindParentInventoryGrid()
    if GridWidget then
        GridWidget:ShowTooltip(self)
    end

    return UE.UWidgetBlueprintLibrary.Handled()
end

-- 鼠标离开时隐藏 Tooltip
function WBP_ItemIcon:OnMouseLeave(MouseEvent)
    print("[WBP_ItemIcon] OnMouseLeave")

    local GridWidget = self:FindParentInventoryGrid()
    if GridWidget then
        GridWidget:HideTooltip()
    end

    return UE.UWidgetBlueprintLibrary.Handled()
end

--[[
============================================================================
                              辅助方法
============================================================================
--]]

-- 获取父级 InventoryGridWidget
function WBP_ItemIcon:FindParentInventoryGrid()
    -- 直接使用 C++ 设置的 OwningGrid 引用
    return self:GetOwningGrid()
end

return WBP_ItemIcon
