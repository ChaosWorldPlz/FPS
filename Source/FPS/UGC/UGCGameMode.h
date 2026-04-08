// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FPS/FPSGameMode.h"
#include "UGCGameMode.generated.h"

/**
 * AUGCGameMode
 *
 * 继承自 AFPSGameMode，用于 UGC 编辑器关卡。
 * 屏蔽 PVP 逻辑（计时、胜负、复活），让玩家专注于场景编辑。
 */
UCLASS()
class FPS_API AUGCGameMode : public AFPSGameMode
{
    GENERATED_BODY()

public:
    AUGCGameMode();

protected:
    // 编辑器关卡不需要等待玩家数量，直接开始
    virtual bool ReadyToStartMatch_Implementation() override { return true; }

    // 屏蔽胜负判定
    virtual void HandleMatchHasEnded() override {}

    // 玩家死亡后立即原地复活，不影响编辑流程
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
};
