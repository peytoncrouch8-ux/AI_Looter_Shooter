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
	Curve.BaseXP = BaseXP;
	Curve.Growth = Growth;
	return Curve;
}

FLevelRules UProgressionSettings::GetLevelRules() const
{
	FLevelRules Rules;
	Rules.KillXPGrowth = KillXPGrowth;
	Rules.KillXPFalloff = KillXPFalloff;
	Rules.KillXPFloor = KillXPFloor;
	Rules.EnemyGrowth = EnemyGrowthPerLevel;
	Rules.HealthPerLevel = HealthPerLevel;
	return Rules;
}
