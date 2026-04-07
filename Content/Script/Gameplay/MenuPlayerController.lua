--[[
    MenuPlayerController.lua
    绑定：BP_MenuPlayerController → LuaFilePath = "Gameplay.MenuPlayerController"

    职责：
    - 主菜单关卡专用 Controller
    - BeginPlay 时打开主菜单
]]

local UIManager = require("Gameplay.Core.UIManager")

local M = UnLua.Class()

function M:ReceiveBeginPlay()
    UIManager:Init(self)
    UIManager:OpenWindow("UI/Menu/WBP_MainMenu")
end

return M
