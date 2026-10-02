#pragma once

#include "CoreMinimal.h"

/** Why the session being played is about to save. */
enum class ESessionSaveReason : uint8
{
	/** The autosave every minute. */
	Autosave,
	/** A few seconds after progress (USessionSubsystem::SaveSoon). */
	Soon,
	/** Asked for now: Save & Quit, Looter.Session.Save. */
	Asked,
	/** The level is ending: quitting the game, stopping Play-In-Editor, or opening another level. */
	LevelEnd,
};

/**
 * When the session being played may save, kept apart from levels so tests can drive it (USessionSubsystem asks it before
 * every save).
 *  - A trip (USessionSubsystem::TravelToMap) has saved the session pointing at its destination. Until the destination
 *    begins, nothing saves: the level being left would file itself as the one the session continues in, its own save
 *    as it tears down most of all.
 *  - Holds (a ride, a fade, a scene) make autosaves and save-soons wait until the last one is released. A save asked for
 *    and the level's end still save.
 */
class AI_LOOTER_SHOOTER_API FSessionSaveGate
{
public:
	bool Allows(ESessionSaveReason Reason) const;

	/** A trip's save is written and its destination is opening. */
	void BeginTrip(const FString& Destination);

	bool IsTravelling() const { return bTravelling; }

	/** Where the trip under way goes; empty when none is. */
	const FString& GetTripDestination() const { return TripDestination; }

	/** Holds autosaves and save-soons for Reason ("SkiffRide"); the same reason twice is one hold. */
	void Hold(FName Reason);

	/** Ends Reason's hold. True when that was the last one, so a save that waited can go now. */
	bool Release(FName Reason);

	bool IsHeld() const { return !Holds.IsEmpty(); }

	/** A level began, or the main menu: no trip under way and nothing held. */
	void Reset();

private:
	TArray<FName> Holds;
	FString TripDestination;
	bool bTravelling = false;
};
