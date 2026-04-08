--[[
    WBP_UGCEditor.lua
    游戏内关卡编辑器 UI

    蓝图绑定：WBP_UGCEditor → GetModuleName = "System.UI.UGC.WBP_UGCEditor"

    Widget 结构（在 UE 编辑器里创建）：
    ┌─────────────────────────────────────────────────────┐
    │  [w_panel_Prefabs]  左栏预制体列表（ScrollBox）       │
    │  [w_panel_Props]    右栏属性面板（Transform 输入）    │
    │  [w_panel_Bottom]   底栏操作按钮                     │
    │    w_btn_Play       试玩/返回编辑                    │
    │    w_btn_Save       保存场景                         │
    │    w_btn_Load       加载场景                         │
    │    w_btn_Clear      清空场景                         │
    │    w_btn_Delete     删除选中                         │
    │    w_btn_Undo       撤销                             │
    │    w_text_Status    状态文字                         │
    │  [w_input_X/Y/Z]    位置输入框                       │
    │  [w_input_P/Yaw/R]  旋转输入框                       │
    │  [w_input_SX/SY/SZ] 缩放输入框                       │
    └─────────────────────────────────────────────────────┘
]]

local EditorCore      = require("Gameplay.UGC.UGCEditorCore")
local PrefabRegistry  = require("Gameplay.UGC.UGCPrefabRegistry")

local M = UnLua.Class()

--============================================================
-- 生命周期
--============================================================

function M:Construct()
    -- 绑定底栏按钮
    self.w_btn_Play.OnClicked:Add(self, M.OnClickPlay)
    self.w_btn_Save.OnClicked:Add(self, M.OnClickSave)
    self.w_btn_Load.OnClicked:Add(self, M.OnClickLoad)
    self.w_btn_Clear.OnClicked:Add(self, M.OnClickClear)
    self.w_btn_Delete.OnClicked:Add(self, M.OnClickDelete)
    self.w_btn_Undo.OnClicked:Add(self, M.OnClickUndo)

    -- 绑定 Transform 输入框（失去焦点时提交）
    self.w_input_X.OnTextCommitted:Add(self, M.OnTransformCommit)
    self.w_input_Y.OnTextCommitted:Add(self, M.OnTransformCommit)
    self.w_input_Z.OnTextCommitted:Add(self, M.OnTransformCommit)
    self.w_input_P.OnTextCommitted:Add(self, M.OnTransformCommit)
    self.w_input_Yaw.OnTextCommitted:Add(self, M.OnTransformCommit)
    self.w_input_R.OnTextCommitted:Add(self, M.OnTransformCommit)
    self.w_input_SX.OnTextCommitted:Add(self, M.OnTransformCommit)
    self.w_input_SY.OnTextCommitted:Add(self, M.OnTransformCommit)
    self.w_input_SZ.OnTextCommitted:Add(self, M.OnTransformCommit)

    -- 动态生成预制体按钮列表
    self:BuildPrefabList()

    -- 监听状态变化（刷新试玩按钮文字）
    EditorCore:OnStateChanged(function(state)
        self:RefreshPlayButton(state)
    end)

    self:SetStatus("就绪 — 点击预制体开始放置")
    print("[WBP_UGCEditor] UI 构建完成")
end

--============================================================
-- 预制体列表（动态生成按钮）
--============================================================

function M:BuildPrefabList()
    -- w_panel_Prefabs 是一个 VerticalBox
    -- 每个分类加一个 SectionHeader + 若干 Button
    for _, category in ipairs(PrefabRegistry.Categories) do
        -- 分类标题（Text Block）
        local header = self:CreateWidget("WBP_UGCPrefabHeader")
        if header then
            header:SetText(category.name)
            self.w_panel_Prefabs:AddChild(header)
        end

        for _, item in ipairs(category.items) do
            -- 预制体按钮
            local btn = self:CreateWidget("WBP_UGCPrefabBtn")
            if btn then
                btn:SetLabel(item.label)
                -- 通过闭包绑定点击
                local prefabID = item.id
                btn.w_btn.OnClicked:Add(self, function()
                    self:OnClickPrefab(prefabID)
                end)
                self.w_panel_Prefabs:AddChild(btn)
            end
        end
    end
end

--============================================================
-- 按钮事件
--============================================================

function M:OnClickPrefab(prefabName)
    EditorCore:SelectPrefab(prefabName)
    self:SetStatus("放置模式: " .. prefabName .. "  （点击场景放置，ESC 取消）")
end

function M:OnClickPlay()
    local state = EditorCore:GetState()
    if state == "Edit" then
        EditorCore:EnterPlayMode()
        self:SetStatus("试玩模式 — 按 F2 返回编辑")
    else
        EditorCore:EnterEditMode()
    end
end

