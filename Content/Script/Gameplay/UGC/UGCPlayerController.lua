--[[
    UGCPlayerController.lua
    绑定：BP_UGCPlayerController → UnLuaInterface → GetModuleName = "Gameplay.UGC.UGCPlayerController"

    继承自 Gameplay.PlayerController，附加 UGC 层初始化：
    - UGCFunctionRegistry：注册可供 LLM 调用的原子函数
    - LLMGateway：绑定 HttpClient 回调，处理 Claude API 响应
    - 提供 SendToLLM(message, callback) 快捷接口供 UI 调用
]]

local UIManager    = require("Gameplay.Core.UIManager")
local UGCRegistry  = require("Gameplay.UGC.UGCFunctionRegistry")
local LLMGateway   = require("Gameplay.UGC.LLMGateway")
local EditorCore   = require("Gameplay.UGC.UGCEditorCore")

local M = UnLua.Class("Gameplay.PlayerController")
local Base = require("Gameplay.PlayerController")

--============================================================
-- 生命周期
--============================================================

function M:ReceiveBeginPlay()
    -- 调用基类（UIManager:Init、GM.Init、OpenHUD）
    Base.ReceiveBeginPlay(self)

    -- 初始化 UGC 层
    UGCRegistry:Init(self)
    LLMGateway:Init(self)
    EditorCore:Init(self)

    print("[UGCPlayerController] UGC 层初始化完成")
end

-- F9 切换关卡编辑器
function M:ToggleEditor()
    EditorCore:ToggleEditMode()
end

-- F8 切换蓝图编辑器（验证用）
function M:ToggleBlueprintEditor()
    local name = "WBP_UGCBlueprintEditor"
    local widget = UIManager:ToggleWindow(name)
    -- 打开时注入 PC 引用，供 Tick 驱动连线
    if UIManager:IsOpen(name) then
        local inst = UIManager:GetWindow(name)
        if inst then
            _bpEditor = inst
            self:SetBlueprintEditor(inst)
        end
    else
        _bpEditor = nil
        self:SetBlueprintEditor(nil)
    end
end

-- 编辑模式鼠标左键点击
function M:EditorClick()
    if EditorCore:GetState() ~= "Edit" then return end

    -- 获取当前鼠标屏幕坐标
    local ok, x, y = self:GetMousePosition()
    if not ok then return end

    EditorCore:OnViewportClick(x, y)
    -- Transform 面板刷新由 EditorCore:OnSelectionChanged 回调驱动
end

-- 鼠标上一帧状态（用于检测按下/松开的边沿）
local _prevMouseDown = false
-- ESC 上一帧状态（用于检测按下边沿，避免持续触发）
local _prevEscDown = false

-- 蓝图编辑器引用（打开时由 UIManager 回调注入）
local _bpEditor = nil

function M:SetBlueprintEditor(editor)
    _bpEditor = editor
end

-- Tick：编辑模式下驱动 Ghost 跟随 / 选中 Actor 拖拽
function M:ReceiveTick(deltaTime)
    -- 蓝图编辑器：节点拖拽 + 连线更新（独立于关卡编辑器状态）
    if _bpEditor then
        local ok, x, y = self:GetMousePosition()
        if ok then
            local bridge    = EditorCore:GetBridge()
            local mouseDown = bridge and bridge:IsMouseButtonDown() or false

            -- 节点拖拽
            if _bpEditor:IsDraggingNode() then
                if mouseDown then
                    _bpEditor:OnDragTick(x, y)
                else
                    _bpEditor:EndNodeDrag()
                end
            end

            -- 连线绘制（含待连预览线，每帧更新）
            _bpEditor:UpdateWires(x, y)
        end
    end

    if EditorCore:GetState() ~= "Edit" then return end
    local ok, x, y = self:GetMousePosition()
    if not ok then return end

    if EditorCore:GetPendingPrefab() then
        -- ESC 取消放置：与鼠标按键轮询同一套机制
        local escDown = EditorCore:IsEscapeDown()
        if escDown and not _prevEscDown then
            EditorCore:CancelPrefab()
        end
        _prevEscDown = escDown

        if EditorCore:GetPendingPrefab() then
            -- 取消后 pending 已清空，跳过 ghost 更新；未取消则继续跟随鼠标
            EditorCore:UpdateGhostPosition(x, y)
        end
        -- 持续同步鼠标状态，避免退出放置模式后 _prevMouseDown 残留旧值误触 BeginDrag
        _prevMouseDown = EditorCore:IsMouseDown()
        return
    end

    _prevEscDown = false   -- 不在放置模式时重置，防止进入放置模式时误判边沿

    -- 拖拽移动已选中 Actor：追踪鼠标按下/持续/松开
    local mouseDown = EditorCore:IsMouseDown()
    if mouseDown and not _prevMouseDown then
        EditorCore:BeginDrag(x, y)
    elseif not mouseDown and _prevMouseDown then
        EditorCore:EndDrag()
    elseif mouseDown then
        EditorCore:OnDragUpdate(x, y)
    end
    _prevMouseDown = mouseDown

    -- 无鼠标按下时，根据相机距离实时刷新 Gizmo 缩放，保持屏幕视觉大小一致
    if not mouseDown and EditorCore:GetSelectedID() then
        EditorCore:UpdateGizmoScale()
    end
end

--============================================================
-- 快捷接口（供 UI Widget 直接调用）
--============================================================

--- 向 LLM 发送自然语言指令
--- @param message  string  用户输入
--- @param onResult function(success:bool, msg:string)  结果回调
function M:SendToLLM(message, onResult)
    LLMGateway:Send(message, onResult)
end

--- 获取已注册的 UGC 函数列表（调试/UI 展示用）
function M:GetUGCFunctionList()
    return UGCRegistry:ListFunctions()
end

return M
