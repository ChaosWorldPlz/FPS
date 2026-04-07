--[[
    PlayerController.lua
    绑定：BP_FPSPlayerControll → UnLuaInterface → GetModuleName = "Gameplay.PlayerController"

    职责：
    - 初始化 UIManager
    - 覆盖 TogglePauseMenu（ESC 键由 C++ Enhanced Input 触发后转到这里）
    - 覆盖 ToggleInventory（TAB 键由 C++ Enhanced Input 触发后转到这里）
    - 所有 UI 打开/关闭通过 UIManager 统一管理
]]

local UIManager = require("Gameplay.Core.UIManager")
local GM = require("Gameplay.Core.GM")

local M = UnLua.Class()

--============================================================
-- 生命周期
--============================================================

function M:ReceiveBeginPlay()
    UIManager:Init(self)
    GM.Init(self)
    M:OpenHUD()
end

function M:ReceiveEndPlay()
    UIManager:Teardown()
end

function M:OpenHUD()
    UIManager:OpenWindow("UI/WBP_HUD")
end

function M:ReceiveTick(DeltaTime)
    GM.Tick(self, DeltaTime)
end

--============================================================
-- 菜单输入（覆盖 C++ BlueprintNativeEvent）
--============================================================

--- ESC 键触发
--- C++ HandlePauseMenuInput → TogglePauseMenu（BlueprintNativeEvent）→ 这里
function M:TogglePauseMenu()
    if UIManager:IsAnyOpen() then
        -- 有窗口打开：逐层关闭栈顶
        UIManager:CloseTop()
    else
        -- 无窗口：打开暂停菜单
        UIManager:OpenWindow("UI/Menu/WBP_PauseMenu")
    end
end

--- TAB 键触发
--- C++ HandleInventoryInput → ToggleInventory（BlueprintNativeEvent）→ 这里
function M:ToggleInventory()
    UIManager:ToggleWindow("UI/WBP_InventoryGrid")
end

--============================================================
-- 快捷接口（供其他 Lua 模块直接拿 UIManager）
--============================================================

function M:GetUIManager()
    return UIManager
end

return M
