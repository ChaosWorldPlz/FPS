--[[
    WBP_MapSelect.lua
    地图选择 Widget 逻辑

    绑定 C++ 类: UFPSMapSelectWidget
]]

local TextManager = require("Gameplay.Core.TextManager")

local WBP_MapSelect = UnLua.Class()

--============================================================
-- 控件引用
--============================================================
-- w_text_Title
-- w_image_MapPreview
-- w_text_MapName
-- w_text_MapDifficulty
-- w_text_MapDuration
-- w_text_MapPlayers
-- w_text_MapDescription
-- w_overlay_Confirm / w_btn_Confirm / w_text_Confirm
-- w_overlay_Back / w_btn_Back / w_text_Back

--============================================================
-- 生命周期
--============================================================

function WBP_MapSelect:Construct()
    print("[MapSelect] Construct")
    self:InitializeTexts()
end

function WBP_MapSelect:Destruct()
    print("[MapSelect] Destruct")
end

--============================================================
-- 文本初始化
--============================================================

function WBP_MapSelect:InitializeTexts()
    self:SetText("w_text_Title", TextManager:Get("MAP_SELECT_TITLE"))
    self:SetText("w_text_Confirm", TextManager:Get("MAP_SELECT_CONFIRM"))
    self:SetText("w_text_Back", TextManager:Get("MAP_SELECT_BACK"))
end

function WBP_MapSelect:SetText(widgetName, text)
    local widget = self[widgetName]
    if widget and widget.SetText then
        widget:SetText(FText(text))
    end
end

function WBP_MapSelect:SetOverlayVisible(overlayName, visible)
    local widget = self[overlayName]
    if widget then
        widget:SetVisibility(visible and ESlateVisibility.Visible or ESlateVisibility.Collapsed)
    end
end

--============================================================
-- 菜单事件
--============================================================

function WBP_MapSelect:OnMenuShown()
    print("[MapSelect] Menu shown")
    self:InitializeTexts()
    self:RefreshMapList()
end

--============================================================
-- 地图
--============================================================

function WBP_MapSelect:RefreshMapList()
    local maps = self:GetAvailableMaps()
    print("[MapSelect] Maps: " .. #maps)
end

function WBP_MapSelect:OnMapButtonClicked(mapId)
    self:SelectMap(mapId)
end

function WBP_MapSelect:OnMapSelected(mapInfo)
    self:SetText("w_text_MapName", mapInfo.DisplayName:ToString())
    self:SetText("w_text_MapDifficulty", TextManager:Get("MAP_DIFFICULTY") .. ": " .. self:GetDifficultyStars(mapInfo.Difficulty))
    self:SetText("w_text_MapDuration", TextManager:Format("MAP_DURATION", mapInfo.DurationMinutes))
    self:SetText("w_text_MapPlayers", TextManager:Format("MAP_PLAYERS", 1, mapInfo.MaxPlayers))
    self:SetText("w_text_MapDescription", mapInfo.Description:ToString())
end

function WBP_MapSelect:OnUpdateMapPreview(cameraLocation, cameraRotation)
    print("[MapSelect] Update preview")
end

--============================================================
-- 按钮
--============================================================

function WBP_MapSelect:OnClicked_Confirm()
    local selectedId = self:GetSelectedMapId()
    if selectedId and not selectedId:IsNone() then
        self:ConfirmSelection()
    end
end

function WBP_MapSelect:OnClicked_Back()
    self:OnBackClicked()
end

--============================================================
-- 辅助
--============================================================

function WBP_MapSelect:GetDifficultyStars(difficulty)
    return string.rep("★", difficulty) .. string.rep("☆", 5 - difficulty)
end

return WBP_MapSelect
