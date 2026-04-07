--[[
    WBP_MapCard.lua
    地图选择卡片（WBP_MapSelect 的子 Widget）

    绑定 C++ 类: UserWidget（无自定义父类）
    LuaFilePath: System/UI/Menu/WBP_MapCard

    控件:
    - w_btn_Card        Button  整张卡片可点击
    - w_image_Thumbnail Image   地图缩略图
    - w_border_Selected Border  选中高亮边框
    - w_text_CardName   Text    地图名称
]]

local WBP_MapCard = UnLua.Class()

-- 由 WBP_MapSelect 在创建后赋值
-- self.RowName   string
-- self.OnClicked function(rowName)

function WBP_MapCard:Construct()
    if self.w_btn_Card then
        self.w_btn_Card.OnClicked:Add(self, self.HandleClicked)
    end
    self:SetSelected(false)
end

function WBP_MapCard:HandleClicked()
    if self.OnCardClicked then
        self.OnCardClicked(self.RowName)
    end
end

--- 外部调用：初始化卡片数据
function WBP_MapCard:SetMapData(rowName, displayName, thumbnail)
    self.RowName = rowName
    if self.w_text_CardName then
        self.w_text_CardName:SetText(tostring(displayName or rowName))
    end
    if thumbnail and self.w_image_Thumbnail then
        self.w_image_Thumbnail:SetBrushFromTexture(thumbnail)
    end
end

--- 外部调用：设置选中状态
function WBP_MapCard:SetSelected(selected)
    if self.w_border_Selected then
        self.w_border_Selected:SetVisibility(
            selected and ESlateVisibility.HitTestInvisible
                      or ESlateVisibility.Collapsed
        )
    end
end

return WBP_MapCard
