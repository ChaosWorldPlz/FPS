// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FPS/Inventory/InventoryTypes.h"
#include "ItemIconWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * 物品图标 Widget 基类
 * 用于在背包网格中显示单个物品
 */
UCLASS()
class FPS_API UItemIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 设置物品数据 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemData(const FInventoryItem& InItem, bool bRotated);

	/** 获取物品实例 ID */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FGuid GetItemInstanceID() const { return ItemData.InstanceID; }

	/** 获取物品定义 ID */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	FName GetItemDefID() const { return ItemData.ItemDefID; }

	/** 是否已旋转 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool IsRotated() const { return bIsRotated; }

	/** 获取物品数据 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const FInventoryItem& GetItemData() const { return ItemData; }

protected:
	/** 物品图标（在蓝图中绑定，名字必须是 Image_Icon） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Icon;

	/** 堆叠数量文本（在蓝图中绑定，名字必须是 Text_StackCount） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount;

	/** 物品数据 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FInventoryItem ItemData;

	/** 是否已旋转 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bIsRotated = false;

	/** 蓝图可重写：物品数据设置后的回调 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnItemDataSet();
};
