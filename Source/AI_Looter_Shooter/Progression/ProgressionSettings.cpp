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
