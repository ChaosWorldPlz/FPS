// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UnLuaInterface.h"
#include "FPSProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UFPSWeaponDataAsset;
class AFPSCharacter;

/**
 * AFPSProjectile
 *
 * 物理弹体 Actor。C++ 负责碰撞/物理/网络同步，
 * 音效、特效、数值由 Lua 脚本（BP_FPSProjectile.lua）热更覆盖。
 *
 * 绑定方式：创建 BP_FPSProjectile（继承本类），
 * 在蓝图 Class Defaults → LuaFilePath 填入脚本路径。
 */
UCLASS(BlueprintType, Blueprintable)
class FPS_API AFPSProjectile : public AActor, public IUnLuaInterface
{
	GENERATED_BODY()

public:
	AFPSProjectile();

	//-------------------------------------------------------------------
	// 组件
	//-------------------------------------------------------------------

	/** 碰撞体（决定命中判定半径） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	TObjectPtr<USphereComponent> CollisionComp;

	/** 弹体运动组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	//-------------------------------------------------------------------
	// 数值（Lua 可在 BeginPlay 或热更时修改）
	//-------------------------------------------------------------------

	/** 初速度（cm/s），由武器 DataAsset 在 Launch 时写入，Lua 可覆盖 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float InitialSpeed = 5000.0f;

	/** 重力缩放（0=无重力，1=正常重力） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float GravityScale = 0.1f;

	/** 最大飞行时间（超时自毁，秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float MaxLifetime = 3.0f;

	/** 伤害值（由武器 DataAsset 传入，Lua 可覆盖） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	float Damage = 25.0f;

	/** 是否穿透（命中后继续飞行） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Stats")
	bool bPenetrating = false;

	//-------------------------------------------------------------------
	// 发射接口
	//-------------------------------------------------------------------

	/**
	 * 发射弹体。由武器 Fire() 调用，传入 DataAsset 数据。
	 * Lua 在此之后可通过 BeginPlay 修改数值。
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void Launch(FVector Direction, UFPSWeaponDataAsset* InWeaponData, AFPSCharacter* InInstigatorChar);

	//-------------------------------------------------------------------
	// Lua 可重写的事件（BlueprintNativeEvent = C++ 默认实现 + Lua/BP 可覆盖）
	//-------------------------------------------------------------------

	/** 命中事件：Lua 重写此函数来播放音效、特效、处理特殊逻辑 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Projectile")
	void OnProjectileHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** 超时自毁事件：Lua 可重写（如播放消散特效） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Projectile")
	void OnProjectileExpired();

	/** 发射特效/音效事件：Lua 重写此函数调用 Wwise 或播放粒子 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Projectile")
	void PlayLaunchEffects();

	/** 命中特效/音效：Lua 根据命中 Actor/物理材质选择音效 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Projectile")
	void PlayImpactEffects(const FHitResult& Hit);

	/**
	 * 计算最终血量伤害（Lua 重写实现穿透档位逻辑）。
	 * C++ 默认实现：直接返回 Damage（爆头倍率已在调用前处理）。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Projectile")
	float CalculateFinalDamage(const FHitResult& Hit, AActor* HitActor);

	/**
	 * 计算最终护甲耐久伤害（Lua 重写实现穿透档位逻辑）。
	 * C++ 默认实现：直接返回 WeaponData->BulletArmorDamage。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Projectile")
	float CalculateFinalArmorDamage(const FHitResult& Hit, AActor* HitActor);

protected:
	virtual void BeginPlay() override;

	/** 武器数据引用（发射时由武器传入） */
	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UFPSWeaponDataAsset> WeaponData;

	/** 发射者角色引用 */
	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	TWeakObjectPtr<AFPSCharacter> InstigatorChar;

private:
	/** 应用伤害到命中目标 */
	void ApplyDamageToTarget(const FHitResult& Hit);

	FTimerHandle LifetimeTimerHandle;
};
