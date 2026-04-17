--[[
    UGCSceneData.lua
    场景数据管理器（Lua 单例）

    职责：
    - 维护当前场景所有 Placeable Actor 的数据表
    - 提供 CRUD 接口（Create/Delete/Modify/Query）
    - 撤销/重做栈（深度 5）
    - JSON 序列化/反序列化（保存/加载）

    对标元梦之星的 SceneDataManager，业务逻辑全在 Lua。
    C++ Bridge 只负责引擎层的实际 Spawn/Destroy/Transform 操作。
]]

local PrefabRegistry  = require("Gameplay.UGC.UGCPrefabRegistry")
local UGCSerialize    = require("Gameplay.UGC.UGCSerialize")

local SceneData = {}
SceneData.__index = SceneData

--============================================================
-- 内部状态
--============================================================

local _bridge     = nil   -- UUGCEditorBridge C++ 组件
local _nextID     = 1     -- 自增 SceneID
local _actors     = {}    -- SceneID → { actor, prefabName, sceneID }
local _undoStack  = {}    -- 撤销栈，最多 5 条
local _redoStack  = {}    -- 重做栈
local _scripts    = {}    -- 蓝图脚本：{["_level"]=graphData, [sceneID]=graphData, ...}
local _isDirty    = false -- 脏标记：有未保存的修改时为 true
local _batches      = {}   -- batchID(string) → { sceneID1, sceneID2, ... } 整批生成追踪
local _nextBatch    = 1
local _activeBatch  = nil  -- 跨多次原子调用共享的 batchID（begin_batch/end_batch 显式管理）

local UNDO_MAX = 5
local ACTOR_MAX = 50

--============================================================
-- 初始化
--============================================================

function SceneData:Init(editorBridge)
    _bridge    = editorBridge
    _nextID    = 1
    _actors    = {}
    _undoStack = {}
    _redoStack = {}
    _scripts   = {}
    _isDirty   = false
    _batches   = {}
    _nextBatch = 1
    _activeBatch = nil
    print("[UGCSceneData] 初始化完成")
end

function SceneData:Clear()
    -- 销毁所有 Actor，并立即 nil 各引用，让 UnLua userdata 尽早失去强引用
    for _, entry in pairs(_actors) do
        if entry.actor and UE.UKismetSystemLibrary.IsValid(entry.actor) then
            _bridge:DestroyActor(entry.actor)
        end
        entry.actor = nil
    end
    _actors     = {}
    _nextID     = 1
    _undoStack  = {}
    _redoStack  = {}
    _scripts    = {}
    _isDirty    = false
    _batches    = {}
    _nextBatch  = 1
    _activeBatch = nil

    -- ★ 强制完整 GC：
    --   _actors = {} 令所有 actor userdata 成孤儿，但 Lua 增量 GC 不会立刻回收。
    --   若延迟到 UWidgetBlueprintLibrary.Create 内部的内存分配才触发，
    --   __gc（RemoveObject）与 TryBind（AddObject）并发修改 UnLua 对象图
    --   → 读到 0xffffffffffffffff 崩溃。
    --   在此处（安全上下文，不在 TryBind 调用链内）强制跑完，消除隐患。
    collectgarbage("collect")

    print("[UGCSceneData] 场景已清空")
end

--============================================================
-- 脏标记
--============================================================

function SceneData:IsDirty()   return _isDirty  end
function SceneData:MarkDirty() _isDirty = true   end
function SceneData:ClearDirty() _isDirty = false end

--============================================================
-- 蓝图脚本存取
--============================================================

--- 通用脚本存取（以 programId 字符串为 key）
--- programId 示例："level_main" / "actor_prog_3"
function SceneData:SetScript(programId, data)
    _scripts[programId] = data
    _isDirty = true
end

function SceneData:GetScript(programId)
    return _scripts[programId]
end

--- 关卡全局蓝图（快捷别名，key = "level_main"）
function SceneData:SetLevelScript(data)
    _scripts["level_main"] = data
end

function SceneData:GetLevelScript()
    return _scripts["level_main"]
end

--- Actor 蓝图（快捷别名，key = "actor_prog_{sceneID}"）
function SceneData:SetActorScript(sceneID, data)
    _scripts["actor_prog_" .. sceneID] = data
