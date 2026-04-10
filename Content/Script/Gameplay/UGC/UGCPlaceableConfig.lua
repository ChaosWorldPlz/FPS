--[[
    UGCPlaceableConfig.lua
    可放置预制体配置表（纯数据）

    新增预制体步骤：
      1. 在 Content/_UGC/Placeables/ 里创建对应蓝图
      2. 在下面加一行 { id, label, category, path }
      3. 不需要改任何其他文件，不需要重新编译

    path 格式："/Game/_UGC/Placeables/BP_Placeable_Xxx.BP_Placeable_Xxx_C"
]]

return {
    -- 几何体
    { id="Box",      label="方块",   category="几何体",   path="/Game/_UGC/Placeables/BP_Placeable_Box.BP_Placeable_Box_C" },
    { id="Sphere",   label="球体",   category="几何体",   path="/Game/_UGC/Placeables/BP_Placeable_Sphere.BP_Placeable_Sphere_C" },
    { id="Cylinder", label="圆柱",   category="几何体",   path="/Game/_UGC/Placeables/BP_Placeable_Cylinder.BP_Placeable_Cylinder_C" },
    { id="Ramp",     label="斜坡",   category="几何体",   path="/Game/_UGC/Placeables/BP_Placeable_Ramp.BP_Placeable_Ramp_C" },

    -- 关卡功能
    { id="SpawnPoint",     label="出生点", category="关卡功能", path="/Game/_UGC/Placeables/BP_Placeable_SpawnPoint.BP_Placeable_SpawnPoint_C" },
    { id="ExtractionZone", label="撤离点", category="关卡功能", path="/Game/_UGC/Placeables/BP_Placeable_Extraction.BP_Placeable_Extraction_C" },
    { id="TriggerZone",    label="触发区", category="关卡功能", path="/Game/_UGC/Placeables/BP_Placeable_TriggerZone.BP_Placeable_TriggerZone_C" },

    -- 武器道具
    { id="WeaponSpawn", label="武器生成", category="武器道具", path="/Game/_UGC/Placeables/BP_Placeable_WeaponSpawn.BP_Placeable_WeaponSpawn_C" },
}
