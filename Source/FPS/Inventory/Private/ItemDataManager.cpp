// Fill out your copyright notice in the Description page of Project Settings.


#include "FPS/Inventory/Public/ItemDataManager.h"

#include "FPS/Inventory/InventoryTypes.h"

void UItemDataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 硬引用方式加载（适合小型项目） 这里我期望改用其他方法，结合lua或者？，总之杜绝硬引用
	static ConstructorHelpers::FObjectFinder<UDataTable> ItemTableFinder(
		TEXT("/Game/Data/Items/DT_ItemDefinition")
	);

	if (ItemTableFinder.Succeeded())
	{
		ItemDefinitionTable = ItemTableFinder.Object;
		BuildCache();
	}
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
	for (FName RowName : RowNames)
	{
		FItemDefinitionRow* Row = ItemDefinitionTable->FindRow<FItemDefinitionRow>(
			RowName, TEXT("GameDataManager")
		);
		if (Row)
		{
			ItemCache.Add(RowName, Row);
		}
	}
}