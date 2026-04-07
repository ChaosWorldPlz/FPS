--[[
    WBP_MapSelect.lua
    地图选择 Widget 逻辑

    绑定 C++ 类: UFPSMapSelectWidget
]]

local TextManager = require("Gameplay.Core.TextManager")
local UIManager = require("Gameplay.Core.UIManager")

-- 地图 DataTable 资产路径
local MAP_TABLE_PATH = "/Game/_FPS/Data/Maps/DT_MapList.DT_MapList"

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

    if self.w_btn_Confirm then
        self.w_btn_Confirm.OnClicked:Add(self, self.ConfirmSelection)
    end
    if self.w_btn_Back then
        self.w_btn_Back.OnClicked:Add(self, self.OnBackClicked)
    end
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
        widget:SetText(text)
    end
end

function WBP_MapSelect:SetOverlayVisible(overlayName, visible)
    local widget = self[overlayName]
    if widget then
        widget:SetVisibility(visible and UE.ESlateVisibility.Visible or UE.ESlateVisibility.Collapsed)
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

local MAP_CARD_CLASS_PATH = "/Game/_FPS/System/UI/Menu/WBP_MapCard.WBP_MapCard_C"

function WBP_MapSelect:RefreshMapList()
    local mapTable = UE.UObject.Load(MAP_TABLE_PATH)
    if not mapTable then
        print("[MapSelect] 找不到地图 DataTable: " .. MAP_TABLE_PATH)
        return
    end
    self:SetMapDataTable(mapTable)

    local rowNames = self:GetMapRowNames()
    local count = rowNames:Num()
    print("[MapSelect] 地图数量: " .. count)

    if self.w_scrollbox_MapList then
        self.w_scrollbox_MapList:ClearChildren()
    end
    self.MapCards = {}

    local cardClass = UE.UClass.Load(MAP_CARD_CLASS_PATH)
    if not cardClass then
        print("[MapSelect] 找不到 WBP_MapCard 类")
        return
    end

    local PC = self:GetOwningPlayer()
    local firstName = nil
    for i = 1, count do
        local rowName = rowNames:Get(i)
        if self:IsMapEnabled(rowName) then
            local card = UE.UWidgetBlueprintLibrary.Create(PC, cardClass, PC)
            if card then
                local displayName = tostring(self:GetMapDisplayName(rowName))
                card:SetMapData(rowName, displayName, nil)
                card.OnCardClicked = function(name) self:OnMapButtonClicked(name) end
                if self.w_scrollbox_MapList then
                    self.w_scrollbox_MapList:AddChild(card)
                end
                self.MapCards[rowName] = card
                if not firstName then firstName = rowName end
            end
        end
    end

    if firstName then
        self:OnMapButtonClicked(firstName)
    end
end

function WBP_MapSelect:GetMapRow(rowName)
    if not self:IsMapEnabled(rowName) then return nil end

    local softPath = self:GetMapLevelPath(rowName)
    local levelPath = tostring(softPath.AssetPath.PackageName)

    return {
        DisplayName    = tostring(self:GetMapDisplayName(rowName)),
        Description    = tostring(self:GetMapDescription(rowName)),
        LevelPath      = levelPath,
        Difficulty     = self:GetMapDifficulty(rowName),
        DurationMinutes= self:GetMapDurationMinutes(rowName),
        MaxPlayers     = self:GetMapMaxPlayers(rowName),
    }
end

function WBP_MapSelect:OnMapButtonClicked(rowName)
    local row = self:GetMapRow(rowName)
    if row then
        -- 更新所有卡片选中态
        if self.MapCards then
            for name, card in pairs(self.MapCards) do
                card:SetSelected(name == rowName)
            end
        end
        self.SelectedMap = { RowName = rowName, Row = row }
        self:OnMapSelected(row)
    end
end

function WBP_MapSelect:OnMapSelected(mapInfo)
    if not mapInfo then return end
    local name = tostring(mapInfo.DisplayName or "")
    local desc = tostring(mapInfo.Description or "")
    self:SetText("w_text_MapName", name)
    self:SetText("w_text_MapDifficulty", TextManager:Get("MAP_DIFFICULTY") .. ": " .. self:GetDifficultyStars(mapInfo.Difficulty or 0))
    self:SetText("w_text_MapDuration", TextManager:Format("MAP_DURATION", mapInfo.DurationMinutes or 0))
    self:SetText("w_text_MapPlayers", TextManager:Format("MAP_PLAYERS", 1, mapInfo.MaxPlayers or 0))
    self:SetText("w_text_MapDescription", desc)
end

function WBP_MapSelect:OnUpdateMapPreview(cameraLocation, cameraRotation)
    print("[MapSelect] Update preview")
end

--============================================================
-- 按钮
--============================================================

function WBP_MapSelect:ConfirmSelection()
    if not self.SelectedMap then return end
    local PC = self:GetOwningPlayer()
    if PC then
        local row = self.SelectedMap.Row
        local levelPath = row.LevelPath or ""
        UIManager:CloseAll()
        PC:HostGame(levelPath, row.MaxPlayers)
    end
end

function WBP_MapSelect:OnBackClicked()
    UIManager:CloseWindow("UI/Menu/WBP_MapSelect")
end

--============================================================
-- 辅助
--============================================================

function WBP_MapSelect:GetDifficultyStars(difficulty)
    return string.rep("★", difficulty) .. string.rep("☆", 5 - difficulty)
end

return WBP_MapSelect
