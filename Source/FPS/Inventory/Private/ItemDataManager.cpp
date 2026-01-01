// Fill out your copyright notice in the Description page of Project Settings.


#include "FPS/Inventory/Public/ItemDataManager.h"

#include "FPS/Inventory/InventoryTypes.h"

void UItemDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Warning, TEXT("[ItemDataManager] ========== 初始化开始 =========="));

	// 使用 LoadObject 加载 DataTable（临时方案，以后会改用异步加载）
	// TODO: 改用 TSoftObjectPtr + 异步加载，详见开发指南 TODO 1
	ItemDefinitionTable = LoadObject<UDataTable>(
		nullptr,
		TEXT("/Game/Data/Items/DT_ItemDefinition")
	);

	if (ItemDefinitionTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemDataManager] DataTable 加载成功！"));
		BuildCache();
		UE_LOG(LogTemp, Warning, TEXT("[ItemDataManager] 缓存构建完成，共 %d 个物品"), ItemCache.Num());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemDataManager] DataTable 加载失败！路径: /Game/Data/Items/DT_ItemDefinition"));
	}

	UE_LOG(LogTemp, Warning, TEXT("[ItemDataManager] ========== 初始化结束 =========="));
}

bool UItemDataManager::IsContainerItem(FName ItemID) const
{
	const FItemDefinitionRow* ItemDef = GetItemDefinition(ItemID);

	if (!ItemDef)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[ItemDataManager] IsContainerItem: Item not found: %s"),
			*ItemID.ToString());
		return false;
	}

	return ItemDef->bIsContainer;
}

void UItemDataManager::BuildCache()
{
	if (!ItemDefinitionTable) return;

	TArray<FName> RowNames = ItemDefinitionTable->GetRowNames();
	UE_LOG(LogTemp, Warning, TEXT("[ItemDataManager] DataTable 有 %d 行"), RowNames.Num());

	for (FName RowName : RowNames)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemDataManager] 处理行: %s"), *RowName.ToString());

		FItemDefinitionRow* Row = ItemDefinitionTable->FindRow<FItemDefinitionRow>(
			RowName, TEXT("ItemDataManager")
		);
		if (Row)
		{
			ItemCache.Add(RowName, Row);
			UE_LOG(LogTemp, Warning, TEXT("[ItemDataManager] 缓存: %s (Size: %dx%d)"),
				*RowName.ToString(), Row->SizeX, Row->SizeY);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[ItemDataManager] 无法找到行数据: %s"), *RowName.ToString());
		}
	}
}