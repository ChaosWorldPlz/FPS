-- Content/Script/Core/InventorySorter.lua
-- 背包自动整理算法
--
-- 独立模块，不持有状态，不依赖 UI。
-- 触发方式由调用方决定（按钮 / 捡拾 / 其他）。
--
-- 用法:
--     local Sorter = require("Core.InventorySorter")
--     local ok = Sorter:Sort(InventoryComponent, DataManager)

local InventorySorter = {}
InventorySorter.__index = InventorySorter

--[[
============================================================================
                        物品类型整理优先级
  EItemType: Weapon=0, Armor=1, Ammo=2, Medical=3, Container=4,
             Key=5, Consumable=6, Quest=7, Collectables=8, Misc=9
  数字越小越靠前放置
============================================================================
--]]

local TYPE_PRIORITY = {
    [0] = 1,   -- Weapon
    [1] = 2,   -- Armor
    [3] = 3,   -- Medical
    [2] = 4,   -- Ammo
    [6] = 5,   -- Consumable
    [4] = 6,   -- Container
    [5] = 7,   -- Key
    [8] = 8,   -- Collectables
    [7] = 9,   -- Quest
    [9] = 10,  -- Misc
}

--[[
============================================================================
                            主接口
============================================================================
--]]

-- 整理背包
-- @param InventoryComponent  UInventoryGridComponent
-- @param DataManager         UItemDataManager
-- @return bool 整理是否成功
function InventorySorter:Sort(InventoryComponent, DataManager)
    if not InventoryComponent or not DataManager then
        print("[InventorySorter] Sort: 参数为空")
        return false
    end

    -- 1. 收集所有物品及其元数据
    local allPlacements = InventoryComponent:GetAllItems()
    local count = #allPlacements
    if count == 0 then
        return true
    end

    local itemsMeta = {}
    for i = 1, count do
        local placement = allPlacements[i]
        local itemDef = DataManager:GetItemDefinition(placement.Item.ItemDefID)
        if not itemDef then
            print(string.format("[InventorySorter] 找不到物品定义: %s，中止整理",
                tostring(placement.Item.ItemDefID)))
            return false
        end

        -- EItemType 在 Lua 中可能是 userdata，转换为数字
        local typeValue = tonumber(tostring(itemDef.ItemType)) or 9

        table.insert(itemsMeta, {
            item      = placement.Item,
            sizeX     = itemDef.SizeX,
            sizeY     = itemDef.SizeY,
            canRotate = itemDef.bCanRotate,
            typeValue = typeValue,
        })
    end

    -- 2. 排序：类型优先 → 面积大优先 → 宽优先
    table.sort(itemsMeta, function(a, b)
        local pa = TYPE_PRIORITY[a.typeValue] or 99
        local pb = TYPE_PRIORITY[b.typeValue] or 99
        if pa ~= pb then return pa < pb end

        local areaA = a.sizeX * a.sizeY
        local areaB = b.sizeX * b.sizeY
        if areaA ~= areaB then return areaA > areaB end

        return a.sizeX > b.sizeX
    end)

    -- 3. 计算新的放置方案（FFD 算法）
    local gridSize = InventoryComponent:GetGridSize()
    local newPlacements = self:ComputePlacements(itemsMeta, gridSize.X, gridSize.Y)
    if not newPlacements then
        print("[InventorySorter] 物品无法全部放下，中止整理")
        return false
    end

    -- 4. 执行移动：先全部移除，再按新方案放回
    for _, meta in ipairs(itemsMeta) do
        InventoryComponent:RemoveItem(meta.item.InstanceID)
    end
    for _, p in ipairs(newPlacements) do
        InventoryComponent:AddItem(p.item, p.gridPos, p.bRotated)
    end

    print(string.format("[InventorySorter] 整理完成，共 %d 个物品", #newPlacements))
    return true
end

--[[
============================================================================
                        FFD 放置算法
============================================================================
--]]

-- 计算最优放置方案
-- @param sortedItems  已排序的物品元数据数组
-- @param gridW/gridH  网格尺寸
-- @return table 放置结果，或 nil（有物品放不下）
function InventorySorter:ComputePlacements(sortedItems, gridW, gridH)
    -- 模拟占用网格（false = 空闲，true = 占用）
    local grid = {}
    for i = 1, gridW * gridH do
        grid[i] = false
    end

    local placements = {}

    for _, meta in ipairs(sortedItems) do
        local placed = false

        -- 尝试两个方向：原始 → 旋转（如果可旋转且非正方形）
        local orientations = { {meta.sizeX, meta.sizeY, false} }
        if meta.canRotate and meta.sizeX ~= meta.sizeY then
            table.insert(orientations, {meta.sizeY, meta.sizeX, true})
        end

        for _, orient in ipairs(orientations) do
            local w, h, rotated = orient[1], orient[2], orient[3]
            local pos = self:FindFirstFit(grid, gridW, gridH, w, h)
            if pos then
                self:MarkGrid(grid, gridW, pos.X, pos.Y, w, h)
                table.insert(placements, {
                    item     = meta.item,
                    gridPos  = pos,
                    bRotated = rotated,
                })
                placed = true
                break
            end
        end

        if not placed then
            return nil
        end
    end

    return placements
end

-- 行优先扫描，返回第一个能容纳 w×h 物品的左上角坐标
-- @return FIntPoint 或 nil
function InventorySorter:FindFirstFit(grid, gridW, gridH, w, h)
    for y = 0, gridH - h do
        for x = 0, gridW - w do
            if self:CanFit(grid, gridW, x, y, w, h) then
                return UE.FIntPoint(x, y)
            end
        end
    end
    return nil
end

-- 检查 (x, y) 起始的 w×h 区域是否全部空闲
function InventorySorter:CanFit(grid, gridW, x, y, w, h)
    for dy = 0, h - 1 do
        for dx = 0, w - 1 do
            if grid[(y + dy) * gridW + (x + dx) + 1] then
                return false
            end
        end
    end
    return true
end

-- 将 (x, y) 起始的 w×h 区域标记为占用
function InventorySorter:MarkGrid(grid, gridW, x, y, w, h)
    for dy = 0, h - 1 do
        for dx = 0, w - 1 do
            grid[(y + dy) * gridW + (x + dx) + 1] = true
        end
    end
end

return InventorySorter
