--[[
    WBP_UGCBlueprintEditor.lua
    游戏内蓝图节点图编辑器（支持关卡图 + per-Actor 图）

    蓝图 Widget 结构（WBP_UGCBlueprintEditor）：
    ┌────────────────────────────────────────────────────────┐
    │  垂直框 VerticalBox (Fill)                              │
    │  ├─ 水平框 HorizontalBox (Fill)                         │
    │  │    ├─ 尺寸框 SizeBox (w=200)                         │
    │  │    │    └─ 边框 Border → 滚动框 [w_scroll_nodeLib]   │
    │  │    └─ 覆层 Overlay (Fill)                            │
    │  │         ├─ 画布面板 [w_canvas_main]                  │
    │  │         └─ UUGCWireOverlay [w_wire_overlay]          │
    │  │              可见性 = 命中测试不可见 HitTestInvisible │
    │  └─ 边框 Border (h=auto) → 水平框       底栏            │
    │       w_btn_compile / w_btn_save / w_btn_load           │
    │       w_btn_clear   / w_btn_close / w_text_status       │
    └────────────────────────────────────────────────────────┘
]]

local NodeRegistry = require("System.UI.UGC.UGCNodeRegistry")

local M = UnLua.Class()

--============================================================
-- 内部状态
--============================================================

-- 多图支持：{[programID] = {nodes, connections, nextID}}
-- programID = "level_main"        → 关卡级全局图（F8 打开）
-- programID = "actor_prog_{id}"   → 单个 Actor 的图
local _graphs   = {}
local _activeID = "level_main"

local function getG()
    local g = _graphs[_activeID]
    if not g then
        _graphs[_activeID] = { nodes = {}, connections = {}, nextID = 1 }
        g = _graphs[_activeID]
    end
    return g
end

-- 以下状态属于编辑器实例，不随图表切换
local _panOffset        = { x = 0, y = 0 }
local _dragNodeID       = nil
local _dragStartMouse   = nil
local _dragStartNodePos = nil
local _pendingPin       = nil   -- {nodeID, pinName, isOutput}
local _isDirtyWires     = false

local NODE_CLASS_PATH    = "/Game/_UGC/UI/WBP_UGCNode.WBP_UGCNode_C"
local NODE_LIB_BTN_PATH  = "/Game/_UGC/UI/WBP_UGCNodeLibBtn.WBP_UGCNodeLibBtn_C"
local _nodeClass         = nil
local _nodeLibBtnClass   = nil

local LOG_TAG       = "[System.UI.UGC.WBP_UGCBlueprintEditor]"
local DEBUG_VERBOSE = false

local function Log(msg)   print(LOG_TAG .. " " .. tostring(msg)) end
local function Warn(msg)  print(LOG_TAG .. "[Warn] " .. tostring(msg)) end
local function Debug(msg) if DEBUG_VERBOSE then print(LOG_TAG .. "[Debug] " .. tostring(msg)) end end

local function getNodeClass()
    if not _nodeClass then _nodeClass = UE.UClass.Load(NODE_CLASS_PATH) end
    return _nodeClass
end
local function getNodeLibBtnClass()
    if not _nodeLibBtnClass then _nodeLibBtnClass = UE.UClass.Load(NODE_LIB_BTN_PATH) end
    return _nodeLibBtnClass
end

-- Drop 时 FDragDropEvent 在 UnLua 未注册 GetScreenSpacePosition，多级降级
local function resolveDropScreenPos(self, dragEvent)
    local ok, pos = pcall(function()
        return UE.UKismetInputLibrary.PointerEvent_GetScreenSpacePosition(dragEvent)
    end)
    if ok and pos then return pos.X, pos.Y, "drag_event" end

    local pc = self:GetOwningPlayer()
    if pc then
        local mouseOk, mx, my = pc:GetMousePosition()
        if mouseOk then return mx, my, "player" end
    end

    if self._cachedMouseX and self._cachedMouseY then
        return self._cachedMouseX, self._cachedMouseY, "cache"
    end
    return nil, nil, "none"
end

--============================================================
-- 生命周期
--============================================================

