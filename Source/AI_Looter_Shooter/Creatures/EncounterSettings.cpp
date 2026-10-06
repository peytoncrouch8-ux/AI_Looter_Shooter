#include "Creatures/EncounterSettings.h"
#include "Creatures/CreatureBase.h"

UEncounterSettings::UEncounterSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Encounters");

	// The Unpaid's 12 at once (Docs/Areas/RansomsRest.md, "Performance plan"), by the class's path: AUnpaidCreature comes at
	// step 16, and the cap holds from the moment it exists. Until then the entry finds no class and caps nothing.
	FEncounterClassCap Unpaid;
	Unpaid.CreatureClass = TSoftClassPtr<ACreatureBase>(FSoftObjectPath(TEXT("/Script/AI_Looter_Shooter.UnpaidCreature")));
	Unpaid.MaxAlive = 12;
	ClassCaps.Add(Unpaid);
}

const UEncounterSettings& UEncounterSettings::Get()
{
	return *GetDefault<UEncounterSettings>();
}

int32 UEncounterSettings::FindClassCap(const UClass* Class, const UClass** OutCountedClass) const
{
	int32 Cap = 0;
	const UClass* Counted = nullptr;
	for (const FEncounterClassCap& Entry : ClassCaps)
	{
		// A native class is found by its path without loading anything; one not made yet finds nothing.
		const UClass* Capped = Entry.CreatureClass.Get();
		if (!Class || !Capped || Entry.MaxAlive <= 0 || !Class->IsChildOf(Capped))
		{
			continue;
		}
		// The nearest kind wins: a cap on a creature's own class over one on a parent of it.
		if (!Counted || Capped->IsChildOf(Counted))
		{
			Cap = Entry.MaxAlive;
			Counted = Capped;
		}
	}
	if (OutCountedClass)
	{
		*OutCountedClass = Counted;
	}
	return Cap;
}
