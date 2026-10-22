#include "Creatures/EncounterSettings.h"
#include "Creatures/CreatureBase.h"

UEncounterSettings::UEncounterSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Encounters");

	// The Unpaid's 16 at once (Docs/Areas/RansomsRest.md, "Performance plan"; 12 until the bigger fights of 2026-10-21, the
	// user's 6-10 a fight and the set pieces' 10 with a Bell-Ringer), by the class's path: AUnpaidCreature comes at
	// step 16, and the cap holds from the moment it exists. Until then the entry finds no class and caps nothing.
	FEncounterClassCap Unpaid;
	Unpaid.CreatureClass = TSoftClassPtr<ACreatureBase>(FSoftObjectPath(TEXT("/Script/AI_Looter_Shooter.UnpaidCreature")));
	Unpaid.MaxAlive = 16;
	ClassCaps.Add(Unpaid);

	// The kinds that may break off when hurt and alone: the animals. By path, as the Unpaid's cap is.
	RetreatKinds.Add(TSoftClassPtr<ACreatureBase>(FSoftObjectPath(TEXT("/Script/AI_Looter_Shooter.SpiderCreature"))));
	RetreatKinds.Add(TSoftClassPtr<ACreatureBase>(FSoftObjectPath(TEXT("/Script/AI_Looter_Shooter.SlimeCreature"))));
}

FCreaturePursuit::FRules UEncounterSettings::MakePursuitRules() const
{
	FCreaturePursuit::FRules Rules;
	Rules.UnseenGrace = UnseenGrace;
	Rules.SniffSeconds = SniffSeconds;
	Rules.LostSeconds = LostSeconds;
	Rules.MaxUnseenSeconds = FMath::Max(MaxUnseenSeconds, LostSeconds);
	Rules.ProwlSeconds = ProwlSeconds;
	Rules.StuckSeconds = StuckSeconds;
	return Rules;
}

bool UEncounterSettings::MayRetreat(const UClass* Class) const
{
	if (!Class)
	{
		return false;
	}
	for (const TSoftClassPtr<ACreatureBase>& Kind : RetreatKinds)
	{
		// A native class is found by its path without loading anything.
		const UClass* Allowed = Kind.Get();
		if (Allowed && Class->IsChildOf(Allowed))
		{
			return true;
		}
	}
	return false;
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
