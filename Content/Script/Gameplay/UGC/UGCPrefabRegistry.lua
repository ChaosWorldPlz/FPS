--[[
    UGCPrefabRegistry.lua
    预制体注册表（静态配置）

    新增预制体：在 Prefabs 表里加一条，对应蓝图路径改成实际资产路径。
    蓝图路径格式："/Game/_UGC/Placeables/BP_Placeable_Box.BP_Placeable_Box_C"
]]

local Registry = {}

--============================================================
-- 预制体列表（PrefabName → 蓝图类路径）
--============================================================

Registry.Prefabs = {
    -- 几何体
    Box        = "/Game/_UGC/Placeables/BP_Placeable_Box.BP_Placeable_Box_C",
    Sphere     = "/Game/_UGC/Placeables/BP_Placeable_Sphere.BP_Placeable_Sphere_C",
    Cylinder   = "/Game/_UGC/Placeables/BP_Placeable_Cylinder.BP_Placeable_Cylinder_C",
    Ramp       = "/Game/_UGC/Placeables/BP_Placeable_Ramp.BP_Placeable_Ramp_C",

    -- 关卡功能
    SpawnPoint    = "/Game/_UGC/Placeables/BP_Placeable_SpawnPoint.BP_Placeable_SpawnPoint_C",
    ExtractionZone= "/Game/_UGC/Placeables/BP_Placeable_Extraction.BP_Placeable_Extraction_C",
    TriggerZone   = "/Game/_UGC/Placeables/BP_Placeable_TriggerZone.BP_Placeable_TriggerZone_C",

    -- 武器道具
    WeaponSpawn   = "/Game/_UGC/Placeables/BP_Placeable_WeaponSpawn.BP_Placeable_WeaponSpawn_C",
}

-- 编辑器 UI 用：分组 + 显示名（按顺序）
Registry.Categories = {
    {
        name = "几何体",
        items = {
            { id = "Box",      label = "方块" },
            { id = "Sphere",   label = "球体" },
            { id = "Cylinder", label = "圆柱" },
            { id = "Ramp",     label = "斜坡" },
        }
    },
    {
        name = "关卡功能",
        items = {
            { id = "SpawnPoint",     label = "出生点" },
            { id = "ExtractionZone", label = "撤离点" },
            { id = "TriggerZone",    label = "触发区" },
        }
    },
    {
        name = "武器道具",
        items = {
            { id = "WeaponSpawn", label = "武器生成" },
        }
    },
}

--============================================================
-- 接口
--============================================================

function Registry.GetPath(prefabName)
    return Registry.Prefabs[prefabName]
end

function Registry.IsValid(prefabName)
    return Registry.Prefabs[prefabName] ~= nil
end

return Registry
