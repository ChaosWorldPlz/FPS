// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FPS/Inventory/InventoryTypes.h"
#include "ItemDataManager.generated.h"

/**
 * 
 */
UCLASS()
class FPS_API UItemDataManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:
	// 策划配表DataTable 引用
	UPROPERTY()
	TObjectPtr<UDataTable> ItemDefinitionTable;

	// 缓存（ItemID -> Row 指针）
	TMap<FName, FItemDefinitionRow*> ItemCache;

	// 从DataTable ItemDefinitionTable中遍历物品，存放进缓存中
	void BuildCache();

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	// 根据ItemId从缓存中获取物品定义指针
	FORCEINLINE  const FItemDefinitionRow* GetItemDefinition(FName ItemID) const {	return ItemCache.FindRef(ItemID); }

	// 获取物品尺寸（考虑旋转）
	FORCEINLINE FIntPoint GetItemSize(FName ItemID, bool bRotated) const{
		const FItemDefinitionRow* ItemDef = GetItemDefinition(ItemID);
		if (!ItemDef)
		{
			UE_LOG(LogTemp, Error, TEXT("[ItemDataManager] GetItemSize : Item Definition not found , return 1*1 , ItemID: %s", ItemID));
			return FIntPoint(1,1);
		}
		if (bRotated && ItemDef->bCanRotate)
		{
			return FIntPoint(ItemDef->SizeY,ItemDef->SizeX);
		}
		return FIntPoint(ItemDef->SizeX,ItemDef->SizeY);
	}

	// 判断是否是容器物品
	bool IsContainerItem(FName ItemID) const;

	
};
