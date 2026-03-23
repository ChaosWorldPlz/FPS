// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSRecoilComponent.h"
#include "FPS/Weapon/FPSRecoilProfile.h"
#include "FPS/FPSCharacter.h"

UFPSRecoilComponent::UFPSRecoilComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

//-------------------------------------------------------------------
// 对外接口
//-------------------------------------------------------------------

void UFPSRecoilComponent::OnShotFired(UFPSRecoilProfile* Profile)
{
	if (!Profile)
	{
		return;
	}

	ActiveProfile = Profile;
	TotalShotsFired++;

	// 以当前真实 Spread 为基准累加（先恢复再增加，模拟点射间隔的收枪效果）
	const float Now = GetWorld()->GetTimeSeconds();
	CurrentSpread = FMath::Min(
		ComputeCurrentSpread() + Profile->SpreadIncreasePerShot,
		Profile->MaxSpread
	);
	SpreadAtLastShot = CurrentSpread;
	LastShotTime     = Now;

	// 推进 Pattern Index
	const int32 PatternLength = Profile->RecoilPattern.Num();
	if (PatternLength > 0)
	{
		if (CurrentPatternIndex >= PatternLength - 1)
		{
			CurrentPatternIndex = OnPatternFinished(TotalShotsFired, PatternLength);
		}
		else
		{
			CurrentPatternIndex++;
		}
	}

	// 每次开枪都重置 Pattern 计时器（停火后 PatternResetTime 秒才真正归零）
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PatternResetTimerHandle);
		World->GetTimerManager().SetTimer(
			PatternResetTimerHandle,
			this,
			&UFPSRecoilComponent::ResetPattern,
			Profile->PatternResetTime,
			false
		);
	}
}

FVector UFPSRecoilComponent::GetFireDirection(FVector BaseAimDirection)
{
	if (!ActiveProfile)
	{
		return BaseAimDirection.GetSafeNormal();
	}

	bool bIsAiming = false;
	if (const AFPSCharacter* Char = Cast<AFPSCharacter>(GetOwner()))
	{
		bIsAiming = Char->bIsAiming;
	}

	// 1. 取 Pattern 偏移
	FVector2D RawPattern = FVector2D::ZeroVector;
	if (ActiveProfile->RecoilPattern.IsValidIndex(CurrentPatternIndex))
	{
		RawPattern = ActiveProfile->RecoilPattern[CurrentPatternIndex];
	}
	FVector2D PatternOffset = CalculatePatternOffset(RawPattern, CurrentPatternIndex, TotalShotsFired);

	// 2. 按时间戳现算 Spread（Lazy Evaluation，无需 Tick）
	float SpreadRadius = CalculateSpreadRadius(
		ComputeCurrentSpread(), CurrentPatternIndex, 0.f, bIsAiming);

	// 3. 生成随机散布偏移
	float RandomPitch = 0.0f;
	float RandomYaw   = 0.0f;
	if (SpreadRadius > 0.0f)
	{
		if (ActiveProfile->bUseGaussianSpread)
		{
			// 高斯分布：中心密集，边缘稀疏
			RandomPitch = GaussianRandom() * SpreadRadius * 0.5f;
			RandomYaw   = GaussianRandom() * SpreadRadius * 0.5f;
		}
		else
		{
			// 均匀圆锥
			float Angle  = FMath::FRand() * 2.0f * PI;
			float Radius = FMath::FRand() * SpreadRadius;
			RandomPitch  = FMath::Sin(Angle) * Radius;
			RandomYaw    = FMath::Cos(Angle) * Radius;
		}
	}

	// 4. 叠加 Pattern + Spread
	FVector Result = ApplyAngularOffset(
		BaseAimDirection,
		PatternOffset.Y + RandomPitch,   // Pitch
		PatternOffset.X + RandomYaw      // Yaw
	);

	return Result;
}

void UFPSRecoilComponent::ForceResetPattern()
{
	CurrentPatternIndex = 0;
	TotalShotsFired     = 0;
}

//-------------------------------------------------------------------
// Tick（当前无逻辑，Spread 用 Lazy Evaluation 计算）
//-------------------------------------------------------------------

void UFPSRecoilComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

//-------------------------------------------------------------------
// Spread 计算
//-------------------------------------------------------------------

float UFPSRecoilComponent::ComputeCurrentSpread() const
{
	if (!ActiveProfile || LastShotTime < 0.0f)
	{
		return ActiveProfile ? ActiveProfile->BaseSpread : 0.0f;
	}

	const float Elapsed   = GetWorld()->GetTimeSeconds() - LastShotTime;
	const float Recovered = Elapsed * ActiveProfile->SpreadRecoveryRate;
	return FMath::Max(ActiveProfile->BaseSpread, SpreadAtLastShot - Recovered);
}

void UFPSRecoilComponent::ResetPattern()
{
	// Lua 可通过 ShouldResetPattern 决定是否真正归零（保留扩展点）
	if (ActiveProfile && ShouldResetPattern(ActiveProfile->PatternResetTime))
	{
		CurrentPatternIndex = 0;
		TotalShotsFired     = 0;
	}
}

//-------------------------------------------------------------------
// BlueprintNativeEvent 默认实现
//-------------------------------------------------------------------

float UFPSRecoilComponent::CalculateSpreadRadius_Implementation(
	float BaseSpread, int32 PatternIndex, float InTimeSinceLastShot, bool bIsAiming)
{
	return CurrentSpread;
}

FVector2D UFPSRecoilComponent::CalculatePatternOffset_Implementation(
	FVector2D RawPatternValue, int32 PatternIndex, int32 InTotalShotsFired)
{
	return RawPatternValue;
}

int32 UFPSRecoilComponent::OnPatternFinished_Implementation(
	int32 InTotalShotsFired, int32 PatternLength)
{
	// 默认：停在最后一发
	return PatternLength - 1;
}

bool UFPSRecoilComponent::ShouldResetPattern_Implementation(float PatternResetTime)
{
	return true;
}

//-------------------------------------------------------------------
// 私有工具函数
//-------------------------------------------------------------------

float UFPSRecoilComponent::GaussianRandom()
{
	// Box-Muller 变换：生成标准正态分布随机数
	const float U = FMath::Max(FMath::FRand(), 1e-6f);
	const float V = FMath::FRand();
	return FMath::Sqrt(-2.0f * FMath::Loge(U)) * FMath::Cos(2.0f * PI * V);
}

FVector UFPSRecoilComponent::ApplyAngularOffset(FVector Direction, float PitchDeg, float YawDeg)
{
	FRotator Rot = Direction.Rotation();
	Rot.Pitch += PitchDeg;
	Rot.Yaw   += YawDeg;
	return Rot.Vector();
}
