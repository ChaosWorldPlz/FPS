// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimGenClient.h"
#include "IAnimGenProvider.h"
#include "Provider/MockProvider.h"
#include "Provider/MeshyProvider.h"
#include "Provider/TripoProvider.h"

#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManager.h"
#include "TimerManager.h"
#include "Engine/World.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnimGenClient, Log, All);

UAnimGenClient::UAnimGenClient()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAnimGenClient::BeginPlay()
{
    Super::BeginPlay();

    // 构造所有 Provider 实例（无 Key 也允许构造，调用时再校验）
    TSharedPtr<FMockProvider> Mock = MakeShared<FMockProvider>();
    if (!MockSampleGLBPath.IsEmpty())
    {
        Mock->SetSampleGLBPath(MockSampleGLBPath);
    }
    Providers.Add(EAnimGenProvider::Mock, Mock);
    Providers.Add(EAnimGenProvider::Meshy, MakeShared<FMeshyProvider>());
    Providers.Add(EAnimGenProvider::Tripo, MakeShared<FTripoProvider>());

    UE_LOG(LogAnimGenClient, Log, TEXT("AnimGenClient ready (Mock fallback=%s)"),
        bAllowMockFallback ? TEXT("on") : TEXT("off"));
}

void UAnimGenClient::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        for (auto& Pair : PollTimers)
        {
            World->GetTimerManager().ClearTimer(Pair.Value);
        }
    }
    PollTimers.Empty();
    Jobs.Empty();
    Providers.Empty();

    Super::EndPlay(EndPlayReason);
}

FString UAnimGenClient::CreateTask(EAnimGenProvider Provider, const FAnimGenRequest& Request)
{
    // 选择实际 Provider：若主选 Provider 没 Key 且允许 mock，回退到 Mock
    EAnimGenProvider Resolved = Provider;
    const bool bMissingKey =
        (Provider == EAnimGenProvider::Meshy && MeshyApiKey.IsEmpty()) ||
        (Provider == EAnimGenProvider::Tripo && TripoApiKey.IsEmpty());

    if (bMissingKey)
    {
        if (bAllowMockFallback)
        {
            UE_LOG(LogAnimGenClient, Warning, TEXT("CreateTask: %d 未配置 Key，回退 Mock"), (int32)Provider);
            Resolved = EAnimGenProvider::Mock;
        }
        else
        {
            UE_LOG(LogAnimGenClient, Error, TEXT("CreateTask: %d 未配置 Key 且禁用 Mock 回退"), (int32)Provider);
            return FString();
        }
    }

    TSharedPtr<IAnimGenProvider>* Found = Providers.Find(Resolved);
    if (!Found || !Found->IsValid())
    {
        UE_LOG(LogAnimGenClient, Error, TEXT("CreateTask: provider %d 未注册"), (int32)Resolved);
        return FString();
    }

    const FString JobUuid = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);

    FAnimGenJob Job;
    Job.JobUuid = JobUuid;
    Job.Provider = Resolved;
    Job.Request = Request;
    Job.Status.State = EAnimGenState::Pending;
    Job.CreatedAtSeconds = FPlatformTime::Seconds();
    Jobs.Add(JobUuid, Job);

    const FString ApiKey = (Resolved == EAnimGenProvider::Meshy) ? MeshyApiKey
        : (Resolved == EAnimGenProvider::Tripo) ? TripoApiKey
        : FString();

    TWeakObjectPtr<UAnimGenClient> WeakThis(this);
    FOnAnimGenTaskCreated OnCreated;
    OnCreated.BindLambda([WeakThis, JobUuid](bool bOk, FString TaskId, FString Error)
    {
        UAnimGenClient* Self = WeakThis.Get();
        if (!Self) return;

        FAnimGenJob* JobPtr = Self->Jobs.Find(JobUuid);
        if (!JobPtr) return;

        if (!bOk)
        {
            JobPtr->Status.State = EAnimGenState::Failed;
            JobPtr->Status.ErrorMessage = Error;
            Self->OnJobUpdated.Broadcast(JobUuid, JobPtr->Status);
            Self->OnJobFailed.Broadcast(JobUuid, Error);
            return;
        }

        JobPtr->ProviderTaskId = TaskId;
        JobPtr->Status.State = EAnimGenState::Running;
        Self->OnJobUpdated.Broadcast(JobUuid, JobPtr->Status);

        // 启动轮询
        if (UWorld* World = Self->GetWorld())
        {
            FTimerHandle Handle;
            FTimerDelegate Del = FTimerDelegate::CreateUObject(Self, &UAnimGenClient::TickPolling, JobUuid);
            World->GetTimerManager().SetTimer(Handle, Del, Self->PollIntervalSeconds, true, 0.5f);
            Self->PollTimers.Add(JobUuid, Handle);
        }
    });

    (*Found)->CreateTextTo3DTask(ApiKey, Request, OnCreated);
    return JobUuid;
}

