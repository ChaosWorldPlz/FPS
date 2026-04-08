// Copyright Epic Games, Inc. All Rights Reserved.

#include "UGCFunctionBridge.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "FPS/FPSCharacter.h"
#include "FPS/GAS/FPSGameplayAbility.h"
#include "FPS/GAS/FPSCombatAttributeSet.h"
#include "FPS/Level/FPSWorldWeapon.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

// -----------------------------------------------------------------------
// 属性白名单：允许 SetAttribute / GetAttribute 访问的字段
// -----------------------------------------------------------------------
namespace UGCAttributeWhitelist
{
    static const TMap<FString, float> MinValues = {
        { TEXT("Health"),        0.f   },
        { TEXT("MaxHealth"),     1.f   },
        { TEXT("Armor"),         0.f   },
        { TEXT("MovementSpeed"), 100.f },
        { TEXT("Stamina"),       0.f   },
    };
    static const TMap<FString, float> MaxValues = {
        { TEXT("Health"),        10000.f },
        { TEXT("MaxHealth"),     10000.f },
        { TEXT("Armor"),         10000.f },
        { TEXT("MovementSpeed"), 1200.f  },
        { TEXT("Stamina"),       10000.f },
    };
}

// -----------------------------------------------------------------------
// 规则白名单
// -----------------------------------------------------------------------
namespace UGCRuleWhitelist
{
    static const TMap<FString, float> MinValues = {
        { TEXT("RoundTime"),     60.f  },
        { TEXT("RespawnDelay"),  0.f   },
        { TEXT("FriendlyFire"),  0.f   },
        { TEXT("GravityScale"),  0.1f  },
    };
    static const TMap<FString, float> MaxValues = {
        { TEXT("RoundTime"),     3600.f },
        { TEXT("RespawnDelay"),  60.f   },
        { TEXT("FriendlyFire"),  1.f    },
        { TEXT("GravityScale"),  3.f    },
    };
}

// -----------------------------------------------------------------------
// 构造 / BeginPlay
// -----------------------------------------------------------------------

UUGCFunctionBridge::UUGCFunctionBridge()
{
    PrimaryComponentTick.bCanEverTick = false;

    // 规则默认值
    GameRules.Add(TEXT("RoundTime"),    300.f);
    GameRules.Add(TEXT("RespawnDelay"), 5.f);
    GameRules.Add(TEXT("FriendlyFire"), 0.f);
    GameRules.Add(TEXT("GravityScale"), 1.f);
}

void UUGCFunctionBridge::BeginPlay()
{
    Super::BeginPlay();
}

// -----------------------------------------------------------------------
// 私有辅助
// -----------------------------------------------------------------------

AFPSCharacter* UUGCFunctionBridge::GetFPSCharacter() const
{
    if (const APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        return Cast<AFPSCharacter>(PC->GetPawn());
    }
    return nullptr;
}

UAbilitySystemComponent* UUGCFunctionBridge::GetASC() const
{
    if (AFPSCharacter* Char = GetFPSCharacter())
    {
        return Char->GetAbilitySystemComponent();
    }
    return nullptr;
}

UFPSCombatAttributeSet* UUGCFunctionBridge::GetCombatAttributes() const
{
    if (UAbilitySystemComponent* ASC = GetASC())
    {
        return const_cast<UFPSCombatAttributeSet*>(
            ASC->GetSet<UFPSCombatAttributeSet>()
        );
    }
    return nullptr;
}

// -----------------------------------------------------------------------
// GAS — 技能操作
// -----------------------------------------------------------------------

bool UUGCFunctionBridge::GrantAbility(TSubclassOf<UFPSGameplayAbility> AbilityClass, int32 Level)
{
    if (!AbilityClass) return false;

    UAbilitySystemComponent* ASC = GetASC();
    if (!ASC || !ASC->GetOwnerActor()->HasAuthority()) return false;

    // 已经授予过同一类型则跳过
    if (GrantedHandles.Contains(AbilityClass)) return true;

    FGameplayAbilitySpec Spec(AbilityClass, Level, INDEX_NONE, GetOwner());
    FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
    if (Handle.IsValid())
    {
        GrantedHandles.Add(AbilityClass, Handle);
        return true;
    }
    return false;
}

bool UUGCFunctionBridge::RemoveAbility(TSubclassOf<UFPSGameplayAbility> AbilityClass)
{
    if (!AbilityClass) return false;

    UAbilitySystemComponent* ASC = GetASC();
    if (!ASC || !ASC->GetOwnerActor()->HasAuthority()) return false;

    FGameplayAbilitySpecHandle* Handle = GrantedHandles.Find(AbilityClass);
    if (!Handle) return false;

    ASC->ClearAbility(*Handle);
    GrantedHandles.Remove(AbilityClass);
    return true;
}

bool UUGCFunctionBridge::ApplyEffect(TSubclassOf<UGameplayEffect> EffectClass, float Magnitude)
{
    if (!EffectClass) return false;

    UAbilitySystemComponent* ASC = GetASC();
    if (!ASC) return false;

    FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
    Ctx.AddSourceObject(GetOwner());
    FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, Magnitude, Ctx);
    if (!Spec.IsValid()) return false;

    return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()).IsValid();
}

