--[[
    WBP_UGCNode.lua
    单个蓝图节点的 Widget 逻辑

    蓝图 Widget 结构（WBP_UGCNode）：
    尺寸框 SizeBox
      最小期望宽度 MinDesiredWidth = 220
      └─ 边框 Border [w_border_bg]
           外观-画刷颜色 Brush Color = #2A2A2A
           内边距 Padding = 0
           └─ 垂直框 VerticalBox
                ├─ 覆层 Overlay
                │    ├─ 边框 Border [w_border_title]  外观-画刷颜色=运行时覆盖  可见性=命中测试不可见
                │    │    内边距 Padding = 8,4,8,4
                │    │    └─ 文本块 TextBlock [w_text_title]  字体大小=12  颜色=#FFFFFF
                │    └─ 按钮 Button [w_btn_title]  槽位-对齐=Fill(1,1)  外观全透明
                │         （无子节点）
                └─ 垂直框 VerticalBox [w_vbox_content]
                     内边距 Padding = 4,4,4,4
                     （引脚行动态填充）
]]

local NodeRegistry = require("System.UI.UGC.UGCNodeRegistry")

local M = UnLua.Class()

local PIN_ROW_PATH = "/Game/_UGC/UI/WBP_UGCNodePinRow.WBP_UGCNodePinRow_C"
local _pinRowClass = nil
local function getPinRowClass()
    if not _pinRowClass then
        _pinRowClass = UE.UClass.Load(PIN_ROW_PATH)
    end
    return _pinRowClass
end

--============================================================
-- 生命周期
--============================================================

function M:Construct()
    if self.w_btn_title and self.w_btn_title.OnPressed then
        self.w_btn_title.OnPressed:Add(self, M.OnTitlePressed)
    end
end

--============================================================
-- 初始化（由编辑器调用）
--============================================================

--- @param nodeData table  {id, type, pos, params}
--- @param editor   table  WBP_UGCBlueprintEditor Lua 实例
function M:InitNode(nodeData, editor)
    self._data    = nodeData
    self._editor  = editor
    self._pinRows = {}   -- {[pinName] = pinRowWidget}

    local def = NodeRegistry.Definitions[nodeData.type]
    if not def then return end

    if self.w_text_title then
        self.w_text_title:SetText(def.label)
    end

    if self.w_border_title then
        local c = def.color
        self.w_border_title:SetBrushColor(UE.FLinearColor(c.r, c.g, c.b, 1.0))
    end

    self:BuildContent(def, nodeData.params)
end

function M:BuildContent(def, params)
    if not self.w_vbox_content then return end
    self.w_vbox_content:ClearChildren()
    self._pinRows = {}

    local pc  = self:GetOwningPlayer()
    local cls = getPinRowClass()
    if not pc or not cls then return end

    -- isInput: true=输入引脚, false=输出引脚
    local function addPinRow(pinName, label, isInput, rightAlign)
        local row = UE.UWidgetBlueprintLibrary.Create(pc, cls, pc)
        if not row then return end
        row:SetLabel(label)
        if rightAlign then
            row:SetLabelJustification(UE.ETextJustify.Right)
        end
        row:InitPin(self._data.id, pinName, isInput, self._editor)
        self.w_vbox_content:AddChild(row)
        self._pinRows[pinName] = row
    end

    local function addParamRow(p)
        local row = UE.UWidgetBlueprintLibrary.Create(pc, cls, pc)
        if not row then return end

        if row.InitParam then
            row:InitParam(self._data, p.name, p.label, p.default)
        else
            local val = (params and params[p.name]) or p.default
            row:SetLabel(p.label .. ": " .. tostring(val))
            row:InitPin(self._data.id, p.name, nil, self._editor)
        end

        self.w_vbox_content:AddChild(row)
        self._pinRows[p.name] = row
    end

    if def.exec_in        then addPinRow("exec_in",        "▶ 执行",  true,  false) end
    if def.exec_out       then addPinRow("exec_out",       "执行 ▶",  false, true)  end
    if def.exec_out_true  then addPinRow("exec_out_true",  "True ▶",  false, true)  end
    if def.exec_out_false then addPinRow("exec_out_false", "False ▶", false, true)  end

    for _, p in ipairs(def.params or {}) do
        addParamRow(p)
    end
end

--============================================================
-- 对外接口
--============================================================

--- 根据引脚名获取对应的引脚行 Widget
--- @param pinName string
--- @return WBP_UGCNodePinRow | nil
function M:GetPinRow(pinName)
    return self._pinRows and self._pinRows[pinName]
end

function M:GetNodeID()  return self._data and self._data.id     end
function M:GetParams()  return self._data and self._data.params end

--============================================================
-- 标题栏拖拽
--============================================================

function M:OnTitlePressed()
    if not self._editor or not self._data then return end
    local pc = self:GetOwningPlayer()
    if not pc then return end
    local ok, sx, sy = pc:GetMousePosition()
    if ok then
        self._editor:BeginNodeDrag(self._data.id, sx, sy)
    end
end

return M