end

function SceneData:GetActorScript(sceneID)
    return _scripts["actor_prog_" .. sceneID]
end

--- 返回全部脚本表（供序列化使用）
function SceneData:GetAllScripts()
    return _scripts
end

--- 批量加载脚本（反序列化时使用）
--- @param scriptMap table  { ["level_main"]=data, ["actor_prog_N"]=data, ... }
function SceneData:LoadAllScripts(scriptMap)
    _scripts = scriptMap or {}
end

--============================================================
-- CRUD
--============================================================

--- 放置一个新 Placeable Actor
--- @param prefabName string  预制体名称（在 PrefabRegistry 中注册的 key）
--- @param location   FVector 世界坐标
--- @param rotation   FRotator 旋转（可选，默认零旋转）
--- @return sceneID int, actor AActor
function SceneData:CreateActor(prefabName, location, rotation)
    if not PrefabRegistry.IsValid(prefabName) then
        print("[UGCSceneData] CreateActor: 未知预制体 " .. tostring(prefabName))
        return nil, nil
    end

    if self:Count() >= ACTOR_MAX then
        print("[UGCSceneData] CreateActor: 已达到 Actor 上限 " .. ACTOR_MAX)
        return nil, nil
    end

    local path = PrefabRegistry.GetPath(prefabName)
    if not path then
        print("[UGCSceneData] CreateActor: 预制体没有可用路径 " .. tostring(prefabName))
        return nil, nil
    end

    local rot  = rotation or UE.FRotator(0, 0, 0)
    local actor = _bridge:SpawnPlaceable(path, location, rot)
    if not actor then
        print("[UGCSceneData] CreateActor: Spawn 失败 prefab=" .. tostring(prefabName) .. " path=" .. tostring(path))
        return nil, nil
    end

    local sceneID = _nextID
    _nextID = _nextID + 1

    local entry = {
        actor      = actor,
        prefabName = prefabName,
        sceneID    = sceneID,
        actorId    = "actor_" .. sceneID,
        programId  = "actor_prog_" .. sceneID,
    }
    _actors[sceneID] = entry
    _isDirty = true

    -- 记录撤销（保存完整信息供 Redo 恢复 — 2026-04-16 补全）
    local transform = _bridge:GetActorTransform(actor)
    self:PushUndo({ op = "Create", sceneID = sceneID, prefabName = prefabName, transform = transform })
    _redoStack = {}

    -- 如果 Actor 支持 SetProgramID（即 AUGCTriggerZone），赋值关联程序
    pcall(function() actor:SetProgramID("actor_prog_" .. tostring(sceneID)) end)
    -- 编辑模式下新放置的 TriggerZone 立即显示可视化方块（非 TriggerZone 的 pcall 静默忽略）
    pcall(function() actor:SetDebugVisible(true) end)

    print("[UGCSceneData] CreateActor: " .. prefabName .. " SceneID=" .. sceneID)
    return sceneID, actor
end

--- 放置一个新 Placeable Actor（完整 Transform 版本，2026-04-16 新增）
--- @param prefabName string  预制体名称
--- @param transform  FTransform  完整变换（位置+旋转+缩放）
--- @return sceneID int, actor AActor
function SceneData:CreateActorWithTransform(prefabName, transform)
    if not PrefabRegistry.IsValid(prefabName) then
        print("[UGCSceneData] CreateActorWithTransform: 未知预制体 " .. tostring(prefabName))
        return nil, nil
    end

    if self:Count() >= ACTOR_MAX then
        print("[UGCSceneData] CreateActorWithTransform: 已达到 Actor 上限 " .. ACTOR_MAX)
        return nil, nil
    end

    local path = PrefabRegistry.GetPath(prefabName)
    if not path then
        print("[UGCSceneData] CreateActorWithTransform: 预制体没有可用路径 " .. tostring(prefabName))
        return nil, nil
    end

    local loc, rot, _ = UE.UKismetMathLibrary.BreakTransform(transform)
    local actor = _bridge:SpawnPlaceable(path, loc, rot)
    if not actor then
        print("[UGCSceneData] CreateActorWithTransform: Spawn 失败")
        return nil, nil
    end

    -- 应用完整 Transform（含缩放）
    _bridge:SetActorTransform(actor, transform)

    local sceneID = _nextID
    _nextID = _nextID + 1

    local entry = {
        actor      = actor,
        prefabName = prefabName,
        sceneID    = sceneID,
        actorId    = "actor_" .. sceneID,
        programId  = "actor_prog_" .. sceneID,
    }
    _actors[sceneID] = entry
    _isDirty = true

    -- 记录撤销（保存完整信息供 Redo 恢复 — 2026-04-16）
    self:PushUndo({ op = "Create", sceneID = sceneID, prefabName = prefabName, transform = transform })
    _redoStack = {}

    pcall(function() actor:SetProgramID("actor_prog_" .. tostring(sceneID)) end)
    pcall(function() actor:SetDebugVisible(true) end)

    print("[UGCSceneData] CreateActorWithTransform: " .. prefabName .. " SceneID=" .. sceneID)
    return sceneID, actor
