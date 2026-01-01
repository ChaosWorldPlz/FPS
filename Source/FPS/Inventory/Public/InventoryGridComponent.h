// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPS/Inventory/InventoryTypes.h"
#include "InventoryGridComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FPS_API UInventoryGridComponent : public UActorComponent
{
	GENERATED_BODY()
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Sets default values for this component's properties
	UInventoryGridComponent();

	// 检查是否可以防止物品（不实际放置）
	bool CanPlaceItem(FName ItemDefID, FIntPoint Position,	bool bRotated);

	// 添加物品到网格
	bool AddItem(const FInventoryItem& Item, FIntPoint Position,	bool bRotated);

	// 移除物品
	bool RemoveItem(FGuid ItemInstanceID);

	// 获取网格尺寸
	FIntPoint GetGridSize() const { return GridSize;}


private:

	// 网格大小
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FIntPoint GridSize;  // 

	// 运行时数据
	UPROPERTY()
	TMap<FGuid, FInventoryItemPlacement> Items;  // 所有已放置的物品 (InstanceID -> Placement)

	UPROPERTY()
	TArray<FGuid> OccupancyGrid;  // 占用状态（一维数组表示二维网格）
	//给每个空格子初始化一个 无效的Guid
	//当它被物品占用时，为这个格子的Guid赋值物品的Guid
	
	// 内部辅助方法
	// 检查 单个网格是否被占用
	bool IsAreaOccupied(FIntPoint Position, FIntPoint Size, FGuid IgnoreItemID);

	// 标记网格被占用
	void MarkGridOccupied(FIntPoint Position, FIntPoint Size, FGuid ItemID);

	// 清除网格的占用状态
	void ClearGridOccupancy(FGuid ItemID);

	// 从 ItemDataManager获取物品尺寸
	TOptional<FIntPoint> GetItemSize(FName ItemDefID, bool bRotated) const;
		
};
