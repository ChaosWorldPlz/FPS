--[[
    UGCProgramRunner.lua
    蓝图执行引擎（Lua 单例）

    职责：
    - RunProgram(programID, eventType)  从 SceneData 读取图，找对应事件节点入口，沿 exec 引脚链执行
    - Tick(deltaTime)                   累计 deltaTime，驱动 Event_OnInterval 定时节点
    - RegisterIntervals(programID)      扫描程序中的 Event_OnInterval 并注册定时器
    - TriggerGameStart()                向所有程序广播 Event_OnGameStart

    节点分发规则：
      Set_Attribute  → UGCFunctionRegistry:Call("set_attribute")
      Set_GameRule   → UGCFunctionRegistry:Call("set_rule")
      Spawn_Weapon   → UGCFunctionRegistry:Call("spawn_weapon")
      Print_Message  → UKismetSystemLibrary.PrintString + print
      Branch         → 评估 condition_node → exec_out_true / exec_out_false
      Delay          → ScheduleCallback(frames) 异步，不阻塞同步链

    调用方式：
        local Runner = require("Gameplay.UGC.UGCProgramRunner")
        Runner:Init(playerController)
        Runner:RunProgram("actor_prog_3", "Event_OnEnter")
        Runner:Tick(deltaTime)   -- 在 PC ReceiveTick 里调
]]

local UGCRegistry = require("Gameplay.UGC.UGCFunctionRegistry")
local SceneData   = require("Gameplay.UGC.UGCSceneData")

local Runner = {}
Runner.__index = Runner

local _pc          = nil
local _initialized = false

-- Event_OnInterval 定时表：{ {programID, nodeID, interval, accumulated} }
local _intervals = {}

local MAX_DEPTH = 32   -- 防止无限循环（循环连线场景）
local LOG_TAG   = "[UGCProgramRunner]"

local function Log(msg)  print(LOG_TAG .. " " .. tostring(msg)) end
local function Warn(msg) print(LOG_TAG .. "[Warn] " .. tostring(msg)) end

--============================================================
-- 初始化
--============================================================

function Runner:Init(playerController)
    _pc          = playerController
    _initialized = true
    _intervals   = {}
    Log("初始化完成")
end

--============================================================
-- 公共入口
--============================================================

--- 执行程序中指定事件类型的所有匹配节点
--- @param programID  string  "level_main" 或 "actor_prog_N"
--- @param eventType  string  节点类型名，如 "Event_OnEnter" / "Event_OnGameStart"
--- @param context    table   可选上下文（触发者信息等，供未来扩展）
function Runner:RunProgram(programID, eventType, context)
    if not _initialized then
        Warn("RunProgram: Runner 未初始化，跳过")
        return
    end

    local graphData = SceneData:GetScript(programID)
    if not graphData then return end

    -- 建节点 map：id → nodeData
    local nodeMap = {}
    for _, n in ipairs(graphData.nodes or {}) do
        if n.id then nodeMap[n.id] = n end
    end

    -- 建 exec 连线索引：[fromID][fromPin] = {to_id, to_pin}
    local connMap = {}
    for _, c in ipairs(graphData.connections or {}) do
        if not connMap[c.from_id] then connMap[c.from_id] = {} end
        connMap[c.from_id][c.from_pin] = { to_id = c.to_id, to_pin = c.to_pin }
    end

    -- 找并触发所有匹配事件节点
    local found = 0
    for _, n in ipairs(graphData.nodes or {}) do
        if n.type == eventType then
            found = found + 1
            Log(string.format("触发 %s → 节点 %s [%s]", eventType, n.id, programID))
            self:_executeFrom(n.id, "exec_out", nodeMap, connMap, context or {}, 0)
        end
    end
end

--- 向关卡蓝图广播 Event_OnGameStart
function Runner:TriggerGameStart()
    self:RunProgram("level_main", "Event_OnGameStart")
    Log("Event_OnGameStart 广播完成")
end