FAnimGenJobStatus UAnimGenClient::GetJobStatus(const FString& JobUuid) const
{
    if (const FAnimGenJob* Job = Jobs.Find(JobUuid))
    {
        return Job->Status;
    }
    FAnimGenJobStatus Empty;
    Empty.State = EAnimGenState::Failed;
    Empty.ErrorMessage = TEXT("Job 不存在");
    return Empty;
}

void UAnimGenClient::CancelJob(const FString& JobUuid)
{
    FAnimGenJob* Job = Jobs.Find(JobUuid);
    if (!Job) return;

    ClearTimer(JobUuid);

    if (TSharedPtr<IAnimGenProvider>* Found = Providers.Find(Job->Provider))
    {
        const FString ApiKey = (Job->Provider == EAnimGenProvider::Meshy) ? MeshyApiKey
            : (Job->Provider == EAnimGenProvider::Tripo) ? TripoApiKey
            : FString();
        (*Found)->CancelTask(ApiKey, Job->ProviderTaskId);
    }

    Job->Status.State = EAnimGenState::Cancelled;
    OnJobUpdated.Broadcast(JobUuid, Job->Status);
}

TArray<FString> UAnimGenClient::ListActiveJobs() const
{
    TArray<FString> Result;
    for (const auto& Pair : Jobs)
    {
        const EAnimGenState S = Pair.Value.Status.State;
        if (S == EAnimGenState::Pending || S == EAnimGenState::Running)
        {
            Result.Add(Pair.Key);
        }
    }
    return Result;
}

//------------------------------------------------------------------
// 私有
//------------------------------------------------------------------

void UAnimGenClient::TickPolling(FString JobUuid)
{
    FAnimGenJob* Job = Jobs.Find(JobUuid);
    if (!Job)
    {
        ClearTimer(JobUuid);
        return;
    }

    // 超时检查
    const double Elapsed = FPlatformTime::Seconds() - Job->CreatedAtSeconds;
    if (Elapsed > TaskTimeoutSeconds)
    {
        ClearTimer(JobUuid);
        Job->Status.State = EAnimGenState::Failed;
        Job->Status.ErrorMessage = FString::Printf(TEXT("任务超时 (%.0fs)"), Elapsed);
        OnJobUpdated.Broadcast(JobUuid, Job->Status);
        OnJobFailed.Broadcast(JobUuid, Job->Status.ErrorMessage);
        return;
    }

    TSharedPtr<IAnimGenProvider>* Found = Providers.Find(Job->Provider);
    if (!Found || !Found->IsValid())
    {
        ClearTimer(JobUuid);
        return;
    }

    const FString ApiKey = (Job->Provider == EAnimGenProvider::Meshy) ? MeshyApiKey
        : (Job->Provider == EAnimGenProvider::Tripo) ? TripoApiKey
        : FString();

    TWeakObjectPtr<UAnimGenClient> WeakThis(this);
    FOnAnimGenTaskQueried OnQueried;
    OnQueried.BindLambda([WeakThis, JobUuid](FAnimGenJobStatus NewStatus)
    {
        if (UAnimGenClient* Self = WeakThis.Get())
        {
            Self->HandleQueried(JobUuid, NewStatus);
        }
    });

    (*Found)->QueryTask(ApiKey, Job->ProviderTaskId, OnQueried);
}

