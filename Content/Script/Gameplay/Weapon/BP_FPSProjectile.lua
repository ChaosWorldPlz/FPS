---
--- BP_FPSProjectile.lua
--- 弹体 Lua 脚本，挂在 BP_FPSProjectile（父类 AFPSProjectile）上。
---
--- C++ 负责：碰撞注册、伤害计算（GAS）、网络同步、自毁计时器。
--- Lua 负责：命中物理冲量、音效/特效（PlayLaunchEffects / PlayImpactEffects）。
---
--- 绑定方式：BP_FPSProjectile → Class Defaults → LuaFilePath = "Gameplay/Weapon/BP_FPSProjectile"
---

local SoundManager = require("Gameplay.Core.SoundManager")

local M = UnLua.Class()

-- 日志封装（只在编辑器/开发版本打印，方便关掉）
local LOG_ENABLED = true
local function Log(msg)
    if not LOG_ENABLED then return end
    UE.UKismetSystemLibrary.PrintString(nil, "[BP_FPSProjectile] " .. msg, true, true, UE.FLinearColor(1, 0.8, 0, 1), 4)
end

-------------------------------------------------------------------
-- 伤害逻辑计算 肉伤/甲伤
-------------------------------------------------------------------
function M:CalculateFinalDamage(Hit, HitActor)
    if not self.WeaponData then
        Log("CalculateFinalDamage: WeaponData 为空，返回原始 Damage=" .. tostring(self.Damage))
        return self.Damage
    end
    if not HitActor then
        Log("CalculateFinalDamage: HitActor 为空，返回原始 Damage=" .. tostring(self.Damage))
        return self.Damage
    end

    -- 爆头倍率
    local baseDamage = self.Damage
    if Hit.BoneName == "head" then
        baseDamage = baseDamage * self.WeaponData.HeadshotMultiplier
        Log("CalculateFinalDamage: 爆头，baseDamage=" .. tostring(baseDamage))
    end

    -- 无护甲组件 → 全额
    local armorComp = HitActor.ArmorComponent
    if not armorComp then
        Log("CalculateFinalDamage: 目标无 ArmorComponent，全额 " .. tostring(baseDamage))
        return baseDamage
    end

    -- 命中部位不在护甲覆盖范围 → 全额
    if not armorComp:IsPartCovered(Hit.BoneName) then
        Log("CalculateFinalDamage: 骨骼 [" .. tostring(Hit.BoneName) .. "] 不在护甲覆盖范围，全额 " .. tostring(baseDamage))
        return baseDamage
    end

    -- 护甲耐久耗尽 → 全额
    local effectiveLevel = armorComp:GetEffectiveArmorLevel()
    if effectiveLevel == 0 then
        Log("CalculateFinalDamage: 护甲耐久耗尽，全额 " .. tostring(baseDamage))
        return baseDamage
    end

    local diff = self.WeaponData.BulletLevel - effectiveLevel
    local mult = self:GetDamageMultiplier(diff)
    local finalDamage = baseDamage * mult
    Log(string.format("CalculateFinalDamage: BulletLv=%d ArmorLv=%d diff=%d mult=%.2f final=%.1f",
        self.WeaponData.BulletLevel, effectiveLevel, diff, mult, finalDamage))
    return finalDamage
end

function M:CalculateFinalArmorDamage(Hit, HitActor)
    if not self.WeaponData then
        Log("CalculateFinalArmorDamage: WeaponData 为空，返回 0")
        return 0
    end
    if not HitActor then
        Log("CalculateFinalArmorDamage: HitActor 为空，返回 0")
        return 0
    end

    local armorComp = HitActor.ArmorComponent
    if not armorComp then
        Log("CalculateFinalArmorDamage: 目标无 ArmorComponent，返回 0")
        return 0
    end

    if not armorComp:IsPartCovered(Hit.BoneName) then
        Log("CalculateFinalArmorDamage: 骨骼 [" .. tostring(Hit.BoneName) .. "] 不在覆盖范围，返回 0")
        return 0
    end

    local effectiveLevel = armorComp:GetEffectiveArmorLevel()
    if effectiveLevel == 0 then
        Log("CalculateFinalArmorDamage: 护甲耐久耗尽，返回 0")
        return 0
    end

    local diff = self.WeaponData.BulletLevel - effectiveLevel
    local mult = self:GetArmorDamageMultiplier(diff)
    local finalArmorDmg = self.WeaponData.BulletArmorDamage * mult
    Log(string.format("CalculateFinalArmorDamage: BulletLv=%d ArmorLv=%d diff=%d mult=%.2f armorDmg=%.1f",
        self.WeaponData.BulletLevel, effectiveLevel, diff, mult, finalArmorDmg))
    return finalArmorDmg
end

function M:GetDamageMultiplier(LevelDiff)
    -- 如果子弹比护甲等级高，认为伤害没有衰减
    -- 甲弹同级，子弹伤害被减免25% 
    -- 护甲比子弹高一级 
     if LevelDiff >= 1 then
        return 1.0
     elseif LevelDiff == 0 then
        return 0.75
     elseif LevelDiff == -1 then
        return 0.5
     else
        return 0.25
     end
end   

-- 根据等级差返回护甲伤害倍率
function M:GetArmorDamageMultiplier(LevelDiff)
    -- 如果子弹比护甲等级高，认为伤害没有衰减
    -- 甲弹同级，子弹伤害被减免25% 
    -- 护甲比子弹高一级，
     if LevelDiff >= 1 then
        return 1.0
     elseif LevelDiff == 0 then
        return 0.75
     elseif LevelDiff == -1 then
        return 0.5
     else
        return 0.25
     end
end    

-------------------------------------------------------------------
-- 发射特效/音效（服务端 Spawn 后立即调用）
-------------------------------------------------------------------
function M:PlayLaunchEffects()
    -- TODO: 调用 Wwise 发射音效
    -- SoundManager:Get():PlayWeaponSound("Fire_Rifle", self)
end
-------------------------------------------------------------------
-- 命中特效/音效（在 OnProjectileHit_Implementation 中由 C++ 调用）
-------------------------------------------------------------------
function M:PlayImpactEffects(Hit)
    -- TODO: 根据物理材质选音效
    -- local physMat = Hit.PhysMaterial and Hit.PhysMaterial:GetName() or nil
    -- SoundManager:Get():PlayImpactSound(physMat, self)
end

-- OnProjectileHit 不在 Lua 覆盖：
--   C++ 构造函数已将 OnComponentHit 委托绑定到 OnProjectileHit_Implementation
--   （auth 检查 + 伤害 + 销毁），无需 Lua 介入。
--   如需对物理对象施加冲量，可在此处覆盖并调用独立的 C++ BlueprintCallable 接口。

-- OnProjectileExpired 不在 Lua 覆盖：
--   C++ 默认实现仅调用 Destroy()，无需 Lua 介入。
--   如需消散特效，在此处覆盖后播放粒子，然后调用 self:Destroy()。

return M
