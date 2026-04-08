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

local PrefabRegistry = require("Gameplay.UGC.UGCPrefabRegistry")

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
    print("[UGCSceneData] 场景已清空")
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
    local rot  = rotation or UE.FRotator(0, 0, 0)
    local actor = _bridge:SpawnPlaceable(path, location, rot)
    if not actor then
        print("[UGCSceneData] CreateActor: Spawn 失败")
        return nil, nil
    end

    local sceneID = _nextID
    _nextID = _nextID + 1

    local entry = {
        actor      = actor,
        prefabName = prefabName,
        sceneID    = sceneID,
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
        local loc  = record.transform:GetLocation()
        local rot  = record.transform:Rotator()
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
-- JSON 序列化
-- 输出格式：{"version":1,"actors":[{"id":1,"prefab":"Box","t":[x,y,z,px,py,pz,sx,sy,sz]},...]}
-- Transform 编码：[qx,qy,qz,qw, px,py,pz, sx,sy,sz]
--============================================================

function SceneData:SerializeToJSON()
    local actorParts = {}

    for sceneID, entry in pairs(_actors) do
        if entry.actor and UE.UKismetSystemLibrary.IsValid(entry.actor) then
            local t   = _bridge:GetActorTransform(entry.actor)
            local loc = t:GetLocation()
            local rot = t:GetRotation()  -- FQuat
            local scl = t:GetScale3D()

            table.insert(actorParts, string.format(
                '{"id":%d,"prefab":"%s","t":[%f,%f,%f,%f,%f,%f,%f,%f,%f,%f]}',
                sceneID,
                entry.prefabName,
                rot.X, rot.Y, rot.Z, rot.W,
                loc.X, loc.Y, loc.Z,
                scl.X, scl.Y, scl.Z
            ))
        end
    end

    local json = string.format(
        '{"version":1,"nextID":%d,"actors":[%s]}',
        _nextID,
        table.concat(actorParts, ",")
    )
    return json
end

function SceneData:DeserializeFromJSON(json)
    self:Clear()

    -- 解析 nextID
    local nextID = json:match('"nextID"%s*:%s*(%d+)')
    if nextID then _nextID = tonumber(nextID) end

    -- 逐条解析 actor 记录
    -- 格式：{"id":N,"prefab":"Name","t":[10 个数字]}
    for idStr, prefab, tData in json:gmatch('"id"%s*:%s*(%d+)%s*,%s*"prefab"%s*:%s*"([^"]+)"%s*,%s*"t"%s*:%s*%[([^%]]+)%]') do
        local sceneID = tonumber(idStr)
        local nums = {}
        for n in tData:gmatch("(%-?%d+%.?%d*e?[%+%-]?%d*)") do
            table.insert(nums, tonumber(n))
        end
        if #nums == 10 then
            local qx,qy,qz,qw = nums[1],nums[2],nums[3],nums[4]
            local px,py,pz    = nums[5],nums[6],nums[7]
            local sx,sy,sz    = nums[8],nums[9],nums[10]

            local loc = UE.FVector(px, py, pz)
            local rot = UE.FQuat(qx, qy, qz, qw)
            local scl = UE.FVector(sx, sy, sz)
            local transform = UE.FTransform(rot, loc, scl)

            local path = PrefabRegistry.GetPath(prefab)
            if path then
                local rotator = transform:Rotator()
                local actor = _bridge:SpawnPlaceable(path, loc, rotator)
                if actor then
                    _bridge:SetActorTransform(actor, transform)
                    _actors[sceneID] = {
                        actor      = actor,
                        prefabName = prefab,
                        sceneID    = sceneID,
                    }
                    print("[UGCSceneData] 加载 Actor: " .. prefab .. " ID=" .. sceneID)
                end
            end
        end
    end

    print("[UGCSceneData] 反序列化完成，Actor 数量: " .. self:Count())
end

return SceneData