function M:OnClickSave()
    local json = EditorCore:SaveSceneJSON()
    -- 暂时写到本地（之后对接 FastAPI）
    -- 用 UnLua 的 io.open 写文件（编辑器模式下可用）
    local path = UE.UKismetSystemLibrary.GetProjectDirectory() .. "Saved/UGC/scene_latest.json"
    local dir  = UE.UKismetSystemLibrary.GetProjectDirectory() .. "Saved/UGC/"
    -- 确保目录存在（通过引擎接口）
    UE.UKismetSystemLibrary.MakeDirectory(dir)
    local f = io.open(path, "w")
    if f then
        f:write(json)
        f:close()
        self:SetStatus("场景已保存 → " .. path)
        print("[WBP_UGCEditor] 保存成功: " .. path)
    else
        self:SetStatus("保存失败（文件写入错误）")
        print("[WBP_UGCEditor] 保存失败")
    end
end

function M:OnClickLoad()
    local path = UE.UKismetSystemLibrary.GetProjectDirectory() .. "Saved/UGC/scene_latest.json"
    local f = io.open(path, "r")
    if f then
        local json = f:read("*a")
        f:close()
        EditorCore:LoadSceneJSON(json)
        self:SetStatus("场景已加载 — " .. tostring(EditorCore:GetSelectedID() or "无选中"))
    else
        self:SetStatus("加载失败（找不到保存文件）")
        print("[WBP_UGCEditor] 找不到保存文件: " .. path)
    end
end

function M:OnClickClear()
    EditorCore:ClearScene()
    self:SetStatus("场景已清空")
    self:ClearTransformInputs()
end

function M:OnClickDelete()
    EditorCore:DeleteSelected()
    self:SetStatus("已删除选中物体")
    self:ClearTransformInputs()
end

function M:OnClickUndo()
    EditorCore:Undo()
    self:SetStatus("撤销完成")
    self:RefreshTransformInputs()
end

--============================================================
-- Transform 输入框
--============================================================

function M:OnTransformCommit(text, commitType)
    -- 任意输入框提交后，读取全部 9 个值并应用
    local function readFloat(widget)
        local t = widget:GetText()
        return tonumber(tostring(t)) or 0
    end

    local x   = readFloat(self.w_input_X)
    local y   = readFloat(self.w_input_Y)
    local z   = readFloat(self.w_input_Z)
    local p   = readFloat(self.w_input_P)
    local yaw = readFloat(self.w_input_Yaw)
    local r   = readFloat(self.w_input_R)
    local sx  = readFloat(self.w_input_SX)
    local sy  = readFloat(self.w_input_SY)
    local sz  = readFloat(self.w_input_SZ)

    EditorCore:SetSelectedTransform(x, y, z, p, yaw, r, sx, sy, sz)
end

function M:RefreshTransformInputs()
    local x,y,z, p,yaw,r, sx,sy,sz = EditorCore:GetSelectedTransformValues()
    local function setVal(widget, v)
        widget:SetText(string.format("%.1f", v))
    end
    setVal(self.w_input_X,   x)
    setVal(self.w_input_Y,   y)
    setVal(self.w_input_Z,   z)
    setVal(self.w_input_P,   p)
    setVal(self.w_input_Yaw, yaw)
    setVal(self.w_input_R,   r)
    setVal(self.w_input_SX,  sx)
    setVal(self.w_input_SY,  sy)
    setVal(self.w_input_SZ,  sz)
end

function M:ClearTransformInputs()
    for _, name in ipairs({"w_input_X","w_input_Y","w_input_Z","w_input_P","w_input_Yaw","w_input_R","w_input_SX","w_input_SY","w_input_SZ"}) do
        self[name]:SetText("")
    end
end

--============================================================
-- 视口点击转发（WBP 的 OnMouseButtonDown 事件绑定到这里）
--============================================================

function M:OnViewportMouseDown(geometry, pointerEvent)
    local pos = pointerEvent:GetScreenSpacePosition()
    EditorCore:OnViewportClick(pos.X, pos.Y)
    -- 点击后刷新 Transform 面板
    self:RefreshTransformInputs()
    -- 返回 Handled，阻止事件继续传递
    return UE.UWidgetBlueprintLibrary.Handled()
end

--============================================================
-- 工具
--============================================================

function M:SetStatus(msg)
    if self.w_text_Status then
        self.w_text_Status:SetText(msg)
    end
end

function M:RefreshPlayButton(state)
    if self.w_btn_Play then
        local label = (state == "Edit") and "▶ 试玩" or "✎ 返回编辑"
        -- Button 的子 Text Block 通常叫 ButtonText
        local textBlock = self.w_btn_Play:GetChildAt(0)
        if textBlock then textBlock:SetText(label) end
    end
end

return M
