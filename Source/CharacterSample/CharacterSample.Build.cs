// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CharacterSample : ModuleRules
{
	public CharacterSample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
