// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CombatDemo : ModuleRules
{
	public CombatDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate", 
			"GameplayAbilities",
			"GameplayTags",
            "GameplayTasks",
            "NavigationSystem"
        });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"CombatDemo",
			"CombatDemo/Variant_Platforming",
			"CombatDemo/Variant_Platforming/Animation",
			"CombatDemo/Variant_Combat",
			"CombatDemo/Variant_Combat/AI",
			"CombatDemo/Variant_Combat/Animation",
			"CombatDemo/Variant_Combat/Gameplay",
			"CombatDemo/Variant_Combat/Interfaces",
			"CombatDemo/Variant_Combat/UI",
			"CombatDemo/Variant_SideScrolling",
			"CombatDemo/Variant_SideScrolling/AI",
			"CombatDemo/Variant_SideScrolling/Gameplay",
			"CombatDemo/Variant_SideScrolling/Interfaces",
			"CombatDemo/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