void UAnimGenClient::HandleQueried(FString JobUuid, FAnimGenJobStatus NewStatus)
{
    FAnimGenJob* Job = Jobs.Find(JobUuid);
    if (!Job) return;

    Job->Status = NewStatus;
    OnJobUpdated.Broadcast(JobUuid, Job->Status);

    if (NewStatus.State == EAnimGenState::Succeeded)
    {
        ClearTimer(JobUuid);
        if (!NewStatus.ModelURL.IsEmpty())
        {
            StartDownload(JobUuid, NewStatus.ModelURL);
        }
        else
        {
            Job->Status.State = EAnimGenState::Failed;
            Job->Status.ErrorMessage = TEXT("Provider 标记完成但缺少 ModelURL");
            OnJobFailed.Broadcast(JobUuid, Job->Status.ErrorMessage);
        }
    }
    else if (NewStatus.State == EAnimGenState::Failed || NewStatus.State == EAnimGenState::Cancelled)
    {
        ClearTimer(JobUuid);
        OnJobFailed.Broadcast(JobUuid, NewStatus.ErrorMessage);
    }
}

void UAnimGenClient::StartDownload(FString JobUuid, FString ModelURL)
{
    // 本地缓存路径：Saved/AnimAgent/assets/{uuid}/source.glb
    const FString TargetDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("AnimAgent"), TEXT("assets"), JobUuid);
    const FString TargetPath = FPaths::Combine(TargetDir, TEXT("source.glb"));
    IFileManager::Get().MakeDirectory(*TargetDir, true);

    // file:// 直接拷贝，否则走 HTTP
    if (ModelURL.StartsWith(TEXT("file://")))
    {
        FString LocalSrc = ModelURL.RightChop(7);
        // 容忍 file:///C:/... 三斜杠形式
        if (LocalSrc.StartsWith(TEXT("/")) && LocalSrc.Len() >= 3 && LocalSrc[2] == TEXT(':'))
        {
            LocalSrc = LocalSrc.RightChop(1);
        }
        FPaths::NormalizeFilename(LocalSrc);
        if (IFileManager::Get().Copy(*TargetPath, *LocalSrc) == COPY_OK)
        {
            if (FAnimGenJob* Job = Jobs.Find(JobUuid))
            {
                Job->LocalGLBPath = TargetPath;
                OnJobCompleted.Broadcast(JobUuid, TargetPath);
            }
        }
        else if (FAnimGenJob* Job = Jobs.Find(JobUuid))
        {
            Job->Status.State = EAnimGenState::Failed;
            Job->Status.ErrorMessage = FString::Printf(TEXT("拷贝 mock 文件失败: %s"), *LocalSrc);
            OnJobFailed.Broadcast(JobUuid, Job->Status.ErrorMessage);
        }
        return;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(ModelURL);
    Req->SetVerb(TEXT("GET"));

    TWeakObjectPtr<UAnimGenClient> WeakThis(this);
    Req->OnProcessRequestComplete().BindLambda(
        [WeakThis, JobUuid, TargetPath](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bOk)
        {
            UAnimGenClient* Self = WeakThis.Get();
            if (!Self) return;

            FAnimGenJob* Job = Self->Jobs.Find(JobUuid);
            if (!Job) return;

            if (!bOk || !Response.IsValid())
            {
                Job->Status.State = EAnimGenState::Failed;
                Job->Status.ErrorMessage = TEXT("下载 glb 失败（网络错误）");
                Self->OnJobFailed.Broadcast(JobUuid, Job->Status.ErrorMessage);
                return;
            }

            const TArray<uint8>& Bytes = Response->GetContent();
            if (!FFileHelper::SaveArrayToFile(Bytes, *TargetPath))
            {
                Job->Status.State = EAnimGenState::Failed;
                Job->Status.ErrorMessage = FString::Printf(TEXT("写入失败: %s"), *TargetPath);
                Self->OnJobFailed.Broadcast(JobUuid, Job->Status.ErrorMessage);
                return;
            }

            Job->LocalGLBPath = TargetPath;
            UE_LOG(LogAnimGenClient, Log, TEXT("Job %s 完成，glb 已保存到 %s (%d bytes)"),
                *JobUuid, *TargetPath, Bytes.Num());
            Self->OnJobCompleted.Broadcast(JobUuid, TargetPath);
        });

    Req->ProcessRequest();
}

void UAnimGenClient::ClearTimer(const FString& JobUuid)
{
    if (FTimerHandle* Handle = PollTimers.Find(JobUuid))
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(*Handle);
        }
        PollTimers.Remove(JobUuid);
    }
}
