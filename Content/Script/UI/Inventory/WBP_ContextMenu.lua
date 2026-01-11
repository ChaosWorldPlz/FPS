-- Content/Script/UI/Inventory/WBP_ContextMenu.lua
-- 背包物品右键菜单 Widget 的 Lua 逻辑

local WBP_ContextMenu = UnLua.Class()

--[[
============================================================================
                              生命周期
============================================================================
--]]

function WBP_ContextMenu:Construct()
    print("[WBP_ContextMenu] Construct")

    -- 初始化状态
    self.TargetItemWidget = nil
    self.OwningGrid = nil

    -- 绑定按钮点击事件
    self:BindButtonEvents()
end

function WBP_ContextMenu:Destruct()
    print("[WBP_ContextMenu] Destruct")
    self.TargetItemWidget = nil
    self.OwningGrid = nil
end

--[[
============================================================================
                           按钮事件绑定
============================================================================
--]]

function WBP_ContextMenu:BindButtonEvents()
    -- 使用按钮
    if self.w_btn_Use then
        self.w_btn_Use.OnClicked:Add(self, self.OnUseClicked)
    end

    -- 装备按钮
    if self.w_btn_Equip then
        self.w_btn_Equip.OnClicked:Add(self, self.OnEquipClicked)
    end

    -- 丢弃按钮
    if self.w_btn_Drop then
        self.w_btn_Drop.OnClicked:Add(self, self.OnDropClicked)
    end

    -- 拆分堆叠按钮
    if self.w_btn_Split then
        self.w_btn_Split.OnClicked:Add(self, self.OnSplitClicked)
    end

    -- 虚位以待（示例占位）
    if self.w_btn_Placeholder then
        self.w_btn_Placeholder.OnClicked:Add(self, self.OnPlaceholderClicked)
    end

    -- 取消按钮
    if self.w_btn_Cancel then
        self.w_btn_Cancel.OnClicked:Add(self, self.OnCancelClicked)
    end
end

--[[
============================================================================
                           显示/隐藏
============================================================================
--]]

-- 在指定位置显示菜单
-- @param ScreenPos: 屏幕坐标 (FVector2D)
-- @param ItemWidget: 目标 ItemIconWidget
-- @param OwningGrid: 所属的 InventoryGridWidget
function WBP_ContextMenu:ShowAtPosition(ScreenPos, ItemWidget, OwningGrid)
    if not ItemWidget then
        print("[WBP_ContextMenu] ShowAtPosition: ItemWidget is nil")
        return
    end

    self.TargetItemWidget = ItemWidget
    self.OwningGrid = OwningGrid

    -- 获取物品数据
    local ItemData = ItemWidget:GetItemData()
    if ItemData then
        print(string.format("[WBP_ContextMenu] Show menu for item: %s", tostring(ItemData.ItemDefID)))
    end

    -- 更新按钮状态（某些按钮可能需要根据物品类型禁用）
    self:UpdateButtonStates(ItemData)

    -- 位置由 InventoryGrid 通过 SetPositionInViewport 设置

    -- 显示菜单
    self:SetVisibility(UE.ESlateVisibility.Visible)

    -- 设置焦点，以便捕获键盘事件
    self:SetKeyboardFocus()
end

-- 隐藏菜单
function WBP_ContextMenu:Hide()
    self:SetVisibility(UE.ESlateVisibility.Collapsed)
    self.TargetItemWidget = nil

    print("[WBP_ContextMenu] Hide")
end

-- 根据物品类型更新按钮状态
function WBP_ContextMenu:UpdateButtonStates(ItemData)
    if not ItemData then
        return
    end

    -- 拆分堆叠：只有堆叠数 > 1 才能拆分
    if self.w_btn_Split then
        local CanSplit = ItemData.StackCount > 1
        self.w_btn_Split:SetIsEnabled(CanSplit)
    end

    -- TODO: 根据物品类型判断是否可以使用/装备
    -- 例如：武器可以装备，药品可以使用
end

