--[[
    WBP_UGCChat.lua
    绑定：WBP_UGCChat → UnLuaInterface → GetModuleName = "System.UI.UGC.WBP_UGCChat"

    UI 结构（蓝图里需要以下控件）：
        w_scrollbox_messages  ScrollBox   消息历史列表
        w_editablebox_input   EditableText 输入框（Fill）
        w_btn_send            Button       发送按钮
        w_btn_close           Button       关闭按钮
        w_text_thinking       TextBlock    "AI 正在思考..." 默认 Collapsed

    调用链：
        w_btn_send.OnPressed → OnClickSend → PC:SendToLLM → OnLLMResult
]]

local M = UnLua.Class()

--============================================================
-- 生命周期
--============================================================

function M:Construct()
    -- 绑定按钮
    if self.w_btn_send then
        self.w_btn_send.OnPressed:Add(self, self.OnClickSend)
    end
    if self.w_btn_close then
        self.w_btn_close.OnPressed:Add(self, self.OnClickClose)
    end
    if self.w_editablebox_input then
        -- 回车发送
        self.w_editablebox_input.OnTextCommitted:Add(self, self.OnInputCommitted)
    end

    -- 默认隐藏 thinking 指示
    if self.w_text_thinking then
        self.w_text_thinking:SetVisibility(UE.ESlateVisibility.Collapsed)
    end

    -- 欢迎语
    self:AddMessage("你好！我是 AI 助手，可以帮你修改场景规则、属性、生成物品等。试试说：「把玩家血量设为 50」", false)
end

--============================================================
-- 发送逻辑
--============================================================

function M:OnClickSend()
    if not self.w_editablebox_input then return end
    local text = tostring(self.w_editablebox_input:GetText())
    if not text or text == "" then return end

    -- 显示用户消息
    self:AddMessage(text, true)
    -- 清空输入框
    self.w_editablebox_input:SetText(FText(""))

    -- 显示 thinking
    if self.w_text_thinking then
        self.w_text_thinking:SetVisibility(UE.ESlateVisibility.SelfHitTestInvisible)
    end

    -- 发送给 LLM
    local pc = self:GetOwningPlayer()
    if pc and pc.SendToLLM then
        pc:SendToLLM(text, function(success, msg)
            self:OnLLMResult(success, msg)
        end)
    else
        self:OnLLMResult(false, "PlayerController 未绑定 SendToLLM")
    end
end

--- 回车键触发发送（commitMethod == Confirmed）
function M:OnInputCommitted(text, commitMethod)
    if commitMethod == UE.ETextCommit.OnEnter then
        self:OnClickSend()
    end
end

function M:OnLLMResult(success, msg)
    -- 隐藏 thinking
    if self.w_text_thinking then
        self.w_text_thinking:SetVisibility(UE.ESlateVisibility.Collapsed)
    end

    local prefix = success and "✓ " or "✗ "
    self:AddMessage(prefix .. tostring(msg), false)
end

function M:OnClickClose()
    local UIManager = require("Gameplay.Core.UIManager")
    UIManager:CloseWindow("WBP_UGCChat")
end

--============================================================
-- 消息气泡（动态追加 TextBlock）
--============================================================

--- @param text    string  消息内容
--- @param isUser  bool    true=玩家（右对齐白色），false=AI（左对齐青色）
function M:AddMessage(text, isUser)
    if not self.w_scrollbox_messages then return end

    -- 用 RichTextBlock 或普通 TextBlock 均可；这里用普通 TextBlock 最简单
    local cls = UE.UClass.Load("/Game/_UGC/UI/WBP_UGCChatMsg.WBP_UGCChatMsg_C")
    if cls then
        -- 如果有消息气泡子 Widget，优先用
        local pc  = self:GetOwningPlayer()
        local row = UE.UWidgetBlueprintLibrary.Create(self, cls, pc)
        if row then
            if row.Init then row:Init(text, isUser) end
            local slot = self.w_scrollbox_messages:AddChild(row)
            if slot then
                -- 玩家消息靠右，AI 消息靠左
                slot:SetHorizontalAlignment(
                    isUser and UE.EHorizontalAlignment.HAlign_Right
                            or UE.EHorizontalAlignment.HAlign_Left)
            end
        end
    else
        -- 没有气泡 Widget，直接用内联 TextBlock fallback
        self:_addTextBlock(text, isUser)
    end

    -- 滚动到底部
    self:_scrollToBottom()
end

--- fallback：直接往 ScrollBox 追加一个 TextBlock
function M:_addTextBlock(text, isUser)
    -- 无法在 UnLua 里动态 new UTextBlock，改用 SizeBox 包裹的分割线方案：
    -- 直接把所有消息拼到同一个 TextBlock 里（最简降级方案）
    if not self._fallbackText then
        -- 第一次：找 ScrollBox 里是否已有一个叫 w_text_fallback 的控件
        self._fallbackLines = {}
    end
    local prefix = isUser and "[你] " or "[AI] "
    table.insert(self._fallbackLines, prefix .. text)

    -- 如果有 w_text_fallback，直接写入
    if self.w_text_fallback then
        self.w_text_fallback:SetText(FText(table.concat(self._fallbackLines, "\n")))
    end
end

function M:_scrollToBottom()
    if self.w_scrollbox_messages then
        self.w_scrollbox_messages:ScrollToEnd()
    end
end

return M
