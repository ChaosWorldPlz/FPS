// Copyright Epic Games, Inc. All Rights Reserved.
// AnimAgentTypes.h — 共享枚举/结构

#pragma once

#include "CoreMinimal.h"
#include "AnimAgentTypes.generated.h"

UENUM(BlueprintType)
enum class EAnimGenProvider : uint8
{
    Mock    UMETA(DisplayName = "Mock"),
    Meshy   UMETA(DisplayName = "Meshy"),
    Tripo   UMETA(DisplayName = "Tripo"),
};

UENUM(BlueprintType)
enum class EAnimGenState : uint8
{
    Pending     UMETA(DisplayName = "Pending"),
    Running     UMETA(DisplayName = "Running"),
    Succeeded   UMETA(DisplayName = "Succeeded"),
    Failed      UMETA(DisplayName = "Failed"),
    Cancelled   UMETA(DisplayName = "Cancelled"),
};

UENUM(BlueprintType)
enum class EAnimGenStyle : uint8
{
    Realistic   UMETA(DisplayName = "Realistic"),
    Cartoon     UMETA(DisplayName = "Cartoon"),
    Sculpture   UMETA(DisplayName = "Sculpture"),
};

USTRUCT(BlueprintType)
struct FAnimGenRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Prompt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString NegativePrompt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EAnimGenStyle Style = EAnimGenStyle::Realistic;

    /** 目标三角面数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TargetPolycount = 30000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bWithTexture = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bPBR = true;

    /** 输出格式：glb / fbx，本期固定 glb */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Format = TEXT("glb");
};

USTRUCT(BlueprintType)
struct FAnimGenJobStatus
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EAnimGenState State = EAnimGenState::Pending;

    /** 0~100 */
    UPROPERTY(BlueprintReadOnly)
    int32 Progress = 0;

    /** 中间预览图 URL（可选） */
    UPROPERTY(BlueprintReadOnly)
    FString PreviewURL;

    /** 完成后产物模型 URL */
    UPROPERTY(BlueprintReadOnly)
    FString ModelURL;

    UPROPERTY(BlueprintReadOnly)
    FString ErrorMessage;
};

USTRUCT(BlueprintType)
struct FAnimGenJob
{
    GENERATED_BODY()

    /** 本地唯一 ID（uuid） */
    UPROPERTY(BlueprintReadOnly)
    FString JobUuid;

    /** 提供商分配的 task id */
    UPROPERTY(BlueprintReadOnly)
    FString ProviderTaskId;

    UPROPERTY(BlueprintReadOnly)
    EAnimGenProvider Provider = EAnimGenProvider::Mock;

    UPROPERTY(BlueprintReadOnly)
    FAnimGenRequest Request;

    UPROPERTY(BlueprintReadOnly)
    FAnimGenJobStatus Status;

    /** 本地 glb 缓存路径，下载完成后填充 */
    UPROPERTY(BlueprintReadOnly)
    FString LocalGLBPath;

    /** 任务创建时间（秒） */
    UPROPERTY(BlueprintReadOnly)
    double CreatedAtSeconds = 0.0;
};