--- 扫描程序中的 Event_OnInterval 节点，注册定时器（不重复注册）
--- 应在进入游戏模式时对所有已加载程序调用
function Runner:RegisterIntervals(programID)
    local graphData = SceneData:GetScript(programID)
    if not graphData then return end

    for _, n in ipairs(graphData.nodes or {}) do
        if n.type == "Event_OnInterval" then
            local interval = tonumber((n.params or {}).interval) or 5.0
            -- 去重检查
            local exists = false
            for _, v in ipairs(_intervals) do
                if v.programID == programID and v.nodeID == n.id then
                    exists = true; break
                end
            end
            if not exists then
                table.insert(_intervals, {
                    programID   = programID,
                    nodeID      = n.id,
                    interval    = interval,
                    accumulated = 0,
                })
                Log(string.format("注册定时器: %s 节点 %s 间隔 %.2fs",
                    programID, n.id, interval))
            end
        end
    end
end

--- 清除所有定时器（退出游戏 / 重置时调用）
function Runner:ClearIntervals()
    _intervals = {}
end

--- PC ReceiveTick 驱动：累计 deltaTime，触发到期的定时事件
function Runner:Tick(deltaTime)
    if not _initialized or #_intervals == 0 then return end
    for _, v in ipairs(_intervals) do
        v.accumulated = v.accumulated + deltaTime
        if v.accumulated >= v.interval then
            v.accumulated = v.accumulated - v.interval
            self:RunProgram(v.programID, "Event_OnInterval")
        end
    end
end

--============================================================
-- 内部执行：exec 链遍历
--============================================================

--- 从 fromID.fromPin 出发，沿连线找到下一节点执行
function Runner:_executeFrom(fromID, fromPin, nodeMap, connMap, ctx, depth)
    if depth > MAX_DEPTH then
        Warn("执行深度超过上限 " .. MAX_DEPTH .. "，中止（请检查是否存在循环连线）")
        return
    end

    local outMap = connMap[fromID]
    if not outMap then return end

    local conn = outMap[fromPin]
    if not conn then return end

    self:_executeNode(conn.to_id, nodeMap, connMap, ctx, depth + 1)
end

