--[[
    WBP_UGCChat.lua
    绑定：WBP_UGCChat → UnLuaInterface → GetModuleName = "System.UI.UGC.WBP_UGCChat"
]]

local M = UnLua.Class()

--============================================================
-- 生命周期
--============================================================

function M:Construct()
    if self.w_btn_send then
        self.w_btn_send.OnPressed:Add(self, self.OnClickSend)
    end
    if self.w_btn_close then
        self.w_btn_close.OnPressed:Add(self, self.OnClickClose)
    end
    if self.w_editablebox_input then
        self.w_editablebox_input.OnTextCommitted:Add(self, self.OnInputCommitted)
    end
    if self.w_text_thinking then
        self.w_text_thinking:SetVisibility(UE.ESlateVisibility.Hidden)
    end

    self:AddMessage("你好！我是 AI 助手，可以帮你修改场景、属性、规则等。试试说：「在0,0,100放一个方块」", false)
end

--============================================================
-- 发送逻辑
--============================================================

function M:OnClickSend()
    if not self.w_editablebox_input then return end
    local text = tostring(self.w_editablebox_input:GetText())
    if not text or text == "" then return end

    self:AddMessage(text, true)
    self.w_editablebox_input:SetText("")

    if self.w_text_thinking then
        self.w_text_thinking:SetText("AI 正在思考...")
        self.w_text_thinking:SetVisibility(UE.ESlateVisibility.SelfHitTestInvisible)
    end

    local pc = self:GetOwningPlayer()
    if pc and pc.SendToLLM then
        pc:SendToLLM(text, function(success, msg)
            self:OnLLMResult(success, msg)
        end)
    else
        self:OnLLMResult(false, "PlayerController 未绑定 SendToLLM")
    end
end

function M:OnInputCommitted(text, commitMethod)
    if commitMethod == UE.ETextCommit.OnEnter then
        self:OnClickSend()
    end
end

function M:OnLLMResult(success, msg)
    if self.w_text_thinking then
        self.w_text_thinking:SetVisibility(UE.ESlateVisibility.Hidden)
    end
    local prefix = success and "✓ " or "✗ "
    self:AddMessage(prefix .. tostring(msg), false)
end

function M:OnClickClose()
    local UIManager = require("Gameplay.Core.UIManager")
    UIManager:CloseWindow("WBP_UGCChat")
end

--============================================================
-- 消息气泡
--============================================================

function M:AddMessage(text, isUser)
    if not self.w_scrollbox_messages then return end

    local cls = UE.UClass.Load("/Game/_UGC/UI/WBP_UGCChatMsg.WBP_UGCChatMsg_C")
    if cls then
        local pc  = self:GetOwningPlayer()
        local row = UE.UWidgetBlueprintLibrary.Create(self, cls, pc)
        if row then
            if row.Init then row:Init(text, isUser) end
            local slot = self.w_scrollbox_messages:AddChild(row)
            if slot then
                slot:SetHorizontalAlignment(
                    isUser and UE.EHorizontalAlignment.HAlign_Right
                            or UE.EHorizontalAlignment.HAlign_Left)
            end
        end
    else
        if not self._fallbackLines then self._fallbackLines = {} end
        table.insert(self._fallbackLines, (isUser and "[你] " or "[AI] ") .. text)
        if self.w_text_fallback then
            self.w_text_fallback:SetText(table.concat(self._fallbackLines, "\n"))
        end
    end

    self.w_scrollbox_messages:ScrollToEnd()
end

return M
