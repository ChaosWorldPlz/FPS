--[[
    WBP_MainMenu.lua
    主菜单 Widget 逻辑

    绑定 C++ 类: UFPSMainMenuWidget

    控件命名规范: w_控件类型_变量名(PascalCase)
    - w_text_Title
    - w_btn_NewGame (btn 缩写)
    - w_overlay_NewGame (包裹 btn，可见性操作针对此)
]]

local TextManager = require("Gameplay.Core.TextManager")
local UIManager = require("Gameplay.Core.UIManager")

local WBP_MainMenu = UnLua.Class()

--============================================================
-- 控件引用
--============================================================
-- w_text_Title
-- w_overlay_NewGame / w_btn_NewGame / w_text_NewGame
-- w_overlay_Continue / w_btn_Continue / w_text_Continue
-- w_overlay_Settings / w_btn_Settings / w_text_Settings
-- w_overlay_Quit / w_btn_Quit / w_text_Quit
-- w_text_Version

--============================================================
-- 生命周期
--============================================================

function WBP_MainMenu:Construct()
    print("[MainMenu] Construct")
    self:InitializeTexts()
end

function WBP_MainMenu:Destruct()
    print("[MainMenu] Destruct")
end

--============================================================
-- 文本初始化
--============================================================

function WBP_MainMenu:InitializeTexts()
    self:SetText("w_text_Title", TextManager:Get("MENU_TITLE"))
    self:SetText("w_text_NewGame", TextManager:Get("MENU_NEW_GAME"))
    self:SetText("w_text_Continue", TextManager:Get("MENU_CONTINUE"))
    self:SetText("w_text_Settings", TextManager:Get("MENU_SETTINGS"))
    self:SetText("w_text_Quit", TextManager:Get("MENU_QUIT"))
    self:SetText("w_text_Version", "v0.1.0")
end

function WBP_MainMenu:SetText(widgetName, text)
    local widget = self[widgetName]
    if widget and widget.SetText then
        widget:SetText(text)
    end
end

function WBP_MainMenu:SetOverlayVisible(overlayName, visible)
    local widget = self[overlayName]
    if widget then
        widget:SetVisibility(visible and UE.ESlateVisibility.Visible or UE.ESlateVisibility.Collapsed)
    end
end

--============================================================
-- 菜单事件
--============================================================

function WBP_MainMenu:OnMenuShown()
    print("[MainMenu] Menu shown")
    self:InitializeTexts()
end

function WBP_MainMenu:OnMenuHidden()
    print("[MainMenu] Menu hidden")
end

--============================================================
-- 按钮点击 (绑定到 w_btn_xxx 的 OnClicked)
--============================================================

function WBP_MainMenu:OnNewGameClicked()
    UIManager:OpenWindow("UI/Menu/WBP_MapSelect")
end

function WBP_MainMenu:OnContinueClicked()
    -- TODO: 读取存档后进入游戏
end

function WBP_MainMenu:OnSettingsClicked()
    UIManager:OpenWindow("UI/Menu/WBP_Settings")
end

function WBP_MainMenu:OnQuitClicked()
    self:ConfirmQuit()
end

--============================================================
-- 退出确认
--============================================================

function WBP_MainMenu:OnQuitRequested()
    -- 显示确认弹窗
end

function WBP_MainMenu:ConfirmQuit()
    UE.UKismetSystemLibrary.QuitGame(self, nil, EQuitPreference.Quit, false)
end

return WBP_MainMenu
