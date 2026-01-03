-- Content/Script/UI/Inventory/WBP_ItemIcon.lua
-- 物品图标 Widget 的 Lua 逻辑

local WBP_ItemIcon = UnLua.Class()

-- ==================== 生命周期 ====================

function WBP_ItemIcon:Construct()
    print("[Lua] WBP_ItemIcon:Construct")
end

function WBP_ItemIcon:Destruct()
    print("[Lua] WBP_ItemIcon:Destruct")
end

-- ==================== C++ 回调 ====================

-- C++ SetItemData 之后会调用这个蓝图事件
function WBP_ItemIcon:OnItemDataSet()
    print(string.format("[Lua] Item set: %s, Stack: %d",
        tostring(self.ItemData.ItemDefID),
        self.ItemData.StackCount))

    -- 可以在这里添加额外逻辑
    -- 比如根据物品类型设置边框颜色
end

return WBP_ItemIcon
