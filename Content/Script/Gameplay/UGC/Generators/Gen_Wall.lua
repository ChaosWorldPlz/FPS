--[[
    Gen_Wall.lua
    在两点之间垒一道墙（自动按 block_size 分段并堆叠层数）
    典型用例："从 (0,0) 到 (1000,0) 拉一道两层高的矮墙"
]]

local Atoms = require("Gameplay.UGC.Generators.Atoms")
local M = {}

function M.Register(Generators)
    Generators.Register("Wall", {
        desc = "沿两点连线垒一道墙，自动按 block_size 分段、按 layers 堆叠层数。",
        params = {
            { name="x1",         type="number", desc="起点 X(cm)",              required=true  },
            { name="y1",         type="number", desc="起点 Y(cm)",              required=true  },
            { name="x2",         type="number", desc="终点 X(cm)",              required=true  },
            { name="y2",         type="number", desc="终点 Y(cm)",              required=true  },
            { name="z",          type="number", desc="地面 Z(cm)，默认 0",      required=false },
            { name="prefab",     type="string", desc="墙体预制体 id，默认 Box", required=false },
            { name="block_size", type="number", desc="单块边长(cm)，默认 100",  required=false },
            { name="layers",     type="number", desc="堆叠层数，默认 2",        required=false },
        },
        func = function(p)
            local seg    = tonumber(p.block_size) or 100
            local layers = tonumber(p.layers) or 2
            local pts = Atoms.Wall(
                { x = tonumber(p.x1) or 0, y = tonumber(p.y1) or 0, z = tonumber(p.z) or 0 },
                { x = tonumber(p.x2) or 0, y = tonumber(p.y2) or 0, z = tonumber(p.z) or 0 },
                seg, layers, seg
            )
            return { points = pts, prefab = tostring(p.prefab or "Box") }
        end,
    })
end

return M
