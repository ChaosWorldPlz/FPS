// Fill out your copyright notice in the Description page of Project Settings.
#include "FPS/Inventory/Public/InventoryGridComponent.h"

#include "IDetailTreeNode.h"
#include "FPS/Inventory/Public/ItemDataManager.h"


// Sets default values for this component's properties
UInventoryGridComponent::UInventoryGridComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// 设置默认网格大小，这里初始定义为3*3，作为安全箱
	// 背包引用时要手动设置更大
	GridSize = FIntPoint(3,3);
	
}
void UInventoryGridComponent::BeginPlay()
{
	Super::BeginPlay();

	// 从编辑器设置的 最新的 GridSize 初始化数组里拿到真正的背包大小
	int32 TotalSize = GridSize.X * GridSize.Y;
	OccupancyGrid.SetNum(TotalSize);
	for (auto& Grid : OccupancyGrid)
	{
		Grid = FGuid();
	}
	
	UE_LOG(LogTemp, Log, TEXT("[InventoryGridComponent] Grid initialized: %dx%d (%d cells)"),
		  GridSize.X, GridSize.Y, TotalSize);

}

bool UInventoryGridComponent::CanPlaceItem(FName ItemDefID, FIntPoint Position, bool bRotated)
{
	// 需要找到连续的，形状匹配的Area
	// 剪枝思想： 需要满足条件的区域里面找
	//  Rule I : Item Size X 、Size Y、 Size X * Size Y 均小于 GridSize
	//  Rule II : 在GridSize 里 找连续的、 
}

bool UInventoryGridComponent::AddItem(const FInventoryItem& Item, FIntPoint Position, bool bRotated)
{
	
}

bool UInventoryGridComponent::RemoveItem(FGuid ItemInstanceID)
{
	
}

bool UInventoryGridComponent::IsAreaOccupied(FIntPoint Position, FIntPoint Size, FGuid IgnoreItemID)
{
	// Position = 起始坐标 (X, Y)
	// Size = 矩形大小 (宽, 高)
	// IgnoreItemID = 要忽略的物品 ID（用于移动物品时）

	// 遍历矩形区域
	for (int32 Y = 0; Y < Size.Y; Y++)
	{
		for (int32 X = 0; X < Size.X; X++)
		{
			int32 GridX = Position.X + X;
			int32 GridY = Position.Y + Y;
			int32 Index = GridY * GridSize.X + GridX;

			FGuid OccupyingID = OccupancyGrid[Index];  // 获取占用者的 GUID    

			// 如果格子被占用 && 不是我们要忽略的物品
			if (OccupyingID.IsValid() && OccupyingID != IgnoreItemID)
			{
				return true;  // 被占用！
			}
		}
	}

	return false;  // 空闲

}

void UInventoryGridComponent::MarkGridOccupied(FIntPoint Position, FIntPoint Size, FGuid ItemID)
{
	// Position = 起始坐标 (X, Y)
	// Size = 矩形大小 (宽, 高)
	// IgnoreItemID = 要忽略的物品 ID（用于移动物品时）

	// 遍历矩形区域
	for (int32 Y = 0; Y < Size.Y; Y++)
	{
		for (int32 X = 0; X < Size.X; X++)
		{
			int32 GridX = Position.X + X;
			int32 GridY = Position.Y + Y;
			int32 Index = GridY * GridSize.X + GridX;
			//为该格子的Guid赋值为ItemID
			OccupancyGrid[Index] = ItemID;
		}
	}
}

void UInventoryGridComponent::ClearGridOccupancy(FGuid ItemID)
{
	for (auto& Grid : OccupancyGrid)
	{
		if (Grid == ItemID)
		{
			//初始化Guid
			Grid = FGuid();
		}
	}
}

FIntPoint UInventoryGridComponent::GetItemSize(FName ItemDefID, bool bRotated) const
{
	// GameInstance是经典的单例模式，只有一个，因此要用指针 ^^_
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("[InventoryGridComponent] GetItemSize: World is null!"));
		return FIntPoint(1, 1);
	}
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("[InventoryGridComponent] GetItemSize: GameInstance is null!"));
		return FIntPoint(1, 1);
	}
	
	UItemDataManager* ItemDataMgr = GameInstance->GetSubsystem<UItemDataManager>();
	if (!ItemDataMgr)
	{
		UE_LOG(LogTemp,Warning,TEXT("[InventoryGridComponent] : GetItemSize ,couldn't find ItemDataManager!"));
		return FIntPoint(1,1);
	}
	
	return ItemDataMgr->GetItemSize(ItemDefID,bRotated);
}


