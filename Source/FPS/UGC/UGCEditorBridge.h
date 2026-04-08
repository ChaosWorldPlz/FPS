// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UGCEditorBridge.generated.h"

/**
 * UUGCEditorBridge
 *
 * UGC 编辑器的 C++ 原子操作层，挂载在 UGCPlayerController 上。
 * 职责极简：只提供引擎层无法在 Lua 直接完成的操作：
 *   1. 按 Blueprint 类路径动态 Spawn Actor
 *   2. Destroy Actor
 *   3. 屏幕坐标射线检测（点击选中）
 *   4. 设置 Actor 选中高亮
 *
 * 所有业务逻辑（选中状态、撤销栈、序列化）均在 Lua 层完成。
 */
UCLASS(ClassGroup = "UGC", meta = (BlueprintSpawnableComponent))
class FPS_API UUGCEditorBridge : public UActorComponent
{
    GENERATED_BODY()

public:
    UUGCEditorBridge();

    /**
     * 在世界中生成一个可放置 Actor
     * @param BlueprintPath  蓝图类资产路径，如 "/Game/_UGC/Placeables/BP_Placeable_Box"
     * @param Location       世界坐标
     * @param Rotation       旋转
     * @return 生成的 Actor，失败返回 nullptr
     */
    UFUNCTION(BlueprintCallable, Category = "UGC|Editor")
    AActor* SpawnPlaceable(const FString& BlueprintPath, FVector Location, FRotator Rotation);

    /**
     * 销毁一个 Actor
     */
    UFUNCTION(BlueprintCallable, Category = "UGC|Editor")
    void DestroyActor(AActor* Actor);

    /**
     * 从屏幕坐标发射射线，返回命中的第一个 Actor
     * 用于鼠标点击选中
     * @param ScreenX / ScreenY  屏幕坐标（像素）
     * @return 命中的 Actor，未命中返回 nullptr
     */
    UFUNCTION(BlueprintCallable, Category = "UGC|Editor")
    AActor* LineTraceScreen(float ScreenX, float ScreenY);

    /**
     * 设置 Actor 的选中高亮（描边）
     * @param Actor   目标 Actor
     * @param bEnable true=高亮，false=取消
     */
    UFUNCTION(BlueprintCallable, Category = "UGC|Editor")
    void SetActorHighlight(AActor* Actor, bool bEnable);

    /**
     * 获取 Actor 的世界 Transform（供 Lua 读取序列化）
     */
    UFUNCTION(BlueprintCallable, Category = "UGC|Editor")
    FTransform GetActorTransform(AActor* Actor) const;

    /**
     * 设置 Actor 的世界 Transform
     */
    UFUNCTION(BlueprintCallable, Category = "UGC|Editor")
    void SetActorTransform(AActor* Actor, const FTransform& NewTransform);

private:
    APlayerController* GetPC() const;
};
