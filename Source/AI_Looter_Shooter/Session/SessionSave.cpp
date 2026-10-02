// ULooterSessionSave: bringing older saves up to date, each map's world, and a trip's bookkeeping.

#include "Session/SessionSave.h"

int32 ULooterSessionSave::Upgrade()
{
	const int32 ReadVersion = Version;
	if (Version >= CurrentVersion)
	{
		// Up to date, or written by a newer game: left as it is.
		return ReadVersion;
	}

	// Before version 2 a session kept one world: the level it was saved in, which Map names. It becomes that map's world.
	// The old fields are emptied so nothing reads them twice; they stay in the class so older saves can still be read.
	if (bHasWorld)
	{
		FSavedMapWorld& MapWorld = FindOrAddWorld(Map.IsEmpty() ? FString(Version1Map) : Map);
		MapWorld.LootWeapons = MoveTemp(LootWeapons);
		MapWorld.AmmoPickups = MoveTemp(AmmoPickups);
		MapWorld.Racks = MoveTemp(Racks);
		MapWorld.TutorialStep = TutorialStep;
	}
	bHasWorld = false;
	LootWeapons.Reset();
	AmmoPickups.Reset();
	Racks.Reset();
	TutorialStep = INDEX_NONE;

	// Nothing else moves: the player (progress, guns and ammo, health, where they stood) is kept as it was, and the
	// campaign record starts empty, as for a session that hasn't left Skyreach.
	Version = CurrentVersion;
	return ReadVersion;
}

const FSavedMapWorld* ULooterSessionSave::FindWorld(const FString& MapPackage) const
{
	return MapPackage.IsEmpty() ? nullptr : Worlds.Find(MapPackage);
}

FSavedMapWorld& ULooterSessionSave::FindOrAddWorld(const FString& MapPackage)
{
	return Worlds.FindOrAdd(MapPackage);
}

bool ULooterSessionSave::HasPlayerSpotOn(const FString& MapPackage) const
{
	return bHasPlayerSpot && !Map.IsEmpty() && Map.Equals(MapPackage, ESearchCase::IgnoreCase);
}

FName ULooterSessionSave::GetArrivalOn(const FString& MapPackage) const
{
	// The landing belongs to the trip's destination: a session that has to continue somewhere else (its level not in
	// the game) starts at that level's start.
	if (HasPlayerSpotOn(MapPackage) || !Map.Equals(MapPackage, ESearchCase::IgnoreCase))
	{
		return NAME_None;
	}
	return ArrivalTag;
}

void ULooterSessionSave::PrepareTrip(const FString& Destination, FName Landing)
{
	// The spot belongs to the level being left; in the destination the player arrives at the landing (or its start).
	Map = Destination;
	bHasPlayerSpot = false;
	PlayerLocation = FVector::ZeroVector;
	PlayerView = FRotator::ZeroRotator;
	ArrivalTag = Landing;
}

int32 ULooterSessionSave::CountGuns() const
{
	return Inventory.Equipped.Num() + Inventory.Backpack.Num();
}
