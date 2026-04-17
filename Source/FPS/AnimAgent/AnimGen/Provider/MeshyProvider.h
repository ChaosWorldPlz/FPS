// Copyright Epic Games, Inc. All Rights Reserved.
// MeshyProvider.h — Meshy text-to-3D 实现

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "FPS/AnimAgent/AnimGen/IAnimGenProvider.h"

/**
 * FMeshyProvider
 *
 * API 文档参考: https://docs.meshy.ai/
 * 当前实现仅 preview 模式，refine 留作 P2。
 */
class FMeshyProvider : public IAnimGenProvider
{
public:
    virtual FString GetName() const override { return TEXT("meshy"); }

    virtual void CreateTextTo3DTask(
        const FString& ApiKey,
        const FAnimGenRequest& Request,
        FOnAnimGenTaskCreated OnCreated) override;

    virtual void QueryTask(
        const FString& ApiKey,
        const FString& TaskId,
        FOnAnimGenTaskQueried OnQueried) override;

private:
    /** 把通用 EAnimGenStyle 映射到 Meshy art_style 字符串 */
    static FString StyleToString(EAnimGenStyle Style);

    static const TCHAR* EndpointBase;
};
