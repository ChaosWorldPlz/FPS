// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimImportBridge.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnimImport, Log, All);

UAnimImportBridge::UAnimImportBridge()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAnimImportBridge::ImportGLBAsync(const FString& JobUuid, const FString& GLBFilePath)
{
    // TODO: 接入 glTFRuntime
    //   1. UglTFRuntimeAsset* Asset = UglTFRuntimeFunctionLibrary::glTFLoadAssetFromFilename(GLBFilePath, false, Cfg);
    //   2. UStaticMesh* Mesh = Asset->LoadStaticMeshRecursive(...);
    //   3. MeshCache.Add(JobUuid, Mesh);
    //   4. OnMeshImported.Broadcast(JobUuid, Mesh);
    //
    // 当前 stub：直接广播失败，提示插件未安装。
    UE_LOG(LogAnimImport, Warning,
        TEXT("ImportGLBAsync stub: glTFRuntime 未安装，无法导入 %s (job=%s)"),
        *GLBFilePath, *JobUuid);

    OnMeshImportFailed.Broadcast(JobUuid, TEXT("glTFRuntime 插件未安装"));
}

AStaticMeshActor* UAnimImportBridge::SpawnMeshActor(UStaticMesh* Mesh, const FTransform& Transform)
{
    if (!Mesh) return nullptr;
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), Transform, Params);
    if (!Actor) return nullptr;

    if (UStaticMeshComponent* SMC = Actor->GetStaticMeshComponent())
    {
        SMC->SetMobility(EComponentMobility::Movable);
        SMC->SetStaticMesh(Mesh);
    }
    return Actor;
}

UStaticMesh* UAnimImportBridge::FindCachedMesh(const FString& JobUuid) const
{
    if (const TWeakObjectPtr<UStaticMesh>* Found = MeshCache.Find(JobUuid))
    {
        return Found->Get();
    }
    return nullptr;
}
