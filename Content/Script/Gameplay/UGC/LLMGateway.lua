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
    -- 回调路由：UGCHttpClient → AUGCPlayerController::OnLLMResponse/OnLLMError
    --           → UGCPlayerController.lua:OnLLMResponse/OnLLMError
    --           → Gateway:OnResponse/OnError（在 UGCPlayerController.lua 里显式调用）
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

    -- OpenAI 兼容格式（DeepSeek）：
    -- tool_calls: choices[0].message.tool_calls[].function.{name, arguments(JSON string)}
    -- 文字回复:   choices[0].message.content

    local toolCalls = {}

    -- 找所有 "function": { "name": "...", "arguments": "..." } 段
    local pos = 1
    while true do
        local s, e, name = responseJSON:find('"name"%s*:%s*"([^"]+)"', pos)
        if not s then break end

        -- 往后找 "arguments": "..."（值是转义的 JSON 字符串）
        local argS, argE, argsRaw = responseJSON:find('"arguments"%s*:%s*"(.-[^\\])"', e)
        if argS then
            -- 反转义：\" → "，\\ → \，\n → 换行
            local argsJSON = argsRaw:gsub('\\"', '"'):gsub('\\\\', '\\'):gsub('\\n', '\n')
            table.insert(toolCalls, { name = name, inputJSON = argsJSON })
            pos = argE + 1
        else
            pos = e + 1
        end
    end

    if #toolCalls == 0 then
        -- 没有函数调用，提取文字回复 choices[0].message.content
        local text = responseJSON:match('"content"%s*:%s*"(.-[^\\])"')
        if text then
            text = text:gsub('\\"', '"'):gsub('\\n', '\n')
            print("[LLMGateway] 文字回复: " .. text)
            if _onResult then _onResult(true, text) end
        else
            if _onResult then _onResult(false, "无法解析响应") end
        end
        _onResult = nil
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
