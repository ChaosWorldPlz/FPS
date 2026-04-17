// Copyright Epic Games, Inc. All Rights Reserved.
// AnimGenClient.h — 文生 3D 总客户端组件，挂在 PlayerController 上

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Containers/Map.h"
#include "FPS/AnimAgent/AnimAgentTypes.h"
#include "AnimGenClient.generated.h"

class IAnimGenProvider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnAnimGenJobUpdated, const FString&, JobUuid, const FAnimGenJobStatus&, Status);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnAnimGenJobCompleted, const FString&, JobUuid, const FString&, LocalGLBPath);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnAnimGenJobFailed, const FString&, JobUuid, const FString&, ErrorMessage);

/**
 * UAnimGenClient
 *
 * 职责：
 * - 持有所有 Provider 实例
 * - 维护本地 Job 表 + 轮询定时器
 * - 下载完成的 glb 到本地缓存
 * - 通过事件广播通知 Lua / UI
 *
 * Lua 用法：
 *   local client = pc:GetAnimGenClient()
 *   client.OnJobUpdated:Add(callback)
 *   local uuid = client:CreateTask(...)
 */
UCLASS(ClassGroup = "AnimAgent", meta = (BlueprintSpawnableComponent))
class FPS_API UAnimGenClient : public UActorComponent
{
    GENERATED_BODY()

public:
    UAnimGenClient();

    //--------------------------------------------------------------
    // 配置
    //--------------------------------------------------------------

    /** Meshy API Key（运行时由 AnimAgentCore 注入；不持久化到 BP） */
    UPROPERTY(BlueprintReadWrite, Category = "AnimAgent|Config")
    FString MeshyApiKey;

    /** Tripo API Key */
    UPROPERTY(BlueprintReadWrite, Category = "AnimAgent|Config")
    FString TripoApiKey;

    /** 没有 Key 时是否走 Mock */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AnimAgent|Config")
    bool bAllowMockFallback = true;

    /** Mock 模式下使用的 sample.glb 资源路径（可空） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AnimAgent|Config")
    FString MockSampleGLBPath;

    /** 轮询间隔（秒） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AnimAgent|Config")
    float PollIntervalSeconds = 5.0f;

    /** 任务超时（秒） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AnimAgent|Config")
    float TaskTimeoutSeconds = 300.0f;

    //--------------------------------------------------------------
    // 事件（Lua 订阅）
    //--------------------------------------------------------------

    UPROPERTY(BlueprintAssignable, Category = "AnimAgent|Events")
    FOnAnimGenJobUpdated OnJobUpdated;

    UPROPERTY(BlueprintAssignable, Category = "AnimAgent|Events")
    FOnAnimGenJobCompleted OnJobCompleted;

    UPROPERTY(BlueprintAssignable, Category = "AnimAgent|Events")
    FOnAnimGenJobFailed OnJobFailed;

    //--------------------------------------------------------------
    // API
    //--------------------------------------------------------------

    /**
     * 创建生成任务
     * @return 本地 jobUuid，失败返回空串
     */
    UFUNCTION(BlueprintCallable, Category = "AnimAgent")
    FString CreateTask(EAnimGenProvider Provider, const FAnimGenRequest& Request);

    /** 查询本地任务状态（不发请求，只读） */
    UFUNCTION(BlueprintCallable, Category = "AnimAgent")
    FAnimGenJobStatus GetJobStatus(const FString& JobUuid) const;

    /** 取消任务（停止轮询，并尝试调 Provider 取消） */
    UFUNCTION(BlueprintCallable, Category = "AnimAgent")
    void CancelJob(const FString& JobUuid);

    /** 列出所有进行中的 Job uuid */
    UFUNCTION(BlueprintCallable, Category = "AnimAgent")
    TArray<FString> ListActiveJobs() const;

    //--------------------------------------------------------------
    // UActorComponent
    //--------------------------------------------------------------
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void TickPolling(FString JobUuid);
    void HandleQueried(FString JobUuid, FAnimGenJobStatus NewStatus);
    void StartDownload(FString JobUuid, FString ModelURL);
    void ClearTimer(const FString& JobUuid);

    /** Provider 表（共享指针，启动时构造） */
    TMap<EAnimGenProvider, TSharedPtr<IAnimGenProvider>> Providers;

    /** Job 表 */
    UPROPERTY()
    TMap<FString, FAnimGenJob> Jobs;

    /** uuid → 定时器句柄 */
    TMap<FString, FTimerHandle> PollTimers;
};
