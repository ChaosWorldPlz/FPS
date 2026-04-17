// Copyright Epic Games, Inc. All Rights Reserved.
// TripoProvider.h — Tripo3D text-to-model 实现

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "FPS/AnimAgent/AnimGen/IAnimGenProvider.h"

/**
 * FTripoProvider
 *
 * API 文档参考: https://platform.tripo3d.ai/docs
 */
class FTripoProvider : public IAnimGenProvider
{
public:
    virtual FString GetName() const override { return TEXT("tripo"); }

    virtual void CreateTextTo3DTask(
        const FString& ApiKey,
        const FAnimGenRequest& Request,
        FOnAnimGenTaskCreated OnCreated) override;

    virtual void QueryTask(
        const FString& ApiKey,
        const FString& TaskId,
        FOnAnimGenTaskQueried OnQueried) override;

    virtual bool SupportsImageInput() const override { return true; }

private:
    static const TCHAR* TaskEndpoint;
};
