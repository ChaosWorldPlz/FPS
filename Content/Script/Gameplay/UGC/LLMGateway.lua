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
local json     = require("Gameplay.UGC.json")

local Gateway = {}
Gateway.__index = Gateway

--============================================================
-- 内部状态
--============================================================

local _pc         = nil
local _httpClient = nil
local _onResult   = nil  -- 结果回调 function(success, message)

-- 多轮对话历史
-- 每轮 = 1 条 user + 1 条 assistant（可能含 tool_calls + tool results）
-- FIFO 策略：超过 MAX_HISTORY_ROUNDS 轮时丢弃最早的
local _history = {}
local MAX_HISTORY_ROUNDS = 20  -- 保留最近 20 轮对话

-- 持久化路径（Init 时确定）
local _historyPath = nil

local function _getSavedDir()
    return UE.UKismetSystemLibrary.GetProjectDirectory() .. "Saved/UGC/"
end

local function _ensureDir()
    pcall(function() UE.UKismetSystemLibrary.MakeDirectory(_getSavedDir()) end)
end

--============================================================
-- 初始化
--============================================================

function Gateway:Init(playerController)
    _pc          = playerController
    _httpClient  = playerController:GetUGCHttpClient()
    _historyPath = _getSavedDir() .. "chat_history.json"
    -- 尝试恢复持久化历史；失败则新建
    if not Gateway:LoadHistory() then
        _history = {}
    end
    print("[LLMGateway] 初始化完成，历史条数: " .. #_history)
end

--- 持久化 LLM 对话历史到文件
function Gateway:SaveHistory()
    if not _historyPath then return end
    _ensureDir()
    local f = io.open(_historyPath, "w")
    if f then
        f:write(json.encode(_history))
        f:close()
    end
end

--- 从文件恢复 LLM 对话历史，返回 bool
function Gateway:LoadHistory()
    if not _historyPath then return false end
    local f = io.open(_historyPath, "r")
    if not f then return false end
    local content = f:read("*a")
    f:close()
    local h = json.decode(content)
    if type(h) == "table" then
        _history = h
        return true
    end
    return false
end

--- 清空对话历史（场景重置时调用）
function Gateway:ClearHistory()
    _history = {}
    -- 同步删除持久化文件
    if _historyPath then
        pcall(function()
            local f = io.open(_historyPath, "w")
            if f then f:write("[]"); f:close() end
        end)
    end
    print("[LLMGateway] 对话历史已清空")
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

    -- 追加 user message 到历史
    table.insert(_history, { role = "user", content = tostring(userMessage) })

    -- 构建完整 messages 数组（system + history）
    -- system prompt 由 C++ UGCHttpClient.SystemPrompt 提供，这里通过 Lua 读取
    local messages = {}

    -- system prompt（从 C++ 组件属性读取）
    local systemPrompt = _httpClient.SystemPrompt
    if systemPrompt and systemPrompt ~= "" then
        table.insert(messages, { role = "system", content = systemPrompt })
    end

    -- 历史消息（FIFO 裁剪：每轮 = 若干条 message，按 MAX_HISTORY_ROUNDS 轮计）
    Gateway:_TrimHistory()
    for _, msg in ipairs(_history) do
        table.insert(messages, msg)
    end

    local messagesJSON = json.encode(messages)
    _httpClient:SendMessageWithHistory(messagesJSON, schemasJSON)
end

--- 裁剪历史：保留最近 MAX_HISTORY_ROUNDS 轮
--- 一轮 = 1 个 user + 后续非 user messages（assistant / tool）
function Gateway:_TrimHistory()
    -- 计算轮数（以 user message 为分界）
    local rounds = 0
    for _, msg in ipairs(_history) do
        if msg.role == "user" then
            rounds = rounds + 1
        end
    end

    -- 超出限制时，从头删除最早的完整轮
    while rounds > MAX_HISTORY_ROUNDS do
        -- 删掉第一个 user + 其后续 assistant/tool messages
        table.remove(_history, 1)
        -- 继续删非 user 消息（同一轮的 assistant / tool 回复）
        while #_history > 0 and _history[1].role ~= "user" do
            table.remove(_history, 1)
        end
        rounds = rounds - 1
    end
end

--============================================================
-- 响应解析
--============================================================

function Gateway:OnResponse(responseJSON)
    print("[LLMGateway] 收到响应，开始解析")

    -- OpenAI 兼容格式（DeepSeek / Qwen）：
    -- tool_calls: choices[0].message.tool_calls[].function.{name, arguments(JSON string)}
    -- 文字回复:   choices[0].message.content

    local data = json.decode(responseJSON)
    if not data then
        print("[LLMGateway] JSON 解析失败，尝试 fallback")
        if _onResult then _onResult(false, "无法解析 LLM 响应") end
        _onResult = nil
        return
    end

    -- 提取 message 对象
    local message = nil
    if data.choices and data.choices[1] and data.choices[1].message then
        message = data.choices[1].message
    end
    if not message then
        if _onResult then _onResult(false, "响应格式异常：缺少 choices[0].message") end
        _onResult = nil
        return
    end

    -- 提取 tool_calls
    local toolCalls = {}
    if message.tool_calls and #message.tool_calls > 0 then
        for _, tc in ipairs(message.tool_calls) do
            if tc["function"] then
                local name = tc["function"].name
                local argsStr = tc["function"].arguments or "{}"
                -- arguments 可能是字符串（需要二次 decode）或已被解析为 table
                local params
                if type(argsStr) == "string" then
                    params = json.decode(argsStr) or {}
                elseif type(argsStr) == "table" then
                    params = argsStr
                else
                    params = {}
                end
                table.insert(toolCalls, { name = name, params = params, id = tc.id })
            end
        end
    end

    if #toolCalls == 0 then
        -- 没有函数调用，提取文字回复
        local text = message.content
        if text and text ~= "" then
            print("[LLMGateway] 文字回复: " .. tostring(text))
            -- 追加 assistant 回复到历史
            table.insert(_history, { role = "assistant", content = text })
            if _onResult then _onResult(true, text) end
        else
            if _onResult then _onResult(false, "LLM 未返回有效内容") end
        end
        _onResult = nil
        return
    end

    -- 追加 assistant tool_calls 到历史（OpenAI 格式要求）
    local assistantMsg = { role = "assistant", content = message.content or "" }
    local tcForHistory = {}
    for _, call in ipairs(toolCalls) do
        table.insert(tcForHistory, {
            id = call.id or ("call_" .. call.name),
            type = "function",
            ["function"] = { name = call.name, arguments = json.encode(call.params) }
        })
    end
    assistantMsg.tool_calls = tcForHistory
    table.insert(_history, assistantMsg)

    -- 执行所有函数调用
    local allOk = true
    local messages = {}

    for _, call in ipairs(toolCalls) do
        print("[LLMGateway] 执行函数: " .. call.name .. " 参数: " .. json.encode(call.params))

        local ok, result = Registry:Call(call.name, call.params)

        if ok then
            table.insert(messages, "✓ " .. call.name .. ": " .. tostring(result))
        else
            allOk = false
            table.insert(messages, "✗ " .. call.name .. ": " .. tostring(result))
        end

        -- 追加 tool 执行结果到历史
        table.insert(_history, {
            role = "tool",
            tool_call_id = call.id or ("call_" .. call.name),
            content = tostring(result)
        })
    end

    local summary = table.concat(messages, "\n")
    if _onResult then _onResult(allOk, summary) end
    _onResult = nil
    Gateway:SaveHistory()
end

function Gateway:OnError(errorMsg)
    print("[LLMGateway] 请求错误: " .. tostring(errorMsg))
    if _onResult then
        _onResult(false, "请求失败: " .. tostring(errorMsg))
        _onResult = nil
    end
    Gateway:SaveHistory()
end

--============================================================
-- JSON 工具（备用）
--============================================================

-- 从 pos 开始提取平衡的 { } 块（紧急 fallback 时使用）
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

-- ParseInputJSON 已废弃（2026-04-16），统一使用 json.decode
-- 旧实现有 3 个 bug：
--   1. tool_calls arguments 正则截断（嵌套 JSON 里的 \" 导致提前终止）
--   2. 布尔值正则 (true|false) 在 Lua pattern 中 | 是字面量，永远不匹配
--   3. 不支持嵌套对象/数组

return Gateway
