// Copyright Epic Games, Inc. All Rights Reserved.

#include "UGCPlayerController.h"
#include "UGCFunctionBridge.h"
#include "UGCHttpClient.h"

AUGCPlayerController::AUGCPlayerController()
{
    UGCBridge = CreateDefaultSubobject<UUGCFunctionBridge>(TEXT("UGCFunctionBridge"));
    UGCHttpClient = CreateDefaultSubobject<UUGCHttpClient>(TEXT("UGCHttpClient"));
}
