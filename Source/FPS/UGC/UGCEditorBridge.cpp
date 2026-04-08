// Copyright Epic Games, Inc. All Rights Reserved.

#include "UGCEditorBridge.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Components/PrimitiveComponent.h"
#include "CollisionQueryParams.h"

UUGCEditorBridge::UUGCEditorBridge()
{
    PrimaryComponentTick.bCanEverTick = false;
}

AActor* UUGCEditorBridge::SpawnPlaceable(const FString& BlueprintPath, FVector Location, FRotator Rotation)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    UClass* Class = LoadClass<AActor>(nullptr, *BlueprintPath);
    if (!Class)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UGCEditorBridge] SpawnPlaceable: 找不到类 '%s'"), *BlueprintPath);
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AActor* Actor = World->SpawnActor<AActor>(Class, Location, Rotation, Params);
    if (!Actor)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UGCEditorBridge] SpawnPlaceable: Spawn 失败 '%s'"), *BlueprintPath);
    }
    return Actor;
}

void UUGCEditorBridge::DestroyActor(AActor* Actor)
{
    if (Actor && IsValid(Actor))
    {
        Actor->Destroy();
    }
}

AActor* UUGCEditorBridge::LineTraceScreen(float ScreenX, float ScreenY)
{
    APlayerController* PC = GetPC();
    if (!PC) return nullptr;

    FVector WorldPos, WorldDir;
    if (!PC->DeprojectScreenPositionToWorld(ScreenX, ScreenY, WorldPos, WorldDir))
        return nullptr;

    FVector Start = WorldPos;
    FVector End   = WorldPos + WorldDir * 50000.f;

    FHitResult Hit;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(PC->GetPawn());

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit, Start, End,
        ECollisionChannel::ECC_Visibility,
        QueryParams
    );

    return bHit ? Hit.GetActor() : nullptr;
}

void UUGCEditorBridge::SetActorHighlight(AActor* Actor, bool bEnable)
{
    if (!Actor || !IsValid(Actor)) return;

    // 遍历所有 PrimitiveComponent 开关描边
    TArray<UPrimitiveComponent*> Prims;
    Actor->GetComponents<UPrimitiveComponent>(Prims);
    for (UPrimitiveComponent* Prim : Prims)
    {
        if (Prim)
        {
            Prim->SetRenderCustomDepth(bEnable);
            Prim->SetCustomDepthStencilValue(bEnable ? 1 : 0);
        }
    }
}

FTransform UUGCEditorBridge::GetActorTransform(AActor* Actor) const
{
    if (!Actor || !IsValid(Actor)) return FTransform::Identity;
    return Actor->GetActorTransform();
}

void UUGCEditorBridge::SetActorTransform(AActor* Actor, const FTransform& NewTransform)
{
    if (!Actor || !IsValid(Actor)) return;
    Actor->SetActorTransform(NewTransform);
}

APlayerController* UUGCEditorBridge::GetPC() const
{
    return Cast<APlayerController>(GetOwner());
}