function M:Construct()
    -- BuildNodeLib 延迟到首次 OpenGraph 调用，避免 Construct 期间创建子 Widget
    -- 导致 UnLua TryBind 重入崩溃（0xffffffffffffffff）
    self._nodeLibBuilt = false

    local function bind(name, fn)
        local w = self[name]
        if w and w.OnClicked then w.OnClicked:Add(self, fn)
        else Warn("缺少按钮: " .. name) end
    end
    bind("w_btn_compile", M.OnClickCompile)
    bind("w_btn_save",    M.OnClickSave)
    bind("w_btn_load",    M.OnClickLoad)
    bind("w_btn_clear",   M.OnClickClear)
    bind("w_btn_close",   M.OnClickClose)

    self:SetStatus("关卡蓝图 — 从左侧选择节点类型放置")
    Log("构建完成")
end

--============================================================
-- 侧边栏：节点库
--============================================================

function M:BuildNodeLib()
    if not self.w_scroll_nodeLib then Warn("缺少 w_scroll_nodeLib"); return end
    self.w_scroll_nodeLib:ClearChildren()
    collectgarbage("collect")   -- 清掉 ClearChildren 产生的孤儿 userdata，再 Create 新按钮

    local pc  = self:GetOwningPlayer()
    local cls = getNodeLibBtnClass()
    if not pc or not cls then return end

    local catColor = {
        ["事件"] = UE.FLinearColor(0.63, 0.06, 0.06, 1),
        ["条件"] = UE.FLinearColor(0.06, 0.44, 0.19, 1),
        ["动作"] = UE.FLinearColor(0.06, 0.31, 0.63, 1),
    }

    for _, cat in ipairs(NodeRegistry.Categories) do
        local header = UE.UWidgetBlueprintLibrary.Create(pc, cls, pc)
        if header then
            if header.w_text_label then
                header.w_text_label:SetText(cat.name)
                local col = catColor[cat.name]
                if col then
                    local ok = pcall(function()
                        header.w_text_label:SetColorAndOpacity(UE.FSlateColor(col))
                    end)
                    if not ok then
                        pcall(function() header.w_text_label:SetColorAndOpacity(col) end)
                    end
                end
            end
            self.w_scroll_nodeLib:AddChild(header)
        end

        for _, item in ipairs(cat.items) do
            local btn = UE.UWidgetBlueprintLibrary.Create(pc, cls, pc)
            if btn then
                if btn.w_text_label then btn.w_text_label:SetText(item.label) end
                if btn.SetNodeType  then btn:SetNodeType(item.type) end
                self.w_scroll_nodeLib:AddChild(btn)
            end
        end
    end
end

--============================================================
-- 拖拽放置
--============================================================

function M:OnDragOver(geometry, pointerEvent, operation)
    -- 在 OnDragOver 里缓存坐标，比 UpdateWires 更贴近 Drop 时刻
    local pc = self:GetOwningPlayer()
    if pc then
        local ok, x, y = pc:GetMousePosition()
        if ok then self._cachedMouseX = x; self._cachedMouseY = y end
    end
    Debug("OnDragOver tag=" .. (operation and tostring(operation.Tag) or "nil"))
    return true
end

function M:OnDrop(geometry, pointerEvent, operation)
    if not operation then Warn("OnDrop: operation 为空"); return false end

    local nodeType = tostring(operation.Tag)
    if not nodeType or nodeType == "" or nodeType == "None" then
        Warn("OnDrop: nodeType 非法"); return false
    end
    if not self.w_canvas_main then Warn("OnDrop: 缺少 w_canvas_main"); return false end

    local mx, my, source = resolveDropScreenPos(self, pointerEvent)
    if not mx or not my then Warn("OnDrop: 无法解析鼠标坐标"); return false end

    local canvasGeo = self.w_canvas_main:GetCachedGeometry()
    local local2d   = UE.USlateBlueprintLibrary.AbsoluteToLocal(canvasGeo, UE.FVector2D(mx, my))
    Log(string.format("OnDrop[%s] %s at %.1f,%.1f", source, nodeType, local2d.X, local2d.Y))
    self:SetStatus("放置中: " .. nodeType)
    self:PlaceNode(local2d.X - _panOffset.x, local2d.Y - _panOffset.y, nodeType)
    return true
end

--============================================================
-- 画布点击
--============================================================

function M:OnMouseButtonDown(geometry, pointerEvent)
    if _pendingPin then
        _pendingPin = nil
        if self.w_wire_overlay then
            self.w_wire_overlay:SetPendingWire(UE.FVector2D(0,0), UE.FVector2D(0,0), false)
        end
        self:SetStatus("就绪")
        return UE.UWidgetBlueprintLibrary.Handled()
    end
    return UE.UWidgetBlueprintLibrary.Unhandled()
