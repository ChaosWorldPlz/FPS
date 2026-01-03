// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPS/Inventory/InventoryTypes.h"
#include "InventoryGridComponent.generated.h"

// 背包变化事件委托（用于通知 UI 刷新）
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

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

	// ============================================================
	// 核心操作
	// ============================================================

	/** 检查是否可以放置物品（不实际放置） */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanPlaceItem(FName ItemDefID, FIntPoint Position, bool bRotated, FGuid IgnoreItemID = FGuid());

	/** 添加物品到网格 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FInventoryItem& Item, FIntPoint Position, bool bRotated);

	/** 移除物品 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FGuid ItemInstanceID);

	/** 移动物品到新位置 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItem(FGuid ItemInstanceID, FIntPoint NewPosition, bool bNewRotated);

	// ============================================================
	// 查询接口
	// ============================================================

	/** 获取网格尺寸 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FIntPoint GetGridSize() const { return GridSize; }

	/** 获取所有已放置的物品 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<FInventoryItemPlacement> GetAllItems() const;

	/** 根据 InstanceID 获取物品放置信息 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetItemPlacement(FGuid ItemInstanceID, FInventoryItemPlacement& OutPlacement) const;

	/** 获取所有区域定义 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FInventoryRegion>& GetRegions() const { return Regions; }

	// ============================================================
	// 事件
	// ============================================================

	/** 背包变化事件（物品添加、移除、移动时触发） */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

private:

	// ============================================================
	// 配置数据
	// ============================================================

	/** 网格总大小 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	FIntPoint GridSize;

	/**
	 * 区域定义（"兜"）
	 * 如果为空，整个网格视为一个区域（无隔断）
	 * 如果有定义，物品必须完全放在某一个区域内，不能跨区
	 */
	UPROPERTY(EditAnywhere, Category = "Inventory")
	TArray<FInventoryRegion> Regions;

	// ============================================================
	// 运行时数据
	// ============================================================

	/** 所有已放置的物品 (InstanceID -> Placement) */
	UPROPERTY()
	TMap<FGuid, FInventoryItemPlacement> Items;

	/** 占用状态（一维数组表示二维网格） */
	UPROPERTY()
	TArray<FGuid> OccupancyGrid;

	// ============================================================
	// 内部辅助方法
	// ============================================================

	/** 检查区域是否被占用 */
	bool IsAreaOccupied(FIntPoint Position, FIntPoint Size, FGuid IgnoreItemID);

	/** 标记网格被占用 */
	void MarkGridOccupied(FIntPoint Position, FIntPoint Size, FGuid ItemID);

	/** 清除网格的占用状态 */
	void ClearGridOccupancy(FGuid ItemID);

	/** 从 ItemDataManager 获取物品尺寸 */
	TOptional<FIntPoint> GetItemSize(FName ItemDefID, bool bRotated) const;

	/**
	 * 查找物品可以放入的区域
	 * @param Position 放置位置
	 * @param Size 物品尺寸
	 * @return 如果找到合适的区域返回区域索引，否则返回 INDEX_NONE
	 */
	int32 FindRegionForPlacement(FIntPoint Position, FIntPoint Size) const;
};
