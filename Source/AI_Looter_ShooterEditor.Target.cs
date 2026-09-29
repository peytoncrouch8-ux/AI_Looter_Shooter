using UnrealBuildTool;

public class AI_Looter_ShooterEditorTarget : TargetRules
{
	public AI_Looter_ShooterEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("AI_Looter_Shooter");
	}
}