end

--- 登记一个由外部系统（如 PCG）生成的 Actor
--- 与 CreateActor 区别：不走 PrefabRegistry，不进撤销栈，不调用 Spawn
--- 用途：让 PCG 等外部生成的 Actor 也能被 SceneData 追踪、清除、序列化
--- @param actor       AActor   已存在的 Actor 实例
--- @param prefabName  string   逻辑名（如 "PCG_Generated"），仅用于显示/调试
--- @param metadata    table    重建所需元数据（如 PCG: {kind="pcg", graph_path=..., x,y,z, radius, seed}）
--- @return sceneID    int      分配的场景 ID
function SceneData:RegisterExternalActor(actor, prefabName, metadata)
    if not actor then
        print("[UGCSceneData] RegisterExternalActor: actor 为 nil")
        return nil
    end

    local sceneID = _nextID
    _nextID = _nextID + 1

    _actors[sceneID] = {
        actor      = actor,
        prefabName = prefabName or "External",
        sceneID    = sceneID,
        actorId    = "actor_" .. sceneID,
        programId  = "actor_prog_" .. sceneID,
        external   = true,
        metadata   = metadata or {},
    }
    _isDirty = true

    print(string.format("[UGCSceneData] RegisterExternalActor: %s SceneID=%d",
        tostring(prefabName), sceneID))
    return sceneID
end

--- 仅从追踪表移除外部 Actor 条目，不调用 DestroyActor
--- 用途：当外部模块（如 PCG Bridge）已自行销毁 Actor 后，同步清理 SceneData 索引
--- @param kind  string|nil  仅移除 metadata.kind 匹配的条目；nil = 移除所有 external 条目
--- @return removed int  实际移除的条目数
function SceneData:UnregisterExternalByKind(kind)
    local removed = 0
    for sceneID, entry in pairs(_actors) do
        if entry.external then
            if (not kind) or ((entry.metadata or {}).kind == kind) then
                _actors[sceneID] = nil
                removed = removed + 1
            end
        end
    end
    if removed > 0 then
        _isDirty = true
        print(string.format("[UGCSceneData] UnregisterExternalByKind(%s): 移除 %d 个条目",
            tostring(kind or "*"), removed))
    end
    return removed
end

--============================================================
-- 整批生成（Batch）
-- 由 Generators 调用：BeginBatch → 多次 CreateActor + AddToBatch → 后续可整批 DeleteBatch
--============================================================

function SceneData:BeginBatch()
    local id = "batch_" .. _nextBatch
    _nextBatch = _nextBatch + 1
    _batches[id] = {}
    return id
end

--- 开启一个具名跨调用 batch（多次原子/生成器调用共享同一 batchID）
--- 同名重复 begin 时复用旧 ID（语义：当前命名 batch 仍然激活）
function SceneData:BeginNamedBatch(name)
    local id = "batch_" .. tostring(name or "anon")
    if not _batches[id] then _batches[id] = {} end
    _activeBatch = id
    print("[UGCSceneData] BeginNamedBatch: " .. id)
    return id
end

--- 结束当前 active batch，返回结束的 batchID（已无 active 时返回 nil）
function SceneData:EndActiveBatch()
    local id = _activeBatch
    _activeBatch = nil
    if id then print("[UGCSceneData] EndActiveBatch: " .. id) end
    return id
