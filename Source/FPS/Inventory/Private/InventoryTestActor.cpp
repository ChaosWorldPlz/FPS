// Fill out your copyright notice in the Description page of Project Settings.


#include "FPS/Inventory/Public/InventoryTestActor.h"


// Sets default values
AInventoryTestActor::AInventoryTestActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	GridComponent = CreateDefaultSubobject<UInventoryGridComponent>(TEXT("GridInventory"));
}

// Called when the game starts or when spawned
void AInventoryTestActor::BeginPlay()
{
	Super::BeginPlay();
	RunTests();
}

void AInventoryTestActor::RunTests()
{
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("=== 背包网格测试开始 ==="));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));

    // 测试 1: 空网格可以放置物品
    bool bCanPlace = GridComponent->CanPlaceItem(TEXT("WPN_Rifle_AK47"), FIntPoint(0, 0), false);
    UE_LOG(LogTemp, Log, TEXT("测试 1 - 空网格放置 AK47 (4x2): %s"),
        bCanPlace ? TEXT("✅ 通过") : TEXT("❌ 失败"));

    // 测试 2: 实际添加物品
    FInventoryItem AK47;
    AK47.ItemDefID = TEXT("WPN_Rifle_AK47");
    AK47.InstanceID = FGuid::NewGuid();
    AK47.StackCount = 1;

    bool bAdded = GridComponent->AddItem(AK47, FIntPoint(0, 0), false);
    UE_LOG(LogTemp, Log, TEXT("测试 2 - 添加 AK47: %s"),
        bAdded ? TEXT("✅ 成功") : TEXT("❌ 失败"));

    // 测试 3: 重叠检测
    bCanPlace = GridComponent->CanPlaceItem(TEXT("WPN_Pistol_Glock"), FIntPoint(1, 0), false);
    UE_LOG(LogTemp, Log, TEXT("测试 3 - 重叠位置放置手枪: %s"),
        !bCanPlace ? TEXT("✅ 正确拒绝") : TEXT("❌ 应该拒绝"));

    // 测试 4: 空闲位置放置
    FInventoryItem Ammo;
    Ammo.ItemDefID = TEXT("AMO_762_Standard");
    Ammo.InstanceID = FGuid::NewGuid();
    Ammo.StackCount = 30;

    bAdded = GridComponent->AddItem(Ammo, FIntPoint(5, 0), false);
    UE_LOG(LogTemp, Log, TEXT("测试 4 - 空闲位置添加弹药: %s"),
        bAdded ? TEXT("✅ 成功") : TEXT("❌ 失败"));

    // 测试 5: 边界检查
    bCanPlace = GridComponent->CanPlaceItem(TEXT("WPN_Rifle_AK47"), FIntPoint(27, 29), false);
    UE_LOG(LogTemp, Log, TEXT("测试 5 - 边界外放置 AK47: %s"),
        !bCanPlace ? TEXT("✅ 正确拒绝") : TEXT("❌ 应该拒绝"));

    // 测试 6: 删除物品
    bool bRemoved = GridComponent->RemoveItem(AK47.InstanceID);
    UE_LOG(LogTemp, Log, TEXT("测试 6 - 删除 AK47: %s"),
        bRemoved ? TEXT("✅ 成功") : TEXT("❌ 失败"));

    // 验证删除后可以重新放置
    bCanPlace = GridComponent->CanPlaceItem(TEXT("WPN_Rifle_AK47"), FIntPoint(0, 0), false);
    UE_LOG(LogTemp, Log, TEXT("测试 6.1 - 删除后可重新放置: %s"),
        bCanPlace ? TEXT("✅ 通过") : TEXT("❌ 失败"));

    // 测试 7: 旋转功能
    FInventoryItem AK47_Rotated;
    AK47_Rotated.ItemDefID = TEXT("WPN_Rifle_AK47");
    AK47_Rotated.InstanceID = FGuid::NewGuid();
    AK47_Rotated.StackCount = 1;

    bAdded = GridComponent->AddItem(AK47_Rotated, FIntPoint(0, 0), true);
    UE_LOG(LogTemp, Log, TEXT("测试 7 - 旋转放置 AK47 (2x4): %s"),
        bAdded ? TEXT("✅ 成功") : TEXT("❌ 失败"));

    UE_LOG(LogTemp, Warning, TEXT("========================================"));
    UE_LOG(LogTemp, Warning, TEXT("=== 背包网格测试结束 ==="));
    UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

