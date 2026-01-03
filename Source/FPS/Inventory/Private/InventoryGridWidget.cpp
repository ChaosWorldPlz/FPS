
#include "FPS/Inventory/Public/InventoryGridWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "FPS/Inventory/Public/InventoryGridComponent.h"
#include "FPS/Inventory/Public/ItemIconWidget.h"
#include "FPS/Inventory/Public/ItemDataManager.h"

void UInventoryGridWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UInventoryGridWidget::NativeDestruct()
{
	// 解绑事件
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UInventoryGridWidget::OnInventoryChangedHandler);
	}
	Super::NativeDestruct();
}

void UInventoryGridWidget::BindToInventory(UInventoryGridComponent* InInventoryComponent)
{
	// 解绑旧的
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UInventoryGridWidget::OnInventoryChangedHandler);
	}

	InventoryComponent = InInventoryComponent;

	// 绑定新的
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddDynamic(this, &UInventoryGridWidget::OnInventoryChangedHandler);
		RefreshInventory();
	}
}

void UInventoryGridWidget::RefreshInventory_Implementation()
{
	if (!InventoryComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryGridWidget] RefreshInventory: InventoryComponent 为空，无法刷新"));
		return;
	}

	ClearItemIcons();
	DrawGridLines();
	GenerateItemIcons();
}

void UInventoryGridWidget::OnInventoryChangedHandler()
{
	RefreshInventory();
}

void UInventoryGridWidget::DrawGridLines()
{
	if (!InventoryComponent || !GridCanvas)
	{
		return;
	}

	FIntPoint GridSize = InventoryComponent->GetGridSize();

	// TODO: 使用 UMG 的 Border 或 Image 绘制网格线
	// 这里可以在蓝图中完成，或者使用 Slate 绘制

	UE_LOG(LogTemp, Log, TEXT("[InventoryGridWidget] 绘制网格: %dx%d"), GridSize.X, GridSize.Y);
}

void UInventoryGridWidget::ClearItemIcons()
{
	for (UItemIconWidget* Icon : ItemIcons)
	{
		if (Icon)
		{
			Icon->RemoveFromParent();
		}
	}
	ItemIcons.Empty();
}

void UInventoryGridWidget::GenerateItemIcons()
{
	if (!InventoryComponent || !GridCanvas || !ItemIconWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InventoryGridWidget] GenerateItemIcons: 缺少必要组件 (Component=%s, Canvas=%s, IconClass=%s)"),
			InventoryComponent ? TEXT("OK") : TEXT("NULL"),
			GridCanvas ? TEXT("OK") : TEXT("NULL"),
			ItemIconWidgetClass ? TEXT("OK") : TEXT("NULL"));
		return;
	}

	// 获取 ItemDataManager
	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UItemDataManager* DataMgr = GI ? GI->GetSubsystem<UItemDataManager>() : nullptr;
	if (!DataMgr)
	{
		UE_LOG(LogTemp, Error, TEXT("[InventoryGridWidget] ItemDataManager not found"));
		return;
	}

	// 遍历所有物品
	TArray<FInventoryItemPlacement> AllItems = InventoryComponent->GetAllItems();
	for (const FInventoryItemPlacement& Placement : AllItems)
	{
		// 创建图标 Widget
		UItemIconWidget* IconWidget = CreateWidget<UItemIconWidget>(this, ItemIconWidgetClass);
		if (!IconWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("[InventoryGridWidget] Failed to create ItemIconWidget"));
			continue;
		}

		// 设置物品数据
		IconWidget->SetItemData(Placement.Item, Placement.bIsRotated);

		// 计算位置
		float PosX = Placement.GridPosition.X * (SlotSize + SlotPadding);
		float PosY = Placement.GridPosition.Y * (SlotSize + SlotPadding);

		// 获取物品尺寸
		TOptional<FIntPoint> ItemSizeOpt = DataMgr->GetItemSize(Placement.Item.ItemDefID, Placement.bIsRotated);
		FIntPoint ItemSize = ItemSizeOpt.IsSet() ? ItemSizeOpt.GetValue() : FIntPoint(1, 1);

		float Width = ItemSize.X * SlotSize + (ItemSize.X - 1) * SlotPadding;
		float Height = ItemSize.Y * SlotSize + (ItemSize.Y - 1) * SlotPadding;

		// 添加到 Canvas
		UCanvasPanelSlot* CanvasSlot = GridCanvas->AddChildToCanvas(IconWidget);
		if (CanvasSlot)
		{
			CanvasSlot->SetPosition(FVector2D(PosX, PosY));
			CanvasSlot->SetSize(FVector2D(Width, Height));
		}

		ItemIcons.Add(IconWidget);
	}

	UE_LOG(LogTemp, Log, TEXT("[InventoryGridWidget] Generated %d item icons"), ItemIcons.Num());
}