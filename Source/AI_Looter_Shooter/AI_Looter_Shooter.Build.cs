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
			"SlateCore",
			"GeometryCore",
			"GeometryFramework",
			"GeometryScriptingCore"
		});

		// Tests scan every weapon definition asset.
		PrivateDependencyModuleNames.Add("AssetRegistry");
	}
}
