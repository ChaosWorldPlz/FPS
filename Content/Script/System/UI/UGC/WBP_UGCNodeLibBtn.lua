-- WBP_UGCNodeLibBtn.lua
-- 节点库侧边栏条目，支持拖拽到画布放置
--
-- Widget 结构：
--   边框 Border [w_border_bg]  Is Variable  Drawing Type=Box  Tint=#2A2A2A  Padding=10,6,8,6
--     └─ 文本块 TextBlock [w_text_label]  Is Variable  Size=12  Color=#DDDDDD

local M = UnLua.Class()

local NODE_CLASS_PATH = "/Game/_UGC/UI/WBP_UGCNode.WBP_UGCNode_C"
local _nodeClass      = nil
local function getNodeClass()
    if not _nodeClass then _nodeClass = UE.UClass.Load(NODE_CLASS_PATH) end
    return _nodeClass
end

local COLOR_NORMAL = UE.FLinearColor(0.16, 0.16, 0.16, 1)
local COLOR_HOVER  = UE.FLinearColor(0.22, 0.22, 0.22, 1)
local COLOR_PRESS  = UE.FLinearColor(0.10, 0.10, 0.10, 1)

local LOG_TAG = "[System.UI.UGC.WBP_UGCNodeLibBtn]"
local DEBUG_VERBOSE = false

local function Debug(msg)
    if DEBUG_VERBOSE then
        print(LOG_TAG .. "[Debug] " .. tostring(msg))
    end
end

local function ApplyColor(self, color)
    if self.w_border_bg then
        self.w_border_bg:SetBrushColor(color)
    end
end

function M:RefreshVisualState()
    if self._isPressed then
        ApplyColor(self, COLOR_PRESS)
    elseif self._isHovered then
        ApplyColor(self, COLOR_HOVER)
    else
        ApplyColor(self, COLOR_NORMAL)
    end
end

function M:Construct()
    self._isHovered = false
    self._isPressed = false
    self:RefreshVisualState()
end

function M:SetNodeType(nodeType)
    self._nodeType = nodeType
end

--============================================================
-- 鼠标事件
--============================================================

function M:OnMouseButtonDown(geometry, pointerEvent)
    if not self._nodeType then
        return UE.UWidgetBlueprintLibrary.Unhandled()
    end

    self._isPressed = true
    self:RefreshVisualState()

    return UE.UWidgetBlueprintLibrary.DetectDragIfPressed(
        pointerEvent, self, UE.EKeys.LeftMouseButton)
end

function M:OnMouseButtonUp(geometry, pointerEvent)
    self._isPressed = false

    local localPos = UE.USlateBlueprintLibrary.AbsoluteToLocal(
        geometry,
        UE.UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(pointerEvent)
    )
    self._isHovered = localPos.X >= 0 and localPos.Y >= 0
        and localPos.X <= geometry:GetLocalSize().X
        and localPos.Y <= geometry:GetLocalSize().Y

    self:RefreshVisualState()
    return UE.UWidgetBlueprintLibrary.Unhandled()
end

function M:OnMouseEnter(geometry, pointerEvent)
    self._isHovered = true
    self:RefreshVisualState()
end

function M:OnMouseLeave(pointerEvent)
    Debug("OnMouseLeave fired, w_border_bg=" .. tostring(self.w_border_bg ~= nil))
    self._isHovered = false
    self._isPressed = false
    self:RefreshVisualState()
end

-- 拖拽取消时也还原颜色（drag 捕获鼠标导致 OnMouseLeave 不触发）
function M:OnDragCancelled(geometry, pointerEvent, operation)
    self._isPressed = false
    self._isHovered = false
    self:RefreshVisualState()
end

--============================================================
-- 拖拽
--============================================================

function M:OnDragDetected(geometry, pointerEvent)
    if not self._nodeType then return end

    self._isPressed = false
    self:RefreshVisualState()

    local op = UE.UWidgetBlueprintLibrary.CreateDragDropOperation(
        UE.UDragDropOperation.StaticClass())
    if not op then return end

    op.Tag   = self._nodeType
    op.Pivot = UE.EDragPivot.MouseDown

    -- 尝试创建节点预览，任何环节为 nil 就降级用 self
    local pc  = self:GetOwningPlayer()
    local cls = pc and getNodeClass() or nil
    if pc and cls then
        local ok, preview = pcall(function()
            return UE.UWidgetBlueprintLibrary.Create(pc, cls, pc)
        end)
        if ok and preview and preview.InitNode then
            local NodeRegistry = require("System.UI.UGC.UGCNodeRegistry")
            local def    = NodeRegistry.Definitions[self._nodeType]
            local params = {}
            for _, p in ipairs(def and def.params or {}) do
                params[p.name] = p.default
            end
            local initOk = pcall(function()
                preview:InitNode({
                    id = "drag_preview", type = self._nodeType,
                    pos = { x=0, y=0 }, params = params,
                }, nil)
            end)
            if initOk then
                op.DefaultDragVisual = preview
            end
        end
    end

    if not op.DefaultDragVisual then
        op.DefaultDragVisual = self
    end

    return op
end

return M
