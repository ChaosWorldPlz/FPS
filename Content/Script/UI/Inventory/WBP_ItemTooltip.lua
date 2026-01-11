-- Content/Script/UI/Inventory/WBP_ItemTooltip.lua
-- 物品 Tooltip Widget 的 Lua 逻辑

local WBP_ItemTooltip = UnLua.Class()

--[[
============================================================================
                              生命周期
============================================================================
--]]

function WBP_ItemTooltip:Construct()
    print("[WBP_ItemTooltip] Construct")

    -- 初始状态为隐藏
    self:SetVisibility(UE.ESlateVisibility.Collapsed)
end

function WBP_ItemTooltip:Destruct()
    print("[WBP_ItemTooltip] Destruct")
end

--[[
============================================================================
                           设置物品数据
============================================================================
--]]

-- 设置要显示的物品数据
-- @param ItemWidget: ItemIconWidget
function WBP_ItemTooltip:SetItemData(ItemWidget)
    if not ItemWidget then
        print("[WBP_ItemTooltip] SetItemData: ItemWidget is nil")
        return
    end

    -- 获取物品实例数据
    local ItemData = ItemWidget:GetItemData()
    if not ItemData then
        print("[WBP_ItemTooltip] SetItemData: ItemData is nil")
        return
    end

    -- 获取物品定义（从 ItemDataManager）
    local ItemDef = self:GetItemDefinition(ItemData.ItemDefID)

    -- 更新 UI 显示
    self:UpdateDisplay(ItemData, ItemDef)
end

-- 获取物品定义
function WBP_ItemTooltip:GetItemDefinition(ItemDefID)
    -- 获取 ItemDataManager
    local GameInstance = UE.UGameplayStatics.GetGameInstance(self)
    if not GameInstance then
        return nil
    end

    local DataManager = GameInstance:GetSubsystem(UE.UItemDataManager)
    if not DataManager then
        print("[WBP_ItemTooltip] GetItemDefinition: ItemDataManager not found")
        return nil
    end

    return DataManager:GetItemDefinition(ItemDefID)
end

-- 更新显示内容
function WBP_ItemTooltip:UpdateDisplay(ItemData, ItemDef)
    -- 物品名称
    if self.w_text_ItemName then
        local ItemName = "Unknown Item"
        if ItemDef and ItemDef.ItemName then
            -- FText 转换为 string
            ItemName = tostring(ItemDef.ItemName)
        elseif ItemData.ItemDefID then
            ItemName = tostring(ItemData.ItemDefID)
        end
        self.w_text_ItemName:SetText(ItemName)
    end

    -- 物品类型
    if self.w_text_ItemType then
        local TypeName = "Unknown"
        if ItemDef and ItemDef.ItemType then
            TypeName = self:GetItemTypeName(ItemDef.ItemType)
        end
        self.w_text_ItemType:SetText(TypeName)
    end

    -- 物品描述
    if self.w_text_Description then
        local Description = ""
        if ItemDef and ItemDef.ItemDescription then
            Description = tostring(ItemDef.ItemDescription)
        end
        self.w_text_Description:SetText(Description)
    end

    -- 重量
    if self.w_text_TextWeight then
        local Weight = 0
        if ItemDef and ItemDef.BaseWeight then
            Weight = ItemDef.BaseWeight * (ItemData.StackCount or 1)
        end
        self.w_text_TextWeight:SetText(string.format("%.2f kg", Weight))
    end

    -- 价格
    if self.w_text_TextPrice then
        local Price = 0
        if ItemDef and ItemDef.SellPrice then
            Price = ItemDef.SellPrice
        end
        self.w_text_TextPrice:SetText(string.format("%d", Price))
    end

    -- 堆叠信息
    if self.w_text_StackInfo then
        local StackCount = ItemData.StackCount or 1
        local MaxStack = 1
        if ItemDef and ItemDef.MaxStackSize then
            MaxStack = ItemDef.MaxStackSize
        end

        if MaxStack > 1 then
            self.w_text_StackInfo:SetText(string.format("%d / %d", StackCount, MaxStack))
            self.w_text_StackInfo:SetVisibility(UE.ESlateVisibility.Visible)
        else
            self.w_text_StackInfo:SetVisibility(UE.ESlateVisibility.Collapsed)
        end
    end

    -- 耐久度
    if self.w_text_Durability then
        local Durability = ItemData.Durability or 100
        if Durability < 100 then
            self.w_text_Durability:SetText(string.format("%.0f%%", Durability))
            self.w_text_Durability:SetVisibility(UE.ESlateVisibility.Visible)
        else
            self.w_text_Durability:SetVisibility(UE.ESlateVisibility.Collapsed)
        end
    end

    print(string.format("[WBP_ItemTooltip] Display updated for: %s", tostring(ItemData.ItemDefID)))
end

-- 获取物品类型名称
function WBP_ItemTooltip:GetItemTypeName(ItemType)
    -- EItemType 枚举转换为可读名称
    local TypeNames = {
        [0] = "其他",
        [1] = "武器",
        [2] = "防具",
        [3] = "弹药",
        [4] = "医疗",
        [5] = "食物",
        [6] = "容器",
        [7] = "任务物品",
        [8] = "材料",
    }

    local TypeValue = 0
    if type(ItemType) == "number" then
        TypeValue = ItemType
    elseif type(ItemType) == "userdata" then
        -- UE 枚举可能是 userdata
        TypeValue = tonumber(tostring(ItemType)) or 0
    end

    return TypeNames[TypeValue] or "未知"
end

--[[
============================================================================
                           位置更新
============================================================================
--]]

-- 更新位置（由 InventoryGrid 调用）
function WBP_ItemTooltip:UpdatePosition(ScreenPos)
    local Slot = self.Slot
    if Slot then
        Slot:SetPosition(ScreenPos)
    end
end

return WBP_ItemTooltip
