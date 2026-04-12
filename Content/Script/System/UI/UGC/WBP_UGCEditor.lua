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

local LOG_TAG = "[System.UI.UGC.WBP_UGCEditor]"

local function Log(msg)
    print(LOG_TAG .. " " .. tostring(msg))
end

local function Warn(msg)
    print(LOG_TAG .. "[Warn] " .. tostring(msg))
end

local function bindButton(self, widgetName, handler)
    local widget = self[widgetName]
    if not widget then
        Warn("缺少按钮控件: " .. widgetName .. "（请检查蓝图命名和 Is Variable）")
        return false
    end
    if not widget.OnClicked then
        Warn("按钮控件没有 OnClicked: " .. widgetName)
        return false
    end
    widget.OnClicked:Add(self, handler)
    return true
end

local function bindTextCommit(self, widgetName)
    local widget = self[widgetName]
    if not widget then
        Warn("缺少输入框控件: " .. widgetName .. "（请检查蓝图命名和 Is Variable）")
        return false
    end
    if not widget.OnTextCommitted then
        Warn("输入框控件没有 OnTextCommitted: " .. widgetName)
        return false
    end
    widget.OnTextCommitted:Add(self, M.OnTransformCommit)
    return true
end

--============================================================
-- 生命周期
--============================================================

function M:Construct()
    local missingCount = 0

    for _, pair in ipairs({
        { "w_btn_Play",      M.OnClickPlay },
        { "w_btn_Save",      M.OnClickSave },
        { "w_btn_Load",      M.OnClickLoad },
        { "w_btn_Clear",     M.OnClickClear },
        { "w_btn_Delete",    M.OnClickDelete },
        { "w_btn_Undo",      M.OnClickUndo },
        { "w_btn_Blueprint",       M.OnClickBlueprint },
        { "w_btn_Blueprint_Actor", M.OnClickActorBlueprint },
    }) do
        if not bindButton(self, pair[1], pair[2]) then
            missingCount = missingCount + 1
        end
    end

    for _, widgetName in ipairs({
        "w_input_X", "w_input_Y", "w_input_Z",
        "w_input_P", "w_input_Yaw", "w_input_R",
        "w_input_SX", "w_input_SY", "w_input_SZ",
    }) do
        if not bindTextCommit(self, widgetName) then
            missingCount = missingCount + 1
        end
    end

    if missingCount > 0 then
        Warn("Construct: 共发现 " .. tostring(missingCount) .. " 个控件未正确绑定")
    end

    self:BuildPrefabList()

    -- 监听状态变化（刷新试玩按钮文字）
    EditorCore:OnStateChanged(function(state)
        self:RefreshPlayButton(state)
    end)

    -- 监听选中变化（刷新 Transform 面板 + Actor 蓝图按钮可见性）
    EditorCore:OnSelectionChanged(function(sceneID)
        if sceneID then
            self:RefreshTransformInputs()
        else
            self:ClearTransformInputs()
        end
        self:RefreshActorBlueprintBtn(sceneID)
    end)

    -- Actor 蓝图按钮初始隐藏（无选中时不显示）
    self:RefreshActorBlueprintBtn(nil)

    self:SetStatus("就绪 — 点击预制体开始放置")

    -- 初始化网格对齐 UI（控件可选，不存在时只打 Warn 不报错）
    self:BuildSnapUI()


    Log("UI 构建完成")
end

--============================================================
-- 预制体列表（动态生成按钮）
--============================================================

--- 重建预制体列表（AddCustomPrefab 后调用刷新 UI）
function M:RebuildPrefabList()
    if not self.w_panel_Prefabs then
        Warn("RebuildPrefabList: 找不到 w_panel_Prefabs")
        return
    end
    self.w_panel_Prefabs:ClearChildren()
    self:BuildPrefabList()
end

local BTN_CLASS_PATH = "/Game/_UGC/UI/WBP_UGCPrefabBtn.WBP_UGCPrefabBtn_C"
local _btnClass = nil

