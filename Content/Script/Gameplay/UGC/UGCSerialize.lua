--[[
    UGCSerialize.lua
    最小 JSON 编码 / 解码工具（无外部依赖）

    用法：
        local S = require("Gameplay.UGC.UGCSerialize")
        local json = S.encode({ version=1, actors={{id="actor_1"}} })
        local data = S.decode(json)
]]

local UGCSerialize = {}

--============================================================
-- encode：Lua table → JSON string
-- 支持：string / number / boolean / nil / 纯 array table / object table
-- 不支持：循环引用、function、userdata
--============================================================

local function encodeValue(v)
    local t = type(v)
    if v == nil then
        return "null"
    elseif t == "boolean" then
        return tostring(v)
    elseif t == "number" then
        if v ~= v then return "null" end   -- NaN 安全
        -- 整数不带小数点
        if v == math.floor(v) and math.abs(v) < 1e15 then
            return string.format("%d", v)
        end
        return string.format("%g", v)
    elseif t == "string" then
        return '"' .. v:gsub('\\', '\\\\')
                       :gsub('"',  '\\"')
                       :gsub('\n', '\\n')
                       :gsub('\r', '\\r')
                       :gsub('\t', '\\t') .. '"'
    elseif t == "table" then
        -- 判断是否为纯 array（连续整数 key 从 1 开始）
        local n = #v
        local isArr = n > 0
        if isArr then
            for i = 1, n do
                if v[i] == nil then isArr = false; break end
            end
        end

        if isArr then
            local parts = {}
            for i = 1, n do
                parts[i] = encodeValue(v[i])
            end
            return "[" .. table.concat(parts, ",") .. "]"
        else
            local parts = {}
            for k, val in pairs(v) do
                if type(k) == "string" or type(k) == "number" then
                    table.insert(parts, encodeValue(tostring(k)) .. ":" .. encodeValue(val))
                end
            end
            table.sort(parts)   -- 保证输出确定性，方便 diff
            return "{" .. table.concat(parts, ",") .. "}"
        end
    else
        return "null"
    end
end

function UGCSerialize.encode(v)
    local ok, result = pcall(encodeValue, v)
    if ok then return result end
    print("[UGCSerialize] encode 失败: " .. tostring(result))
    return "null"
end

--============================================================
-- decode：JSON string → Lua table
-- 最小递归下降解析器，覆盖所有 UGC 用到的结构
--============================================================

function UGCSerialize.decode(s)
    if not s or s == "" then return nil end

    local pos = 1
    local len = #s

    local function skipWS()
        while pos <= len and s:sub(pos, pos):match("[ \t\n\r]") do
            pos = pos + 1
        end
    end

    local parseValue  -- 前向声明

    local function parseString()
        pos = pos + 1   -- 跳过 "
        local buf = {}
        while pos <= len do
            local c = s:sub(pos, pos)
            if c == '"' then
                pos = pos + 1
                return table.concat(buf)
            elseif c == '\\' then
                pos = pos + 1
                local esc = s:sub(pos, pos)
                if     esc == '"'  then buf[#buf+1] = '"'
                elseif esc == '\\' then buf[#buf+1] = '\\'
                elseif esc == '/'  then buf[#buf+1] = '/'
                elseif esc == 'n'  then buf[#buf+1] = '\n'
                elseif esc == 'r'  then buf[#buf+1] = '\r'
                elseif esc == 't'  then buf[#buf+1] = '\t'
                else                    buf[#buf+1] = esc
                end
                pos = pos + 1
            else
                buf[#buf+1] = c
                pos = pos + 1
            end
        end
        return table.concat(buf)
    end

    local function parseObject()
        pos = pos + 1   -- 跳过 {
        local obj = {}
        skipWS()
        if s:sub(pos, pos) == '}' then pos = pos + 1; return obj end
        while pos <= len do
            skipWS()
            if s:sub(pos, pos) ~= '"' then break end
            local key = parseString()
            skipWS()
            if s:sub(pos, pos) == ':' then pos = pos + 1 end
            skipWS()
            local val = parseValue()
            obj[key] = val
            skipWS()
            local c = s:sub(pos, pos)
            if     c == ',' then pos = pos + 1
            elseif c == '}' then pos = pos + 1; break
            else break end
        end
        return obj
    end

    local function parseArray()
        pos = pos + 1   -- 跳过 [
        local arr = {}
        skipWS()
        if s:sub(pos, pos) == ']' then pos = pos + 1; return arr end
        while pos <= len do
            skipWS()
            local val = parseValue()
            arr[#arr+1] = val
            skipWS()
            local c = s:sub(pos, pos)
            if     c == ',' then pos = pos + 1
            elseif c == ']' then pos = pos + 1; break
            else break end
        end
        return arr
    end

    parseValue = function()
        skipWS()
        if pos > len then return nil end
        local c = s:sub(pos, pos)
        if     c == '"' then return parseString()
        elseif c == '{' then return parseObject()
        elseif c == '[' then return parseArray()
        elseif c == 't' then pos = pos + 4; return true
        elseif c == 'f' then pos = pos + 5; return false
        elseif c == 'n' then pos = pos + 4; return nil
        else
            -- 数字（含负号和小数）
            local num = s:match("^-?%d+%.?%d*[eE]?[+-]?%d*", pos)
            if num then
                pos = pos + #num
                return tonumber(num)
            end
            pos = pos + 1
            return nil
        end
    end

    local ok, result = pcall(parseValue)
    if ok then return result end
    print("[UGCSerialize] decode 失败: " .. tostring(result))
    return nil
end

return UGCSerialize
