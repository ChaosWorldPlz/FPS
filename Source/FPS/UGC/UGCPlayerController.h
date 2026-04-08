// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPS/FPSPlayerController.h"
#include "UGCPlayerController.generated.h"

class UUGCFunctionBridge;
class UUGCHttpClient;

/**
 * AUGCPlayerController
 *
 * 继承自 AFPSPlayerController，附加 UGC 相关组件：
 *   - UUGCFunctionBridge : 原子操作白名单（GAS / 武器 / 规则）
 *   - UUGCHttpClient     : Claude API HTTP 客户端
 *
 * 设计意图：
 *   保持基类 AFPSPlayerController 纯净（仅输入/菜单/会话）。
 *   在需要 UGC / LLM 能力的游戏模式里，将 GameMode 的 PlayerControllerClass
 *   设置为 BP_UGCPlayerController（继承本类的蓝图）即可。
 *
 * Lua 绑定：
 *   BP_UGCPlayerController → GetModuleName = "Gameplay.UGC.UGCPlayerController"
 */
UCLASS()
class FPS_API AUGCPlayerController : public AFPSPlayerController
{
    GENERATED_BODY()

public:
    AUGCPlayerController();

    //-------------------------------------------------------------------
    // UGC 组件访问（供 Lua 调用）
    //-------------------------------------------------------------------

    /** 获取 UGC 原子操作组件 */
    UFUNCTION(BlueprintCallable, Category = "UGC")
    UUGCFunctionBridge* GetUGCBridge() const { return UGCBridge; }

    /** 获取 LLM HTTP 客户端组件 */
    UFUNCTION(BlueprintCallable, Category = "UGC")
    UUGCHttpClient* GetUGCHttpClient() const { return UGCHttpClient; }

protected:
    /** UGC 原子操作组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UGC")
    UUGCFunctionBridge* UGCBridge;

    /** Claude API HTTP 客户端组件 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UGC")
    UUGCHttpClient* UGCHttpClient;
};