--- 执行单个节点，根据类型分发
function Runner:_executeNode(nodeID, nodeMap, connMap, ctx, depth)
    local node = nodeMap[nodeID]
    if not node then
        Warn("节点不存在: " .. tostring(nodeID))
        return
    end

    local t = node.type
    local p = node.params or {}
    Log(string.format("  执行 [%s] (%s)", nodeID, t))

    -- ── 动作节点 ──────────────────────────────────────────────

    if t == "Set_Attribute" then
        local ok, msg = UGCRegistry:Call("set_attribute", {
            attribute = tostring(p.name  or ""),
            value     = tonumber(p.value) or 0,
        })
        if not ok then Warn("Set_Attribute 失败: " .. tostring(msg)) end
        self:_executeFrom(nodeID, "exec_out", nodeMap, connMap, ctx, depth)

    elseif t == "Set_GameRule" then
        local ok, msg = UGCRegistry:Call("set_rule", {
            rule  = tostring(p.rule  or ""),
            value = tonumber(p.value) or 0,
        })
        if not ok then Warn("Set_GameRule 失败: " .. tostring(msg)) end
        self:_executeFrom(nodeID, "exec_out", nodeMap, connMap, ctx, depth)

    elseif t == "Spawn_Weapon" then
        local ok, msg = UGCRegistry:Call("spawn_weapon", {
            weapon_id = tostring(p.weapon_id or ""),
            x         = tonumber(p.x) or 0,
            y         = tonumber(p.y) or 0,
            z         = tonumber(p.z) or 100,
        })
        if not ok then Warn("Spawn_Weapon 失败: " .. tostring(msg)) end
        self:_executeFrom(nodeID, "exec_out", nodeMap, connMap, ctx, depth)

    elseif t == "PCG_Generate" then
        local ok, msg = UGCRegistry:Call("pcg_generate", {
            x          = tonumber(p.x) or 0,
            y          = tonumber(p.y) or 0,
            z          = tonumber(p.z) or 0,
            radius     = tonumber(p.radius) or 1000,
            seed       = tonumber(p.seed) or 0,
            graph_path = tostring(p.graph_path or ""),
        })
        if not ok then Warn("PCG_Generate 失败: " .. tostring(msg)) end
        self:_executeFrom(nodeID, "exec_out", nodeMap, connMap, ctx, depth)

    elseif t == "PCG_Clear" then
        local ok, msg = UGCRegistry:Call("pcg_clear", {})
        if not ok then Warn("PCG_Clear 失败: " .. tostring(msg)) end
        self:_executeFrom(nodeID, "exec_out", nodeMap, connMap, ctx, depth)

    elseif t == "Print_Message" then
        local msg = tostring(p.msg or "")
        Log("PrintMsg: " .. msg)
        if _pc then
            local w = _pc:GetWorld()
            if w then
                pcall(UE.UKismetSystemLibrary.PrintString, w, msg,
                    true, true, UE.FLinearColor(0.1, 0.9, 1.0, 1.0), 5.0)
            end
        end
        self:_executeFrom(nodeID, "exec_out", nodeMap, connMap, ctx, depth)

    -- ── 条件节点 ──────────────────────────────────────────────

    elseif t == "Branch" then
        local condID = tostring(p.condition_node or "")
        local result = self:_evalCondition(condID, nodeMap, ctx)
        local nextPin = result and "exec_out_true" or "exec_out_false"
        Log(string.format("  Branch [%s] → %s", condID, nextPin))
        self:_executeFrom(nodeID, nextPin, nodeMap, connMap, ctx, depth)

    -- ── 延迟节点 ──────────────────────────────────────────────

    elseif t == "Delay" then
        local seconds = tonumber(p.seconds) or 1.0
        local frames  = math.max(1, math.floor(seconds * 60))  -- 60fps 近似
        Log(string.format("  Delay %.2fs ≈ %d 帧", seconds, frames))

        if _pc and _pc.ScheduleCallback then
            -- 捕获上下文，避免闭包持有大型表引用
            local s_ = self
            local a, b, c, d = nodeID, nodeMap, connMap, ctx
            _pc:ScheduleCallback(function()
                s_:_executeFrom(a, "exec_out", b, c, d, depth)
            end, frames)
        else
            -- 降级同步（忽略延迟时长）
            Warn("ScheduleCallback 不可用，Delay 降级为同步执行")
            self:_executeFrom(nodeID, "exec_out", nodeMap, connMap, ctx, depth)
        end
        -- Delay 是异步节点：此处 return，不继续同步链

    else
        Warn("未知节点类型: " .. tostring(t) .. " (节点 " .. nodeID .. ")")
    end
end

--============================================================
-- 条件评估
--============================================================

--- 评估条件节点，返回 bool
function Runner:_evalCondition(condNodeID, nodeMap, ctx)
    if not condNodeID or condNodeID == "" then
        Warn("Branch 未指定 condition_node，默认 false")
        return false
    end

    local node = nodeMap[condNodeID]
    if not node then
        Warn("条件节点不存在: " .. condNodeID)
        return false
    end

    local p   = node.params or {}
    local t   = node.type
    local lhs = 0
    local rhs = tonumber(p.value) or 0

    if t == "Compare_Attribute" then
        local ok, val = UGCRegistry:Call("get_attribute",
            { attribute = tostring(p.attr or "") })
        if not ok then return false end
        lhs = tonumber(val) or 0

    elseif t == "Compare_GameRule" then
        local ok, val = UGCRegistry:Call("get_rule",
            { rule = tostring(p.rule or "") })
        if not ok then return false end
        lhs = tonumber(val) or 0

    else
        Warn("不支持的条件节点类型: " .. tostring(t))
        return false
    end

    return self:_compare(lhs, tostring(p.op or ">"), rhs)
end

function Runner:_compare(lhs, op, rhs)
    if     op == ">"  then return lhs >  rhs
    elseif op == ">=" then return lhs >= rhs
    elseif op == "<"  then return lhs <  rhs
    elseif op == "<=" then return lhs <= rhs
    elseif op == "==" then return lhs == rhs
    elseif op == "!=" then return lhs ~= rhs
    end
    Warn("未知运算符: " .. op .. "，默认 false")
    return false
end

return Runner
