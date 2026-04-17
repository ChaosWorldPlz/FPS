--[[
    AnimAgentCore.lua
    AnimAgent 顶层编排器（Phase 1 极简版）

    职责：
    - 持有 UAnimGenClient（C++ 组件）的引用
    - 注入 API Key（运行时从配置读取，不持久化到蓝图）
    - 订阅 OnJobUpdated / OnJobCompleted / OnJobFailed
    - Job 完成后：
        ① 写入 AnimAssetLibrary
        ② 调 UAnimImportBridge 导入为 UStaticMesh
        ③ 注册到 UGCPrefabRegistry 的 dyn:{uuid} 命名空间
    - 提供 Lua 侧 API 给 UI 调用：
        Core:Generate(provider, prompt, opts) → uuid
        Core:GetJobStatus(uuid)
        Core:CancelJob(uuid)

    依赖：
    - PlayerController 必须挂 UAnimGenClient（蓝图配置）
    - 可选挂 UAnimImportBridge（无则跳过导入步骤）

    用法（UI / LLM 工具）：
        local Core = require("Gameplay.AnimAgent.AnimAgentCore")
        Core:Init(playerController)
        Core:SetApiKey("meshy", "msy_xxx")
        local uuid = Core:Generate("meshy", "a fire sword", { style = "realistic" })
]]

local Library  = require("Gameplay.AnimAgent.AnimAssetLibrary")
local Registry = require("Gameplay.UGC.UGCPrefabRegistry")

local Core = {}

--============================================================
-- 内部状态
--============================================================

local _pc       = nil
local _client   = nil   -- UAnimGenClient*
local _import   = nil   -- UAnimImportBridge*  (可选)
local _initialized = false

-- uuid → { prompt, provider, name, on_done }
local _jobMeta = {}

local PROVIDER_ENUM = {
    mock  = 0,    -- EAnimGenProvider::Mock
    meshy = 1,
    tripo = 2,
}

local function _providerEnum(name)
    return PROVIDER_ENUM[string.lower(name or "mock")] or 0
end

local function _providerName(enumVal)
    for k, v in pairs(PROVIDER_ENUM) do
        if v == enumVal then return k end
    end
    return "mock"
end

--============================================================
-- 初始化 / 反初始化
--============================================================

--- @param playerController APlayerController*
--- @param opts? { import_bridge?: UAnimImportBridge*, mock_glb?: string }
function Core:Init(playerController, opts)
    if _initialized then return true end
    if not playerController then
        print("[AnimAgentCore] Init 失败：playerController 为 nil")
        return false
    end

    -- 优先用 GetComponentByClass 兜底（蓝图未生成 getter 时）
    local client = nil
    pcall(function()
        client = playerController:GetAnimGenClient()
    end)
    if not client then
        pcall(function()
            client = playerController:GetComponentByClass(UE.UAnimGenClient)
        end)
    end
    if not client then
        print("[AnimAgentCore] Init 失败：未找到 UAnimGenClient 组件")
        return false
    end

    _pc     = playerController
    _client = client
    _import = opts and opts.import_bridge or nil

    if opts and opts.mock_glb then
        _client.MockSampleGLBPath = opts.mock_glb
    end

    Library:Init()

    -- 订阅事件
    _client.OnJobUpdated:Add(self, Core.OnJobUpdated)
    _client.OnJobCompleted:Add(self, Core.OnJobCompleted)
    _client.OnJobFailed:Add(self, Core.OnJobFailed)

    _initialized = true
    print("[AnimAgentCore] 初始化完成")
    return true
end

function Core:Shutdown()
    if not _initialized then return end
    if _client then
        pcall(function() _client.OnJobUpdated:Remove(self, Core.OnJobUpdated) end)
        pcall(function() _client.OnJobCompleted:Remove(self, Core.OnJobCompleted) end)
        pcall(function() _client.OnJobFailed:Remove(self, Core.OnJobFailed) end)
    end
    _client, _pc, _import = nil, nil, nil
    _jobMeta = {}
    _initialized = false
end

function Core:IsReady()
    return _initialized and _client ~= nil
end

--============================================================
-- 配置
--============================================================

--- 注入 API Key（不持久化；玩家在 UI 输入后调用）
function Core:SetApiKey(provider, key)
    if not _client then return end
    local p = string.lower(provider or "")
    if p == "meshy" then
        _client.MeshyApiKey = key or ""
    elseif p == "tripo" then
        _client.TripoApiKey = key or ""
    end
