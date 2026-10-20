#include "Creatures/CreatureTraitSettings.h"

UCreatureTraitSettings::UCreatureTraitSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Creature Traits");
}

const UCreatureTraitSettings& UCreatureTraitSettings::Get()
{
	return *GetDefault<UCreatureTraitSettings>();
}
