// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FPS : ModuleRules
{
	public FPS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"Niagara",
			"AIModule",
			"UnLua",
			"HTTP",
			"Json",
			"DesktopPlatform",
			"ApplicationCore",
			"PCG"
			// AnimAgent: 安装 glTFRuntime 插件后追加 "glTFRuntime"
		});
	}
}
