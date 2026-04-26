--[[
    WBP_FabPanel.lua
    Fab 平台面板（路线 B 核心 UMG）

    控件树（需在编辑器里按这个命名搭建）：
      Root (Canvas Panel)
      └── Border "FabRoot"
          └── Vertical Box
              ├── Horizontal Box "TitleBar"
              │   ├── Text "TxtTitle"
              │   └── Button "BtnClose"
              ├── WebBrowser "WebBrowser"
              └── Horizontal Box "StatusBar"
                  ├── Text "TxtStatus"
                  └── ProgressBar "BarProgress"

    职责划分：
    - Lua：
        * 初始化：Init FabClient，拿到 bridge / base_url
        * 注入：每次 OnLoadCompleted 往 CEF 注 cookie + 客户端标识
        * 拦截：OnUrlChanged 时识别 uefab:// scheme，调 BP 派发；回退 URL
        * 下载完成回调（BP 拿到 C++ 结果后回灌 Lua）：走 FabClient:RegisterDownloadedAsset
        * 状态栏显示 + 关闭按钮

    - Blueprint：
        * 承接 UFabUrlDispatcher 的动态委托（Lua 直接 Bind 动态委托坑多）
        * 订阅 Bridge 的 OnDownloadProgress 多播事件 → 调 Lua SetProgress
        * 接收 Lua 信号后 CreateWidget / AddToViewport / RemoveFromParent 等容器管理

    约定：
    - Lua 里所有 "需要 BP 承接 / 派发" 的函数都以 BP_ 前缀命名（BP 里用 Custom Event 同名节点调过来）
]]

local UEClass = UE.UClass

local WBP_FabPanel = {}

-- 静态：打开面板前要用 "BP 里 CreateWidget + AddToViewport" 组合；Lua 没法直接实例化 UMG

--============================================================
-- 生命周期
--============================================================

function WBP_FabPanel:Construct()
    self.BaseUrl   = "http://127.0.0.1:8000"
    self.LastGoodUrl = ""
    self.PendingDownloads = {}    -- { [asset_id] = true } 去重
    self.Initialized = false

    -- 读取 FabClient（单例）拿到 bridge
    local ok, FabClient = pcall(function()
        return require("Gameplay.Fab.FabClient")
    end)
    if not ok or not FabClient then
        self:_setStatus("加载 FabClient 失败", true)
        return
    end
    self.FabClient = FabClient

    -- 基于 UWorld 去找 PlayerController；用 UGameplayStatics
    local pc = nil
    pcall(function()
        pc = UE.UGameplayStatics.GetPlayerController(self, 0)
    end)
    if not pc then
        self:_setStatus("未找到 PlayerController", true)
        return
    end

    if not FabClient:Init(pc) then
        self:_setStatus("FabClient 初始化失败", true)
        return
    end
    self.Bridge = FabClient:GetBridge()
    if not self.Bridge then
        self:_setStatus("未找到 FabClientBridge 组件", true)
        return
    end

    -- 读取配置里的 BaseUrl（若 UFabConfig UCLASS 可访问）
    pcall(function()
        local cfg = UE.UFabConfig.Get()
        if cfg and cfg.BaseUrl and cfg.BaseUrl ~= "" then
            self.BaseUrl = cfg.BaseUrl
        end
    end)

    -- 绑定 WebBrowser 事件（BP 里会把 OnUrlChanged/OnLoadCompleted 委托路由到这里）
    if self.BtnClose then
        self.BtnClose.OnClicked:Add(self, WBP_FabPanel.OnClickClose)
    end

    -- 初始状态
    self:_setStatus("加载中…", false)
    self:_setProgress(-1)  -- 隐藏进度条

    -- 首次导航：交给 BP 做 WebBrowser:LoadURL(BaseUrl)
    -- （UnLua 对 UWebBrowser:LoadURL 的绑定不稳定，放 BP 更保险）
    self:BP_RequestInitialNavigate(self.BaseUrl)

    self.Initialized = true
    print(string.format("[WBP_FabPanel] Construct 完成，BaseUrl=%s", self.BaseUrl))
end

function WBP_FabPanel:Destruct()
    self.PendingDownloads = nil
    print("[WBP_FabPanel] Destruct")
end

--============================================================
-- WebBrowser 事件入口（由 BP 在 OnUrlChanged / OnLoadCompleted 里调过来）
--============================================================

--- 每次页面加载完成：注 cookie + 注 UE 客户端标识
--- BP 实现示例：
---   OnLoadCompleted(Url):
---     Lua:OnWebLoadCompleted(Url)
function WBP_FabPanel:OnWebLoadCompleted(Url)
    if not self.Initialized then return end

    local access = self.FabClient:GetAccessToken() or ""
    local refresh = self.FabClient:GetRefreshToken() or ""

    -- 注入 cookie + 全局标识；SameSite=Lax 可让浏览器带到同源请求
    local js = string.format([[
        (function() {
            document.cookie = "access_token=%s; path=/; SameSite=Lax";
            document.cookie = "refresh_token=%s; path=/; SameSite=Lax";
            window.__FAB_UE_CLIENT__ = true;
        })();
    ]], access:gsub('"', '\\"'), refresh:gsub('"', '\\"'))

    self:BP_ExecuteJavascript(js)

    -- 不是 scheme URL 才记为"上一个有效 URL"
    if Url and not self:_isFabScheme(Url) then
        self.LastGoodUrl = Url
    end
end

