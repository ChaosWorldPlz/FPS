-- Content/Script/UI/Inventory/WBP_WeaponModify.lua
-- 武器改装界面 Lua 逻辑
--
-- 用法：
--   从背包右键武器物品 → 选择"改装" → 此界面弹出
--   界面左侧：当前武器配件槽（Sight / Muzzle / Handguard / Magazine）
--   界面右侧：背包中所有 Attachment 类型物品（可拖入槽位）
--   底部：属性对比（基础值 vs 装配后）

local WBP_WeaponModify = UnLua.Class()

-- 配件槽类型枚举（对应 C++ EFPSAttachmentSlotType）
local EAttachmentSlotType = {
    None      = 0,
    Sight     = 1,
    Muzzle    = 2,
    Handguard = 3,
    Magazine  = 4,
}

-- 槽位显示名
local SlotNames = {
    [EAttachmentSlotType.Sight]     = "瞄具",
    [EAttachmentSlotType.Muzzle]    = "枪口",
    [EAttachmentSlotType.Handguard] = "握把/导轨",
    [EAttachmentSlotType.Magazine]  = "弹匣",
}

-- 所有可改装的槽位列表（顺序决定 UI 排布）
local AllSlots = {
    EAttachmentSlotType.Sight,
    EAttachmentSlotType.Muzzle,
    EAttachmentSlotType.Handguard,
    EAttachmentSlotType.Magazine,
}

--[[
============================================================================
                              生命周期
============================================================================
--]]

function WBP_WeaponModify:Construct()
    print("[WBP_WeaponModify] Construct")

    self.TargetWeapon        = nil  -- AFPSWeaponBase*（C++ 武器 Actor）
    self.InventoryComponent  = nil  -- UInventoryGridComponent*
    self.SelectedSlot        = EAttachmentSlotType.None
    self.DraggedAttachItem   = nil  -- 当前拖拽中的 FInventoryItem

    self:BindButtonEvents()
end

function WBP_WeaponModify:Destruct()
    print("[WBP_WeaponModify] Destruct")
    self.TargetWeapon = nil
    self.InventoryComponent = nil
end

--[[
============================================================================
                             打开界面
============================================================================
--]]

-- 打开改装界面
-- @param Weapon            AFPSWeaponBase*
-- @param InventoryComp     UInventoryGridComponent*
function WBP_WeaponModify:OpenForWeapon(Weapon, InventoryComp)
    if not Weapon then
        print("[WBP_WeaponModify] OpenForWeapon: Weapon is nil")
        return
    end

    self.TargetWeapon       = Weapon
    self.InventoryComponent = InventoryComp

    -- 刷新全部面板
    self:RefreshAttachmentSlots()
    self:RefreshInventoryPanel()
    self:RefreshStatComparison()

    self:SetVisibility(UE.ESlateVisibility.Visible)
    print(string.format("[WBP_WeaponModify] Opened for weapon: %s", tostring(Weapon:GetName())))
end

-- 关闭界面
function WBP_WeaponModify:Close()
    self:SetVisibility(UE.ESlateVisibility.Collapsed)
    self.TargetWeapon = nil
    self.InventoryComponent = nil
    print("[WBP_WeaponModify] Closed")
end

--[[
============================================================================
                         配件槽面板刷新
============================================================================
--]]

-- 刷新左侧 4 个配件槽
function WBP_WeaponModify:RefreshAttachmentSlots()
    if not self.TargetWeapon then
        return
    end

    for _, Slot in ipairs(AllSlots) do
        local SlotWidget = self:GetSlotWidget(Slot)
        if SlotWidget then
            -- 检查该武器是否支持此槽位
            local CanInstall = self.TargetWeapon:CanInstallAttachment(Slot)
            SlotWidget:SetIsEnabled(CanInstall)

            -- 获取当前已安装的配件
            local InstalledAtt = self.TargetWeapon:GetAttachment(Slot)
            if InstalledAtt then
                -- 显示配件图标
                SlotWidget:SetBrushFromTexture(InstalledAtt.Icon:LoadSynchronous())
                SlotWidget:SetToolTipText(InstalledAtt.AttachmentName)
            else
                -- 显示空槽占位图标
                SlotWidget:SetBrushFromTexture(self:GetEmptySlotIcon(Slot))
                SlotWidget:SetToolTipText(FText.FromString(SlotNames[Slot] or ""))
            end
        end
    end
