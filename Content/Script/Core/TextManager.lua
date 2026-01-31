--[[
    TextManager.lua
    文本管理器 - 所有 UI 文本的统一入口

    用法:
        local TextManager = require("Core.TextManager")
        local text = TextManager:Get("MENU_NEW_GAME")
        local formatted = TextManager:Format("HUD_HEALTH", 80, 100)
]]

local TextManager = {}
TextManager.__index = TextManager

-- 单例实例
local instance = nil

-- 文本表缓存
local textTable = {}

-- 是否已加载
local isLoaded = false

--============================================================
-- 初始化
--============================================================

function TextManager:Init()
    if isLoaded then
        return
    end

    self:LoadTextTable()
    isLoaded = true
    print("[TextManager] Initialized with " .. self:GetTextCount() .. " entries")
end

function TextManager:LoadTextTable()
    -- 尝试加载 JSON 文件
    local jsonPath = "/Game/Data/TextTable.json"

    -- 方案1: 通过 UE 的 JsonUtilities 加载 (需要 C++ 暴露接口)
    -- 方案2: 直接在 Lua 中定义文本表 (开发期间使用)

    -- 当前使用方案2: 直接定义
    self:LoadDefaultTextTable()
end

function TextManager:LoadDefaultTextTable()
    -- 默认文本表 (后续从 JSON/CSV 加载)
    textTable = {
        -- ==================== 主菜单 ====================
        MENU_TITLE = "FPS Demo",
        MENU_NEW_GAME = "新游戏",
        MENU_CONTINUE = "继续游戏",
        MENU_SETTINGS = "设置",
        MENU_QUIT = "退出",
        MENU_QUIT_CONFIRM = "确定要退出游戏吗？",
        MENU_YES = "确定",
        MENU_NO = "取消",

        -- ==================== 暂停菜单 ====================
        PAUSE_TITLE = "暂停",
        PAUSE_RESUME = "继续",
        PAUSE_SETTINGS = "设置",
        PAUSE_QUIT_TO_MENU = "退出到主菜单",
        PAUSE_QUIT_CONFIRM = "退出将丢失未保存的进度，确定吗？",

        -- ==================== 设置菜单 ====================
        SETTINGS_TITLE = "设置",
        SETTINGS_TAB_GRAPHICS = "画面",
        SETTINGS_TAB_AUDIO = "音频",
        SETTINGS_TAB_CONTROLS = "操作",
        SETTINGS_TAB_KEYBINDINGS = "按键",
        SETTINGS_TAB_ABOUT = "关于",

        -- 画面设置
        SETTINGS_QUALITY = "画质",
        SETTINGS_QUALITY_LOW = "低",
        SETTINGS_QUALITY_MEDIUM = "中",
        SETTINGS_QUALITY_HIGH = "高",
        SETTINGS_QUALITY_ULTRA = "极致",
        SETTINGS_FOV = "视野 (FOV)",

        -- 音频设置
        SETTINGS_SFX_VOLUME = "音效音量",
        SETTINGS_MUSIC_VOLUME = "音乐音量",

        -- 操作设置
        SETTINGS_SENSITIVITY = "鼠标灵敏度",

        -- 按钮
        SETTINGS_APPLY = "应用",
        SETTINGS_RESET = "重置",
        SETTINGS_BACK = "返回",

        -- 关于
        SETTINGS_ABOUT_TITLE = "关于作者",
        SETTINGS_ABOUT_CONTENT = "FPS Demo\n作者: [Your Name]\n版本: 0.1.0",
        SETTINGS_CONTROLS_GUIDE = "操作说明",

        -- ==================== 地图选择 ====================
        MAP_SELECT_TITLE = "选择地图",
        MAP_SELECT_CONFIRM = "确认选择",
        MAP_SELECT_BACK = "返回",
        MAP_DIFFICULTY = "难度",
        MAP_DURATION = "时长: {1}分钟",
        MAP_PLAYERS = "玩家: {1}-{2}人",

        -- ==================== 装备界面 ====================
        LOADOUT_TITLE = "出战准备",
        LOADOUT_START_RAID = "开始战局",
        LOADOUT_BACK = "返回",

        -- ==================== 结算界面 ====================
        RESULT_SUCCESS = "撤离成功",
        RESULT_FAILED = "撤离失败",
        RESULT_EXIT = "退出",
        RESULT_SURVIVAL_TIME = "存活: {1}",
        RESULT_KILLS = "击杀: {1}",
        RESULT_XP = "经验: +{1}",

        -- ==================== HUD ====================
        HUD_HEALTH = "{1}/{2}",
        HUD_ARMOR = "{1}/{2}",
        HUD_STAMINA = "{1}%",
        HUD_AMMO = "{1}/{2}",
        HUD_RAID_TIME = "{1}",

        -- ==================== 通用 ====================
        COMMON_LOADING = "加载中...",
        COMMON_ERROR = "错误",
        COMMON_OK = "确定",
        COMMON_CANCEL = "取消",
    }
end

--============================================================
-- 公共接口
--============================================================

--- 获取文本 (无参数)
---@param textId string 文本ID
---@return string 文本内容
function TextManager:Get(textId)
    local text = textTable[textId]
    if text then
        return text
    else
        print("[TextManager] Warning: Text not found: " .. tostring(textId))
        return "[" .. tostring(textId) .. "]"
    end
end

--- 获取格式化文本 (带参数)
---@param textId string 文本ID
---@param ... any 参数列表
---@return string 格式化后的文本
function TextManager:Format(textId, ...)
    local text = self:Get(textId)
    local args = {...}

    -- 替换 {1}, {2}, {3}... 占位符
    for i, arg in ipairs(args) do
        text = text:gsub("{" .. i .. "}", tostring(arg))
    end

    return text
end

--- 检查文本是否存在
---@param textId string 文本ID
---@return boolean
function TextManager:Has(textId)
    return textTable[textId] ~= nil
end

--- 获取文本数量
---@return number
function TextManager:GetTextCount()
    local count = 0
    for _ in pairs(textTable) do
        count = count + 1
    end
    return count
end

--- 重新加载文本表 (热更新)
function TextManager:Reload()
    isLoaded = false
    textTable = {}
    self:Init()
    print("[TextManager] Reloaded")
end

--- 设置文本 (运行时动态添加)
---@param textId string 文本ID
---@param text string 文本内容
function TextManager:Set(textId, text)
    textTable[textId] = text
end

--- 批量设置文本 (从表加载)
---@param texts table {textId = text, ...}
function TextManager:SetBatch(texts)
    for textId, text in pairs(texts) do
        textTable[textId] = text
    end
end

--============================================================
-- 单例获取
--============================================================

function TextManager:GetInstance()
    if not instance then
        instance = setmetatable({}, TextManager)
        instance:Init()
    end
    return instance
end

-- 返回单例
return TextManager:GetInstance()
