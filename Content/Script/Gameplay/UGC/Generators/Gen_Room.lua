--[[
    Gen_Room.lua
    用方块预制体围出矩形房间的四壁（不含屋顶/地板）
    典型用例："在 (0,0) 造一个 8m × 8m 的房间"
]]

local Atoms = require("Gameplay.UGC.Generators.Atoms")
local M = {}

function M.Register(Generators)
    Generators.Register("Room", {
        desc = "用方块预制体围出矩形房间的四壁（不含屋顶和地板）。",
        params = {
            { name="x",             type="number", desc="房间中心 X(cm)",                required=true  },
            { name="y",             type="number", desc="房间中心 Y(cm)",                required=true  },
            { name="z",             type="number", desc="地面 Z(cm)，默认 0",            required=false },
            { name="w",             type="number", desc="房间宽度(cm)，默认 800",        required=false },
            { name="h",             type="number", desc="房间长度(cm)，默认 800",        required=false },
            { name="wall_prefab",   type="string", desc="墙体预制体 id，默认 Box",       required=false },
            { name="block_size",    type="number", desc="单块墙体边长(cm)，默认 100",    required=false },
            { name="height_layers", type="number", desc="墙体堆叠层数，默认 2",          required=false },
        },
        func = function(p)
            local cx = tonumber(p.x) or 0
            local cy = tonumber(p.y) or 0
            local cz = tonumber(p.z) or 0
            local w  = tonumber(p.w) or 800
            local h  = tonumber(p.h) or 800
            local seg    = tonumber(p.block_size) or 100
            local layers = tonumber(p.height_layers) or 2

            local nw = { x = cx - w/2, y = cy - h/2, z = cz }
            local ne = { x = cx + w/2, y = cy - h/2, z = cz }
            local sw = { x = cx - w/2, y = cy + h/2, z = cz }
            local se = { x = cx + w/2, y = cy + h/2, z = cz }

            local pts = {}
            local function append(arr) for _, pt in ipairs(arr) do pts[#pts+1] = pt end end
            append(Atoms.Wall(nw, ne, seg, layers, seg))   -- 北
            append(Atoms.Wall(se, sw, seg, layers, seg))   -- 南
            append(Atoms.Wall(ne, se, seg, layers, seg))   -- 东
            append(Atoms.Wall(sw, nw, seg, layers, seg))   -- 西

            return { points = pts, prefab = tostring(p.wall_prefab or "Box") }
        end,
    })
end

return M