end

-- 根据槽位类型返回对应的 UI Image Widget
-- 约定：蓝图中 Image 命名为 w_slot_sight / w_slot_muzzle 等
function WBP_WeaponModify:GetSlotWidget(Slot)
    if Slot == EAttachmentSlotType.Sight     then return self.w_slot_sight end
    if Slot == EAttachmentSlotType.Muzzle    then return self.w_slot_muzzle end
    if Slot == EAttachmentSlotType.Handguard then return self.w_slot_handguard end
    if Slot == EAttachmentSlotType.Magazine  then return self.w_slot_magazine end
    return nil
end

-- 返回空槽的占位纹理（蓝图中配置 EmptySlotTextures 表，否则返回 nil）
function WBP_WeaponModify:GetEmptySlotIcon(Slot)
    if self.EmptySlotTextures then
        return self.EmptySlotTextures[Slot]
    end
    return nil
end

--[[
============================================================================
                         背包配件列表刷新
============================================================================
--]]

-- 刷新右侧背包面板（只显示 Attachment 类型物品）
function WBP_WeaponModify:RefreshInventoryPanel()
    if not self.InventoryComponent then
        return
    end

    -- 清空列表
    if self.w_attachment_list then
        self.w_attachment_list:ClearChildren()
    end

    -- 获取所有背包物品
    local AllItems = self.InventoryComponent:GetAllItems()
    local GameInstance = UE.UGameplayStatics.GetGameInstance(self)
    if not GameInstance then return end

    local DataManager = GameInstance:GetSubsystem(UE.UItemDataManager)
    if not DataManager then return end

    for i = 1, AllItems:Num() do
        local Placement = AllItems:Get(i)
        local Item = Placement.Item
        local ItemDef = DataManager:GetItemDefinition(Item.ItemDefID)

        -- 过滤：只显示配件类型（EItemType::Attachment = 9）
        if ItemDef and ItemDef.ItemType == 9 then
            self:AddAttachmentEntry(Item, ItemDef)
        end
    end
end

-- 为单个配件物品创建列表条目（简单文本按钮）
function WBP_WeaponModify:AddAttachmentEntry(Item, ItemDef)
    if not self.w_attachment_list then
        return
    end

    -- 需要蓝图中预设 AttachmentEntryWidgetClass
    if not self.AttachmentEntryWidgetClass then
        print("[WBP_WeaponModify] AttachmentEntryWidgetClass not set")
        return
    end

    local EntryWidget = UE.UWidgetBlueprintLibrary.Create(
        self, self.AttachmentEntryWidgetClass, nil)

    if not EntryWidget then
        return
    end

    -- 设置条目数据（蓝图中 EntryWidget 需要暴露 SetData 方法）
    if EntryWidget.SetData then
        EntryWidget:SetData(Item, ItemDef)
    end

    -- 绑定点击：安装配件
    if EntryWidget.OnEntryClicked then
        EntryWidget.OnEntryClicked:Add(self, function(_, ClickedItem)
            self:OnAttachmentEntryClicked(ClickedItem)
        end)
    end

    self.w_attachment_list:AddChild(EntryWidget)
end

--[[
============================================================================
                           安装 / 拆卸逻辑
============================================================================
--]]

