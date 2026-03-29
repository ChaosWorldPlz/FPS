// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSProjectile.h"
#include "FPSWeaponDataAsset.h"
#include "FPS/FPSCharacter.h"
#include "FPS/Armor/FPSArmorComponent.h"
#include "FPS/Team/FPSPlayerState.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GenericTeamAgentInterface.h"
#include "Net/UnrealNetwork.h"

AFPSProjectile::AFPSProjectile()
{
	bReplicates = true;
	SetReplicatingMovement(true);

	// 碰撞体：半径 5cm，只和 WorldStatic/WorldDynamic/Pawn 碰
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->SetCollisionProfileName(TEXT("Projectile"));
	CollisionComp->OnComponentHit.AddDynamic(this, &AFPSProjectile::OnProjectileHit);
	RootComponent = CollisionComp;

	// 弹体运动：初速/重力由 Launch() 写入
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;

	// 默认 3s 超时（Launch 时会根据 DataAsset 重设）
	InitialLifeSpan = 0.0f; // 手动管理生命周期
}

void AFPSProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 设置超时自毁
	if (MaxLifetime > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			LifetimeTimerHandle,
			this,
			&AFPSProjectile::OnProjectileExpired_Implementation,
			MaxLifetime,
			false
		);
	}
}

void AFPSProjectile::Launch(FVector Direction, UFPSWeaponDataAsset* InWeaponData, AFPSCharacter* InInstigatorChar)
{
	WeaponData = InWeaponData;
	InstigatorChar = InInstigatorChar;

	// 从 DataAsset 写入数值（Lua 的 BeginPlay 会在此之后执行，可以覆盖）
	if (WeaponData)
	{
		InitialSpeed    = WeaponData->ProjectileSpeed;
		GravityScale    = WeaponData->ProjectileGravityScale;
		MaxLifetime     = WeaponData->ProjectileLifetime;
		Damage          = WeaponData->BaseDamage;
	}

	// 应用到运动组件
	ProjectileMovement->InitialSpeed    = InitialSpeed;
	ProjectileMovement->MaxSpeed        = InitialSpeed;
	ProjectileMovement->ProjectileGravityScale = GravityScale;
	ProjectileMovement->Velocity        = Direction.GetSafeNormal() * InitialSpeed;

	// 重设超时计时器（BeginPlay 里的计时器用的是修改前的值）
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);
	if (MaxLifetime > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			LifetimeTimerHandle,
			this,
			&AFPSProjectile::OnProjectileExpired_Implementation,
			MaxLifetime,
			false
		);
	}

	PlayLaunchEffects();
}

//-------------------------------------------------------------------
// BlueprintNativeEvent 默认实现
//-------------------------------------------------------------------

void AFPSProjectile::OnProjectileHit_Implementation(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}

	// 不打自己
	if (OtherActor && OtherActor == InstigatorChar.Get())
	{
		return;
	}

	ApplyDamageToTarget(Hit);
	PlayImpactEffects(Hit);

	if (!bPenetrating)
	{
		Destroy();
	}
}

void AFPSProjectile::OnProjectileExpired_Implementation()
{
	Destroy();
}

void AFPSProjectile::PlayLaunchEffects_Implementation()
{
	// C++ 默认空实现，交给 Lua 重写
}

void AFPSProjectile::PlayImpactEffects_Implementation(const FHitResult& Hit)
{
	// C++ 默认空实现，交给 Lua 重写
}

float AFPSProjectile::CalculateFinalDamage_Implementation(const FHitResult& Hit, AActor* HitActor)
{
	// 默认实现：不考虑穿透，直接返回当前 Damage
	// Lua 重写此函数以实现等级差档位逻辑
	return Damage;
}

float AFPSProjectile::CalculateFinalArmorDamage_Implementation(const FHitResult& Hit, AActor* HitActor)
{
	// 默认实现：直接返回武器数据里的护甲伤害值
	// Lua 重写此函数以实现等级差档位逻辑
	return WeaponData ? WeaponData->BulletArmorDamage : 0.0f;
}

//-------------------------------------------------------------------
// 内部：伤害应用
//-------------------------------------------------------------------

void AFPSProjectile::ApplyDamageToTarget(const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor || !WeaponData)
	{
		return;
	}

	// 队友过滤
	if (InstigatorChar.IsValid())
	{
		if (AFPSCharacter* TargetChar = Cast<AFPSCharacter>(HitActor))
		{
			ETeamAttitude::Type Attitude = InstigatorChar->GetTeamAttitudeTowards(*TargetChar);
			if (Attitude == ETeamAttitude::Friendly)
			{
				return;
			}
		}
	}

	// 血量伤害：由 Lua 重写 CalculateFinalDamage 实现穿透档位逻辑
	// C++ 默认实现直接返回 Damage（无穿透计算）
	const float FinalDamage = CalculateFinalDamage(Hit, HitActor);

	// 护甲耐久伤害：由 Lua 重写 CalculateFinalArmorDamage 实现档位逻辑
	const float FinalArmorDamage = CalculateFinalArmorDamage(Hit, HitActor);

	// 扣除护甲耐久
	if (AFPSCharacter* TargetChar = Cast<AFPSCharacter>(HitActor))
	{
		if (TargetChar->ArmorComponent)
		{
			TargetChar->ArmorComponent->TakeDurabilityDamage(FinalArmorDamage);
		}
	}

	// 血量伤害 → GAS GE
	if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor))
	{
		UAbilitySystemComponent* SourceASC = InstigatorChar.IsValid()
			? InstigatorChar->GetAbilitySystemComponent() : nullptr;

		if (SourceASC && WeaponData->DamageEffectClass)
		{
			FGameplayEffectContextHandle Ctx = SourceASC->MakeEffectContext();
			Ctx.AddSourceObject(this);
			Ctx.AddHitResult(Hit);

			FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(WeaponData->DamageEffectClass, 1.0f, Ctx);
			if (Spec.IsValid())
			{
				Spec.Data->SetSetByCallerMagnitude(
					FGameplayTag::RequestGameplayTag(TEXT("FPS.Effect.Damage")), FinalDamage);
				TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
			return;
		}
	}

	// Fallback（无 GAS 时）
	AController* InstigatorController = InstigatorChar.IsValid() ? InstigatorChar->GetController() : nullptr;
	UGameplayStatics::ApplyPointDamage(HitActor, FinalDamage,
		ProjectileMovement->Velocity.GetSafeNormal(), Hit,
		InstigatorController, this, nullptr);
}