--- URL 变化时检查是否命中 uefab:// scheme；命中则派发并回退 URL
--- BP 实现示例：
---   OnUrlChanged(Text):
---     Url = Text:ToString()
---     Lua:OnWebUrlChanged(Url)
function WBP_FabPanel:OnWebUrlChanged(Url)
    if not self.Initialized or not Url or Url == "" then return end

    if not self:_isFabScheme(Url) then
        -- 正常导航，更新 LastGood
        self.LastGoodUrl = Url
        return
    end

    print(string.format("[WBP_FabPanel] 拦截 uefab scheme: %s", Url))

    -- 回退：立即导航回上一个有效 URL，避免 CEF 停在 "unsupported scheme" 错误页
    if self.LastGoodUrl and self.LastGoodUrl ~= "" then
        self:BP_NavigateTo(self.LastGoodUrl)
    else
        self:BP_NavigateTo(self.BaseUrl)
    end

    -- 派发：交给 BP 调 UFabUrlDispatcher.DispatchDownload(bridge, Url, BP_OnDownloadDone)
    self:BP_DispatchFabUrl(Url)
    self:_setStatus("处理请求: " .. Url, false)
end

--============================================================
-- 下载生命周期回调（由 BP 在 UFabUrlDispatcher 完成 / 进度事件里调过来）
--============================================================

--- BP 绑 UFabClientBridge::OnDownloadProgress 多播委托，把参数透传给 Lua
--- 签名: int32 AssetId, int32 BytesReceived, int32 TotalBytes, FString LocalPath
function WBP_FabPanel:OnDownloadProgress(AssetId, BytesReceived, TotalBytes, LocalPath)
    self.PendingDownloads[AssetId] = true

    local pct = -1
    if TotalBytes and TotalBytes > 0 then
        pct = BytesReceived / TotalBytes
    end
    self:_setProgress(pct)
    self:_setStatus(string.format("下载中 #%d  %.1f KB", AssetId, (BytesReceived or 0) / 1024), false)
end

--- BP 在 UFabUrlDispatcher.DispatchDownload 的完成回调里调过来
--- @param Err   FFabError（Lua 镜像：HttpCode / BizCode / Message）
--- @param Result FFabDownloadResult（AssetId / LocalFilePath / LocalUuid / SizeBytes）
function WBP_FabPanel:OnDownloadDone(Err, Result)
    self:_setProgress(-1)  -- 隐藏

    local assetId = (Result and Result.AssetId) or 0
    self.PendingDownloads[assetId] = nil

    -- err.Message 在 UnLua 里是 FString，转 string 保险
    local msg = Err and Err.Message and tostring(Err.Message) or ""
    local httpCode = Err and Err.HttpCode or 0
    local bizCode = Err and Err.BizCode or 0
    local ok = (httpCode == 200 or httpCode == 0) and bizCode == 0

    if not ok then
        self:_setStatus(string.format("下载失败: %s (http=%d biz=%d)",
            msg ~= "" and msg or "unknown", httpCode, bizCode), true)
        return
    end

    -- 成功：走 FabClient:RegisterDownloadedAsset（入库 + 注册 + glTFRuntime 预加载）
    local uuid = self.FabClient:RegisterDownloadedAsset({
        AssetId       = assetId,
        LocalFilePath = Result and Result.LocalFilePath and tostring(Result.LocalFilePath) or "",
        LocalUuid     = Result and Result.LocalUuid and tostring(Result.LocalUuid) or "",
        SizeBytes     = Result and Result.SizeBytes or 0,
    }, nil)

    if uuid and uuid ~= "" then
        self:_setStatus(string.format("已添加到 Project: %s", uuid), false)
    else
        self:_setStatus("下载成功但入库失败", true)
    end
end

--============================================================
-- 关闭按钮
--============================================================

function WBP_FabPanel:OnClickClose()
    self:BP_RequestClose()
end

--============================================================
-- 内部工具
--============================================================

function WBP_FabPanel:_isFabScheme(Url)
    if not Url then return false end
    local lower = tostring(Url):lower()
    return lower:sub(1, 8) == "uefab://"
end

function WBP_FabPanel:_setStatus(text, isError)
    if not self.TxtStatus then return end
    local ft = UE.FText(text or "")
    pcall(function() self.TxtStatus:SetText(ft) end)
    -- 颜色变化交给 BP 实现（Lua 改字体颜色需要 FSlateFontInfo 的写法略繁，先忽略）
end

--- @param pct number|nil  0.0~1.0 显示进度；-1 隐藏；nil=indeterminate
function WBP_FabPanel:_setProgress(pct)
    if not self.BarProgress then return end
    if not pct or pct < 0 then
        pcall(function() self.BarProgress:SetVisibility(UE.ESlateVisibility.Collapsed) end)
    else
        pcall(function()
            self.BarProgress:SetVisibility(UE.ESlateVisibility.SelfHitTestInvisible)
            self.BarProgress:SetPercent(pct)
        end)
    end
end

--============================================================
-- BP 侧占位（Lua 发信号 / 等接收；在 BP 图表里用 Custom Event 实现）
--============================================================
-- 只是文档性质：Lua 自己不会 override，实际由 BP 图表实现后 overload

function WBP_FabPanel:BP_RequestInitialNavigate(Url) end
function WBP_FabPanel:BP_NavigateTo(Url) end
function WBP_FabPanel:BP_ExecuteJavascript(Js) end
function WBP_FabPanel:BP_DispatchFabUrl(SchemeUrl) end
function WBP_FabPanel:BP_RequestClose() end

return WBP_FabPanel
