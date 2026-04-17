// Copyright Epic Games, Inc. All Rights Reserved.
// IAnimGenProvider.h — 文生 3D 提供商抽象接口

#pragma once

#include "CoreMinimal.h"
#include "FPS/AnimAgent/AnimAgentTypes.h"

/**
 * 异步任务回调
 * @param bSuccess  接口调用是否成功（不代表生成完成）
 * @param TaskId    厂商分配的任务 id
 * @param Error     失败时的错误信息
 */
DECLARE_DELEGATE_ThreeParams(FOnAnimGenTaskCreated, bool /*bSuccess*/, FString /*TaskId*/, FString /*Error*/);

/** 单次状态查询回调 */
DECLARE_DELEGATE_OneParam(FOnAnimGenTaskQueried, FAnimGenJobStatus /*Status*/);

/**
 * IAnimGenProvider
 *
 * 所有第三方文生 3D 服务的统一接口。
 * 实现类包括 FMeshyProvider / FTripoProvider / FMockProvider。
 *
 * 设计说明：
 * - 接口本身是异步的（CreateTask / QueryTask），轮询节奏由外层 UAnimGenClient 控制
 * - 实现类不持有定时器，只做单次 HTTP 调用
 * - Provider 接受运行时传入的 ApiKey，不持久化
 */
class IAnimGenProvider
{
public:
    virtual ~IAnimGenProvider() = default;

    /** 提供商名称（"meshy" / "tripo" / "mock"） */
    virtual FString GetName() const = 0;

    /** 创建文生 3D 任务 */
    virtual void CreateTextTo3DTask(
        const FString& ApiKey,
        const FAnimGenRequest& Request,
        FOnAnimGenTaskCreated OnCreated) = 0;

    /** 单次查询任务状态（由外层定时器周期调用） */
    virtual void QueryTask(
        const FString& ApiKey,
        const FString& TaskId,
        FOnAnimGenTaskQueried OnQueried) = 0;

    /** 取消任务（厂商不支持时返回 false，外层应停止轮询） */
    virtual bool CancelTask(
        const FString& ApiKey,
        const FString& TaskId) { return false; }

    /** 是否支持图生 3D */
    virtual bool SupportsImageInput() const { return false; }

    /** 是否支持自动绑骨 */
    virtual bool SupportsAutoRig() const { return false; }
};
