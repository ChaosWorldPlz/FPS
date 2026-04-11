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
    print("[UGCSceneData] 初始化完成")
end

function SceneData:Clear()
    -- 销毁所有 Actor
    for _, entry in pairs(_actors) do
        if entry.actor and UE.UKismetSystemLibrary.IsValid(entry.actor) then
            _bridge:DestroyActor(entry.actor)
        end
    end
    _actors     = {}
    _nextID     = 1
    _undoStack  = {}
    _redoStack  = {}
    _scripts    = {}
    print("[UGCSceneData] 场景已清空")
end

--============================================================
-- 蓝图脚本存取
--============================================================

--- 通用脚本存取（以 programId 字符串为 key）
--- programId 示例："level_main" / "actor_prog_3"
function SceneData:SetScript(programId, data)
    _scripts[programId] = data
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

    -- 记录撤销
    self:PushUndo({ op = "Create", sceneID = sceneID })
    _redoStack = {}

    print("[UGCSceneData] CreateActor: " .. prefabName .. " SceneID=" .. sceneID)
    return sceneID, actor
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
        -- 这里需要重新 Spawn，但 Redo 后 sceneID 需复原
        -- 简化：Redo Create = 重新 Spawn 并用原 sceneID
        -- （要求 prefabName 和 transform 都记录）
        print("[UGCSceneData] Redo Create 暂不支持，跳过")

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

            table.insert(actorList, {
                sceneID   = sceneID,
                actorId   = entry.actorId   or ("actor_" .. sceneID),
                programId = entry.programId or ("actor_prog_" .. sceneID),
                prefab    = entry.prefabName,
                t         = { loc.X, loc.Y, loc.Z,
                              rot.Pitch, rot.Yaw, rot.Roll,
                              scl.X, scl.Y, scl.Z },
            })
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
    for _, a in ipairs(data.actors or {}) do
        local sceneID  = a.sceneID or a.id   -- 兼容 v1 的 "id" 字段
        local prefab   = a.prefab or a.prefabName
        local nums     = a.t or {}

        if sceneID and prefab and #nums == 9 then
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
