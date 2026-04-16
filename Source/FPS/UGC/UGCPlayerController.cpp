// Copyright Epic Games, Inc. All Rights Reserved.

#include "UGCPlayerController.h"
#include "UGCFunctionBridge.h"
#include "UGCHttpClient.h"
#include "UGCEditorBridge.h"
#include "UGCPCGBridge.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "HAL/PlatformApplicationMisc.h"

AUGCPlayerController::AUGCPlayerController()
{
    UGCBridge     = CreateDefaultSubobject<UUGCFunctionBridge>(TEXT("UGCFunctionBridge"));
    UGCHttpClient = CreateDefaultSubobject<UUGCHttpClient>(TEXT("UGCHttpClient"));
    EditorBridge  = CreateDefaultSubobject<UUGCEditorBridge>(TEXT("UGCEditorBridge"));
    PCGBridge     = CreateDefaultSubobject<UUGCPCGBridge>(TEXT("UGCPCGBridge"));
}

void AUGCPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (ToggleEditorAction)
            EIC->BindAction(ToggleEditorAction, ETriggerEvent::Started, this,
                &AUGCPlayerController::HandleToggleEditorInput);

        if (EditorClickAction)
            EIC->BindAction(EditorClickAction, ETriggerEvent::Started, this,
                &AUGCPlayerController::HandleEditorClickInput);
    }
}

void AUGCPlayerController::CopyToClipboard(const FString& Text)
{
    FPlatformApplicationMisc::ClipboardCopy(*Text);
}

void AUGCPlayerController::HandleToggleEditorInput()
{
    ToggleEditor();
}

void AUGCPlayerController::HandleEditorClickInput()
{
    EditorClick();
}
