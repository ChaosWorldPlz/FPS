// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPS.h"
#include "Modules/ModuleManager.h"
#include "GAS/FPSGameplayTags.h"

class FFPSModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// Initialize native gameplay tags
		FFPSGameplayTags::InitializeNativeTags();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FFPSModule, FPS, "FPS");
