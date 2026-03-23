---
--- BP_FPSProjectile.lua
--- 弹体 Lua 脚本，挂在 BP_FPSProjectile（父类 AFPSProjectile）上。
---
--- C++ 负责：碰撞注册、伤害计算（GAS）、网络同步、自毁计时器。
--- Lua 负责：命中物理冲量、音效/特效（PlayLaunchEffects / PlayImpactEffects）。
---
--- 绑定方式：BP_FPSProjectile → Class Defaults → LuaFilePath = "Script/Weapon/BP_FPSProjectile"
---

local SoundManager = require("Core.SoundManager")

local M = UnLua.Class()

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
