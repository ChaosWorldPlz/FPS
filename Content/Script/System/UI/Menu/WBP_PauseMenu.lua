--[[
    WBP_PauseMenu.lua
    暂停菜单 Widget 逻辑

    绑定 C++ 类: UFPSPauseMenuWidget
]]

local TextManager = require("Gameplay.Core.TextManager")
local UIManager = require("Gameplay.Core.UIManager")

local WBP_PauseMenu = UnLua.Class()

--============================================================
-- 控件引用
--============================================================
-- w_text_Title
-- w_overlay_Resume / w_btn_Resume / w_text_Resume
-- w_overlay_Settings / w_btn_Settings / w_text_Settings
-- w_overlay_QuitToMenu / w_btn_QuitToMenu / w_text_QuitToMenu

--============================================================
-- 生命周期
--============================================================

function WBP_PauseMenu:Construct()
    print("[PauseMenu] Construct")
    self:InitializeTexts()
end

function WBP_PauseMenu:Destruct()
    print("[PauseMenu] Destruct")
end

--============================================================
-- 文本初始化
--============================================================

function WBP_PauseMenu:InitializeTexts()
    self:SetText("w_text_Title", TextManager:Get("PAUSE_TITLE"))
    self:SetText("w_text_Resume", TextManager:Get("PAUSE_RESUME"))
    self:SetText("w_text_Settings", TextManager:Get("PAUSE_SETTINGS"))
    self:SetText("w_text_QuitToMenu", TextManager:Get("PAUSE_QUIT_TO_MENU"))
end

function WBP_PauseMenu:SetText(widgetName, text)
    local widget = self[widgetName]
    if widget and widget.SetText then
        widget:SetText(text)
    end
end

function WBP_PauseMenu:SetOverlayVisible(overlayName, visible)
    local widget = self[overlayName]
    if widget then
        widget:SetVisibility(visible and UE.ESlateVisibility.Visible or UE.ESlateVisibility.Collapsed)
    end
end

--============================================================
-- 菜单事件
--============================================================

function WBP_PauseMenu:OnMenuShown()
    print("[PauseMenu] Menu shown")
    self:InitializeTexts()
end

function WBP_PauseMenu:OnMenuHidden()
    print("[PauseMenu] Menu hidden")
end

--============================================================
-- 按钮点击
--============================================================

function WBP_PauseMenu:OnResumeClicked()
    UIManager:CloseWindow("UI/Menu/WBP_PauseMenu")
end

function WBP_PauseMenu:OnSettingsClicked()
    UIManager:OpenWindow("UI/Menu/WBP_Settings")
end

function WBP_PauseMenu:OnQuitToMenuClicked()
    local PC = self:GetOwningPlayer()
    if PC then
        UIManager:CloseAll()
        PC:LeaveGame()
    end
end

return WBP_PauseMenu
