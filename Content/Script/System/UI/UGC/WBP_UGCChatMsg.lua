--[[
    WBP_UGCChatMsg.lua
    单条聊天消息气泡
    绑定：WBP_UGCChatMsg → GetModuleName = "System.UI.UGC.WBP_UGCChatMsg"

    蓝图控件：
        w_border_bg   Border   气泡背景
        w_text_msg    TextBlock 消息内容（Auto Wrap）

    由 WBP_UGCChat:AddMessage 创建并初始化。
]]

local M = UnLua.Class()

-- 玩家气泡颜色
local COLOR_USER = UE.FLinearColor(0.118, 0.227, 0.373, 1.0)  -- #1E3A5F
-- AI 气泡颜色
local COLOR_AI   = UE.FLinearColor(0.059, 0.157, 0.094, 1.0)  -- #0F2818

--- 由父 Widget 调用，设置内容和方向
--- @param text   string
--- @param isUser bool
function M:Init(text, isUser)
    if self.w_text_msg then
        self.w_text_msg:SetText(FText(tostring(text)))

        local color = isUser
            and UE.FSlateColor(UE.FLinearColor(0.886, 0.910, 0.941, 1.0))  -- #E2E8F0
            or  UE.FSlateColor(UE.FLinearColor(0.310, 0.765, 0.969, 1.0))  -- #4FC3F7
        self.w_text_msg:SetColorAndOpacity(color)

        -- 玩家消息右对齐，AI 消息左对齐
        self.w_text_msg:SetJustification(
            isUser and UE.ETextJustify.Right or UE.ETextJustify.Left)
    end

    if self.w_border_bg then
        local bgColor = isUser and COLOR_USER or COLOR_AI
        self.w_border_bg:SetBrushColor(bgColor)
    end
end

return M