-- 点击背包配件条目 → 安装到当前选中槽位
function WBP_WeaponModify:OnAttachmentEntryClicked(Item)
    if not self.TargetWeapon or not Item then
        return
    end

    if self.SelectedSlot == EAttachmentSlotType.None then
        print("[WBP_WeaponModify] No slot selected. Click a slot first.")
        return
    end

    -- 通过 AttachmentID 找到对应的 UFPSWeaponAttachmentData
    -- 简化：直接调用 C++ InstallAttachmentByItemDefID (需要 C++ 暴露该方法)
    -- 此处演示调用 InstallAttachment（传入 AttData 对象）
    local AttData = self:FindAttachmentDataByItemDefID(Item.ItemDefID)
    if not AttData then
        print(string.format("[WBP_WeaponModify] No AttachmentData found for %s", tostring(Item.ItemDefID)))
        return
    end

    -- 检查槽位匹配
    if AttData.SlotType ~= self.SelectedSlot then
        print("[WBP_WeaponModify] Attachment slot type mismatch")
        return
    end

    -- 若该槽位已有配件，先退回背包
    local OldAttData = self.TargetWeapon:GetAttachment(self.SelectedSlot)
    if OldAttData then
        self:ReturnAttachmentToInventory(OldAttData)
    end

    -- 安装
    local Success = self.TargetWeapon:InstallAttachment(self.SelectedSlot, AttData)
    if Success then
        -- 从背包移除
        if self.InventoryComponent then
            self.InventoryComponent:RemoveItem(Item.InstanceID)
        end
        print(string.format("[WBP_WeaponModify] Installed %s into slot %d",
            tostring(AttData.AttachmentName), self.SelectedSlot))
    end

    -- 刷新界面
    self:RefreshAttachmentSlots()
    self:RefreshInventoryPanel()
    self:RefreshStatComparison()
end

-- 点击已安装配件 → 拆卸回背包
function WBP_WeaponModify:OnSlotClicked(Slot)
    if not self.TargetWeapon then
        return
    end

    local InstalledAtt = self.TargetWeapon:GetAttachment(Slot)
    if not InstalledAtt then
        -- 空槽被选中 → 记录为当前目标槽
        self.SelectedSlot = Slot
        print(string.format("[WBP_WeaponModify] Selected slot %d (%s)", Slot, SlotNames[Slot] or ""))
        return
    end

    -- 有配件：拆卸并退回背包
    local OutData = nil
    local Removed = self.TargetWeapon:RemoveAttachment(Slot, OutData)
    if Removed and OutData then
        self:ReturnAttachmentToInventory(OutData)
        print(string.format("[WBP_WeaponModify] Removed attachment from slot %d", Slot))
    end

    self:RefreshAttachmentSlots()
    self:RefreshInventoryPanel()
    self:RefreshStatComparison()
end

-- 将配件数据退回背包（按 ItemDefID 创建物品实例）
function WBP_WeaponModify:ReturnAttachmentToInventory(AttData)
    if not self.InventoryComponent or not AttData then
        return
    end

    local ReturnItem = UE.FInventoryItem()
    ReturnItem.ItemDefID = AttData.ItemDefID

    -- 尝试放入背包（位置由背包自动决定）
    local Ok = self.InventoryComponent:AddItem(ReturnItem, UE.FIntPoint(0, 0), false)
    if not Ok then
        print(string.format("[WBP_WeaponModify] ReturnAttachmentToInventory: failed to add %s",
            tostring(AttData.ItemDefID)))
    end
end

-- 通过 ItemDefID 查找对应的 UFPSWeaponAttachmentData
-- 需要 C++ 侧暴露 UItemDataManager::GetAttachmentDataByItemDefID 方法
-- 或通过 AssetManager 加载
function WBP_WeaponModify:FindAttachmentDataByItemDefID(ItemDefID)
    local GameInstance = UE.UGameplayStatics.GetGameInstance(self)
    if not GameInstance then return nil end

    local DataManager = GameInstance:GetSubsystem(UE.UItemDataManager)
    if not DataManager then return nil end

    -- 若 DataManager 暴露了该方法：
    if DataManager.GetAttachmentDataByItemDefID then
        return DataManager:GetAttachmentDataByItemDefID(ItemDefID)
    end

    return nil
