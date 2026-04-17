// Copyright Epic Games, Inc. All Rights Reserved.
// MockProvider.h — 不依赖外部 API 的 mock 实现，用于无 Key 联调

#pragma once

#include "CoreMinimal.h"
#include "FPS/AnimAgent/AnimGen/IAnimGenProvider.h"

/**
 * FMockProvider
 *
 * 行为：
 * - CreateTextTo3DTask: 立即返回固定 task_id，记录 prompt
 * - QueryTask: 5 秒内返回 Running，5 秒后返回 Succeeded + 内置 sample.glb 的 file:// URL
 *
 * 用途：
 * - UI / 流程联调
 * - CI / 自动化测试
 * - 演示时无 Key 兜底
 */
class FMockProvider : public IAnimGenProvider
{
public:
    virtual FString GetName() const override { return TEXT("mock"); }

    virtual void CreateTextTo3DTask(
        const FString& ApiKey,
        const FAnimGenRequest& Request,
        FOnAnimGenTaskCreated OnCreated) override;

    virtual void QueryTask(
        const FString& ApiKey,
        const FString& TaskId,
        FOnAnimGenTaskQueried OnQueried) override;

    /** 设置内置 sample 路径（项目启动时由 AnimGenClient 注入） */
    void SetSampleGLBPath(const FString& InPath) { SampleGLBPath = InPath; }

private:
    /** task_id → 创建时间（秒） */
    TMap<FString, double> TaskCreatedAt;

    /** 模拟生成耗时（秒） */
    double SimulatedDurationSeconds = 5.0;

    /** sample.glb 路径（绝对路径） */
    FString SampleGLBPath;
};
