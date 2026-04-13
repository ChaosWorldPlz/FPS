// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UGCTriggerZone.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * AUGCTriggerZone
 *
 * UGC 可放置触发区域。玩家（Pawn）进入/离开时通过其 PlayerController
 * 向 Lua UGCProgramRunner 发送 Event_OnEnter / Event_OnExit 事件。
 *
 * 编辑模式下显示半透明蓝色方块（DebugMesh）作为可视化标志；
 * 游玩模式下自动隐藏，仅保留碰撞体正常工作。
 */
UCLASS(BlueprintType, Blueprintable,
    meta = (DisplayName = "UGC Trigger Zone"))
class FPS_API AUGCTriggerZone : public AActor
{
    GENERATED_BODY()

public:
    AUGCTriggerZone();

    //-------------------------------------------------------------------
    // 属性
    //-------------------------------------------------------------------

    /** 关联的程序 ID，格式 "actor_prog_N"，SceneData Spawn 后自动赋值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC|TriggerZone")
    FString ProgramID;

    /** 触发区域盒体半尺寸（单位 cm），默认 100×100×100 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UGC|TriggerZone")
    FVector BoxExtent = FVector(100.f, 100.f, 100.f);

    //-------------------------------------------------------------------
    // 接口（供 Lua 调用）
    //-------------------------------------------------------------------

    /** 设置关联程序 ID（SceneData:CreateActor 后由 Lua 调用） */
    UFUNCTION(BlueprintCallable, Category = "UGC|TriggerZone")
    void SetProgramID(const FString& InProgramID) { ProgramID = InProgramID; }

    /**
     * 控制编辑模式可视化方块的显示/隐藏
     * EditorCore:EnterEditMode()  → SetDebugVisible(true)
     * EditorCore:EnterPlayMode()  → SetDebugVisible(false)
     */
    UFUNCTION(BlueprintCallable, Category = "UGC|TriggerZone")
    void SetDebugVisible(bool bVisible);

protected:
    /** 碰撞触发体 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* Box;

    /**
     * 编辑模式可视化方块（大小与 Box 一致）
     * 在 Blueprint 里为其指定半透明蓝色材质即可。
     * 默认无材质，由 BP_Placeable_TriggerZone 覆盖。
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* DebugMesh;

    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnBoxEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
