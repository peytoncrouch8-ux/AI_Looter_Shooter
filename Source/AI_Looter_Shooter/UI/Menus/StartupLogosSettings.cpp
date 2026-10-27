#include "UI/Menus/StartupLogosSettings.h"

const UStartupLogosSettings& UStartupLogosSettings::Get()
{
	return *GetDefault<UStartupLogosSettings>();
}
