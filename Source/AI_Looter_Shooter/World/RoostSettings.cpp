#include "World/RoostSettings.h"

URoostSettings::URoostSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("HobsRoosts");
}

const URoostSettings& URoostSettings::Get()
{
	return *GetDefault<URoostSettings>();
}

FLinearColor URoostSettings::GetBeaconColor() const
{
	// As a look sheet writes it (sRGB bytes), turned linear as the renderer takes it.
	return FLinearColor(BeaconColor);
}

FLinearColor URoostSettings::GetFlameCoreColor() const
{
	return FLinearColor(FlameCoreColor);
}
