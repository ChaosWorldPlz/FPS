-- Content/Script/Core/SoundManager.lua
-- 音效管理器 - Wwise 统一调用入口
--
-- 用法:
--     local SoundManager = require("Core.SoundManager")
--     SoundManager:Get():PlayInventorySound("PickUp", self)
--
-- 接入 Wwise 后，只需在下方 EventConfig 中填入真实 Event 名称

local SoundManager = {}
SoundManager.__index = SoundManager

local instance = nil

--[[
============================================================================
                        Wwise Event 配置表
  TODO: 在 Wwise 项目中创建对应 Event 后，将占位符替换为实际名称
============================================================================
--]]

local EventConfig = {

    -- 背包交互
    Inventory = {
        PickUp  = "Play_Inventory_PickUp",   -- TODO: 待 Wwise 配置
        PutDown = "Play_Inventory_PutDown",  -- TODO: 待 Wwise 配置
        Rotate  = "Play_Inventory_Rotate",   -- TODO: 待 Wwise 配置
        Error   = "Play_Inventory_Error",    -- TODO: 待 Wwise 配置
        Sort    = "Play_Inventory_Sort",     -- TODO: 待 Wwise 配置
    },

}

--[[
============================================================================
                              单例
============================================================================
--]]

function SoundManager:Get()
    if not instance then
        instance = setmetatable({}, SoundManager)
        instance:Init()
    end
    return instance
end

function SoundManager:Init()
    self.WwiseAvailable = (UE.UAkGameplayStatics ~= nil)
    if self.WwiseAvailable then
        print("[SoundManager] Wwise 就绪")
    else
        print("[SoundManager] Wwise 不可用，音效将静默（占位模式）")
    end
end

--[[
============================================================================
                           核心播放接口
============================================================================
--]]

-- 通过 Wwise Event 名称触发音效
-- @param EventName  string   Wwise Event 名称
-- @param Actor      AActor   触发源（用于3D空间定位）
function SoundManager:PostEvent(EventName, Actor)
    if not EventName or EventName == "" then
        return
    end

    if not self.WwiseAvailable then
        print(string.format("[SoundManager] 占位: %s", EventName))
        return
    end

    UE.UAkGameplayStatics.PostEventByName(EventName, Actor)
end

--[[
============================================================================
                         分类播放接口
============================================================================
--]]

-- 背包音效
-- @param SoundKey   string   配置表键名（PickUp / PutDown / Rotate / Error）
-- @param Actor      AActor   触发源
function SoundManager:PlayInventorySound(SoundKey, Actor)
    local EventName = EventConfig.Inventory[SoundKey]
    if not EventName then
        print(string.format("[SoundManager] 未知音效键: Inventory.%s", tostring(SoundKey)))
        return
    end
    self:PostEvent(EventName, Actor)
end

return SoundManager