--[[
============================================================================
                           按钮点击回调
============================================================================
--]]

-- 使用物品
function WBP_ContextMenu:OnUseClicked()
    print("[WBP_ContextMenu] OnUseClicked")

    if self.TargetItemWidget then
        local ItemData = self.TargetItemWidget:GetItemData()
        if ItemData then
            print(string.format("[WBP_ContextMenu] Use item: %s", tostring(ItemData.ItemDefID)))
            -- TODO: 实现使用物品逻辑
            -- 可以调用 InventoryComponent 的方法或触发事件
        end
    end

    self:Hide()
end

-- 装备物品
function WBP_ContextMenu:OnEquipClicked()
    print("[WBP_ContextMenu] OnEquipClicked")

    if self.TargetItemWidget then
        local ItemData = self.TargetItemWidget:GetItemData()
        if ItemData then
            print(string.format("[WBP_ContextMenu] Equip item: %s", tostring(ItemData.ItemDefID)))
            -- TODO: 实现装备物品逻辑
        end
    end

    self:Hide()
end

-- 丢弃物品
function WBP_ContextMenu:OnDropClicked()
    print("[WBP_ContextMenu] OnDropClicked")

    if self.TargetItemWidget and self.OwningGrid then
        local ItemData = self.TargetItemWidget:GetItemData()
        if ItemData then
            print(string.format("[WBP_ContextMenu] Drop item: %s", tostring(ItemData.ItemDefID)))

            -- 从背包移除物品
            local InventoryComponent = self.OwningGrid.InventoryComponent
            if InventoryComponent then
                local Success = InventoryComponent:RemoveItem(ItemData.InstanceID)
                if Success then
                    print("[WBP_ContextMenu] Item dropped successfully")
                    -- TODO: 可以在世界中生成掉落物品
                else
                    print("[WBP_ContextMenu] Failed to drop item")
                end
            end
        end
    end

    self:Hide()
end

-- 拆分堆叠
function WBP_ContextMenu:OnSplitClicked()
    print("[WBP_ContextMenu] OnSplitClicked")

    if self.TargetItemWidget then
        local ItemData = self.TargetItemWidget:GetItemData()
        if ItemData and ItemData.StackCount > 1 then
            print(string.format("[WBP_ContextMenu] Split stack: %s (count: %d)",
                tostring(ItemData.ItemDefID), ItemData.StackCount))
            -- TODO: 实现拆分堆叠逻辑
            -- 可以弹出一个数量选择框
        end
    end

    self:Hide()
end

-- 虚位以待（示例占位按钮）
function WBP_ContextMenu:OnPlaceholderClicked()
    print("[WBP_ContextMenu] OnPlaceholderClicked - This is a placeholder for future features")

    -- 这是一个示例占位按钮，方便后续添加新功能
    -- 例如：检视、修理、强化、分解等

    self:Hide()
end

-- 取消
function WBP_ContextMenu:OnCancelClicked()
    print("[WBP_ContextMenu] OnCancelClicked")
    self:Hide()
end

--[[
============================================================================
                           键盘事件
============================================================================
--]]

-- ESC 键关闭菜单
function WBP_ContextMenu:OnKeyDown(MyGeometry, InKeyEvent)
    local Key = UE.UKismetInputLibrary.GetKey(InKeyEvent)

    if Key == UE.EKeys.Escape then
        self:Hide()
        return UE.UWidgetBlueprintLibrary.Handled()
    end

    return UE.UWidgetBlueprintLibrary.Unhandled()
end

--[[
============================================================================
                           点击外部关闭
============================================================================
--]]

-- 当焦点丢失时关闭菜单
function WBP_ContextMenu:OnFocusLost(InFocusEvent)
    -- 延迟一帧关闭，避免点击按钮时立即关闭
    -- 使用 Delay 或在下一帧检查
    print("[WBP_ContextMenu] Focus lost")

    -- 简单方案：直接关闭
    -- 更好的方案是检查焦点是否转移到了菜单内的按钮
    -- self:Hide()
end

return WBP_ContextMenu
