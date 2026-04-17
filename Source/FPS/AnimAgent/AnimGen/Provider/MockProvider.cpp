// Copyright Epic Games, Inc. All Rights Reserved.

#include "MockProvider.h"
#include "Misc/Guid.h"

void FMockProvider::CreateTextTo3DTask(
    const FString& /*ApiKey*/,
    const FAnimGenRequest& Request,
    FOnAnimGenTaskCreated OnCreated)
{
    const FString TaskId = FString::Printf(TEXT("mock-%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::DigitsLower).Left(8));

    TaskCreatedAt.Add(TaskId, FPlatformTime::Seconds());

    UE_LOG(LogTemp, Log, TEXT("[MockProvider] 创建任务 TaskId=%s Prompt=%s"),
        *TaskId, *Request.Prompt);

    OnCreated.ExecuteIfBound(true, TaskId, FString());
}

void FMockProvider::QueryTask(
    const FString& /*ApiKey*/,
    const FString& TaskId,
    FOnAnimGenTaskQueried OnQueried)
{
    FAnimGenJobStatus Status;

    const double* CreatedPtr = TaskCreatedAt.Find(TaskId);
    if (!CreatedPtr)
    {
        Status.State = EAnimGenState::Failed;
        Status.ErrorMessage = TEXT("Mock 任务不存在");
        OnQueried.ExecuteIfBound(Status);
        return;
    }

    const double Elapsed = FPlatformTime::Seconds() - (*CreatedPtr);
    const double Ratio = FMath::Clamp(Elapsed / SimulatedDurationSeconds, 0.0, 1.0);

    if (Ratio < 1.0)
    {
        Status.State = EAnimGenState::Running;
        Status.Progress = static_cast<int32>(Ratio * 100.0);
    }
    else
    {
        Status.State = EAnimGenState::Succeeded;
        Status.Progress = 100;
        Status.ModelURL = SampleGLBPath.IsEmpty()
            ? FString(TEXT("file:///__missing_sample__.glb"))
            : (TEXT("file:///") + SampleGLBPath);
    }

    OnQueried.ExecuteIfBound(Status);
}