end

--[[
============================================================================
                           属性对比面板
============================================================================
--]]

-- 刷新底部属性对比（基础值 vs 当前装配效果）
function WBP_WeaponModify:RefreshStatComparison()
    if not self.TargetWeapon then
        return
    end

    -- 从武器数据获取基础值
    local WeaponData = self.TargetWeapon.WeaponData
    if not WeaponData then return end

    local BaseDamage    = WeaponData.BaseDamage
    local BaseSpread    = WeaponData.BaseSpread
    local BaseReload    = WeaponData.ReloadTime
    local BaseMag       = WeaponData.MagazineSize
    local BaseRange     = WeaponData.MaxRange

    -- 从武器获取当前有效值（含配件加成）
    local EffDamage     = self.TargetWeapon:GetEffectiveDamage()
    local EffSpread     = self.TargetWeapon:GetEffectiveSpread()
    local EffReload     = self.TargetWeapon:GetEffectiveReloadTime()
    local EffMag        = self.TargetWeapon:GetEffectiveMagazineSize()
    local EffRange      = self.TargetWeapon:GetEffectiveRange()

    -- 更新 UI 文本（蓝图中创建对应的 TextBlock）
    self:UpdateStatRow(self.w_stat_damage,   BaseDamage, EffDamage,   "%.1f")
    self:UpdateStatRow(self.w_stat_spread,   BaseSpread, EffSpread,   "%.2f°")
    self:UpdateStatRow(self.w_stat_reload,   BaseReload, EffReload,   "%.2fs")
    self:UpdateStatRow(self.w_stat_magazine, BaseMag,    EffMag,      "%d 发")
    self:UpdateStatRow(self.w_stat_range,    BaseRange,  EffRange,    "%.0f")
end

-- 更新单行属性对比文字，绿色=提升，红色=削弱，白色=无变化
function WBP_WeaponModify:UpdateStatRow(Widget, BaseVal, EffVal, Fmt)
    if not Widget then return end

    local BaseText = string.format(Fmt, BaseVal)
    local EffText  = string.format(Fmt, EffVal)

    local Delta = EffVal - BaseVal
    local Color
    if math.abs(Delta) < 0.001 then
        Color = UE.FLinearColor(1, 1, 1, 1)   -- 白：无变化
    elseif Delta > 0 then
        Color = UE.FLinearColor(0.2, 1, 0.2, 1) -- 绿：提升
    else
        Color = UE.FLinearColor(1, 0.2, 0.2, 1) -- 红：削弱
    end

    if Widget.SetText then
        Widget:SetText(UE.FText.FromString(
            string.format("%s → %s", BaseText, EffText)))
    end
    if Widget.SetColorAndOpacity then
        Widget:SetColorAndOpacity(Color)
    end
end

--[[
============================================================================
                            按钮事件绑定
============================================================================
--]]

function WBP_WeaponModify:BindButtonEvents()
    -- 关闭按钮
    if self.w_btn_close then
        self.w_btn_close.OnClicked:Add(self, self.Close)
    end

    -- 槽位按钮（每个槽对应一个 Button）
    local SlotButtons = {
        [EAttachmentSlotType.Sight]     = self.w_btn_slot_sight,
        [EAttachmentSlotType.Muzzle]    = self.w_btn_slot_muzzle,
        [EAttachmentSlotType.Handguard] = self.w_btn_slot_handguard,
        [EAttachmentSlotType.Magazine]  = self.w_btn_slot_magazine,
    }

    for Slot, Btn in pairs(SlotButtons) do
        if Btn then
            -- Lua 闭包捕获 Slot 值
            local capturedSlot = Slot
            Btn.OnClicked:Add(self, function()
                self:OnSlotClicked(capturedSlot)
            end)
        end
    end
end

return WBP_WeaponModify
