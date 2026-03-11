--[[
    BP_WeaponBase.lua
    武器 Lua 逻辑

    职责：
    - 覆盖 GetFireDirectionWithSpread()：读取 JSON 弹道数据，应用后坐力 Pattern + 随机扰动
    - 覆盖 OnShotFired()：推进 Pattern 索引，记录时间用于归零
    - ReceiveTick：检测停止射击后 Pattern 重置

    JSON 数据位于 Content/Data/WeaponBallistics.json
    Key 为 WeaponData.WeaponID（字符串，如 "AK47"）

    绑定：BP_WeaponBase → UnLuaInterface → GetModuleName = "Weapon.BP_WeaponBase"
]]

local M = UnLua.Class()

-- 弹道数据缓存（所有武器实例共享，只读一次）
local BallisticsCache = nil

local function LoadBallisticsData()
    if BallisticsCache then return BallisticsCache end

    -- 读取 JSON 文件
    local ContentDir = UE.UKismetSystemLibrary.GetProjectContentDirectory()
    local FilePath = ContentDir .. "Data/WeaponBallistics.json"

    local f = io.open(FilePath, "r")
    if not f then
        UE.UKismetSystemLibrary.PrintString(nil, "[Ballistics] 找不到文件: " .. FilePath, true, true, UE.FLinearColor(1,0,0,1), 5)
        BallisticsCache = {}
        return BallisticsCache
    end

    local content = f:read("*all")
    f:close()

    local ok, data = pcall(function()
        return require("rapidjson").decode(content)
    end)

    if ok and data then
        BallisticsCache = data
    else
        UE.UKismetSystemLibrary.PrintString(nil, "[Ballistics] JSON 解析失败", true, true, UE.FLinearColor(1,0,0,1), 5)
        BallisticsCache = {}
    end

    return BallisticsCache
end

-- 获取当前武器的弹道配置
function M:GetBallisticsData()
    local cache = LoadBallisticsData()
    if not self.WeaponData then return nil end
    local weaponID = tostring(self.WeaponData.WeaponID)
    return cache[weaponID]
end

function M:ReceiveBeginPlay()
    LoadBallisticsData()
    self._PatternIndex    = 0
    self._LastFireTime    = -999

    local data = self:GetBallisticsData()
    self._PatternResetTime = data and (data.pattern_reset_time or 0.4) or 0.4
end

-- ── 覆盖 GetFireDirectionWithSpread ──────────────────────────────────────────
-- C++ Fire() 调用此函数获取最终弹道方向
function M:GetFireDirectionWithSpread()
    -- 基础方向：角色控制器朝向
    local BaseDir = UE.FVector(1, 0, 0)
    if self.OwningCharacter then
        local CtrlRot = self.OwningCharacter:GetControlRotation()
        BaseDir = UE.UKismetMathLibrary.GetForwardVector(CtrlRot)
    end

    local data = self:GetBallisticsData()
    if not data then return BaseDir end

    -- 1. Pattern 偏移（确定性后坐力趋势）
    local patYaw, patPitch = 0, 0
    local pattern = data.recoil_pattern
    if pattern and #pattern > 0 then
        local idx = math.min(self._PatternIndex + 1, #pattern)
        patYaw   = pattern[idx][1] or 0
        patPitch = pattern[idx][2] or 0
    end

    -- 2. 随机扰动（圆盘均匀采样，sqrt 保证均匀分布）
    local radius    = data.random_spread_radius or 0.4
    local angle     = math.random() * 2 * math.pi
    local r         = math.sqrt(math.random()) * radius
    local randYaw   = math.cos(angle) * r
    local randPitch = math.sin(angle) * r

    local totalYaw   = patYaw   + randYaw
    local totalPitch = patPitch + randPitch

    -- 3. 应用旋转
    local Up    = UE.FVector(0, 0, 1)
    local Right = UE.UKismetMathLibrary.Cross_VectorVector(BaseDir, Up)
    Right = UE.UKismetMathLibrary.Normal(Right)
    Up    = UE.UKismetMathLibrary.Cross_VectorVector(Right, BaseDir)
    Up    = UE.UKismetMathLibrary.Normal(Up)

    BaseDir = UE.UKismetMathLibrary.RotateAngleAxis(BaseDir,  totalYaw,   Up)
    BaseDir = UE.UKismetMathLibrary.RotateAngleAxis(BaseDir, -totalPitch, Right)

    return UE.UKismetMathLibrary.Normal(BaseDir)
end

-- ── 覆盖 OnShotFired ─────────────────────────────────────────────────────────
-- C++ IncreaseSpread() 每发子弹后调用此函数
function M:OnShotFired()
    self._PatternIndex = self._PatternIndex + 1
    self._LastFireTime = UE.UGameplayStatics.GetTimeSeconds(self)
end

-- ── Tick：检测 Pattern 重置 ───────────────────────────────────────────────────
function M:ReceiveTick(DeltaTime)
    if self._PatternIndex > 0 then
        local now = UE.UGameplayStatics.GetTimeSeconds(self)
        if now - self._LastFireTime >= self._PatternResetTime then
            self._PatternIndex = 0
        end
    end
end

return M
