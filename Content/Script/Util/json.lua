--[[
    Util/json.lua
    轻量 JSON 编码/解码，供 UGC 模块共用

    用法：
        local json = require("Util.json")
        local t    = json.decode('{"a":1}')   --> {a=1}
        local s    = json.encode(t, "  ")     --> 美化输出
]]

local json = {}

--============================================================
-- 解码
--============================================================

local function skipWS(s, i)
    while i <= #s and s:byte(i) <= 32 do i = i + 1 end
    return i
end

local parseValue  -- 前向声明

local function parseString(s, i)
    i = i + 1  -- 跳过 "
    local buf = {}
    local escMap = { ['"']='"', ['\\']='\\', ['/']='\/', ['n']='\n', ['r']='\r', ['t']='\t', ['b']='\b', ['f']='\f' }
    while i <= #s do
        local c = s:sub(i, i)
        if c == '"' then
            return table.concat(buf), i + 1
        elseif c == '\\' then
            i = i + 1
            buf[#buf+1] = escMap[s:sub(i,i)] or s:sub(i,i)
        else
            buf[#buf+1] = c
        end
        i = i + 1
    end
    error("unterminated string")
end

local function parseArray(s, i)
    local arr = {}
    i = skipWS(s, i + 1)
    if s:sub(i,i) == ']' then return arr, i + 1 end
    while true do
        i = skipWS(s, i)
        local val, ni = parseValue(s, i)
        arr[#arr+1] = val
        i = skipWS(s, ni)
        local c = s:sub(i,i)
        if c == ']' then return arr, i + 1 end
        assert(c == ',', "json: expected ',' or ']' at " .. i)
        i = skipWS(s, i + 1)
    end
end

local function parseObject(s, i)
    local obj = {}
    i = skipWS(s, i + 1)
    if s:sub(i,i) == '}' then return obj, i + 1 end
    while true do
        i = skipWS(s, i)
        local key, ni = parseString(s, i)
        i = skipWS(s, ni)
        assert(s:sub(i,i) == ':', "json: expected ':' at " .. i)
        i = skipWS(s, i + 1)
        local val
        val, i = parseValue(s, i)
        obj[key] = val
        i = skipWS(s, i)
        local c = s:sub(i,i)
        if c == '}' then return obj, i + 1 end
        assert(c == ',', "json: expected ',' or '}' at " .. i)
        i = skipWS(s, i + 1)
    end
end

parseValue = function(s, i)
    i = skipWS(s, i)
    local c = s:sub(i, i)
    if     c == '"' then return parseString(s, i)
    elseif c == '{' then return parseObject(s, i)
    elseif c == '[' then return parseArray(s, i)
    elseif c == 't' then return true,  i + 4
    elseif c == 'f' then return false, i + 5
    elseif c == 'n' then return nil,   i + 4
    else
        -- 数字
        local num = s:sub(i):match("^-?%d+%.?%d*[eE]?[+-]?%d*")
        if num then return tonumber(num), i + #num end
        error("json: unexpected '" .. c .. "' at " .. i)
    end
end

function json.decode(s)
    if not s or s == "" then return nil end
    local ok, result = pcall(parseValue, s, 1)
    if ok then return result end
    print("[json.decode] error: " .. tostring(result))
    return nil
end

--============================================================
-- 编码
--============================================================

local function encodeVal(v, indent, level)
    local t = type(v)
    if t == "nil"     then return "null"
    elseif t == "boolean" then return v and "true" or "false"
    elseif t == "number"  then
        return (math.floor(v) == v) and string.format("%d", v) or string.format("%.6g", v)
    elseif t == "string"  then
        return '"' .. v:gsub('\\','\\\\'):gsub('"','\\"'):gsub('\n','\\n'):gsub('\r','\\r'):gsub('\t','\\t') .. '"'
    elseif t == "table"   then
        -- 判断是否为纯数组（连续整数键从 1 开始）
        local isArr = true
        for k in pairs(v) do
            if type(k) ~= "number" or k ~= math.floor(k) then isArr = false; break end
        end
        isArr = isArr and (#v > 0 or next(v) == nil)

        if isArr then
            local parts = {}
            for _, item in ipairs(v) do
                parts[#parts+1] = encodeVal(item, indent, level+1)
            end
            if indent then
                local pad  = string.rep(indent, level+1)
                local cpad = string.rep(indent, level)
                return "[\n"..pad..table.concat(parts, ",\n"..pad).."\n"..cpad.."]"
            end
            return "["..table.concat(parts, ",").."]"
        else
            local parts = {}
            for k, val in pairs(v) do
                if type(k) == "string" then
                    local sep = indent and ": " or ":"
                    parts[#parts+1] = '"'..k..'"'..sep..encodeVal(val, indent, level+1)
                end
            end
            table.sort(parts)
            if indent then
                local pad  = string.rep(indent, level+1)
                local cpad = string.rep(indent, level)
                return "{\n"..pad..table.concat(parts, ",\n"..pad).."\n"..cpad.."}"
            end
            return "{"..table.concat(parts, ",").."}"
        end
    end
    return "null"
end

--- json.encode(val [, indent])
--- indent: 缩进字符串，nil=紧凑，"  "=两空格美化输出
function json.encode(val, indent)
    return encodeVal(val, indent, 0)
end

return json
