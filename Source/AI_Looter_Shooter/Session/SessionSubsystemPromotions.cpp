// USessionSubsystem: when each map's creatures are promoted on arrival, at most once per 20 minutes of play per map.

#include "Session/SessionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Session/SessionSave.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"

bool USessionSubsystem::IsPromotionRollDue(double RolledAt, double PlayedSeconds)
{
	// Never rolled, or rolled "after" now (a save whose time played went back), rolls; otherwise only after the cooldown.
	return RolledAt < 0.0 || PlayedSeconds < RolledAt || PlayedSeconds - RolledAt >= PromotionCooldown;
}

bool USessionSubsystem::ClaimPromotionRoll(UWorld* World)
{
	// Only the level being played has a world in the session to note it in (not the main menu's island, not a test's
	// level), and without even a trip's save in memory there's nowhere to note it: every start rolls.
	if (!World || World != PlayWorld.Get() || !Current)
	{
		return true;
	}
	const FString MapPackage = MapOf(World);
	const FSavedMapWorld* Here = Current->FindWorld(MapPackage);
	const double RolledAt = Here ? Here->PromotionsRolledAt : -1.0;
	const double PlayedNow = Current->PlayedSeconds + FMath::Max(World->GetTimeSeconds() - PlayClock, 0.0);
	if (IsPlayingSession() && !IsPromotionRollDue(RolledAt, PlayedNow))
	{
		UE_LOG(LogLooter, Log, TEXT("Promotions on %s rolled %.0f min of play ago: its creatures stay as placed until an arrival after %.0f min."),
			*FPackageName::GetShortName(MapPackage), (PlayedNow - RolledAt) / 60.0, PromotionCooldown / 60.0);
		return false;
	}

	// Noted in the map's world once the level has begun. On a first visit the session has no world for the map yet, and
	// one made before the level is put back would read as a visit that left nothing lying around.
	auto NoteRoll = [this, MapPackage, PlayedNow]()
	{
		if (Current)
		{
			Current->FindOrAddWorld(MapPackage).PromotionsRolledAt = PlayedNow;
		}
	};
	if (bWorldRestored)
	{
		NoteRoll();
	}
	else
	{
		World->OnWorldBeginPlay.AddWeakLambda(this, MoveTemp(NoteRoll));
	}
	UE_LOG(LogLooter, Log, TEXT("Promotions on %s roll at %.1f min of play%s."), *FPackageName::GetShortName(MapPackage), PlayedNow / 60.0,
		IsPlayingSession() ? TEXT("") : TEXT(" (no session: every start rolls)"));
	return true;
}
