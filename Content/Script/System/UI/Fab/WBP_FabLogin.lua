--[[
    WBP_FabLogin.lua
    Fab 登录 / 注册面板 Lua VM（路线 B 保留的唯一 UMG 登录入口）

    登录成功后，BP 图应该：
      1) 调 Lua M:OnLoginResult(err, result)  → Lua 写状态
      2) 若 err 为 ok → BP CreateWidget(WBP_FabPanel) + AddToViewport + RemoveFromParent(self)
         （Lua 发信号 M:BP_RequestOpenFabPanel()，BP 在同名 Custom Event 里实现）

    蓝图绑定：WBP_FabLogin → GetModuleName = "System.UI.Fab.WBP_FabLogin"

    Widget 结构（在 UE 编辑器里创建 WBP_FabLogin.uasset，控件名务必一致）：
    ┌──────────────────────────────────────┐
    │ [w_tab_Login]      EditableTextBox   │  ← 切 Login tab 的按钮（CheckBox 或 Button）
    │ [w_tab_Register]   EditableTextBox   │
    │ [w_input_Account]  EditableTextBox   │
    │ [w_input_Password] EditableTextBox   │
    │ [w_input_Name]     EditableTextBox   │  ← 注册时才显示
    │ [w_btn_Submit]     Button            │
    │ [w_btn_Cancel]     Button            │
    │ [w_text_Status]    TextBlock         │
    └──────────────────────────────────────┘

    Async 调用的接线说明：
    --------
    UnLua 对 dynamic delegate 参数的支持在不同版本有差异；为稳妥起见，
    推荐在 WBP_FabLogin 的 **BP 图**里：
        1) 在 w_btn_Submit Clicked 节点里，先收集 Lua 层暴露的 pending payload
           （Lua 侧把 account/password/name 暂存到 `self.pending_*`，BP 读这些字段）
        2) 调用 bridge:Login(acc, pwd, Event OnLoginResult)
        3) Event OnLoginResult 里调 Lua 函数 M:OnLoginResult(err, result)

    其它回调（OnAuthChanged / OnGlobalError / OnAuthExpired）是 multicast，
    在 Construct 里直接 :Add(self, M.OnXxx) 即可。
]]

local FabClient = require("Gameplay.Fab.FabClient")

local M = UnLua.Class()

local LOG_TAG = "[System.UI.Fab.WBP_FabLogin]"
local function Log(s)  print(LOG_TAG .. " " .. tostring(s)) end
local function Warn(s) print(LOG_TAG .. "[Warn] " .. tostring(s)) end

local TAB_LOGIN    = "login"
local TAB_REGISTER = "register"

local function bindButton(self, name, handler)
    local w = self[name]
    if not w or not w.OnClicked then
        Warn("缺少按钮或未暴露为变量: " .. name)
        return false
    end
    w.OnClicked:Add(self, handler)
    return true
end

--============================================================
-- 生命周期
--============================================================

function M:Construct()
    self.current_tab = TAB_LOGIN

    -- 缓存用户输入（Async Submit 时 BP 读这些字段）
    self.pending_account  = ""
    self.pending_password = ""
    self.pending_name     = ""

    bindButton(self, "w_tab_Login",    M.OnClickTabLogin)
    bindButton(self, "w_tab_Register", M.OnClickTabRegister)
    bindButton(self, "w_btn_Submit",   M.OnClickSubmit)
    bindButton(self, "w_btn_Cancel",   M.OnClickCancel)

    -- 输入框同步到 pending_*
    if self.w_input_Account and self.w_input_Account.OnTextChanged then
        self.w_input_Account.OnTextChanged:Add(self, M.OnTextAccountChanged)
    end
    if self.w_input_Password and self.w_input_Password.OnTextChanged then
        self.w_input_Password.OnTextChanged:Add(self, M.OnTextPasswordChanged)
    end
    if self.w_input_Name and self.w_input_Name.OnTextChanged then
        self.w_input_Name.OnTextChanged:Add(self, M.OnTextNameChanged)
    end

    -- 订阅 Bridge 的 multicast 事件
    local bridge = FabClient:GetBridge()
    if bridge then
        bridge.OnAuthChanged:Add(self, M.OnAuthChanged)
        bridge.OnGlobalError:Add(self, M.OnGlobalError)
    else
        Warn("FabClient 未初始化或 Bridge 未挂载在 PC 上")
    end

    self:ApplyTab(TAB_LOGIN)
    self:SetStatus("")
    Log("构建完成")
end

