using UnrealBuildTool;

/**
 * Editor-only tools: procedural props for building levels and baking them into assets, and importing Blender models.
 * Never part of a packaged game.
 */
public class LooterEditor : ModuleRules
{
	public LooterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;

		PrivateIncludePaths.Add(ModuleDirectory);

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"AssetTools",
			"AssetRegistry",
			"Json",
			"PhysicsCore",
			"GeometryCore",
			"GeometryFramework",
			"GeometryScriptingCore",
			"GeometryScriptingEditor",
			"SkeletalMeshUtilitiesCommon",
			"TargetPlatform",
			"AI_Looter_Shooter"
		});
	}
}