// -----------------------------------------------------------------------
// GAS — 属性操作
// -----------------------------------------------------------------------

bool UUGCFunctionBridge::SetAttribute(const FString& AttributeName, float Value)
{
    // 白名单检查
    const float* MinPtr = UGCAttributeWhitelist::MinValues.Find(AttributeName);
    const float* MaxPtr = UGCAttributeWhitelist::MaxValues.Find(AttributeName);
    if (!MinPtr || !MaxPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UGCBridge] SetAttribute: '%s' 不在白名单"), *AttributeName);
        return false;
    }

    // 范围 clamp
    Value = FMath::Clamp(Value, *MinPtr, *MaxPtr);

    UFPSCombatAttributeSet* Attrs = GetCombatAttributes();
    UAbilitySystemComponent* ASC  = GetASC();
    if (!Attrs || !ASC) return false;

    // 通过 GE 临时修改（SetBaseAttributeValueFromReplication 仅服务端有效）
    // 直接用 ForceSetAttributeBaseValue 修改基础值
    FGameplayAttribute Attr;

    if (AttributeName == TEXT("Health"))           Attr = UFPSCombatAttributeSet::GetHealthAttribute();
    else if (AttributeName == TEXT("MaxHealth"))   Attr = UFPSCombatAttributeSet::GetMaxHealthAttribute();
    else if (AttributeName == TEXT("Armor"))       Attr = UFPSCombatAttributeSet::GetArmorAttribute();
    else if (AttributeName == TEXT("MovementSpeed")) Attr = UFPSCombatAttributeSet::GetMovementSpeedAttribute();
    else if (AttributeName == TEXT("Stamina"))     Attr = UFPSCombatAttributeSet::GetStaminaAttribute();
    else return false;

    ASC->SetNumericAttributeBase(Attr, Value);
    return true;
}

float UUGCFunctionBridge::GetAttribute(const FString& AttributeName) const
{
    if (!UGCAttributeWhitelist::MinValues.Contains(AttributeName)) return -1.f;

    const UFPSCombatAttributeSet* Attrs = GetCombatAttributes();
    if (!Attrs) return -1.f;

    if (AttributeName == TEXT("Health"))           return Attrs->GetHealth();
    if (AttributeName == TEXT("MaxHealth"))        return Attrs->GetMaxHealth();
    if (AttributeName == TEXT("Armor"))            return Attrs->GetArmor();
    if (AttributeName == TEXT("MovementSpeed"))    return Attrs->GetMovementSpeed();
    if (AttributeName == TEXT("Stamina"))          return Attrs->GetStamina();

    return -1.f;
}

// -----------------------------------------------------------------------
// 武器操作
// -----------------------------------------------------------------------

AFPSWorldWeapon* UUGCFunctionBridge::SpawnWeapon(const FName& WeaponID, FVector Location)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AFPSWorldWeapon* Spawned = World->SpawnActor<AFPSWorldWeapon>(
        AFPSWorldWeapon::StaticClass(),
        Location,
        FRotator::ZeroRotator,
        Params
    );

    if (Spawned)
    {
        // WeaponItemDefID 对应 DT_ItemDefinition 的行名，用于背包物品创建
        Spawned->WeaponItemDefID = WeaponID;
    }

    return Spawned;
}

// -----------------------------------------------------------------------
// 游戏规则
// -----------------------------------------------------------------------

bool UUGCFunctionBridge::SetGameRule(const FString& RuleName, float Value)
{
    const float* MinPtr = UGCRuleWhitelist::MinValues.Find(RuleName);
    const float* MaxPtr = UGCRuleWhitelist::MaxValues.Find(RuleName);
    if (!MinPtr || !MaxPtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UGCBridge] SetGameRule: '%s' 不在白名单"), *RuleName);
        return false;
    }

    Value = FMath::Clamp(Value, *MinPtr, *MaxPtr);
    GameRules.Add(RuleName, Value);

    // GravityScale 直接作用于 WorldSettings
    if (RuleName == TEXT("GravityScale"))
    {
        if (UWorld* World = GetWorld())
        {
            if (AWorldSettings* WS = World->GetWorldSettings())
            {
                WS->WorldGravityZ = -980.f * Value;
            }
        }
    }

    return true;
}

float UUGCFunctionBridge::GetGameRule(const FString& RuleName) const
{
    if (!UGCRuleWhitelist::MinValues.Contains(RuleName)) return -1.f;
    const float* Val = GameRules.Find(RuleName);
    return Val ? *Val : -1.f;
}

// -----------------------------------------------------------------------
// LLM 统一入口
// -----------------------------------------------------------------------

FString UUGCFunctionBridge::ExecuteFunction(const FString& FuncName, const FString& ParamsJSON)
{
    // 转发给 UGCFunctionRegistry.lua 的 :Call()
    // 通过 UnLua 调用 Lua 函数
    // 暂时返回占位，Lua 层对接后替换
    UE_LOG(LogTemp, Log, TEXT("[UGCBridge] ExecuteFunction: %s(%s)"), *FuncName, *ParamsJSON);

    // TODO: 接入 UnLua 调用 UGCFunctionRegistry:Call(FuncName, ParamsJSON)
    return FString::Printf(TEXT("{\"ok\":false,\"error\":\"Lua registry not connected yet\"}"));
}
