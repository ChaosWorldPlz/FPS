// Copyright Epic Games, Inc. All Rights Reserved.

#include "UGCGameMode.h"
#include "GameFramework/SpectatorPawn.h"

AUGCGameMode::AUGCGameMode()
{
    // 幽灵视角：自由飞行，无碰撞，无重力
    DefaultPawnClass = ASpectatorPawn::StaticClass();

    bDelayedStart = false;
}

void AUGCGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    // 直接调用基类逻辑（出生点选择 + Pawn 生成），跳过队伍分配等 PVP 逻辑
    Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}