end

--============================================================
-- 放置节点
--============================================================

function M:PlaceNode(canvasX, canvasY, nodeType)
    local pc  = self:GetOwningPlayer()
    local cls = getNodeClass()
    if not pc or not cls or not self.w_canvas_main then
        Warn("PlaceNode 中止: pc=" .. tostring(pc~=nil)
            .. " cls=" .. tostring(cls~=nil)
            .. " canvas=" .. tostring(self.w_canvas_main~=nil))
        return
    end

    local g  = getG()
    local id = "node_" .. g.nextID
    g.nextID = g.nextID + 1

    local def    = NodeRegistry.Definitions[nodeType]
    local params = {}
    for _, p in ipairs(def and def.params or {}) do
        params[p.name] = p.default
    end

    local nodeData = { id=id, type=nodeType, pos={x=canvasX, y=canvasY}, params=params }

    local widget = UE.UWidgetBlueprintLibrary.Create(pc, cls, pc)
    if not widget then Warn("创建节点 Widget 失败: " .. nodeType); return end

    if widget.InitNode then
        local ok, err = pcall(function() widget:InitNode(nodeData, self) end)
        if not ok then Warn("InitNode 异常: " .. tostring(err)) end
    else
        Warn("节点 Widget 没有 InitNode 方法")
    end

    widget:SetVisibility(UE.ESlateVisibility.Visible)
    nodeData.widget = widget
    g.nodes[id]     = nodeData

    -- 优先 AddChildToCanvas（返回已有 slot），回退 AddChild + SlotAsCanvasSlot
    local slot = nil
    if self.w_canvas_main.AddChildToCanvas then
        local ok, cs = pcall(function() return self.w_canvas_main:AddChildToCanvas(widget) end)
        if ok then slot = cs end
    end
    if not slot then
        self.w_canvas_main:AddChild(widget)
        slot = UE.UWidgetLayoutLibrary.SlotAsCanvasSlot(widget)
    end

    if slot then
        local px = canvasX + _panOffset.x
        local py = canvasY + _panOffset.y
        slot:SetAutoSize(true)
        slot:SetAlignment(UE.FVector2D(0.5, 0.5))
        slot:SetPosition(UE.FVector2D(px, py))
        Log(string.format("SetPosition %.1f,%.1f", px, py))
    else
        Warn("PlaceNode: 未获取到 CanvasSlot，节点可能不可见")
    end

    _isDirtyWires = true
    Log("放置节点: " .. id .. " (" .. nodeType .. ")")
    self:SetStatus("已放置: " .. id)
    return id
end

--============================================================
-- 多图管理（核心 API）
--============================================================

