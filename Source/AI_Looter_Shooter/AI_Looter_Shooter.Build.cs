using UnrealBuildTool;

public class AI_Looter_Shooter : ModuleRules
{
	public AI_Looter_Shooter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Compile every .cpp on its own. Unity builds merge files, so private names and file-wide `using` directives
		// clash with whatever lands in the same merged file, and every added file reshuffles the groups.
		bUseUnity = false;

		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput", "AIModule",
			"AnimationCore",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore"
		});

		// Tests scan every weapon definition asset.
		PrivateDependencyModuleNames.Add("AssetRegistry");

		// The meadow's ground fit filter is a PCG node, and the meadow's graph ships with the level.
		PrivateDependencyModuleNames.Add("PCG");

		// Looter.Tour reads its viewpoints from JSON and the frame timings from the renderer.
		PrivateDependencyModuleNames.AddRange(new string[] { "Json", "RHI", "RenderCore" });

		// The experience curve is tuned in Project Settings (UProgressionSettings).
		PrivateDependencyModuleNames.Add("DeveloperSettings");

		// Sessions open the default map and the main menu from the project's map settings (UGameMapsSettings).
		PrivateDependencyModuleNames.Add("EngineSettings");

		// Ellis's body copies the mannequin's pose through an IK retargeter (UBodyRetargetAnimInstance).
		PrivateDependencyModuleNames.Add("IKRig");

		// The first-person support hand finds a gun's underside on its parts' cooked collision meshes (FirstPersonHandsCupFit.cpp).
		PrivateDependencyModuleNames.AddRange(new string[] { "Chaos", "PhysicsCore" });
	}
}
