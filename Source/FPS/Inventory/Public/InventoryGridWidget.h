// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Layout/Geometry.h"
#include "InventoryGridWidget.generated.h"

class UInventoryGridComponent;
class UCanvasPanel;
class UImage;
class UItemIconWidget;

/**
 * 网格背包 UI Widget
 */
UCLASS()
class FPS_API UInventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 绑定到背包组件 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void BindToInventory(UInventoryGridComponent* InInventoryComponent);

	/** 刷新整个背包显示 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	void RefreshInventory();
	virtual void RefreshInventory_Implementation();

protected:
	/** 背包组件引用 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UInventoryGridComponent> InventoryComponent;

	/** 格子尺寸（像素） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	float SlotSize = 64.0f;

	/** 格子间距 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	float SlotPadding = 2.0f;

	/** 物品图标 Widget 类（在蓝图中设置） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TSubclassOf<UItemIconWidget> ItemIconWidgetClass;

	/** 右键菜单 Widget 类（在蓝图中设置） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|UI")
	TSubclassOf<UUserWidget> ContextMenuWidgetClass;

	/** Tooltip Widget 类（在蓝图中设置） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|UI")
	TSubclassOf<UUserWidget> TooltipWidgetClass;

	// UMG 组件（在蓝图中绑定）
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> GridCanvas;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BackgroundImage;

private:
	/** 背包变化回调 */
	UFUNCTION()
	void OnInventoryChangedHandler();

	/** 绘制网格线 */
	void DrawGridLines();

	/** 清空所有物品图标 */
	void ClearItemIcons();

	/** 生成物品图标 */
	void GenerateItemIcons();

	/** 已生成的物品图标 */
	UPROPERTY()
	TArray<TObjectPtr<UItemIconWidget>> ItemIcons;
};