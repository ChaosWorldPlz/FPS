--[[
    WBP_Settings.lua
    设置菜单 Widget 逻辑

    绑定 C++ 类: UFPSSettingsWidget
]]

local TextManager = require("Gameplay.Core.TextManager")
local UIManager = require("Gameplay.Core.UIManager")

local WBP_Settings = UnLua.Class()

local SettingsTab = {
    Graphics = 1,
    Audio = 2,
    Controls = 3,
    KeyBindings = 4,
    About = 5
}

--============================================================
-- 控件引用
--============================================================
-- w_text_Title
-- w_overlay_TabGraphics / w_btn_TabGraphics / w_text_TabGraphics
-- w_overlay_TabAudio / w_btn_TabAudio / w_text_TabAudio
-- w_overlay_TabControls / w_btn_TabControls / w_text_TabControls
-- w_overlay_TabKeybindings / w_btn_TabKeybindings / w_text_TabKeybindings
-- w_overlay_TabAbout / w_btn_TabAbout / w_text_TabAbout
--
-- w_text_Quality, w_combobox_Quality
-- w_text_FOV, w_slider_FOV, w_text_FOVValue
-- w_text_SFXVolume, w_slider_SFXVolume
-- w_text_MusicVolume, w_slider_MusicVolume
-- w_text_Sensitivity, w_slider_Sensitivity
--
-- w_overlay_Apply / w_btn_Apply / w_text_Apply
-- w_overlay_Reset / w_btn_Reset / w_text_Reset
-- w_overlay_Back / w_btn_Back / w_text_Back

--============================================================
-- 生命周期
--============================================================

function WBP_Settings:Construct()
    print("[Settings] Construct")
    self.CurrentTab = SettingsTab.Graphics
    self:InitializeTexts()
end

function WBP_Settings:Destruct()
    print("[Settings] Destruct")
end

--============================================================
-- 文本初始化
--============================================================

function WBP_Settings:InitializeTexts()
    self:SetText("w_text_Title", TextManager:Get("SETTINGS_TITLE"))

    -- 选项卡
    self:SetText("w_text_TabGraphics", TextManager:Get("SETTINGS_TAB_GRAPHICS"))
    self:SetText("w_text_TabAudio", TextManager:Get("SETTINGS_TAB_AUDIO"))
    self:SetText("w_text_TabControls", TextManager:Get("SETTINGS_TAB_CONTROLS"))
    self:SetText("w_text_TabKeybindings", TextManager:Get("SETTINGS_TAB_KEYBINDINGS"))
    self:SetText("w_text_TabAbout", TextManager:Get("SETTINGS_TAB_ABOUT"))

    -- 画面
    self:SetText("w_text_Quality", TextManager:Get("SETTINGS_QUALITY"))
    self:SetText("w_text_FOV", TextManager:Get("SETTINGS_FOV"))

    -- 音频
    self:SetText("w_text_SFXVolume", TextManager:Get("SETTINGS_SFX_VOLUME"))
    self:SetText("w_text_MusicVolume", TextManager:Get("SETTINGS_MUSIC_VOLUME"))

    -- 操作
    self:SetText("w_text_Sensitivity", TextManager:Get("SETTINGS_SENSITIVITY"))

    -- 按钮
    self:SetText("w_text_Apply", TextManager:Get("SETTINGS_APPLY"))
    self:SetText("w_text_Reset", TextManager:Get("SETTINGS_RESET"))
    self:SetText("w_text_Back", TextManager:Get("SETTINGS_BACK"))

    -- 关于
    self:SetText("w_text_AboutTitle", TextManager:Get("SETTINGS_ABOUT_TITLE"))
    self:SetText("w_text_AboutContent", TextManager:Get("SETTINGS_ABOUT_CONTENT"))
end

function WBP_Settings:SetText(widgetName, text)
    local widget = self[widgetName]
    if widget and widget.SetText then
        widget:SetText(text)
    end
end

function WBP_Settings:SetOverlayVisible(overlayName, visible)
    local widget = self[overlayName]
    if widget then
        widget:SetVisibility(visible and ESlateVisibility.Visible or ESlateVisibility.Collapsed)
    end
end

--============================================================
-- 菜单事件
--============================================================

function WBP_Settings:OnMenuShown()
    print("[Settings] Menu shown")
    self:InitializeTexts()
    self:RefreshAllSettings()
end

--============================================================
-- 选项卡
--============================================================

function WBP_Settings:SwitchToTab(tabIndex)
    self.CurrentTab = tabIndex
    self:OnTabChanged(tabIndex)
end

function WBP_Settings:OnClicked_TabGraphics()
    self:SwitchToTab(SettingsTab.Graphics)
end

function WBP_Settings:OnClicked_TabAudio()
    self:SwitchToTab(SettingsTab.Audio)
end

function WBP_Settings:OnClicked_TabControls()
    self:SwitchToTab(SettingsTab.Controls)
end

function WBP_Settings:OnClicked_TabKeybindings()
    self:SwitchToTab(SettingsTab.KeyBindings)
end

function WBP_Settings:OnClicked_TabAbout()
    self:SwitchToTab(SettingsTab.About)
end

function WBP_Settings:OnTabChanged(tabIndex)
    print("[Settings] Tab: " .. tabIndex)
end

--============================================================
-- 设置刷新
--============================================================

function WBP_Settings:RefreshAllSettings()
    local quality = self:GetGraphicsQuality()
    local fov = self:GetFieldOfView()
    local sfx = self:GetSFXVolume()
    local music = self:GetMusicVolume()
    local sens = self:GetMouseSensitivity()
    print(string.format("[Settings] Q:%d FOV:%.0f SFX:%.0f Music:%.0f Sens:%.2f", quality, fov, sfx, music, sens))
end

--============================================================
-- 按钮
--============================================================

function WBP_Settings:OnClicked_Apply()
    self:ApplySettings()
end

function WBP_Settings:OnClicked_Reset()
    self:ResetToDefault()
    self:RefreshAllSettings()
end

function WBP_Settings:OnClicked_Back()
    UIManager:CloseWindow("UI/Menu/WBP_Settings")
end

--============================================================
-- 设置读写（TODO: 接存档系统后替换）
--============================================================

function WBP_Settings:GetGraphicsQuality() return 2 end
function WBP_Settings:GetFieldOfView()      return 90 end
function WBP_Settings:GetSFXVolume()        return 80 end
function WBP_Settings:GetMusicVolume()      return 60 end
function WBP_Settings:GetMouseSensitivity() return 0.5 end

function WBP_Settings:ApplySettings()
    print("[Settings] Applied")
    -- TODO: 接存档系统保存
end

function WBP_Settings:ResetToDefault()
    print("[Settings] Reset to default")
    -- TODO: 接存档系统重置
end

return WBP_Settings
