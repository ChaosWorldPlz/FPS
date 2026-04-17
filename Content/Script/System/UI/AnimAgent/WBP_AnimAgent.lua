--[[
    WBP_AnimAgent.lua
    绑定：WBP_AnimAgent → UnLuaInterface → GetModuleName = "System.UI.AnimAgent.WBP_AnimAgent"

    职责（Phase 1 极简版）：
    - 选择 Provider（Mock / Meshy / Tripo）
    - 输入 Prompt + Style + Polycount
    - 点 Generate → 调 AnimAgentCore:Generate
    - 进度条轮询任务状态
    - 完成后弹出 Toast + 列表刷新

    UMG 期望子控件名：
        w_combo_provider          ComboBoxString  ("mock" / "meshy" / "tripo")
        w_editbox_prompt          MultiLineEditableTextBox
        w_combo_style             ComboBoxString  ("realistic" / "cartoon" / "sculpture")
        w_spinbox_polycount       SpinBox（可选）
        w_btn_generate            Button
        w_btn_cancel              Button
        w_btn_close               Button
        w_btn_open_settings       Button
        w_progressbar             ProgressBar
        w_text_status             TextBlock
        w_listview_assets         ListView（绑定 WBP_AnimAssetItem）
]]

local Core    = require("Gameplay.AnimAgent.AnimAgentCore")
local Library = require("Gameplay.AnimAgent.AnimAssetLibrary")

local M = UnLua.Class()

local POLL_INTERVAL = 1.0   -- UI 侧轮询周期（秒），独立于 C++ 内部 PollIntervalSeconds

--============================================================
-- 内部状态
--============================================================

function M:_resetState()
    self._currentJobUuid   = nil
    self._pollTimerHandle  = nil
end

--============================================================
-- 生命周期
--============================================================

function M:Construct()
    self:_resetState()

    -- 初始化 Core（懒初始化：第一次打开面板时尝试）
    if not Core:IsReady() then
        local pc = self:GetOwningPlayer()
        if pc then
            Core:Init(pc)
        end
    end

    -- 绑定按钮
    if self.w_btn_generate then
        self.w_btn_generate.OnPressed:Add(self, self.OnClickGenerate)
    end
    if self.w_btn_cancel then
        self.w_btn_cancel.OnPressed:Add(self, self.OnClickCancel)
    end
    if self.w_btn_close then
        self.w_btn_close.OnPressed:Add(self, self.OnClickClose)
    end
    if self.w_btn_open_settings then
        self.w_btn_open_settings.OnPressed:Add(self, self.OnClickOpenSettings)
    end

    -- ComboBox 选项填充
    if self.w_combo_provider then
        self.w_combo_provider:ClearOptions()
        self.w_combo_provider:AddOption("mock")
        self.w_combo_provider:AddOption("meshy")
        self.w_combo_provider:AddOption("tripo")
        self.w_combo_provider:SetSelectedOption("mock")
    end
    if self.w_combo_style then
        self.w_combo_style:ClearOptions()
        self.w_combo_style:AddOption("realistic")
        self.w_combo_style:AddOption("cartoon")
        self.w_combo_style:AddOption("sculpture")
        self.w_combo_style:SetSelectedOption("realistic")
    end

    self:_setProgress(0)
    self:_setStatus("就绪")
    self:RefreshAssetList()
end

function M:Destruct()
    self:_stopPolling()
end

--============================================================
-- 事件
--============================================================

