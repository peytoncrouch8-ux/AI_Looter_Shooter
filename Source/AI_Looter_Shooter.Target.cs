using UnrealBuildTool;

public class AI_Looter_ShooterTarget : TargetRules
{
	public AI_Looter_ShooterTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("AI_Looter_Shooter");
	}
}