end

function SceneData:GetActiveBatch()
    return _activeBatch
end

function SceneData:AddToBatch(batchID, sceneID)
    if not batchID or not sceneID then return end
    local list = _batches[batchID]
    if not list then return end
    list[#list+1] = sceneID
end

function SceneData:GetBatchActors(batchID)
    return _batches[batchID]
end

--- 整批删除：返回实际删除数量
function SceneData:DeleteBatch(batchID)
    local list = _batches[batchID]
    if not list then
        print("[UGCSceneData] DeleteBatch: 不存在 " .. tostring(batchID))
        return 0
    end
    local n = 0
    for _, sceneID in ipairs(list) do
        if _actors[sceneID] and self:DeleteActor(sceneID) then
            n = n + 1
        end
    end
    _batches[batchID] = nil
    print(string.format("[UGCSceneData] DeleteBatch %s 完成，删除 %d 个 Actor", batchID, n))
    return n
end

function SceneData:ListBatches()
    local out = {}
    for id, list in pairs(_batches) do
        out[#out+1] = { id = id, count = #list }
    end
    table.sort(out, function(a, b) return a.id < b.id end)
    return out
end

--- 删除指定 Actor
function SceneData:DeleteActor(sceneID)
    local entry = _actors[sceneID]
    if not entry then
        print("[UGCSceneData] DeleteActor: 找不到 SceneID=" .. tostring(sceneID))
        return false
    end

    -- 保存 Transform 供撤销恢复
    local transform = _bridge:GetActorTransform(entry.actor)

    _bridge:DestroyActor(entry.actor)
    _actors[sceneID] = nil
    _isDirty = true

    self:PushUndo({
        op         = "Delete",
        sceneID    = sceneID,
        prefabName = entry.prefabName,
        transform  = transform,
    })
    _redoStack = {}

    print("[UGCSceneData] DeleteActor: SceneID=" .. sceneID)
    return true
end

--- 修改 Actor 的 Transform
function SceneData:ModifyActor(sceneID, newTransform)
    local entry = _actors[sceneID]
    if not entry then return false end

    local oldTransform = _bridge:GetActorTransform(entry.actor)
    _bridge:SetActorTransform(entry.actor, newTransform)
    _isDirty = true

    self:PushUndo({
        op           = "Modify",
        sceneID      = sceneID,
        oldTransform = oldTransform,
        newTransform = newTransform,
    })
    _redoStack = {}
    return true
end

--- 查询 Actor 数据
function SceneData:QueryActor(sceneID)
    return _actors[sceneID]
end

--- 当前 Actor 总数
function SceneData:Count()
    local n = 0
    for _ in pairs(_actors) do n = n + 1 end
    return n
end

--- 遍历所有 Actor（callback(entry)）
function SceneData:ForEach(callback)
    for _, entry in pairs(_actors) do
        callback(entry)
    end
end

--- 返回全部 Actor 表（供 EditorCore:setAllTriggerZoneDebugVisible 等使用）
function SceneData:GetAllActors()
    return _actors
end

--============================================================
-- 撤销/重做
--============================================================

function SceneData:PushUndo(record)
    table.insert(_undoStack, record)
    if #_undoStack > UNDO_MAX then
        table.remove(_undoStack, 1)
    end
end

function SceneData:Undo()
    if #_undoStack == 0 then
        print("[UGCSceneData] 撤销栈为空")
        return false
    end

    local record = table.remove(_undoStack)
    table.insert(_redoStack, record)

    if record.op == "Create" then
        -- 撤销创建 = 删除（不入撤销栈）
        local entry = _actors[record.sceneID]
        if entry then
            _bridge:DestroyActor(entry.actor)
            _actors[record.sceneID] = nil
        end

    elseif record.op == "Delete" then
        -- 撤销删除 = 重新 Spawn
        local path = PrefabRegistry.GetPath(record.prefabName)
        local loc, rot, _ = UE.UKismetMathLibrary.BreakTransform(record.transform)
        local actor = _bridge:SpawnPlaceable(path, loc, rot)
        if actor then
            _bridge:SetActorTransform(actor, record.transform)
            _actors[record.sceneID] = {
                actor      = actor,
                prefabName = record.prefabName,
                sceneID    = record.sceneID,
            }
        end

    elseif record.op == "Modify" then
        -- 撤销修改 = 恢复旧 Transform
        local entry = _actors[record.sceneID]
        if entry then
            _bridge:SetActorTransform(entry.actor, record.oldTransform)
        end
    end

    _isDirty = true
    print("[UGCSceneData] Undo: " .. record.op)
    return true
end

function SceneData:Redo()
    if #_redoStack == 0 then
        print("[UGCSceneData] 重做栈为空")
        return false
    end

    local record = table.remove(_redoStack)

    if record.op == "Create" then
        -- Redo Create：重新 Spawn 并用原 sceneID（2026-04-16 补全）
        -- 注意：record 需包含 prefabName 和 transform，旧记录若缺失则跳过
        if not record.prefabName or not record.transform then
            print("[UGCSceneData] Redo Create: 旧记录缺少 prefabName/transform，无法恢复，跳过")
        else
            local path = PrefabRegistry.GetPath(record.prefabName)
            if path then
                local loc, rot, _ = UE.UKismetMathLibrary.BreakTransform(record.transform)
                local actor = _bridge:SpawnPlaceable(path, loc, rot)
                if actor then
                    _bridge:SetActorTransform(actor, record.transform)
                    _actors[record.sceneID] = {
                        actor      = actor,
                        prefabName = record.prefabName,
                        sceneID    = record.sceneID,
                        actorId    = "actor_" .. record.sceneID,
                        programId  = "actor_prog_" .. record.sceneID,
                    }
                    pcall(function() actor:SetProgramID("actor_prog_" .. tostring(record.sceneID)) end)
                    pcall(function() actor:SetDebugVisible(true) end)
                else
                    print("[UGCSceneData] Redo Create: Spawn 失败 prefab=" .. tostring(record.prefabName))
                end
            else
                print("[UGCSceneData] Redo Create: 预制体路径不存在 " .. tostring(record.prefabName))
            end
        end

    elseif record.op == "Delete" then
        local entry = _actors[record.sceneID]
        if entry then
            _bridge:DestroyActor(entry.actor)
            _actors[record.sceneID] = nil
        end

    elseif record.op == "Modify" then
        local entry = _actors[record.sceneID]
        if entry then
            _bridge:SetActorTransform(entry.actor, record.newTransform)
        end
    end

    _isDirty = true
    table.insert(_undoStack, record)
    print("[UGCSceneData] Redo: " .. record.op)
    return true
end

--============================================================
-- scene.json 序列化（v2）
-- 格式：{
--   "version": 2,
--   "nextID":  N,
--   "levelProgramId": "level_main",
--   "actors": [
--     { "sceneID":1, "actorId":"actor_1", "programId":"actor_prog_1",
--       "prefab":"Box", "t":[px,py,pz,pitch,yaw,roll,sx,sy,sz] },
--     ...
--   ]
-- }
--============================================================

function SceneData:SerializeToJSON()
    local actorList = {}

    for sceneID, entry in pairs(_actors) do
        if entry.actor and UE.UKismetSystemLibrary.IsValid(entry.actor) then
            local t             = _bridge:GetActorTransform(entry.actor)
            local loc, rot, scl = UE.UKismetMathLibrary.BreakTransform(t)

            local rec = {
                sceneID   = sceneID,
                actorId   = entry.actorId   or ("actor_" .. sceneID),
                programId = entry.programId or ("actor_prog_" .. sceneID),
                prefab    = entry.prefabName,
                t         = { loc.X, loc.Y, loc.Z,
                              rot.Pitch, rot.Yaw, rot.Roll,
                              scl.X, scl.Y, scl.Z },
            }
            -- 外部生成 Actor（PCG 等）：写入元数据，加载时由对应模块重放
            if entry.external then
                rec.external = true
                rec.metadata = entry.metadata or {}
            end
            table.insert(actorList, rec)
        end
    end

    local root = {
        version        = 2,
        nextID         = _nextID,
        levelProgramId = "level_main",
        actors         = actorList,
    }
    return UGCSerialize.encode(root)
end

function SceneData:DeserializeFromJSON(json)
    self:Clear()

    local data = UGCSerialize.decode(json)
    if not data then
        print("[UGCSceneData] DeserializeFromJSON: decode 失败")
        return
    end

    local ver = data.version or 1
    if data.nextID then _nextID = data.nextID end

    -- v1 格式（旧版）：actors 为整数 id
    -- v2 格式：actors 含 actorId / programId 字符串
    -- v2+：a.external=true 时表示外部生成（PCG 等），由 metadata.kind 路由到对应模块重放
    for _, a in ipairs(data.actors or {}) do
        local sceneID  = a.sceneID or a.id   -- 兼容 v1 的 "id" 字段
        local prefab   = a.prefab or a.prefabName
        local nums     = a.t or {}

        if a.external and a.metadata then
            -- 外部 Actor 重放：当前支持 PCG，后续可扩展其他类型
            local md = a.metadata
            if md.kind == "pcg" then
                local UGCRegistry = require("Gameplay.UGC.UGCFunctionRegistry")
                local ok, msg = UGCRegistry:Call("pcg_generate", {
                    x          = tonumber(md.x) or 0,
                    y          = tonumber(md.y) or 0,
                    z          = tonumber(md.z) or 0,
                    radius     = tonumber(md.radius) or 1000,
                    seed       = tonumber(md.seed) or 0,
                    graph_path = tostring(md.graph_path or ""),
                })
                if not ok then
                    print("[UGCSceneData] PCG 重放失败 SceneID=" .. tostring(sceneID) .. ": " .. tostring(msg))
                end
            else
                print("[UGCSceneData] 未知外部 Actor 类型: " .. tostring(md.kind))
            end

        elseif sceneID and prefab and #nums == 9 then
            local loc       = UE.FVector(nums[1], nums[2], nums[3])
            local rot       = UE.FRotator(nums[4], nums[5], nums[6])
            local scl       = UE.FVector(nums[7], nums[8], nums[9])
            local transform = UE.UKismetMathLibrary.MakeTransform(loc, rot, scl)

            local path = PrefabRegistry.GetPath(prefab)
            if path then
                local actor = _bridge:SpawnPlaceable(path, loc, rot)
                if actor then
                    _bridge:SetActorTransform(actor, transform)
                    _actors[sceneID] = {
                        actor      = actor,
                        prefabName = prefab,
                        sceneID    = sceneID,
                        actorId    = a.actorId   or ("actor_" .. sceneID),
                        programId  = a.programId or ("actor_prog_" .. sceneID),
                    }
                    print("[UGCSceneData] 加载 Actor: " .. prefab .. " ID=" .. sceneID)
                end
            end
        end
    end

    _isDirty = false   -- 刚加载的数据视为"已保存"
    collectgarbage("collect")   -- 清掉 DeserializeFromJSON 产生的临时 userdata，
                                -- 避免延迟到下次 widget Create 的内存分配时触发 GC
    print("[UGCSceneData] 反序列化完成 v" .. ver .. "，Actor 数量: " .. self:Count())
end

--============================================================
-- programs.json 序列化
-- 格式：{
--   "version": 1,
--   "programs": {
--     "level_main":      { nodes={...}, connections={...}, nextID=N },
--     "actor_prog_1":    { ... },
--     ...
--   }
-- }
--============================================================

function SceneData:SerializeProgramsJSON()
    local root = {
        version  = 1,
        programs = _scripts,
    }
    return UGCSerialize.encode(root)
end

function SceneData:DeserializeProgramsJSON(json)
    if not json or json == "" then return end
    local data = UGCSerialize.decode(json)
    if data and data.programs then
        _scripts = data.programs
        _isDirty = false   -- 刚加载的脚本视为"已保存"
        print("[UGCSceneData] programs 加载完成，图数量: " .. (function()
            local n = 0; for _ in pairs(_scripts) do n = n + 1 end; return n
        end)())
    end
end

--============================================================
-- editor.json 序列化（UI 状态存根，Day 5 扩展）
-- 当前只存版本号，后续可加展开/折叠、摄像机位置等
--============================================================

function SceneData:SerializeEditorJSON()
    return UGCSerialize.encode({ version = 1, blueprintEditors = {} })
end

function SceneData:DeserializeEditorJSON(json)
    -- 暂无需恢复的 UI 状态
end

return SceneData
