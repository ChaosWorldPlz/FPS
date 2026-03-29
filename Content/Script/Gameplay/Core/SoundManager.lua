-- Content/Script/Core/SoundManager.lua
-- 音效管理器 - Wwise 统一调用入口
--
-- 用法:
--     local SoundManager = require("Gameplay.Core.SoundManager")
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

    -- 武器
    Weapon = {
        -- 开火
        Fire_Rifle   = "Play_Weapon_Fire_Rifle",    -- TODO: 待 Wwise 配置
        Fire_Pistol  = "Play_Weapon_Fire_Pistol",   -- TODO: 待 Wwise 配置
        Fire_Shotgun = "Play_Weapon_Fire_Shotgun",  -- TODO: 待 Wwise 配置
        DryFire      = "Play_Weapon_DryFire",       -- TODO: 待 Wwise 配置

        -- 换弹
        Reload_Start  = "Play_Weapon_Reload_Start",  -- TODO: 待 Wwise 配置
        Reload_Finish = "Play_Weapon_Reload_Finish", -- TODO: 待 Wwise 配置

        -- 装备/收起
        Equip    = "Play_Weapon_Equip",    -- TODO: 待 Wwise 配置
        Unequip  = "Play_Weapon_Unequip",  -- TODO: 待 Wwise 配置

        -- 弹体命中（按材质分类）
        Impact_Flesh   = "Play_Weapon_Impact_Flesh",   -- TODO: 待 Wwise 配置
        Impact_Metal   = "Play_Weapon_Impact_Metal",   -- TODO: 待 Wwise 配置
        Impact_Stone   = "Play_Weapon_Impact_Stone",   -- TODO: 待 Wwise 配置
        Impact_Wood    = "Play_Weapon_Impact_Wood",    -- TODO: 待 Wwise 配置
        Impact_Default = "Play_Weapon_Impact_Default", -- TODO: 待 Wwise 配置
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
-- @param SoundKey   string   配置表键名（PickUp / PutDown / Rotate / Error / Sort）
-- @param Actor      AActor   触发源
function SoundManager:PlayInventorySound(SoundKey, Actor)
    local EventName = EventConfig.Inventory[SoundKey]
    if not EventName then
        print(string.format("[SoundManager] 未知音效键: Inventory.%s", tostring(SoundKey)))
        return
    end
    self:PostEvent(EventName, Actor)
end

-- 武器音效
-- @param SoundKey   string   配置表键名（Fire_Rifle / Reload_Start / Impact_Flesh 等）
-- @param Actor      AActor   触发源（武器或弹体 Actor，用于3D定位）
function SoundManager:PlayWeaponSound(SoundKey, Actor)
    local EventName = EventConfig.Weapon[SoundKey]
    if not EventName then
        print(string.format("[SoundManager] 未知音效键: Weapon.%s", tostring(SoundKey)))
        return
    end
    self:PostEvent(EventName, Actor)
end

-- 武器命中音效（根据物理材质自动选择）
-- @param PhysMatName  string   物理材质名（为空则播 Impact_Default）
-- @param Actor        AActor   命中点附近的 Actor
function SoundManager:PlayImpactSound(PhysMatName, Actor)
    local key = "Impact_Default"
    if PhysMatName then
        local name = string.lower(PhysMatName)
        if string.find(name, "flesh") or string.find(name, "body") then
            key = "Impact_Flesh"
        elseif string.find(name, "metal") then
            key = "Impact_Metal"
        elseif string.find(name, "stone") or string.find(name, "concrete") then
            key = "Impact_Stone"
        elseif string.find(name, "wood") then
            key = "Impact_Wood"
        end
    end
    self:PlayWeaponSound(key, Actor)
end

return SoundManager
