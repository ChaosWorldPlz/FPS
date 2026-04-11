--[[
    WBP_UGCNodePinRow.lua
    单条引脚行

    蓝图 Widget 结构（WBP_UGCNodePinRow）：
    水平框 HorizontalBox
      内边距 Padding = 4,2,4,2
      ├─ 图像 Image  [w_img_pin_in]    宽高=10×10  颜色=#FFFFFF  （输入锚点）
      ├─ 文本块 TextBlock [w_text_label]  槽位-尺寸=填充Fill
      └─ 图像 Image  [w_img_pin_out]   宽高=10×10  颜色=#FFFFFF  （输出锚点）

    调用方需先调 InitPin，再由编辑器通过 GetPinInAbsPos / GetPinOutAbsPos 拿坐标。
]]

local M = UnLua.Class()

--============================================================
-- 初始化
--============================================================

--- @param nodeID  string   所属节点 ID
--- @param pinName string   引脚名（exec_in / exec_out / exec_out_true / exec_out_false / 参数名）
--- @param isInput boolean  true = 输入引脚，false = 输出引脚，nil = 双向均隐藏锚点
--- @param editor  table    WBP_UGCBlueprintEditor 实例
function M:InitPin(nodeID, pinName, isInput, editor)
    self._nodeID  = nodeID
    self._pinName = pinName
    self._isInput = isInput
    self._editor  = editor

    -- 按引脚方向显示/隐藏锚点
    if self.w_img_pin_in then
        self.w_img_pin_in:SetVisibility(
            isInput == true
            and UE.ESlateVisibility.Visible
            or  UE.ESlateVisibility.Hidden)
    end
    if self.w_img_pin_out then
        self.w_img_pin_out:SetVisibility(
            isInput == false
            and UE.ESlateVisibility.Visible
            or  UE.ESlateVisibility.Hidden)
    end

    -- 绑定点击
    local anchor = isInput and self.w_img_pin_in or self.w_img_pin_out
    -- Image 没有 OnClicked，使用父行的 OnMouseButtonDown（由外层 Button 包裹）
    -- 实际点击由 WBP_UGCNode 层的 Button 路由过来，见 OnPinAnchorClicked
end

--- 由 WBP_UGCNode 在用户点击该行锚点区域时调用
function M:OnPinAnchorClicked()
    if self._editor and self._nodeID and self._pinName then
        self._editor:OnPinClicked(self._nodeID, self._pinName, self._isInput == false)
    end
end

--============================================================
-- 坐标查询（供编辑器获取连线端点）
--============================================================

--- 返回输入锚点中心的绝对屏幕坐标，不可用时返回 nil
function M:GetPinInAbsPos()
    return self:_getAnchorAbsPos(self.w_img_pin_in)
end

--- 返回输出锚点中心的绝对屏幕坐标，不可用时返回 nil
function M:GetPinOutAbsPos()
    return self:_getAnchorAbsPos(self.w_img_pin_out)
end

function M:_getAnchorAbsPos(anchorWidget)
    if not anchorWidget then return nil end
    local geo  = anchorWidget:GetCachedGeometry()
    local size = UE.USlateBlueprintLibrary.GetLocalSize(geo)
    if size.X == 0 and size.Y == 0 then return nil end  -- 未完成首帧布局
    return UE.USlateBlueprintLibrary.LocalToAbsolute(
        geo, UE.FVector2D(size.X * 0.5, size.Y * 0.5))
end

--============================================================
-- 标签
--============================================================

function M:SetLabel(text)
    if self.w_text_label then
        self.w_text_label:SetText(text)
    end
end

function M:SetLabelJustification(justification)
    if self.w_text_label then
        self.w_text_label:SetJustification(justification)
    end
end

return M
