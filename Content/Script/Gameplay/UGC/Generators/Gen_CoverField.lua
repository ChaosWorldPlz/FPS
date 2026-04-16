--[[
    Gen_CoverField.lua
    在矩形区域内随机散布掩体（Box / Sandbag / 任意 prefab）
    典型用例："在出生点周围 20m × 20m 范围放 15 个 Box 当掩体"
]]

local Atoms = require("Gameplay.UGC.Generators.Atoms")
local M = {}

function M.Register(Generators)
    Generators.Register("CoverField", {
        desc = "在矩形区域内随机散布掩体。常用于野外或开阔地形布置遮蔽物。",
        params = {
            { name="x",          type="number",  desc="中心 X 坐标(cm)",                   required=true  },
            { name="y",          type="number",  desc="中心 Y 坐标(cm)",                   required=true  },
            { name="z",          type="number",  desc="中心 Z 坐标(cm)，默认 0",           required=false },
            { name="size_x",     type="number",  desc="X 方向范围(cm)，默认 2000",         required=false },
            { name="size_y",     type="number",  desc="Y 方向范围(cm)，默认 2000",         required=false },
            { name="count",      type="number",  desc="掩体数量，默认 10，上限 30",        required=false },
            { name="prefab",     type="string",  desc="使用的预制体 id，默认 Box",         required=false },
            { name="seed",       type="number",  desc="随机种子（整数，0 = 默认），相同种子结果一致", required=false },
            { name="yaw_random", type="boolean", desc="是否随机 yaw，默认 true",            required=false },
        },
        func = function(p)
            local count = math.min(tonumber(p.count) or 10, 30)
            local pts = Atoms.RandomScatter({
                kind   = "box",
                center = { x = tonumber(p.x) or 0, y = tonumber(p.y) or 0, z = tonumber(p.z) or 0 },
                sx     = tonumber(p.size_x) or 2000,
                sy     = tonumber(p.size_y) or 2000,
                sz     = 0,
            }, count, tonumber(p.seed) or 0, {
                rotation = (p.yaw_random ~= false) and "yaw_random" or "none",
            })
            return { points = pts, prefab = tostring(p.prefab or "Box") }
        end,
    })
end

return M
