// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnLuaInterface.h"
#include "FPSArmorComponent.generated.h"

/**
 * UFPSArmorComponent
 *
 * 挂在 AFPSCharacter 上，管理护甲等级和骨骼覆盖判断。
 * 护甲耐久由 GAS Armor 属性（UFPSCombatAttributeSet::Armor）管理，
 * 本组件不重复存储耐久值，通过 ASC 读写。
 *
 * 穿透规则（等级差 = 子弹等级 - 护甲等级）：
 *   >= +2 : 血量/甲耐久各自 100%，护甲不减血
 *   == +1 : 血量 100%，甲耐久 100%，优先扣甲，溢出丢弃
 *   ==  0 : 有甲时各 75%；无甲时血量 100%
 *   == -1 : 有甲时各 50%；无甲时血量 100%
 *   <= -2 : 有甲时各 25%；无甲时血量 100%
 */
UCLASS(ClassGroup = "FPS", BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class FPS_API UFPSArmorComponent : public UActorComponent, public IUnLuaInterface
{
	GENERATED_BODY()

public:
	UFPSArmorComponent();

	//-------------------------------------------------------------------
	// 配置（编辑器设置）
	//-------------------------------------------------------------------

	/** 护甲等级（1-6，与子弹等级比较决定穿透档位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Armor",
		meta = (ClampMin = "0", ClampMax = "6"))
	int32 ArmorLevel = 3;

	/** 该护甲覆盖的骨骼（命中这些骨骼时才触发护甲计算） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
	TSet<FName> CoveredBones;

	//-------------------------------------------------------------------
	// 对外接口
	//-------------------------------------------------------------------

	/**
	 * 命中骨骼是否被本护甲覆盖。
	 * @param BoneName  物理资产命中骨骼名
	 */
	UFUNCTION(BlueprintCallable, Category = "Armor")
	bool IsPartCovered(FName BoneName) const;

	/**
	 * 获取有效护甲等级。
	 * GAS Armor 属性耗尽后返回 0（视为裸身）。
	 */
	UFUNCTION(BlueprintCallable, Category = "Armor")
	int32 GetEffectiveArmorLevel() const;

	/**
	 * 扣除护甲耐久，修改 GAS Armor 属性（仅服务端调用）。
	 * @param Amount  要扣除的耐久量
	 * @return        实际扣除量（不超过剩余耐久）
	 */
	UFUNCTION(BlueprintCallable, Category = "Armor")
	float TakeDurabilityDamage(float Amount);

	// UActorComponent interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
