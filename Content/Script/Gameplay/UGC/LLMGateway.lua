--[[
    LLMGateway.lua
    LLM Function Calling 网关

    职责：
    - 接收玩家自然语言输入
    - 从 UGCFunctionRegistry 取 Schema 喂给 Claude
    - 解析 Claude 返回的 tool_use 并调用 Registry:Call()
    - 将执行结果反馈给玩家

    用法：
        local Gateway = require("Gameplay.UGC.LLMGateway")
        Gateway:Init(playerController)
        Gateway:Send("给我一个10秒冷却的火球技能")
]]

local Registry = require("Gameplay.UGC.UGCFunctionRegistry")

local Gateway = {}
Gateway.__index = Gateway

--============================================================
-- 内部状态
--============================================================

local _pc         = nil
local _httpClient = nil
local _onResult   = nil  -- 结果回调 function(success, message)

--============================================================
-- 初始化
--============================================================

function Gateway:Init(playerController)
    _pc         = playerController
    _httpClient = playerController:GetUGCHttpClient()

    -- 覆盖 C++ BlueprintNativeEvent，接管 HTTP 回调
    _httpClient.OnMessageComplete = function(self, responseJSON)
        Gateway:OnResponse(responseJSON)
    end

    _httpClient.OnMessageError = function(self, errorMsg)
        Gateway:OnError(errorMsg)
    end

    print("[LLMGateway] 初始化完成")
end

--============================================================
-- 发送请求
--============================================================

function Gateway:Send(userMessage, onResult)
    if not _httpClient then
        print("[LLMGateway] 未初始化")
        if onResult then onResult(false, "网关未初始化") end
        return
    end

    if _httpClient:IsRequestInProgress() then
        if onResult then onResult(false, "上一条消息还在处理中，请稍候") end
        return
    end

    _onResult = onResult

    local schemasJSON = Registry:GetSchemas()
    print("[LLMGateway] 发送消息: " .. tostring(userMessage))

    _httpClient:SendMessage(tostring(userMessage), schemasJSON)
end

--============================================================
-- 响应解析
--============================================================

function Gateway:OnResponse(responseJSON)
    print("[LLMGateway] 收到响应，开始解析")

    -- 简单 JSON 解析（UnLua 环境无标准 JSON 库，手动解析关键字段）
    -- Claude 响应结构：
    -- { "content": [ { "type": "tool_use", "name": "...", "input": {...} } ] }
    -- 或  { "content": [ { "type": "text", "text": "..." } ] }

    local results = {}

    -- 提取所有 tool_use 块
    for toolBlock in responseJSON:gmatch('"type"%s*:%s*"tool_use".-"name"%s*:%s*"([^"]+)".-"input"%s*:%s*(%b{})') do
        -- 这个 pattern 提取不出两个捕获，需要分步处理
    end

    -- 分步解析：先找所有 tool_use 段
    local hasToolCall = false
    for segment in responseJSON:gmatch('"type"%s*:%s*"tool_use"(.-)(?="type"|%])') do
        hasToolCall = true
    end

    -- 更可靠的方式：逐段匹配 name + input
    local toolCalls = {}
    local pos = 1
    while true do
        local s, e, name = responseJSON:find('"name"%s*:%s*"([^"]+)"', pos)
        if not s then break end
        -- 找 name 后面最近的 "input": { ... }
        local inputStart = responseJSON:find('"input"%s*:%s*%{', e)
        if inputStart then
            local braceStart = responseJSON:find('%{', inputStart)
            if braceStart then
                local inputJSON = Gateway:ExtractBalanced(responseJSON, braceStart)
                if inputJSON then
                    table.insert(toolCalls, { name = name, inputJSON = inputJSON })
                end
            end
        end
        pos = e + 1
    end

    if #toolCalls == 0 then
        -- 没有函数调用，提取纯文本回复
        local text = responseJSON:match('"type"%s*:%s*"text".-"text"%s*:%s*"(.-)"')
        if text then
            text = text:gsub('\\n', '\n'):gsub('\\"', '"')
            print("[LLMGateway] 文字回复: " .. text)
            if _onResult then _onResult(true, text) end
        else
            if _onResult then _onResult(false, "无法解析响应") end
        end
        return
    end

    -- 执行所有函数调用
    local allOk = true
    local messages = {}

    for _, call in ipairs(toolCalls) do
        print("[LLMGateway] 执行函数: " .. call.name .. " 参数: " .. call.inputJSON)

        local params = Gateway:ParseInputJSON(call.inputJSON)
        local ok, result = Registry:Call(call.name, params)

        if ok then
            table.insert(messages, "✓ " .. call.name .. ": " .. tostring(result))
        else
            allOk = false
            table.insert(messages, "✗ " .. call.name .. ": " .. tostring(result))
        end
    end

    local summary = table.concat(messages, "\n")
    if _onResult then _onResult(allOk, summary) end
    _onResult = nil
end

function Gateway:OnError(errorMsg)
    print("[LLMGateway] 请求错误: " .. tostring(errorMsg))
    if _onResult then
        _onResult(false, "请求失败: " .. tostring(errorMsg))
        _onResult = nil
    end
end

--============================================================
-- JSON 工具
--============================================================

-- 从 pos 开始提取平衡的 { } 块
function Gateway:ExtractBalanced(str, pos)
    local depth = 0
    local start = pos
    for i = pos, #str do
        local c = str:sub(i, i)
        if c == '{' then depth = depth + 1
        elseif c == '}' then
            depth = depth - 1
            if depth == 0 then
                return str:sub(start, i)
            end
        end
    end
    return nil
end

-- 简单解析 {"key":"value","key2":number} 为 Lua table
-- 仅处理字符串和数字值，满足 UGC 函数调用场景
function Gateway:ParseInputJSON(jsonStr)
    local params = {}
    -- 字符串值
    for k, v in jsonStr:gmatch('"([^"]+)"%s*:%s*"([^"]*)"') do
        params[k] = v
    end
    -- 数值
    for k, v in jsonStr:gmatch('"([^"]+)"%s*:%s*(%-?%d+%.?%d*)') do
        if params[k] == nil then  -- 不覆盖已解析的字符串
            params[k] = tonumber(v)
        end
    end
    -- 布尔值
    for k, v in jsonStr:gmatch('"([^"]+)"%s*:%s*(true|false)') do
        if params[k] == nil then
            params[k] = (v == "true")
        end
    end
    return params
end

return Gateway
