// UGCPCGBridge.cpp — PCG 过程化生成桥接组件（2026-04-16）

#include "UGCPCGBridge.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

UUGCPCGBridge::UUGCPCGBridge()
{
    PrimaryComponentTick.bCanEverTick = false;
}

// -----------------------------------------------------------------------
// Generate
// -----------------------------------------------------------------------

AActor* UUGCPCGBridge::Generate(FVector Location, float Radius, int32 Seed, const FString& GraphPath)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[UGCPCGBridge] Generate: World 为空"));
        return nullptr;
    }

    // 解析 PCG Graph
    UPCGGraphInterface* Graph = nullptr;
    if (!GraphPath.IsEmpty())
    {
        Graph = Cast<UPCGGraphInterface>(
            StaticLoadObject(UPCGGraphInterface::StaticClass(), nullptr, *GraphPath));
    }
    if (!Graph && DefaultPCGGraph.IsValid())
    {
        Graph = DefaultPCGGraph.LoadSynchronous();
    }
    if (!Graph)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UGCPCGBridge] Generate: 无可用 PCG Graph"));
        return nullptr;
    }

    // 参数默认值
    float UseRadius = (Radius > 0.f) ? Radius : DefaultRadius;
    int32 UseSeed   = (Seed != 0) ? Seed : DefaultSeed;
    if (UseSeed == 0)
    {
        UseSeed = FMath::RandRange(1, 999999);
    }

    // Spawn 一个空 Actor 作为 PCG 宿主
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AActor* PCGActor = World->SpawnActor<AActor>(AActor::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
    if (!PCGActor)
    {
        UE_LOG(LogTemp, Error, TEXT("[UGCPCGBridge] Generate: Spawn PCG Actor 失败"));
        return nullptr;
    }

    // 添加 PCG Component
    UPCGComponent* PCGComp = NewObject<UPCGComponent>(PCGActor, TEXT("PCGComponent"));
    if (!PCGComp)
    {
        PCGActor->Destroy();
        return nullptr;
    }

    PCGComp->RegisterComponent();
    PCGComp->SetGraphInterface(Graph);
    PCGComp->Seed = UseSeed;

    // 设置生成范围（通过 Actor Scale 间接控制 PCG 的 Volume）
    PCGActor->SetActorScale3D(FVector(UseRadius / 100.f));

    // 触发生成
    PCGComp->Generate();

    ActivePCGActors.Add(PCGActor);

    UE_LOG(LogTemp, Log, TEXT("[UGCPCGBridge] Generate: 位置=(%.0f,%.0f,%.0f) 半径=%.0f 种子=%d"),
        Location.X, Location.Y, Location.Z, UseRadius, UseSeed);

    return PCGActor;
}

// -----------------------------------------------------------------------
// Cleanup
// -----------------------------------------------------------------------

bool UUGCPCGBridge::Cleanup(AActor* PCGActor)
{
    if (!PCGActor) return false;

    // 找到 PCG Component 并清理
    UPCGComponent* PCGComp = PCGActor->FindComponentByClass<UPCGComponent>();
    if (PCGComp)
    {
        PCGComp->Cleanup();
    }

    PCGActor->Destroy();
    ActivePCGActors.Remove(PCGActor);

    UE_LOG(LogTemp, Log, TEXT("[UGCPCGBridge] Cleanup: Actor 已清理"));
    return true;
}

void UUGCPCGBridge::CleanupAll()
{
    for (AActor* Actor : ActivePCGActors)
    {
        if (Actor && IsValid(Actor))
        {
            UPCGComponent* PCGComp = Actor->FindComponentByClass<UPCGComponent>();
            if (PCGComp)
            {
                PCGComp->Cleanup();
            }
            Actor->Destroy();
        }
    }
    int32 Count = ActivePCGActors.Num();
    ActivePCGActors.Empty();
    UE_LOG(LogTemp, Log, TEXT("[UGCPCGBridge] CleanupAll: 已清理 %d 个 PCG Actor"), Count);
}
