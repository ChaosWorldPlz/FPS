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
	//  Rule II : 在GridSize 里 找连续的满足 bRotated 状态下的 ItemDef 的 Area
	
	// 拿到 bRotated 状态 Item 的 Size X 和 Size Y
	TOptional<FIntPoint> IsItemValid = GetItemSize(ItemDefID,bRotated);

	// 如果 根据ItemDefID 拿不到ItemSize, 说明有报错
	if (!IsItemValid.IsSet())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[InventoryGridComponent] CanPlaceItem ItemDefID %s 没有有效的 ItemSize"),
			*ItemDefID.ToString());
		return false;
	}
	
	//起始位置+物品尺寸 不能超过GridSize , 
	if (Position.X <0 || Position.Y <0)
	{
		UE_LOG(LogTemp, Error, TEXT("[InventoryGridComponent] CanPlaceItem 起始位置为负数！"));
		return false; //起始位置为负数
	}

	FIntPoint ItemSize = IsItemValid.GetValue();
	// 超出边界
	if (Position.X + ItemSize.X>GridSize.X ||Position.Y + ItemSize.Y>GridSize.Y)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryGridComponent] CanPlaceItem 物品 %s 超出 GridSize (%d, %d)"),
			*ItemDefID.ToString(),
			GridSize.X,
			GridSize.Y);
		return false;
	}

	// 如果返回 { 只要存在哪怕一个【装得下Item的格子】没有被任何物品标记占据 } 是 True
	// 认为装得下 ， 但还没装
	return !IsAreaOccupied(Position,ItemSize,FGuid());
}

bool UInventoryGridComponent::AddItem(const FInventoryItem& Item, FIntPoint Position, bool bRotated)
{
	// 是否装得下
	if (!CanPlaceItem(Item.ItemDefID, Position, bRotated))
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryGridComponent] AddItem Cannot place item %s at (%d, %d)"),   
			 *Item.ItemDefID.ToString(), Position.X, Position.Y);
		return false;
	}
	
	// 可以通过CanPlaceItem 说明这个Item是有效值
	FIntPoint ItemSize = GetItemSize(Item.ItemDefID,bRotated).GetValue();

	// 标记占用 ， 要用实例的Guid
	MarkGridOccupied(Position,ItemSize,Item.InstanceID);

	// 构建物品放置数据
	FInventoryItemPlacement ItemPlacement;
	ItemPlacement.Item = Item;
	ItemPlacement.bIsRotated = bRotated;
	ItemPlacement.GridPosition = Position;

	// 添加到 Items Map（Key: InstanceID, Value: Placement）- O(1)
	Items.Add(Item.InstanceID, ItemPlacement);
	
	UE_LOG(LogTemp, Log,
		TEXT("[InventoryGridComponent] AddItem Successfully added item %s at (%d, %d), InstanceID: %s"),
		 *Item.ItemDefID.ToString(),
		 Position.X, Position.Y,
		 *Item.InstanceID.ToString());
	
	return true;
}

bool UInventoryGridComponent::RemoveItem(FGuid ItemInstanceID)
{
	// 从 Items Map 中移除 - O(1)
	// Remove() 返回删除的元素数量，0 表示没找到
	if (Items.Remove(ItemInstanceID) == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RemoveItem] Item not found: %s"),
			*ItemInstanceID.ToString());
		return false;
	}

	// 清除网格占用
	ClearGridOccupancy(ItemInstanceID);

	UE_LOG(LogTemp, Log,
		TEXT("[RemoveItem] Successfully removed item: %s"),
		*ItemInstanceID.ToString());

	return true;
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

			// 边界检查（防御性编程）
			if (Index < 0 || Index >= OccupancyGrid.Num())
			{
				UE_LOG(LogTemp, Error,
					TEXT("[IsAreaOccupied] Index out of bounds: %d"),
					Index);   
				return true;  // 越界视为被占用
			}

			
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
	// Size = 背包大小 (宽, 高)
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

TOptional<FIntPoint> UInventoryGridComponent::GetItemSize(FName ItemDefID, bool bRotated) const
{
	// GameInstance是经典的单例模式，只有一个，因此要用指针 ^^_
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("[InventoryGridComponent] GetItemSize: World is null!"));
		return TOptional<FIntPoint>();
	}
	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("[InventoryGridComponent] GetItemSize: GameInstance is null!"));
		return TOptional<FIntPoint>();
	}
	
	UItemDataManager* ItemDataMgr = GameInstance->GetSubsystem<UItemDataManager>();
	if (!ItemDataMgr)
	{
		UE_LOG(LogTemp,Warning,TEXT("[InventoryGridComponent] : GetItemSize ,couldn't find ItemDataManager!"));
		return TOptional<FIntPoint>();
	}

	// 这里为什么不统一用ItemDataMgr->GetItemSize(ItemDefID,bRotated); 是为了让日志直观
	// ItemDataMgr->GetItemSize里面还有关于 ItemDefID是否有效的判断贝贝
	return ItemDataMgr->GetItemSize(ItemDefID,bRotated);
}


