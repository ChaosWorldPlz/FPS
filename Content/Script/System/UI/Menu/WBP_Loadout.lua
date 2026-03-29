--[[
    WBP_Loadout.lua
    装备界面 Widget 逻辑 (双模式)

    绑定 C++ 类: UFPSLoadoutWidget

    模式:
    1. Preparing - 出战准备
    2. RaidResult - 战局结算
]]

local TextManager = require("Gameplay.Core.TextManager")

local WBP_Loadout = UnLua.Class()

-- 模式枚举 (对应 C++ EFPSLoadoutMode)
local LoadoutMode = {
    Preparing = 0,
    RaidResult = 1
}

--============================================================
-- 控件引用
--============================================================
-- w_text_Title
-- w_text_ResultStatus (左上角，结算模式)
-- w_panel_Stats (统计信息栏，结算模式)
--   w_text_SurvivalTime
--   w_text_Kills
--   w_text_XP
-- w_overlay_Action / w_btn_Action / w_text_Action (开始战局 / 退出)
-- w_overlay_Back / w_btn_Back / w_text_Back

--============================================================
-- 生命周期
--============================================================

function WBP_Loadout:Construct()
    print("[Loadout] Construct")
    self:InitializeTexts()
end

function WBP_Loadout:Destruct()
    print("[Loadout] Destruct")
end

--============================================================
-- 文本初始化
--============================================================

function WBP_Loadout:InitializeTexts()
    self:SetText("w_text_Back", TextManager:Get("LOADOUT_BACK"))
    self:UpdateUIForCurrentMode()
end

function WBP_Loadout:SetText(widgetName, text)
    local widget = self[widgetName]
    if widget and widget.SetText then
        widget:SetText(FText(text))
    end
end

function WBP_Loadout:SetOverlayVisible(overlayName, visible)
    local widget = self[overlayName]
    if widget then
        widget:SetVisibility(visible and ESlateVisibility.Visible or ESlateVisibility.Collapsed)
    end
end

function WBP_Loadout:SetWidgetVisibility(widgetName, visible)
    local widget = self[widgetName]
    if widget then
        widget:SetVisibility(visible and ESlateVisibility.Visible or ESlateVisibility.Collapsed)
    end
end

--============================================================
-- 菜单事件
--============================================================

function WBP_Loadout:OnMenuShown()
    print("[Loadout] Menu shown - Mode: " .. self:GetModeString())
    self:InitializeTexts()
end

--============================================================
-- 模式管理
--============================================================

function WBP_Loadout:GetModeString()
    if self:IsPreparingMode() then
        return "Preparing"
    else
        return "RaidResult"
    end
end

function WBP_Loadout:OnLoadoutModeChanged(newMode)
    print("[Loadout] Mode changed to: " .. (newMode == LoadoutMode.Preparing and "Preparing" or "RaidResult"))
    self:UpdateUIForCurrentMode()
end

function WBP_Loadout:UpdateUIForCurrentMode()
    if self:IsPreparingMode() then
        self:SetupPreparingMode()
    else
        self:SetupRaidResultMode()
    end
end

function WBP_Loadout:SetupPreparingMode()
    -- 标题
    self:SetText("w_text_Title", TextManager:Get("LOADOUT_TITLE"))

    -- 隐藏结算状态
    self:SetWidgetVisibility("w_text_ResultStatus", false)
    self:SetWidgetVisibility("w_panel_Stats", false)

    -- 按钮文本
    self:SetText("w_text_Action", TextManager:Get("LOADOUT_START_RAID"))

    print("[Loadout] Setup Preparing mode")
end

function WBP_Loadout:SetupRaidResultMode()
    local result = self:GetRaidResult()

    -- 结算状态 (左上角)
    local statusText = result.bSuccess and TextManager:Get("RESULT_SUCCESS") or TextManager:Get("RESULT_FAILED")
    self:SetText("w_text_ResultStatus", statusText)
    self:SetWidgetVisibility("w_text_ResultStatus", true)

    -- 统计信息栏
    self:SetText("w_text_SurvivalTime", TextManager:Format("RESULT_SURVIVAL_TIME", self:GetSurvivalTimeString()))
    self:SetText("w_text_Kills", TextManager:Format("RESULT_KILLS", self:GetKillCount()))
    self:SetText("w_text_XP", TextManager:Format("RESULT_XP", self:GetExperienceGained()))
    self:SetWidgetVisibility("w_panel_Stats", true)

    -- 按钮文本
    self:SetText("w_text_Action", TextManager:Get("RESULT_EXIT"))

    print("[Loadout] Setup RaidResult mode")
end

--============================================================
-- 结算数据更新
--============================================================

function WBP_Loadout:OnRaidResultUpdated(result)
    print("[Loadout] Raid result updated - Success: " .. tostring(result.bSuccess))
    self:SetupRaidResultMode()
end

--============================================================
-- 按钮事件
--============================================================

function WBP_Loadout:OnClicked_Action()
    if self:IsPreparingMode() then
        print("[Loadout] Starting raid...")
        self:StartRaid()
    else
        print("[Loadout] Exiting to main menu...")
        self:ExitToMainMenu()
    end
end

function WBP_Loadout:OnClicked_Back()
    self:OnBackClicked()
end

--============================================================
-- 库存刷新
--============================================================

function WBP_Loadout:OnRefreshInventory()
    print("[Loadout] Refresh inventory")
    -- 连接到现有的背包系统
end

return WBP_Loadout
