--[[
    WBP_AnimSettings.lua
    绑定：WBP_AnimSettings → UnLuaInterface → GetModuleName = "System.UI.AnimAgent.WBP_AnimSettings"

    职责：
    - 录入 / 保存 Meshy / Tripo API Key
    - Key 持久化到 Saved/AnimAgent/keys.json（明文；正式版本应加密）
    - 保存后立即注入到 UAnimGenClient

    UMG 期望的子控件名：
        w_editbox_meshy_key   EditableTextBox
        w_editbox_tripo_key   EditableTextBox
        w_check_mock_fallback CheckBox
        w_btn_save            Button
        w_btn_close           Button
        w_text_status         TextBlock（保存状态提示）
]]

local Core = require("Gameplay.AnimAgent.AnimAgentCore")
local json = require("Gameplay.UGC.json")

local M = UnLua.Class()

local function getSavedDir()
    return UE.UKismetSystemLibrary.GetProjectDirectory() .. "Saved/AnimAgent/"
end

local function getKeyPath()
    return getSavedDir() .. "keys.json"
end

local function ensureDir()
    pcall(function() UE.UKismetSystemLibrary.MakeDirectory(getSavedDir()) end)
end

local function loadKeys()
    local f = io.open(getKeyPath(), "r")
    if not f then return {} end
    local content = f:read("*a")
    f:close()
    return json.decode(content) or {}
end

local function saveKeys(keys)
    ensureDir()
    local f = io.open(getKeyPath(), "w")
    if f then
        f:write(json.encode(keys))
        f:close()
        return true
    end
    return false
end

--============================================================
-- 生命周期
--============================================================

function M:Construct()
    -- 绑定按钮
    if self.w_btn_save then
        self.w_btn_save.OnPressed:Add(self, self.OnClickSave)
    end
    if self.w_btn_close then
        self.w_btn_close.OnPressed:Add(self, self.OnClickClose)
    end

    -- 加载已保存的 Key 回填
    local keys = loadKeys()
    if self.w_editbox_meshy_key and keys.meshy then
        self.w_editbox_meshy_key:SetText(keys.meshy)
    end
    if self.w_editbox_tripo_key and keys.tripo then
        self.w_editbox_tripo_key:SetText(keys.tripo)
    end
    if self.w_check_mock_fallback then
        self.w_check_mock_fallback:SetIsChecked(keys.allow_mock ~= false)
    end

    -- 启动时也把 Key 推一遍到 Core（Core 已 Init 的情况下）
    if Core:IsReady() then
        if keys.meshy then Core:SetApiKey("meshy", keys.meshy) end
        if keys.tripo then Core:SetApiKey("tripo", keys.tripo) end
    end

    if self.w_text_status then
        self.w_text_status:SetText("")
    end
end

--============================================================
-- 事件
--============================================================

function M:OnClickSave()
    local meshyKey = self.w_editbox_meshy_key
        and tostring(self.w_editbox_meshy_key:GetText()) or ""
    local tripoKey = self.w_editbox_tripo_key
        and tostring(self.w_editbox_tripo_key:GetText()) or ""
    local allowMock = self.w_check_mock_fallback
        and self.w_check_mock_fallback:IsChecked() or true

    local keys = {
        meshy      = meshyKey,
        tripo      = tripoKey,
        allow_mock = allowMock,
    }

    if saveKeys(keys) then
        if Core:IsReady() then
            Core:SetApiKey("meshy", meshyKey)
            Core:SetApiKey("tripo", tripoKey)
        end
        if self.w_text_status then
            self.w_text_status:SetText("已保存")
        end
    else
        if self.w_text_status then
            self.w_text_status:SetText("保存失败")
        end
    end
end

function M:OnClickClose()
    self:RemoveFromParent()
end
