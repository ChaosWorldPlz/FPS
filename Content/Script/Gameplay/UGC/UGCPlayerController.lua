--[[
    UGCPlayerController.lua
    绑定：BP_UGCPlayerController → UnLuaInterface → GetModuleName = "Gameplay.UGC.UGCPlayerController"

    继承自 Gameplay.PlayerController，附加 UGC 层初始化：
    - UGCFunctionRegistry：注册可供 LLM 调用的原子函数
    - LLMGateway：绑定 HttpClient 回调，处理 Claude API 响应
    - 提供 SendToLLM(message, callback) 快捷接口供 UI 调用
]]

local Base         = require("Gameplay.PlayerController")
local UGCRegistry  = require("Gameplay.UGC.UGCFunctionRegistry")
local LLMGateway   = require("Gameplay.UGC.LLMGateway")

local M = UnLua.Class(Base)

--============================================================
-- 生命周期
--============================================================

function M:ReceiveBeginPlay()
    -- 调用基类（UIManager:Init、GM.Init、OpenHUD）
    Base.ReceiveBeginPlay(self)

    -- 初始化 UGC 层
    UGCRegistry:Init(self)
    LLMGateway:Init(self)

    print("[UGCPlayerController] UGC 层初始化完成")
end

--============================================================
-- 快捷接口（供 UI Widget 直接调用）
--============================================================

--- 向 LLM 发送自然语言指令
--- @param message  string  用户输入
--- @param onResult function(success:bool, msg:string)  结果回调
function M:SendToLLM(message, onResult)
    LLMGateway:Send(message, onResult)
end

--- 获取已注册的 UGC 函数列表（调试/UI 展示用）
function M:GetUGCFunctionList()
    return UGCRegistry:ListFunctions()
end

return M
