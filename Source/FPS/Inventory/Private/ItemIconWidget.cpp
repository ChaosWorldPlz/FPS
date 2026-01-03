// Fill out your copyright notice in the Description page of Project Settings.

#include "FPS/Inventory/Public/ItemIconWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "FPS/Inventory/Public/ItemDataManager.h"

void UItemIconWidget::SetItemData(const FInventoryItem& InItem, bool bRotated)
{
	ItemData = InItem;
	bIsRotated = bRotated;

	// 获取物品定义
	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UItemDataManager* DataMgr = GI ? GI->GetSubsystem<UItemDataManager>() : nullptr;

	if (!DataMgr)
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemIconWidget] ItemDataManager not found"));
		return;
	}

	const FItemDefinitionRow* ItemDef = DataMgr->GetItemDefinition(ItemData.ItemDefID);
	if (!ItemDef)
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemIconWidget] Item definition not found: %s"), *ItemData.ItemDefID.ToString());
		return;
	}

	// 设置图标
	if (Image_Icon && !ItemDef->Icon.IsNull())
	{
		UTexture2D* IconTexture = ItemDef->Icon.LoadSynchronous();
		if (IconTexture)
		{
			Image_Icon->SetBrushFromTexture(IconTexture);

			// 如果旋转，旋转图标
			if (bIsRotated)
			{
				Image_Icon->SetRenderTransformAngle(90.0f);
			}
			else
			{
				Image_Icon->SetRenderTransformAngle(0.0f);
			}
		}
	}

	// 设置堆叠数量
	if (Text_StackCount)
	{
		if (ItemData.StackCount > 1)
		{
			Text_StackCount->SetText(FText::AsNumber(ItemData.StackCount));
			Text_StackCount->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// 调用蓝图回调
	OnItemDataSet();

	UE_LOG(LogTemp, Log, TEXT("[ItemIconWidget] Item data set: %s, Stack: %d, Rotated: %s"),
		*ItemData.ItemDefID.ToString(),
		ItemData.StackCount,
		bIsRotated ? TEXT("Yes") : TEXT("No"));
}
