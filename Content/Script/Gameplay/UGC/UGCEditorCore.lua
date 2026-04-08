--[[
    UGCEditorCore.lua
    编辑器状态机（Lua 单例）

    状态：
      Idle      → 未进入编辑器
      Edit      → 编辑模式（放置/选中/移动）
      Play      → 试玩模式（正常 FPS 游戏逻辑）

    对标元梦之星的 UGCEditorGameV2，业务逻辑全在 Lua。
    C++ EditorBridge 只提供 SpawnPlaceable / LineTraceScreen 等原子操作。
]]

local UIManager       = require("Gameplay.Core.UIManager")
local PrefabRegistry  = require("Gameplay.UGC.UGCPrefabRegistry")
local SceneData       = require("Gameplay.UGC.UGCSceneData")

local EditorCore = {}
EditorCore.__index = EditorCore

--============================================================
-- 内部状态
--============================================================

local State = { Idle = "Idle", Edit = "Edit", Play = "Play" }

local _pc              = nil
local _bridge          = nil   -- UUGCEditorBridge
local _currentState    = State.Idle
local _selectedID      = nil   -- 当前选中的 SceneID
local _pendingPrefab   = nil   -- 待放置的预制体名，nil=选择模式
local _onStateChanged  = nil   -- 外部监听回调

--============================================================
-- 初始化
--============================================================

function EditorCore:Init(playerController)
    _pc     = playerController
    _bridge = playerController:GetUGCEditorBridge()

    SceneData:Init(_bridge)

    print("[UGCEditorCore] 初始化完成")
end

--============================================================
-- 状态切换
--============================================================

function EditorCore:GetState()
    return _currentState
end

function EditorCore:EnterEditMode()
    if _currentState == State.Edit then return end
    _currentState  = State.Edit
    _selectedID    = nil
    _pendingPrefab = nil

    if _pc then
        _pc.bShowMouseCursor = true
    end

    UIManager:OpenWindow("WBP_UGCEditor")

    if _onStateChanged then _onStateChanged(State.Edit) end
    print("[UGCEditorCore] 进入编辑模式")
end

function EditorCore:EnterPlayMode()
    if _currentState == State.Play then return end

    -- 取消选中
    self:ClearSelection()
    _pendingPrefab = nil
    _currentState  = State.Play

    if _pc then
        _pc.bShowMouseCursor = false
    end

    UIManager:CloseWindow("WBP_UGCEditor")

    if _onStateChanged then _onStateChanged(State.Play) end
    print("[UGCEditorCore] 进入试玩模式")
end

function EditorCore:ToggleEditMode()
    if _currentState == State.Edit then
        self:EnterPlayMode()
    else
        self:EnterEditMode()
    end
end

--- 注册状态切换监听（供 WBP_UGCEditor 按钮刷新用）
function EditorCore:OnStateChanged(callback)
    _onStateChanged = callback
end

--============================================================
-- 预制体放置
--============================================================

--- 选择要放置的预制体（点击 UI 预制体列表后调用）
function EditorCore:SelectPrefab(prefabName)
    if not PrefabRegistry.IsValid(prefabName) then
        print("[UGCEditorCore] 未知预制体: " .. tostring(prefabName))
        return
    end
    _pendingPrefab = prefabName
    self:ClearSelection()
    print("[UGCEditorCore] 准备放置: " .. prefabName)
end

--- 取消放置模式，切回选择模式
function EditorCore:CancelPrefab()
    _pendingPrefab = nil
end

function EditorCore:GetPendingPrefab()
    return _pendingPrefab
end

--============================================================
-- 点击处理（由 WBP_UGCEditor 的 Viewport MouseDown 触发）
-- screenX / screenY 为屏幕像素坐标
--============================================================