--- 切换到指定 programID 的图，清空画布并重建节点 Widget
--- @param programID string  图标识（"level_main" 或 "actor_prog_N"）
--- @param title     string  可选标题，显示在底栏
--- @param graphData table   可选：直接传入图数据，避免调用方先 LoadGraphData 再 OpenGraph
---                          （分两步调用会导致旧 widget 孤立，Lua GC 在后续 Create 中
---                          触发 UnLua 对象图并发修改崩溃）
function M:OpenGraph(programID, title, graphData)
    -- 首次调用时构建节点库（延迟自 Construct，避免 TryBind 重入崩溃）
    if not self._nodeLibBuilt then
        self._nodeLibBuilt = true
        self:BuildNodeLib()
    end

    -- 先保存当前图到 SceneData（避免切换丢失）
    self:SaveCurrentGraphToSceneData()

    -- 取消中间状态
    _pendingPin    = nil
    _dragNodeID    = nil
    _isDirtyWires  = false

    -- ★ 关键：在切换 _activeID 之前，先显式 RemoveFromParent 旧图所有 widget。
    --   这样旧 widget 的 UnLua 绑定立即释放，不会留给 Lua GC 在后续
    --   UWidgetBlueprintLibrary.Create 的内存分配时机触发 __gc 修改对象图。
    local oldG = _graphs[_activeID]
    if oldG then
        for _, node in pairs(oldG.nodes) do
            if node.widget then
                -- IsValid 检查：UE GC 可能已回收（编辑器关闭后 _graphs 模块变量残留引用）
                local ok = pcall(function()
                    if UE.UKismetSystemLibrary.IsValid(node.widget) then
                        node.widget:RemoveFromParent()
                    end
                end)
                if not ok then
                    Log("RemoveFromParent 失败（widget 已失效），跳过")
                end
                node.widget = nil
            end
        end
    end
    if self.w_canvas_main then
        self.w_canvas_main:ClearChildren()
    end

    -- 旧 widget 已全部 nil / RemoveFromParent，强制 GC 立即回收孤儿 userdata，
    -- 防止其 __gc(RemoveObject) 在随后 Create 的内存分配里触发，
    -- 与 TryBind(AddObject) 并发修改 UnLua 对象图导致崩溃
    collectgarbage("collect")

    if self.w_wire_overlay then
        self.w_wire_overlay:BeginWireUpdate()
        self.w_wire_overlay:EndWireUpdate()
        self.w_wire_overlay:SetPendingWire(UE.FVector2D(0,0), UE.FVector2D(0,0), false)
    end

    -- 切换活动图
    _activeID = programID or "level_main"

    -- 如果直接传入了图数据，写入 _graphs（替代单独调用 LoadGraphData）
    if graphData then
        local g = {
            nodes       = {},
            connections = graphData.connections or {},
            nextID      = graphData.nextID or 1,
        }
        for _, n in ipairs(graphData.nodes or {}) do
            if n.id then
                g.nodes[n.id] = {
                    id     = n.id,
                    type   = n.type or "Unknown",
                    pos    = n.pos or { x = 0, y = 0 },
                    params = n.params or {},
                }
            end
        end
        _graphs[_activeID] = g
    end

    -- 更新底栏提示
    local displayTitle = title
        or ((_activeID == "level_main") and "关卡蓝图 — 全局逻辑"
            or ("Actor 蓝图: " .. _activeID))
    self:SetStatus(displayTitle)

    -- 重建新图的节点 Widget
    if self.w_canvas_main then
        local pc  = self:GetOwningPlayer()
        local cls = getNodeClass()
        if pc and cls then
            for _, nodeData in pairs(getG().nodes) do
                local widget = UE.UWidgetBlueprintLibrary.Create(pc, cls, pc)
                if widget then
                    nodeData.widget = widget
                    self.w_canvas_main:AddChild(widget)
                    local slot = UE.UWidgetLayoutLibrary.SlotAsCanvasSlot(widget)
                    if slot then
                        slot:SetAutoSize(true)
                        slot:SetAlignment(UE.FVector2D(0.5, 0.5))
                        slot:SetPosition(UE.FVector2D(
                            nodeData.pos.x + _panOffset.x,
                            nodeData.pos.y + _panOffset.y))
                    end
                    if widget.InitNode then
                        pcall(function() widget:InitNode(nodeData, self) end)
                    end
                    widget:SetVisibility(UE.ESlateVisibility.Visible)
                end
            end
        end
    end

    _isDirtyWires = true
    Log("OpenGraph: " .. _activeID)
end

--- 返回当前活动图的可序列化数据（不含 Widget 引用）
function M:GetCurrentGraphData()
    local g = getG()
    local nodes = {}
    for id, n in pairs(g.nodes) do
        table.insert(nodes, {
            id     = id,
            type   = n.type,
            pos    = { x = n.pos.x, y = n.pos.y },
            params = n.params or {},
        })
    end
    return {
        programID   = _activeID,
        nodes       = nodes,
        connections = g.connections,
        nextID      = g.nextID,
    }
end

