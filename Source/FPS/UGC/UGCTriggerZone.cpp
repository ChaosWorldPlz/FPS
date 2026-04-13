// Copyright Epic Games, Inc. All Rights Reserved.

#include "UGCTriggerZone.h"
#include "UGCPlayerController.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Pawn.h"

AUGCTriggerZone::AUGCTriggerZone()
{
    PrimaryActorTick.bCanEverTick = false;

    // ── 碰撞触发体 ──────────────────────────────────────────
    Box = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
    Box->SetBoxExtent(BoxExtent);
    Box->SetCollisionProfileName(TEXT("Trigger"));
    Box->SetGenerateOverlapEvents(true);
    RootComponent = Box;

    // ── 编辑模式可视化方块 ───────────────────────────────────
    // 使用引擎内置 Cube 网格（100×100×100 cm），缩放到 BoxExtent×2
    DebugMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugMesh"));
    DebugMesh->SetupAttachment(RootComponent);
    // 只响应 Visibility 通道（编辑器 LineTrace 选取用），其余全忽略
    // 默认 NoCollision，SetDebugVisible(true) 时才开启，避免游玩模式误判
    DebugMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DebugMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
    DebugMesh->SetCollisionResponseToChannel(ECC_Visibility, ECollisionResponse::ECR_Block);
    DebugMesh->SetCastShadow(false);
    DebugMesh->SetVisibility(false);    // 默认隐藏，EnterEditMode 时再显示

    // 引擎内置 1m 立方体；BoxExtent 是半尺寸 → scale = BoxExtent * 2 / 100
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        DebugMesh->SetStaticMesh(CubeFinder.Object);
        const FVector Scale = BoxExtent * 2.f / 100.f;
        DebugMesh->SetRelativeScale3D(Scale);
    }
}

void AUGCTriggerZone::SetDebugVisible(bool bVisible)
{
    if (DebugMesh)
    {
        DebugMesh->SetVisibility(bVisible);
        // 同步开关碰撞：编辑模式可点选，游玩模式完全无碰撞
        DebugMesh->SetCollisionEnabled(
            bVisible ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    }
}

void AUGCTriggerZone::BeginPlay()
{
    Super::BeginPlay();

    // 运行时用 BoxExtent 属性同步碰撞体和可视化大小（允许编辑器调整）
    Box->SetBoxExtent(BoxExtent);
    if (DebugMesh)
    {
        DebugMesh->SetRelativeScale3D(BoxExtent * 2.f / 100.f);
    }

    Box->OnComponentBeginOverlap.AddDynamic(
        this, &AUGCTriggerZone::OnBoxBeginOverlap);
    Box->OnComponentEndOverlap.AddDynamic(
        this, &AUGCTriggerZone::OnBoxEndOverlap);
}

void AUGCTriggerZone::OnBoxBeginOverlap(
    UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
    bool bFromSweep, const FHitResult& SweepResult)
{
    if (ProgramID.IsEmpty()) return;
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (!Pawn) return;
    if (AUGCPlayerController* PC =
            Cast<AUGCPlayerController>(Pawn->GetController()))
    {
        PC->OnTriggerZoneEnter(ProgramID);
    }
}

void AUGCTriggerZone::OnBoxEndOverlap(
    UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (ProgramID.IsEmpty()) return;
    APawn* Pawn = Cast<APawn>(OtherActor);
    if (!Pawn) return;
    if (AUGCPlayerController* PC =
            Cast<AUGCPlayerController>(Pawn->GetController()))
    {
        PC->OnTriggerZoneExit(ProgramID);
    }
}