function EditorCore:OnViewportClick(screenX, screenY)
    if _currentState ~= State.Edit then return end

    if _pendingPrefab then
        -- 放置模式：在点击位置生成 Actor
        local hitActor = _bridge:LineTraceScreen(screenX, screenY)
        local loc

        if hitActor then
            -- 放在命中点的表面上方
            loc = hitActor:GetActorLocation()
            loc.Z = loc.Z + 50  -- 抬高半个单位，避免穿地
        else
            -- 没命中地面，放在固定高度
            loc = UE.FVector(0, 0, 100)
        end

        local sceneID, actor = SceneData:CreateActor(_pendingPrefab, loc)
        if actor then
            self:SelectByID(sceneID)
        end
        -- 放置后继续保持放置模式（和大多数关卡编辑器一致）

    else
        -- 选择模式：点击选中/取消选中
        local hitActor = _bridge:LineTraceScreen(screenX, screenY)
        if hitActor then
            -- 找出对应的 SceneID
            local found = nil
            SceneData:ForEach(function(entry)
                if entry.actor == hitActor then
                    found = entry.sceneID
                end
            end)
            if found then
                self:SelectByID(found)
            else
                self:ClearSelection()
            end
        else
            self:ClearSelection()
        end
    end
end

--============================================================
-- 选中管理
--============================================================

function EditorCore:SelectByID(sceneID)
    -- 取消旧选中的高亮
    if _selectedID then
        local old = SceneData:QueryActor(_selectedID)
        if old and old.actor then
            _bridge:SetActorHighlight(old.actor, false)
        end
    end

    _selectedID = sceneID
    local entry = SceneData:QueryActor(sceneID)
    if entry and entry.actor then
        _bridge:SetActorHighlight(entry.actor, true)
    end

    print("[UGCEditorCore] 选中 SceneID=" .. tostring(sceneID))
end

function EditorCore:ClearSelection()
    if _selectedID then
        local entry = SceneData:QueryActor(_selectedID)
        if entry and entry.actor then
            _bridge:SetActorHighlight(entry.actor, false)
        end
        _selectedID = nil
    end
end

function EditorCore:GetSelectedID()
    return _selectedID
end

function EditorCore:GetSelectedEntry()
    if not _selectedID then return nil end
    return SceneData:QueryActor(_selectedID)
end

--============================================================
-- 删除选中
--============================================================

function EditorCore:DeleteSelected()
    if not _selectedID then return end
    local id = _selectedID
    _selectedID = nil
    SceneData:DeleteActor(id)
end

--============================================================
-- Transform 修改（由 UI 输入框触发）
--============================================================

function EditorCore:SetSelectedTransform(x, y, z, pitch, yaw, roll, sx, sy, sz)
    if not _selectedID then return end
    local loc = UE.FVector(x, y, z)
    local rot = UE.FRotator(pitch, yaw, roll)
    local scl = UE.FVector(sx, sy, sz)
    local t   = UE.FTransform(rot:Quaternion(), loc, scl)
    SceneData:ModifyActor(_selectedID, t)
end

--- 读取当前选中的 Transform（返回 6 个数字：x,y,z,p,y,r）
function EditorCore:GetSelectedTransformValues()
    if not _selectedID then return 0,0,0, 0,0,0, 1,1,1 end
    local entry = SceneData:QueryActor(_selectedID)
    if not entry or not entry.actor then return 0,0,0, 0,0,0, 1,1,1 end

    local t   = _bridge:GetActorTransform(entry.actor)
    local loc = t:GetLocation()
    local rot = t:Rotator()
    local scl = t:GetScale3D()
    return loc.X, loc.Y, loc.Z, rot.Pitch, rot.Yaw, rot.Roll, scl.X, scl.Y, scl.Z
end

--============================================================
-- 撤销/重做（快捷键由 UGCPlayerController.lua 绑定）
--============================================================

function EditorCore:Undo()
    if _currentState ~= State.Edit then return end
    SceneData:Undo()
    self:ClearSelection()
end

function EditorCore:Redo()
    if _currentState ~= State.Edit then return end
    SceneData:Redo()
end

--============================================================
-- 保存/加载（JSON，对接 UGCAssetClient）
--============================================================

function EditorCore:SaveSceneJSON()
    return SceneData:SerializeToJSON()
end

function EditorCore:LoadSceneJSON(json)
    self:ClearSelection()
    SceneData:DeserializeFromJSON(json)
end

function EditorCore:ClearScene()
    self:ClearSelection()
    SceneData:Clear()
end

return EditorCore
