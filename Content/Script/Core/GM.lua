--[[
    GM.lua
    GM 调试工具——给武器、回血等开发期作弊指令
    绑定：BP_FPSPlayerController → UnLuaInterface → GetModuleName = "Core.GM"

    默认按键：
      F1  —— 给 Primary1 槽默认步枪
      F2  —— 给 Primary2 槽第二把步枪
      F3  —— 给 Pistol 槽手枪
      F4  —— 清空所有武器槽
      F5  —— 满血

    武器蓝图路径在 GM_WEAPONS 表里改
]]

local M = UnLua.Class()

-- ── 配置区 ────────────────────────────────────────────────────────────────
local GM_WEAPONS = {
    Primary1 = "/Game/_FPS/Weapon/BP_WeaponBase.BP_WeaponBase_C",   -- 改成你实际的路径
    Primary2 = "/Game/Blueprints/Weapons/BP_Rifle.BP_Rifle_C",
    Pistol   = "/Game/Blueprints/Weapons/BP_Pistol.BP_Pistol_C",
}
-- ─────────────────────────────────────────────────────────────────────────

function M:ReceiveBeginPlay()
    UE.UKismetSystemLibrary.PrintString(self, "[GM] ReceiveBeginPlay 被调用了！", true, true, UE.FLinearColor(1,0,1,1), 10)

    -- 只在本地客户端/单机注册按键（GM 工具不走网络）
    if not self:IsLocalPlayerController() then 
        UE.UKismetSystemLibrary.PrintString(self, "[GM] 不是本地玩家控制器！", true, true, UE.FLinearColor(1,0,0,1), 10)
        return 
    end

    UE.UKismetSystemLibrary.PrintString(self, "[GM] GM 模块加载，T, F2~F5 可用", true, true, UE.FLinearColor(1,0.5,0,1), 5)

    -- 启用 Tick，用于在 ReceiveTick 中轮询按键
    self.PrimaryActorTick.bCanEverTick = true
    self:SetActorTickEnabled(true)
    
    self.GMKeyStates = {}
    self.DebugTickTimer = 0
    UE.UKismetSystemLibrary.PrintString(self, "[GM] Tick 初始化完成", true, true, UE.FLinearColor(1,0.5,0,1), 5)
end

function M:ReceiveTick(DeltaTime)
    if not self:IsLocalPlayerController() then return end

    -- 每2秒打印一次，确认 Tick 是否在跑
    if not self.DebugTickTimer then self.DebugTickTimer = 0 end
    self.DebugTickTimer = self.DebugTickTimer + DeltaTime
    if self.DebugTickTimer > 2 then
        self.DebugTickTimer = 0
        UE.UKismetSystemLibrary.PrintString(self, "[GM] Tick 正在运行中...", true, false, UE.FLinearColor(0,1,1,1), 2)
    end

    self:CheckGMKey("T", self.GM_GivePrimary1)
    self:CheckGMKey("F2", self.GM_GivePrimary2)
    self:CheckGMKey("F3", self.GM_GivePistol)
    self:CheckGMKey("F4", self.GM_ClearWeapons)
    self:CheckGMKey("F5", self.GM_FullHP)
end

function M:CheckGMKey(KeyName, Callback)
    local Key = UE.FKey()
    Key.KeyName = KeyName
    
    -- 我们每10秒打印一下 T 键的构建是否正确，防止是 FKey 没创建对
    if KeyName == "T" and (self.DebugTickTimer > 1.99 and self.DebugTickTimer < 2.01) then
        UE.UKismetSystemLibrary.PrintString(self, "[GM] 构建的KeyName: " .. tostring(Key.KeyName), true, false, UE.FLinearColor(1,1,1,1), 2)
    end

    local IsDown = self:IsInputKeyDown(Key)
    
    if IsDown then
        UE.UKismetSystemLibrary.PrintString(self, "[GM] 监测到按键按住状态: " .. tostring(KeyName), true, false, UE.FLinearColor(0,1,0,1), 0.1)
    end
    
    if IsDown and not self.GMKeyStates[KeyName] then
        UE.UKismetSystemLibrary.PrintString(self, "[GM] 触发 GM 指令: " .. tostring(KeyName), true, true, UE.FLinearColor(1,1,0,1), 5)
        Callback(self)
    end
    self.GMKeyStates[KeyName] = IsDown
end

-- ── 内部：获取当前 Pawn 的 WeaponSlotComp ─────────────────────────────────
function M:GetSlotComp()
    local Pawn = self.Pawn
    if not Pawn then return nil end
    return Pawn.WeaponSlotComp
end

-- ── 内部：给武器到指定槽 ───────────────────────────────────────────────────
function M:GiveWeapon(SlotEnum, ClassPath)
    local SlotComp = self:GetSlotComp()
    if not SlotComp then
        UE.UKismetSystemLibrary.PrintString(self, "[GM] 找不到 WeaponSlotComp", true, true, UE.FLinearColor(1,0,0,1), 5)
        return
    end

    local WeaponClass = UE.UClass.Load(ClassPath)
    if not WeaponClass then
        UE.UKismetSystemLibrary.PrintString(self, "[GM] 找不到武器蓝图: " .. ClassPath, true, true, UE.FLinearColor(1,0,0,1), 5)
        return
    end

    local Ok = SlotComp:SetWeaponInSlot(SlotEnum, UE.FInventoryItem(), WeaponClass)
    local SlotName = tostring(SlotEnum)
    UE.UKismetSystemLibrary.PrintString(self,
        string.format("[GM] 给武器 %s → 槽 %s %s", ClassPath, SlotName, Ok and "✓" or "✗"),
        true, true, UE.FLinearColor(0,1,0,1), 5)
end

-- ── GM 指令 ────────────────────────────────────────────────────────────────
function M:GM_GivePrimary1()
    self:GiveWeapon(UE.EFPSWeaponSlot.Primary1, GM_WEAPONS.Primary1)
end

function M:GM_GivePrimary2()
    self:GiveWeapon(UE.EFPSWeaponSlot.Primary2, GM_WEAPONS.Primary2)
end

function M:GM_GivePistol()
    self:GiveWeapon(UE.EFPSWeaponSlot.Pistol, GM_WEAPONS.Pistol)
end

function M:GM_ClearWeapons()
    local SlotComp = self:GetSlotComp()
    if not SlotComp then return end
    local Dummy = UE.FInventoryItem()
    SlotComp:RemoveWeaponFromSlot(UE.EFPSWeaponSlot.Primary1, Dummy)
    SlotComp:RemoveWeaponFromSlot(UE.EFPSWeaponSlot.Primary2, Dummy)
    SlotComp:RemoveWeaponFromSlot(UE.EFPSWeaponSlot.Pistol,   Dummy)
    UE.UKismetSystemLibrary.PrintString(self, "[GM] 武器槽已清空", true, true, UE.FLinearColor(1,1,0,1), 5)
end

function M:GM_FullHP()
    local Pawn = self.Pawn
    if not Pawn then return end
    local ASC = Pawn:GetAbilitySystemComponent()
    if not ASC then return end
    -- 触发 GameplayEffect 或直接设属性均可，这里用 PrintString 占位
    UE.UKismetSystemLibrary.PrintString(self, "[GM] 满血（TODO：接 GE_FullHP）", true, true, UE.FLinearColor(0,1,1,1), 5)
end

return M
