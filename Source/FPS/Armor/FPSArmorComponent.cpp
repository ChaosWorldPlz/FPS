// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSArmorComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "FPS/GAS/FPSCombatAttributeSet.h"
#include "Net/UnrealNetwork.h"

UFPSArmorComponent::UFPSArmorComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;

	// 默认覆盖胸腹骨骼（UE Mannequin 骨架命名）
	// 如需适配其他骨架，在 Blueprint 子类 Class Defaults 里修改 CoveredBones
	CoveredBones = {
		TEXT("spine_01"),
		TEXT("spine_02"),
		TEXT("spine_03"),
		TEXT("pelvis"),
		TEXT("clavicle_l"),
		TEXT("clavicle_r"),
	};
}

void UFPSArmorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// ArmorLevel 广播给所有人（死亡结算/观战时需要显示）
	DOREPLIFETIME(UFPSArmorComponent, ArmorLevel);
}

bool UFPSArmorComponent::IsPartCovered(FName BoneName) const
{
	return CoveredBones.Contains(BoneName);
}

int32 UFPSArmorComponent::GetEffectiveArmorLevel() const
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!ASC)
	{
		return 0;
	}

	bool bFound = false;
	float CurrentArmor = ASC->GetGameplayAttributeValue(
		UFPSCombatAttributeSet::GetArmorAttribute(), bFound);

	return (bFound && CurrentArmor > 0.0f) ? ArmorLevel : 0;
}

float UFPSArmorComponent::TakeDurabilityDamage(float Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return 0.0f;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!ASC)
	{
		return 0.0f;
	}

	bool bFound = false;
	float CurrentArmor = ASC->GetGameplayAttributeValue(
		UFPSCombatAttributeSet::GetArmorAttribute(), bFound);
	if (!bFound)
	{
		return 0.0f;
	}

	const float Actual = FMath::Min(Amount, CurrentArmor);
	const float NewArmor = FMath::Max(0.0f, CurrentArmor - Amount);

	// 直接设置属性（服务端权威，GAS 负责复制和 OnRep → OnArmorChanged 广播给 HUD）
	ASC->SetNumericAttributeBase(UFPSCombatAttributeSet::GetArmorAttribute(), NewArmor);

	return Actual;
}
