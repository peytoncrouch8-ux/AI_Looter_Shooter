#include "Progression/ProgressionSettings.h"

UProgressionSettings::UProgressionSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Progression");
}

FXPCurve UProgressionSettings::GetCurve() const
{
	FXPCurve Curve;
	Curve.MaxLevel = MaxLevel;
	Curve.CurveScale = CurveScale;
	Curve.CurvePower = CurvePower;
	return Curve;
}

FLevelRules UProgressionSettings::GetLevelRules() const
{
	FLevelRules Rules;
	Rules.KillXPScale = KillXPScale;
	Rules.KillXPPower = KillXPPower;
	Rules.KillXPFalloff = KillXPFalloff;
	Rules.KillXPFloor = KillXPFloor;
	Rules.EnemyGrowth = EnemyGrowthPerLevel;
	Rules.HealthPerLevel = HealthPerLevel;
	return Rules;
}