local function getBtnClass()
    if not _btnClass then
        _btnClass = UE.UClass.Load(BTN_CLASS_PATH)
        if not _btnClass then
            Warn("找不到 WBP_UGCPrefabBtn，路径: " .. BTN_CLASS_PATH)
        end
    end
    return _btnClass
end

function M:BuildPrefabList()
    if not self.w_panel_Prefabs then
        Warn("BuildPrefabList: 找不到 w_panel_Prefabs，请检查蓝图是否勾选 Is Variable")
        self:SetStatus("预制体面板未绑定")
        return
    end

    local cats = PrefabRegistry.Categories or {}
    Log(string.format("BuildPrefabList: %d 个分类", #cats))

    if #cats == 0 then
        Warn("当前没有可用预制体，请检查 Placeables 目录、manifest 或注册表日志")
        self:SetStatus("未找到预制体")
        return
    end

    local pc = self:GetOwningPlayer()
    if not pc then
        Warn("BuildPrefabList: GetOwningPlayer 返回 nil")
        self:SetStatus("无法获取 OwningPlayer")
        return
    end

    local btnClass = getBtnClass()
    if not btnClass then
        self:SetStatus("列表项蓝图丢失")
        return
    end

    local addedCount = 0

    for _, category in ipairs(cats) do
        local header = UE.UWidgetBlueprintLibrary.Create(pc, btnClass, pc)
        if header then
            if header.w_label then
                header.w_label:SetText("── " .. category.name .. " ──")
            else
                Warn("分类按钮缺少 w_label: " .. tostring(category.name))
            end
            if header.w_btn then
                header.w_btn:SetIsEnabled(false)
            else
                Warn("分类按钮缺少 w_btn: " .. tostring(category.name))
            end
            self.w_panel_Prefabs:AddChild(header)
        else
            Warn("创建分类标题失败: " .. tostring(category.name))
        end

        for _, item in ipairs(category.items or {}) do
            local btn = UE.UWidgetBlueprintLibrary.Create(pc, btnClass, pc)
            if btn then
                if btn.w_label then
                    btn.w_label:SetText(item.label)
                else
                    Warn("预制体按钮缺少 w_label: " .. tostring(item.id))
                end

                local prefabID = item.id
                if btn.w_btn and btn.w_btn.OnPressed then
                    btn.w_btn.OnPressed:Add(self, function()
                        self:OnClickPrefab(prefabID)
                    end)
                else
                    Warn("预制体按钮缺少 w_btn 或 OnPressed: " .. tostring(item.id))
                end

                self.w_panel_Prefabs:AddChild(btn)
                addedCount = addedCount + 1
                Log("添加: " .. tostring(item.id))
            else
                Warn("创建预制体按钮失败: " .. tostring(item.id))
            end
        end
    end

    Log(string.format("BuildPrefabList 完成，共添加 %d 个预制体按钮", addedCount))
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
    local bridge = EditorCore:GetBridge()
    local defDir = UE.UKismetSystemLibrary.GetProjectDirectory() .. "Saved/UGC/"
    os.execute('mkdir "' .. defDir:gsub("/", "\\") .. '" 2>NUL')

    -- 先把蓝图编辑器当前图存回 SceneData
    local UIManager = require("Gameplay.Core.UIManager")
    local bpInst = UIManager:GetWindow("WBP_UGCBlueprintEditor")
    if bpInst and bpInst.SaveCurrentGraphToSceneData then
        bpInst:SaveCurrentGraphToSceneData()
    end

    -- 对话框：用户输入场景名，返回路径作为文件夹名使用（无需扩展名）
    local picked = bridge:ShowSaveFileDialog("保存场景（输入场景名）", defDir, "my_scene", "所有文件|*.*")
    if not picked or picked == "" then
        self:SetStatus("保存已取消")
        return
    end

    -- 统一正斜杠，去掉用户可能多打的扩展名，末尾补 /
    picked = picked:gsub("\\", "/")
    local folderPath = picked:gsub("%.[^/]+$", "") .. "/"

    -- 创建场景子文件夹
    local folderWin = folderPath:gsub("/", "\\"):gsub("\\$", "")
    os.execute('mkdir "' .. folderWin .. '" 2>NUL')

    local SceneData = require("Gameplay.UGC.UGCSceneData")

    local function writeFile(p, content)
        local f = io.open(p, "w")
        if f then f:write(content); f:close(); return true end
        return false
    end

    local ok1 = writeFile(folderPath .. "scene.json",    SceneData:SerializeToJSON())
    local ok2 = writeFile(folderPath .. "programs.json", SceneData:SerializeProgramsJSON())
    local ok3 = writeFile(folderPath .. "editor.json",   SceneData:SerializeEditorJSON())

    if ok1 and ok2 and ok3 then
        SceneData:ClearDirty()
        self:SetStatus("场景已保存 → " .. folderPath)
        Log("保存成功: " .. folderPath)
    else
        self:SetStatus("保存部分失败，请检查日志")
        Warn("保存失败 scene=" .. tostring(ok1) .. " prog=" .. tostring(ok2) .. " editor=" .. tostring(ok3))
    end
end

function M:OnClickLoad()
    local bridge = EditorCore:GetBridge()
    local defDir = UE.UKismetSystemLibrary.GetProjectDirectory() .. "Saved/UGC/"

    -- 用户导航到场景文件夹，选中里面的 scene.json
    local path = bridge:ShowOpenFileDialog("加载场景（选择文件夹内的 scene.json）", defDir, "JSON 文件|*.json")
    if not path or path == "" then
        self:SetStatus("加载已取消")
        return
    end

    -- 立刻显示加载遮罩，阻止加载期间的误操作
    self:ShowLoading("场景加载中…")

    path = path:gsub("\\", "/")

    -- 从选中文件推断出所在文件夹
    local folderPath = path:match("^(.*/)") or defDir

    local function readFile(p)
        local f = io.open(p, "r")
        if not f then return nil end
        local s = f:read("*a"); f:close(); return s
    end

    -- 加载 scene.json（优先从同目录读，兼容用户直接选中 scene.json 或选了别的 json）
    local sceneJSON = readFile(folderPath .. "scene.json")
    if not sceneJSON then
        self:HideLoading()
        self:SetStatus("加载失败（文件夹内找不到 scene.json）")
        Warn("找不到: " .. folderPath .. "scene.json")
        return
    end
    EditorCore:LoadSceneJSON(sceneJSON)

    -- 加载 programs.json（可选）
    local programsJSON = readFile(folderPath .. "programs.json")
    if programsJSON then
        local SceneData = require("Gameplay.UGC.UGCSceneData")
        SceneData:DeserializeProgramsJSON(programsJSON)
        Log("programs.json 加载成功")
    else
        Log("programs.json 不存在，跳过")
    end

    -- editor.json（目前仅存根，跳过处理）

    local folderName = folderPath:match("([^/]+)/$") or folderPath
    local doneMsg    = "场景已加载 ← " .. folderName
    Log("加载成功: " .. folderPath)

    -- 延迟 3 帧后隐藏遮罩：给引擎足够时间完成 Actor BeginPlay 等延迟初始化，
    -- 避免玩家在同一帧或次帧立刻操作引发 TryBind 重入崩溃
    local pc = self:GetOwningPlayer()
    if pc and pc.ScheduleCallback then
        pc:ScheduleCallback(function()
            self:HideLoading()
            self:SetStatus(doneMsg)
        end, 3)
    else
        -- 降级：直接隐藏（不延迟）
        self:HideLoading()
        self:SetStatus(doneMsg)
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

function M:OnClickBlueprint()
    local UIManager = require("Gameplay.Core.UIManager")
    local name = "WBP_UGCBlueprintEditor"
    local inst = UIManager:GetWindow(name) or UIManager:OpenWindow(name)
    if inst then
        local SceneData = require("Gameplay.UGC.UGCSceneData")
        -- 把已保存的图数据直接传给 OpenGraph，由其在切换前先 RemoveFromParent 旧 widget，
        -- 避免 LoadGraphData+OpenGraph 分步调用导致旧 widget 孤立触发 Lua GC 重入崩溃
        inst:OpenGraph("level_main", "关卡蓝图 — 全局逻辑", SceneData:GetLevelScript())
        local pc = self:GetOwningPlayer()
        if pc and pc.SetBlueprintEditor then pc:SetBlueprintEditor(inst) end
    end
    self:SetStatus("关卡蓝图编辑器已打开")
end

--- 选中 Actor 后点击「配置逻辑」，打开该 Actor 专属的蓝图图
function M:OnClickActorBlueprint()
    local sceneID = EditorCore:GetSelectedID()
    if not sceneID then
        self:SetStatus("请先选中一个 Actor")
        return
    end

    local UIManager = require("Gameplay.Core.UIManager")
    local name = "WBP_UGCBlueprintEditor"
    local inst = UIManager:GetWindow(name) or UIManager:OpenWindow(name)
    if not inst then
        self:SetStatus("打开蓝图编辑器失败")
        return
    end

    local SceneData = require("Gameplay.UGC.UGCSceneData")
    local entry     = SceneData:QueryActor(sceneID)
    local programID = (entry and entry.programId) or ("actor_prog_" .. sceneID)

    -- 把已保存的图数据直接传给 OpenGraph，由其在切换前先 RemoveFromParent 旧 widget，
    -- 避免 LoadGraphData+OpenGraph 分步调用导致旧 widget 孤立触发 Lua GC 重入崩溃
    inst:OpenGraph(programID, "Actor #" .. tostring(sceneID) .. " 蓝图", SceneData:GetScript(programID))

    local pc = self:GetOwningPlayer()
    if pc and pc.SetBlueprintEditor then pc:SetBlueprintEditor(inst) end
    self:SetStatus("Actor #" .. tostring(sceneID) .. " 蓝图编辑器已打开")
end

--- 根据是否有选中 Actor 控制「配置逻辑」按钮可见性
function M:RefreshActorBlueprintBtn(sceneID)
    if not self.w_btn_Blueprint_Actor then return end
    local vis = sceneID and UE.ESlateVisibility.Visible or UE.ESlateVisibility.Collapsed
    self.w_btn_Blueprint_Actor:SetVisibility(vis)
end

--============================================================
-- Transform 输入框
--============================================================

function M:OnTransformCommit(text, commitType)
    local function readFloat(widget)
        if not widget then return 0 end
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
    local function setVal(widget, v, widgetName)
        if widget then
            widget:SetText(string.format("%.1f", v))
        elseif widgetName then
            Warn("RefreshTransformInputs: 缺少控件 " .. widgetName)
        end
    end
    setVal(self.w_input_X,   x,   "w_input_X")
    setVal(self.w_input_Y,   y,   "w_input_Y")
    setVal(self.w_input_Z,   z,   "w_input_Z")
    setVal(self.w_input_P,   p,   "w_input_P")
    setVal(self.w_input_Yaw, yaw, "w_input_Yaw")
    setVal(self.w_input_R,   r,   "w_input_R")
    setVal(self.w_input_SX,  sx,  "w_input_SX")
    setVal(self.w_input_SY,  sy,  "w_input_SY")
    setVal(self.w_input_SZ,  sz,  "w_input_SZ")
end

function M:ClearTransformInputs()
    for _, name in ipairs({"w_input_X","w_input_Y","w_input_Z","w_input_P","w_input_Yaw","w_input_R","w_input_SX","w_input_SY","w_input_SZ"}) do
        if self[name] then
            self[name]:SetText("")
        end
    end
end

--============================================================
-- 视口点击转发（WBP 的 OnMouseButtonDown 事件绑定到这里）
-- 注意：点击逻辑已由 IA_EditorClick → UGCPlayerController:EditorClick() 统一处理，
--       此处仅返回 Handled 消费掉 Slate 事件，不重复调用 OnViewportClick，
--       避免双重触发（InputAction + Widget 各一次）。
--============================================================

function M:OnViewportMouseDown(geometry, pointerEvent)
    return UE.UWidgetBlueprintLibrary.Handled()
end

--============================================================
-- 工具
--============================================================

function M:SetStatus(msg)
    if self.w_text_Status then
        -- 有未保存修改时在状态栏末尾附加 * 提示
        local SceneData = require("Gameplay.UGC.UGCSceneData")
        local dirty = SceneData and SceneData:IsDirty()
        self.w_text_Status:SetText(dirty and (msg .. "  ●未保存") or msg)
    else
        Warn("缺少状态文本控件 w_text_Status，消息: " .. tostring(msg))
    end
end

--============================================================
-- 加载遮罩（w_panel_Loading 控件可选，不存在时只禁用交互）
-- WBP_UGCEditor 蓝图结构参考：
--   在根 Overlay 的最顶层加一个 w_panel_Loading（Border 或 Overlay），
--   填满父级，颜色 #CC000000（半透明黑），默认 Visibility = Collapsed；
--   里面放一个居中的 TextBlock 写"加载中…"即可。
--============================================================

function M:ShowLoading(msg)
    -- 显示遮罩
    if self.w_panel_Loading then
        self.w_panel_Loading:SetVisibility(UE.ESlateVisibility.Visible)
        -- 把加载提示写到遮罩内部文本（w_text_Loading 可选）
        if self.w_text_Loading then
            self.w_text_Loading:SetText(msg or "加载中…")
        end
    end
    -- 禁用底部操作栏，防止加载期间误操作（loading 遮罩也能阻挡，双保险）
    if self.w_panel_Bottom then self.w_panel_Bottom:SetIsEnabled(false) end
    self:SetStatus(msg or "加载中…")
end

function M:HideLoading()
    if self.w_panel_Loading then
        self.w_panel_Loading:SetVisibility(UE.ESlateVisibility.Collapsed)
    end
    if self.w_panel_Bottom then self.w_panel_Bottom:SetIsEnabled(true) end
end

--============================================================
-- 网格对齐 UI（控件可选：w_btn_SnapToggle / w_combo_SnapSize）
--============================================================

function M:BuildSnapUI()
    -- Snap 开关按钮
    local btnSnap = self.w_btn_SnapToggle
    if btnSnap then
        -- 设置初始文字
        local textBlock = btnSnap:GetChildAt(0)
        if textBlock then
            textBlock:SetText("Snap: ON")
        end
        -- 绑定点击事件
        bindButton(self, "w_btn_SnapToggle", function()
            local enabled = not EditorCore:GetSnapEnabled()
            EditorCore:SetSnapEnabled(enabled)
            -- 刷新按钮文字
            local tb = self.w_btn_SnapToggle:GetChildAt(0)
            if tb then
                tb:SetText(enabled and "Snap: ON" or "Snap: OFF")
            end
        end)
    else
        Warn("BuildSnapUI: 未找到 w_btn_SnapToggle（可在蓝图中添加以启用 Snap 切换按钮）")
    end

    -- 网格尺度下拉框
    local combo = self.w_combo_SnapSize
    if combo then
        -- 添加尺度选项
        for _, v in ipairs({ "5", "25", "50", "100", "200" }) do
            combo:AddOption(v)
        end
        -- 默认选中 50
        combo:SetSelectedOption("50")
        -- 绑定选项变化事件
        combo.OnSelectionChanged:Add(self, function(val, selType)
            local size = tonumber(val)
            if size then
                EditorCore:SetSnapSize(size)
                Log("网格尺度设置为: " .. tostring(size))
            end
        end)
    else
        Warn("BuildSnapUI: 未找到 w_combo_SnapSize（可在蓝图中添加以启用尺度选择下拉框）")
    end
end


function M:RefreshPlayButton(state)
    if self.w_btn_Play then
        local label = (state == "Edit") and "▶ 试玩" or "✎ 返回编辑"
        local textBlock = self.w_btn_Play:GetChildAt(0)
        if textBlock then
            textBlock:SetText(label)
        else
            Warn("试玩按钮缺少子文本控件")
        end
    end
end

return M