function M:Destruct()
    local bridge = FabClient:GetBridge()
    if bridge then
        pcall(function() bridge.OnAuthChanged:Remove(self, M.OnAuthChanged) end)
        pcall(function() bridge.OnGlobalError:Remove(self, M.OnGlobalError) end)
    end
end

--============================================================
-- Tab 切换
--============================================================

function M:ApplyTab(tab)
    self.current_tab = tab
    local isRegister = (tab == TAB_REGISTER)
    if self.w_input_Name then
        self.w_input_Name:SetVisibility(
            isRegister and UE.ESlateVisibility.Visible or UE.ESlateVisibility.Collapsed)
    end
    if self.w_text_Status then
        self.w_text_Status:SetText(FText(""))
    end
end

function M:OnClickTabLogin()    self:ApplyTab(TAB_LOGIN) end
function M:OnClickTabRegister() self:ApplyTab(TAB_REGISTER) end

--============================================================
-- 输入捕获
--============================================================

function M:OnTextAccountChanged(text)  self.pending_account  = text:ToString() end
function M:OnTextPasswordChanged(text) self.pending_password = text:ToString() end
function M:OnTextNameChanged(text)     self.pending_name     = text:ToString() end

--============================================================
-- 提交 / 取消
--============================================================

--- BP 读 self.current_tab / self.pending_* 决定调 Bridge:Login 还是 Bridge:Register，
--- 回调最终进到 M:OnLoginResult / M:OnRegisterResult。
function M:OnClickSubmit()
    if self.pending_account == "" or self.pending_password == "" then
        self:SetStatus("账号和密码不能为空")
        return
    end
    if self.current_tab == TAB_REGISTER and #self.pending_password < 8 then
        self:SetStatus("密码长度至少 8 位")
        return
    end

    self:SetStatus(self.current_tab == TAB_LOGIN and "登录中..." or "注册中...")
    self:SetSubmitEnabled(false)

    -- BP 图节点：Event OnSubmit → Bridge:Login / Bridge:Register → Event OnResult → M:OnLoginResult
    -- 此处只是准备就绪信号，实际 HTTP 调用交给 BP
end

function M:OnClickCancel()
    self.pending_account  = ""
    self.pending_password = ""
    self.pending_name     = ""
    if self.w_input_Account  then self.w_input_Account:SetText(FText("")) end
    if self.w_input_Password then self.w_input_Password:SetText(FText("")) end
    if self.w_input_Name     then self.w_input_Name:SetText(FText("")) end
    self:SetStatus("")
    self:SetSubmitEnabled(true)
    -- BP 图里可以在这里 RemoveFromParent 或 Close Panel
end

--============================================================
-- 回调（由 BP 图在 Bridge:Login/Register 完成事件里调用）
--============================================================

function M:OnLoginResult(err, result)
    self:SetSubmitEnabled(true)
    if err and err.BizCode and err.BizCode ~= 0 then
        self:SetStatus(string.format("登录失败(%d): %s", err.BizCode, err.Message or ""))
        return
    end
    self:SetStatus("登录成功，正在打开 Fab 面板…")
    -- BP 接这个信号：CreateWidget(WBP_FabPanel) + AddToViewport + RemoveFromParent(self)
    self:BP_RequestOpenFabPanel()
end

function M:OnRegisterResult(err, result)
    self:SetSubmitEnabled(true)
    if err and err.BizCode and err.BizCode ~= 0 then
        self:SetStatus(string.format("注册失败(%d): %s", err.BizCode, err.Message or ""))
        return
    end
    self:SetStatus("注册成功，已自动登录，正在打开 Fab 面板…")
    self:BP_RequestOpenFabPanel()
end

--- BP 占位：Lua 发信号；BP 图里用同名 Custom Event 承接
function M:BP_RequestOpenFabPanel() end

--============================================================
-- Bridge 事件
--============================================================

function M:OnAuthChanged(user)
    if user and user.Id and user.Id > 0 then
        Log(string.format("用户登录: id=%d account=%s", user.Id, user.UserAccount))
    else
        Log("用户已登出")
    end
end

function M:OnGlobalError(err)
    if not err then return end
    self:SetStatus(string.format("错误(%d): %s", err.BizCode or -1, err.Message or ""))
end

--============================================================
-- 内部辅助
--============================================================

function M:SetStatus(msg)
    if self.w_text_Status then
        self.w_text_Status:SetText(FText(msg or ""))
    end
end

function M:SetSubmitEnabled(enabled)
    if self.w_btn_Submit then
        self.w_btn_Submit:SetIsEnabled(enabled)
    end
end

return M
