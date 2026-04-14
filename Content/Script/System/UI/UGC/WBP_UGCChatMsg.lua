--[[
    WBP_UGCChatMsg.lua
    单条聊天消息气泡
    绑定：WBP_UGCChatMsg → GetModuleName = "System.UI.UGC.WBP_UGCChatMsg"

    蓝图控件：
        w_btn_bg   Border   气泡背景
        w_text_msg    TextBlock 消息内容（Auto Wrap）

    由 WBP_UGCChat:AddMessage 创建并初始化。
]]

local M = UnLua.Class()

-- 玩家气泡颜色
local COLOR_USER = UE.FLinearColor(0.118, 0.227, 0.373, 1.0)  -- #1E3A5F
-- AI 气泡颜色
local COLOR_AI   = UE.FLinearColor(0.059, 0.157, 0.094, 1.0)  -- #0F2818

function M:Construct()
    if self.w_btn_copy then
        self.w_btn_copy.OnPressed:Add(self, self.OnClickCopy)
    end
end

function M:OnClickCopy()
    local text = self._msgText or ""
    local pc = self:GetOwningPlayer()
    if pc and pc.CopyToClipboard then
        pc:CopyToClipboard(text)
        print("[ChatMsg] 已复制: " .. text)
    end
end

--- 由父 Widget 调用，设置内容和方向
--- @param text   string
--- @param isUser bool
function M:Init(text, isUser)
    self._msgText = tostring(text)  -- 实例变量，每条消息独立
    if self.w_text_msg then
        self.w_text_msg:SetText(tostring(text))

        local color = isUser
            and UE.FSlateColor(UE.FLinearColor(0.886, 0.910, 0.941, 1.0))  -- #E2E8F0
            or  UE.FSlateColor(UE.FLinearColor(0.310, 0.765, 0.969, 1.0))  -- #4FC3F7

        -- TextBlock 用 SetColorAndOpacity，MultiLineEditableText 用 SetForegroundColor
        pcall(function() self.w_text_msg:SetColorAndOpacity(color) end)
        pcall(function() self.w_text_msg:SetForegroundColor(color) end)

        pcall(function()
            self.w_text_msg:SetJustification(
                isUser and UE.ETextJustify.Right or UE.ETextJustify.Left)
        end)
    end

    if self.w_btn_bg then
        local bgColor = isUser and COLOR_USER or COLOR_AI
        -- Button 用 SetBackgroundColor，Border 用 SetBrushColor，pcall 兼容两者
        pcall(function() self.w_btn_bg:SetBackgroundColor(bgColor) end)
        pcall(function() self.w_btn_bg:SetBrushColor(bgColor) end)
    end

    -- w_btn_copy 的显示/隐藏由蓝图 OnHovered/OnUnhovered 控制，Lua 不干预
end

return M