end

function Core:GetApiKey(provider)
    if not _client then return "" end
    local p = string.lower(provider or "")
    if p == "meshy" then return _client.MeshyApiKey end
    if p == "tripo" then return _client.TripoApiKey end
    return ""
end

--============================================================
-- 公共 API
--============================================================

--- 创建生成任务
--- @param provider "mock" | "meshy" | "tripo"
--- @param prompt string
--- @param opts? { name?, negative_prompt?, style?, polycount?, on_done?(uuid, glbPath, success, err) }
--- @return string uuid（失败返回 ""）
function Core:Generate(provider, prompt, opts)
    if not self:IsReady() then
        print("[AnimAgentCore] Generate 失败：未初始化")
        return ""
    end
    if not prompt or prompt == "" then
        print("[AnimAgentCore] Generate 失败：prompt 为空")
        return ""
    end

    opts = opts or {}

    local req = UE.FAnimGenRequest()
    req.Prompt          = prompt
    req.NegativePrompt  = opts.negative_prompt or ""
    req.TargetPolycount = opts.polycount or 30000
    req.bWithTexture    = opts.with_texture ~= false
    req.bPBR            = opts.pbr ~= false

    if opts.style == "cartoon" then
        req.Style = UE.EAnimGenStyle.Cartoon
    elseif opts.style == "sculpture" then
        req.Style = UE.EAnimGenStyle.Sculpture
    else
        req.Style = UE.EAnimGenStyle.Realistic
    end

    local uuid = _client:CreateTask(_providerEnum(provider), req)
    if not uuid or uuid == "" then
        print("[AnimAgentCore] CreateTask 返回空 uuid")
        return ""
    end

    _jobMeta[uuid] = {
        prompt   = prompt,
        provider = string.lower(provider or "mock"),
        name     = opts.name or prompt,
        on_done  = opts.on_done,
    }

    print(string.format("[AnimAgentCore] Generate uuid=%s provider=%s prompt=%s",
        uuid, provider, prompt))
    return uuid
end

function Core:GetJobStatus(uuid)
    if not _client then return nil end
    return _client:GetJobStatus(uuid)
end

function Core:CancelJob(uuid)
    if not _client then return end
    _client:CancelJob(uuid)
end

function Core:ListActiveJobs()
    if not _client then return {} end
    local arr = _client:ListActiveJobs()
    local out = {}
    for i = 1, arr:Num() do out[i] = arr:Get(i - 1) end
    return out
end

--============================================================
-- 事件回调（C++ → Lua）
--============================================================

function Core:OnJobUpdated(uuid, status)
    -- status: FAnimGenJobStatus { State, Progress, PreviewURL, ModelURL, ErrorMessage }
    local meta = _jobMeta[uuid]
    print(string.format("[AnimAgentCore] OnJobUpdated uuid=%s state=%d progress=%d",
        uuid, status.State, status.Progress))
end

function Core:OnJobCompleted(uuid, glbPath)
    local meta = _jobMeta[uuid] or {}
    print(string.format("[AnimAgentCore] OnJobCompleted uuid=%s path=%s", uuid, glbPath))

    -- ① 写入资产库
    Library:Add({
        uuid       = uuid,
        name       = meta.name or uuid,
        prompt     = meta.prompt or "",
        provider   = meta.provider or "mock",
        glb_path   = glbPath,
        created_at = os.time(),
    })

    -- ② 注册到 UGCPrefabRegistry 的 dyn: 命名空间
    Registry:RegisterDynamicGLB({
        uuid     = uuid,
        name     = meta.name or uuid,
        glb_path = glbPath,
        provider = meta.provider or "mock",
        prompt   = meta.prompt or "",
    })

    -- ③ 触发运行时导入（若 import bridge 可用）
    if _import then
        pcall(function() _import:ImportGLBAsync(uuid, glbPath) end)
    end

    if meta.on_done then
        pcall(meta.on_done, uuid, glbPath, true, nil)
    end
end

function Core:OnJobFailed(uuid, errorMessage)
    print(string.format("[AnimAgentCore] OnJobFailed uuid=%s err=%s", uuid, errorMessage))
    local meta = _jobMeta[uuid]
    if meta and meta.on_done then
        pcall(meta.on_done, uuid, nil, false, errorMessage)
    end
end

return Core