function M:OnClickGenerate()
    if self._currentJobUuid then
        self:_setStatus("已有任务进行中，请先取消")
        return
    end

    if not Core:IsReady() then
        self:_setStatus("AnimAgent 未初始化（PlayerController 是否挂了 UAnimGenClient？）")
        return
    end

    local provider = self.w_combo_provider
        and self.w_combo_provider:GetSelectedOption() or "mock"
    local prompt = self.w_editbox_prompt
        and tostring(self.w_editbox_prompt:GetText()) or ""
    local style = self.w_combo_style
        and self.w_combo_style:GetSelectedOption() or "realistic"
    local polycount = self.w_spinbox_polycount
        and math.floor(self.w_spinbox_polycount:GetValue() + 0.5) or 30000

    if prompt == "" then
        self:_setStatus("请先输入 Prompt")
        return
    end

    local uuid = Core:Generate(provider, prompt, {
        style     = style,
        polycount = polycount,
    })

    if uuid == "" then
        self:_setStatus("生成失败：CreateTask 返回空 uuid")
        return
    end

    self._currentJobUuid = uuid
    self:_setStatus(string.format("任务已提交 [%s] %s", provider, uuid:sub(1, 8)))
    self:_setProgress(0)
    self:_startPolling()
end

function M:OnClickCancel()
    if not self._currentJobUuid then return end
    Core:CancelJob(self._currentJobUuid)
    self:_setStatus("已取消")
    self:_resetState()
    self:_stopPolling()
end

function M:OnClickClose()
    self:RemoveFromParent()
end

function M:OnClickOpenSettings()
    local pc = self:GetOwningPlayer()
    if not pc then return end
    local cls = UE.UClass.Load("/Game/_FPS/UI/AnimAgent/WBP_AnimSettings.WBP_AnimSettings_C")
    if not cls then
        self:_setStatus("WBP_AnimSettings 资产未找到")
        return
    end
    local widget = UE.UWidgetBlueprintLibrary.Create(self, cls, pc)
    if widget then widget:AddToViewport(50) end
end

--============================================================
-- 轮询任务进度
--============================================================

function M:_startPolling()
    self:_stopPolling()
    local world = self:GetWorld()
    if not world then return end
    local mgr = UE.UKismetSystemLibrary.GetGameInstance(self)
    -- 用 SetTimer by function name 的形式触发 Lua 轮询
    self._pollTimerHandle = UE.UKismetSystemLibrary.K2_SetTimer(self, "_OnPollTick", POLL_INTERVAL, true)
end

function M:_stopPolling()
    if self._pollTimerHandle then
        pcall(function()
            UE.UKismetSystemLibrary.K2_ClearTimerHandle(self, self._pollTimerHandle)
        end)
        self._pollTimerHandle = nil
    end
end

function M:_OnPollTick()
    if not self._currentJobUuid then
        self:_stopPolling()
        return
    end
    local status = Core:GetJobStatus(self._currentJobUuid)
    if not status then return end

    self:_setProgress(status.Progress or 0)

    -- EAnimGenState: Pending=0, Running=1, Succeeded=2, Failed=3, Cancelled=4
    if status.State == 2 then          -- Succeeded
        self:_setStatus(string.format("生成完成 (%d%%)", status.Progress or 100))
        self:_resetState()
        self:_stopPolling()
        self:RefreshAssetList()
    elseif status.State == 3 then      -- Failed
        self:_setStatus("失败: " .. tostring(status.ErrorMessage or ""))
        self:_resetState()
        self:_stopPolling()
    elseif status.State == 4 then      -- Cancelled
        self:_setStatus("已取消")
        self:_resetState()
        self:_stopPolling()
    else
        self:_setStatus(string.format("生成中... %d%%", status.Progress or 0))
    end
end

--============================================================
-- 资产列表刷新（极简实现：只更新计数文字；真实 ListView 绑定待 UMG 设计）
--============================================================

function M:RefreshAssetList()
    if not self.w_listview_assets then return end
    local list = Library:GetAll()
    -- 真实绑定时：清空 ListView，按记录创建 WBP_AnimAssetItem
    -- 当前 stub：只 print
    print(string.format("[WBP_AnimAgent] 资产数量: %d", #list))
end

--============================================================
-- UI 工具
--============================================================

function M:_setStatus(msg)
    if self.w_text_status then
        self.w_text_status:SetText(tostring(msg or ""))
    end
end

function M:_setProgress(pct)
    if self.w_progressbar then
        self.w_progressbar:SetPercent(math.max(0, math.min(100, pct or 0)) / 100.0)
    end
end
