using UnrealBuildTool;

/** Editor-only tools: baking the procedural props into assets. Never part of a packaged game. */
public class LooterEditor : ModuleRules
{
	public LooterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"AssetTools",
			"GeometryCore",
			"GeometryFramework",
			"GeometryScriptingCore",
			"GeometryScriptingEditor",
			"AI_Looter_Shooter"
		});
	}
}
