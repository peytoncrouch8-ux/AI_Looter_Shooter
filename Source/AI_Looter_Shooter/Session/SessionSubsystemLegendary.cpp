// USessionSubsystem: when a map's Legendary monsters come back (Docs/Areas/RansomsRest.md, Side 3: "She comes back on an
// arrival after at least 20 minutes of play since her last death"). Each one's last death is kept with its map's world in
// the session (FSavedMapWorld::LegendaryDefeatedAt), in the session's time played; its lair asks as the level begins.

#include "Session/SessionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Session/SessionSave.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"

bool USessionSubsystem::IsLegendaryReturnDue(double DefeatedAt, double PlayedSeconds)
{
	// Never beaten, or beaten "after" now (a save whose time played went back), it's there; otherwise only after its time.
	return DefeatedAt < 0.0 || PlayedSeconds < DefeatedAt || PlayedSeconds - DefeatedAt >= LegendaryReturnTime;
}

double USessionSubsystem::GetPlayedSecondsNow(const UWorld* World) const
{
	if (!Current)
	{
		return 0.0;
	}
	// The saved time played, and this level's play since the last save when World is the level being played.
	const bool bPlayedHere = World && World == PlayWorld.Get();
	return Current->PlayedSeconds + (bPlayedHere ? FMath::Max(World->GetTimeSeconds() - PlayClock, 0.0) : 0.0);
}

double USessionSubsystem::GetLegendaryDefeatedAt(const UWorld* World, FName LegendaryId) const
{
	const FSavedMapWorld* Here = Current && World ? Current->FindWorld(MapOf(World)) : nullptr;
	const double* DefeatedAt = Here ? Here->LegendaryDefeatedAt.Find(LegendaryId) : nullptr;
	return DefeatedAt ? *DefeatedAt : -1.0;
}

bool USessionSubsystem::IsLegendaryBack(const UWorld* World, FName LegendaryId) const
{
	if (!Current || !World || LegendaryId.IsNone())
	{
		return true;
	}
	const double DefeatedAt = GetLegendaryDefeatedAt(World, LegendaryId);
	const double PlayedNow = GetPlayedSecondsNow(World);
	const bool bBack = IsLegendaryReturnDue(DefeatedAt, PlayedNow);
	if (DefeatedAt >= 0.0)
	{
		const FString Map = FPackageName::GetShortName(MapOf(World));
		if (bBack)
		{
			UE_LOG(LogLooter, Log, TEXT("%s on %s: beaten %.0f min of play ago, and back this arrival."), *LegendaryId.ToString(), *Map,
				(PlayedNow - DefeatedAt) / 60.0);
		}
		else
		{
			UE_LOG(LogLooter, Log, TEXT("%s on %s: beaten %.0f min of play ago, away until an arrival %.0f min of play from now."),
				*LegendaryId.ToString(), *Map, (PlayedNow - DefeatedAt) / 60.0, (LegendaryReturnTime - (PlayedNow - DefeatedAt)) / 60.0);
		}
	}
	return bBack;
}

void USessionSubsystem::NoteLegendaryDefeat(UWorld* World, FName LegendaryId)
{
	// Without even a trip's save in memory there's nowhere to keep it: the level played straight from the editor, where
	// every start brings it back anyway.
	if (!World || LegendaryId.IsNone() || !Current)
	{
		return;
	}
	const FString MapPackage = MapOf(World);
	const double PlayedNow = GetPlayedSecondsNow(World);
	auto Note = [this, MapPackage, LegendaryId, PlayedNow]()
	{
		if (Current)
		{
			Current->FindOrAddWorld(MapPackage).LegendaryDefeatedAt.Add(LegendaryId, PlayedNow);
		}
	};
	// Into the map's world once it has been put back (always, in play): a world made before then would read as a visit
	// that left nothing lying around (as promotions' rolls are noted).
	if (World != PlayWorld.Get() || bWorldRestored)
	{
		Note();
	}
	else
	{
		World->OnWorldBeginPlay.AddWeakLambda(this, MoveTemp(Note));
	}
	UE_LOG(LogLooter, Log, TEXT("%s beaten on %s at %.1f min of play: back on an arrival from %.1f min."), *LegendaryId.ToString(),
		*FPackageName::GetShortName(MapPackage), PlayedNow / 60.0, (PlayedNow + LegendaryReturnTime) / 60.0);
	SaveSoon();
}

int32 USessionSubsystem::ForgetLegendaryDefeats(UWorld* World, FName LegendaryId)
{
	FSavedMapWorld* Here = Current && World ? Current->Worlds.Find(MapOf(World)) : nullptr;
	if (!Here)
	{
		return 0;
	}
	int32 Forgotten = 0;
	if (LegendaryId.IsNone())
	{
		Forgotten = Here->LegendaryDefeatedAt.Num();
		Here->LegendaryDefeatedAt.Reset();
	}
	else
	{
		Forgotten = Here->LegendaryDefeatedAt.Remove(LegendaryId);
	}
	if (Forgotten > 0)
	{
		SaveSoon();
	}
	return Forgotten;
}
