using UnrealBuildTool;

public class AI_Looter_Shooter : ModuleRules
{
	public AI_Looter_Shooter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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
			"SlateCore",
			"GeometryCore",
			"GeometryFramework",
			"GeometryScriptingCore"
		});

		// Tests scan every weapon definition asset.
		PrivateDependencyModuleNames.Add("AssetRegistry");
	}
}