--- 从序列化数据加载指定图（不立即重建 Widget，需调 OpenGraph 触发重建）
function M:LoadGraphData(programID, data)
    if not data then return end
    local g = {
        nodes       = {},
        connections = data.connections or {},
        nextID      = data.nextID or 1,
    }
    for _, n in ipairs(data.nodes or {}) do
        if n.id then
            g.nodes[n.id] = {
                id     = n.id,
                type   = n.type or "Unknown",
                pos    = n.pos or { x = 0, y = 0 },
                params = n.params or {},
                -- widget = nil，等 OpenGraph 时重建
            }
        end
    end
    _graphs[programID] = g
    Log("LoadGraphData: " .. tostring(programID)
        .. " nodes=" .. tostring(#(data.nodes or {})))
end

--- 返回所有图的可序列化数据（供场景保存一次性打包）
function M:GetAllGraphsData()
    local result = {}
    for pid, g in pairs(_graphs) do
        local nodes = {}
        for id, n in pairs(g.nodes) do
            table.insert(nodes, {
                id     = id,
                type   = n.type,
                pos    = { x = n.pos.x, y = n.pos.y },
                params = n.params or {},
            })
        end
        result[pid] = {
            nodes       = nodes,
            connections = g.connections,
            nextID      = g.nextID,
        }
    end
    return result
end

--- 将当前图写入 UGCSceneData（内存级，文件保存由 WBP_UGCEditor 统一触发）
function M:SaveCurrentGraphToSceneData()
    local ok, SceneData = pcall(require, "Gameplay.UGC.UGCSceneData")
    if not ok or not SceneData then return end

    local data = self:GetCurrentGraphData()
    -- 通用调用：key 就是 programID 字符串（"level_main" / "actor_prog_N"）
    SceneData:SetScript(_activeID, data)
end

--============================================================
-- 引脚连线
--============================================================

function M:OnPinClicked(nodeID, pinName, isOutput)
    if not _pendingPin then
        _pendingPin = { nodeID=nodeID, pinName=pinName, isOutput=isOutput }
        self:SetStatus("连线中 — 点击目标引脚完成，点击空白取消")
        return
    end

    -- 同向引脚：重置起点
    if _pendingPin.isOutput == isOutput then
        _pendingPin = { nodeID=nodeID, pinName=pinName, isOutput=isOutput }
        return
    end

    -- 自连检测
    if _pendingPin.nodeID == nodeID then
        self:SetStatus("不能连接同一节点的引脚")
        return
    end

    local fromID, fromPin, toID, toPin
    if _pendingPin.isOutput then
        fromID, fromPin = _pendingPin.nodeID, _pendingPin.pinName
        toID,   toPin   = nodeID, pinName
    else
        fromID, fromPin = nodeID, pinName
        toID,   toPin   = _pendingPin.nodeID, _pendingPin.pinName
    end

    local g = getG()
    table.insert(g.connections, {
        from_id  = fromID, from_pin = fromPin,
        to_id    = toID,   to_pin   = toPin,
    })
    _pendingPin   = nil
    _isDirtyWires = true

    if self.w_wire_overlay then
        self.w_wire_overlay:SetPendingWire(UE.FVector2D(0,0), UE.FVector2D(0,0), false)
    end
    self:SetStatus("连线已创建")
    Log(string.format("连线: %s.%s → %s.%s", fromID, fromPin, toID, toPin))
end

--============================================================
-- 连线绘制（PC Tick 驱动）
--============================================================

function M:UpdateWires(mouseAbsX, mouseAbsY)
    self._cachedMouseX = mouseAbsX
    self._cachedMouseY = mouseAbsY

    if not self.w_wire_overlay then return end
    if not self.w_canvas_main  then return end
    if not _isDirtyWires and not _pendingPin then return end

    local canvasGeo = self.w_canvas_main:GetCachedGeometry()
    local function absToCanvas(absPos)
        return UE.USlateBlueprintLibrary.AbsoluteToLocal(canvasGeo, absPos)
    end

    local g = getG()
    self.w_wire_overlay:BeginWireUpdate()

    for _, conn in ipairs(g.connections) do
        local fromNode = g.nodes[conn.from_id]
        local toNode   = g.nodes[conn.to_id]
        if fromNode and fromNode.widget and toNode and toNode.widget then
            local fromRow = fromNode.widget:GetPinRow(conn.from_pin)
            local toRow   = toNode.widget:GetPinRow(conn.to_pin)
            if fromRow and toRow then
                local startAbs = fromRow:GetPinOutAbsPos()
                local endAbs   = toRow:GetPinInAbsPos()
                if startAbs and endAbs then
                    local s = absToCanvas(startAbs)
                    local e = absToCanvas(endAbs)
                    self.w_wire_overlay:AddWire(s.X, s.Y, e.X, e.Y, 1, 1, 1, 1, 2.0)
                end
            end
        end
    end

    self.w_wire_overlay:EndWireUpdate()
    if not _pendingPin then _isDirtyWires = false end

    if _pendingPin then
        local fromNode = g.nodes[_pendingPin.nodeID]
        if fromNode and fromNode.widget then
            local row = fromNode.widget:GetPinRow(_pendingPin.pinName)
            if row then
                local pinAbs = _pendingPin.isOutput
                    and row:GetPinOutAbsPos()
                    or  row:GetPinInAbsPos()
                if pinAbs then
                    local s = absToCanvas(pinAbs)
                    local e = absToCanvas(UE.FVector2D(mouseAbsX, mouseAbsY))
                    self.w_wire_overlay:SetPendingWire(s, e, true)
                end
            end
        end
    end
end

function M:IsDraggingPin()
    return _pendingPin ~= nil
end

--============================================================
-- 节点拖拽（PC Tick 驱动）
--============================================================

function M:BeginNodeDrag(nodeID, sx, sy)
    local node = getG().nodes[nodeID]
    if not node then return end
    _dragNodeID       = nodeID
    _dragStartMouse   = { x=sx, y=sy }
    _dragStartNodePos = { x=node.pos.x, y=node.pos.y }
end

function M:OnDragTick(sx, sy)
    if not _dragNodeID or not _dragStartMouse then return end
    local dx = sx - _dragStartMouse.x
    local dy = sy - _dragStartMouse.y
    self:MoveNodeTo(_dragNodeID,
        _dragStartNodePos.x + dx,
        _dragStartNodePos.y + dy)
    _isDirtyWires = true
end

function M:EndNodeDrag()
    _dragNodeID       = nil
    _dragStartMouse   = nil
    _dragStartNodePos = nil
end

function M:IsDraggingNode()
    return _dragNodeID ~= nil
end

function M:MoveNodeTo(nodeID, cx, cy)
    local node = getG().nodes[nodeID]
    if not node or not node.widget then return end
    node.pos.x = cx
    node.pos.y = cy
    local slot = UE.UWidgetLayoutLibrary.SlotAsCanvasSlot(node.widget)
    if slot then
        slot:SetPosition(UE.FVector2D(cx + _panOffset.x, cy + _panOffset.y))
    end
end

--============================================================
-- 按钮事件
--============================================================

function M:OnClickClose()
    self:SaveCurrentGraphToSceneData()

    -- 关闭前清空所有图里的 widget 引用，防止 _graphs 模块变量持有已回收 UObject，
    -- 导致下次 OpenGraph 调用 RemoveFromParent 时 UnLua 访问死亡对象卡死
    for _, g in pairs(_graphs) do
        for _, node in pairs(g.nodes) do
            node.widget = nil
        end
    end
    if self.w_canvas_main then self.w_canvas_main:ClearChildren() end

    local UIManager = require("Gameplay.Core.UIManager")
    local pc = self:GetOwningPlayer()
    if pc and pc.SetBlueprintEditor then pc:SetBlueprintEditor(nil) end
    UIManager:CloseWindow("WBP_UGCBlueprintEditor")
end

function M:OnClickCompile()
    self:SetStatus("编译成功（执行引擎 Day 4 实现）")
end

function M:OnClickSave()
    self:SaveCurrentGraphToSceneData()
    self:SetStatus("蓝图已暂存 — 请在关卡编辑器点击「保存场景」写入文件")
end

function M:OnClickLoad()
    self:SetStatus("加载 — Day 5 实现")
end

function M:OnClickClear()
    local g = getG()
    for _, node in pairs(g.nodes) do
        if node.widget then
            node.widget:RemoveFromParent()
            node.widget = nil
        end
    end
    g.nodes       = {}
    g.connections = {}
    g.nextID      = 1
    _pendingPin   = nil
    _isDirtyWires = false
    collectgarbage("collect")   -- 清掉 RemoveFromParent 后的孤儿 userdata
    if self.w_wire_overlay then
        self.w_wire_overlay:BeginWireUpdate()
        self.w_wire_overlay:EndWireUpdate()
        self.w_wire_overlay:SetPendingWire(UE.FVector2D(0,0), UE.FVector2D(0,0), false)
    end
    self:SetStatus("画布已清空")
end

--============================================================
-- 工具
--============================================================

function M:SetStatus(msg)
    if self.w_text_status then self.w_text_status:SetText(msg) end
end

function M:GetActiveID()        return _activeID             end
function M:GetNodes()           return getG().nodes          end
function M:GetConnections()     return getG().connections     end

return M
