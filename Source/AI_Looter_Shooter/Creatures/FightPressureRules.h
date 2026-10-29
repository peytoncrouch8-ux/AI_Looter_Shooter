#pragma once

#include "CoreMinimal.h"

/**
 * What the fight director reads of a player it runs a fight at (FIGHTS.md section 2, 2026-10-28), refreshed at each look at
 * the fights (Looter.Fight.RingRefresh): UFightDirectorSubsystem::GetPlayerRead. The kinds answer a camping player their own
 * way (the Spitter's flush, the Unpaid's sharper lunges); the director eases off a nearly dead one.
 */
struct AI_LOOTER_SHOOTER_API FFightPlayerRead
{
	/** Seconds they have stayed within Looter.Fight.StillRadius of one spot (flat). */
	float StillFor = 0.f;
	/** Seconds since any creature fighting them had a clear eye line at them. */
	float HiddenFor = 0.f;
	/** Their health and ward against their most of both (0-1). */
	float HealthShare = 1.f;
	/** Creatures fighting them at the last look. */
	int32 Fighters = 0;
	/** Standing still or hiding too long while enough fight them (FightPressureRules::IsCamping). */
	bool bCamping = false;
	/** The close-call ease is on: one turn fewer at them, of each kind, for a while. */
	bool bEasing = false;

	bool IsCamping() const { return bCamping; }
};

/**
 * How long a player has stood still: the spot they were last seen leave a circle round (an anchor), and since when. Moving
 * Radius or more from the anchor starts a new one there. Plain data, so the tests drive it without a level.
 */
struct AI_LOOTER_SHOOTER_API FFightStillWatch
{
	/** They are at Where at Now. */
	void Note(const FVector& Where, double Now, float Radius);
	/** Seconds they have kept within the circle (0 before the first note). */
	float StillFor(double Now) const;
	void Reset() { bStarted = false; }

private:
	FVector Anchor = FVector::ZeroVector;
	double Since = 0.0;
	bool bStarted = false;
};

/** How long no fighter has seen a player: the last look at which one had a clear line. */
struct AI_LOOTER_SHOOTER_API FFightSightWatch
{
	/** At Now, some fighter saw them (bSeen) or none did. The first note counts as seen: a fight starts with sight. */
	void Note(bool bSeen, double Now);
	/** Seconds since the last sight (0 before the first note). */
	float HiddenFor(double Now) const;
	void Reset() { bStarted = false; }

private:
	double LastSeen = 0.0;
	bool bStarted = false;
};

/**
 * The close-call ease (Looter.Fight.Ease): a player's health share falls under Below, and the director gives one turn fewer of
 * each kind for Seconds, once in Gap seconds. Nobody sees it; it turns deaths into near-deaths (player enjoyment first).
 */
struct AI_LOOTER_SHOOTER_API FFightEaseWatch
{
	/** Opens the ease if it's due (on, under Below, Gap since the last): true when it opened now. */
	bool TryOpen(float HealthShare, double Now, bool bOn, float Below, float Seconds, float Gap);
	bool IsEasing(double Now) const { return Now < Until; }
	/** Seconds left of it (0: none). */
	float Left(double Now) const { return static_cast<float>(FMath::Max(Until - Now, 0.0)); }

private:
	double Until = -1.0e9;
	double Last = -1.0e9;
};

/** One leap (a spider's pounce) at a target in a gap, whoever leaps (UFightDirectorSubsystem::MayLeap / NoteLeap). */
struct AI_LOOTER_SHOOTER_API FFightLeapGate
{
	bool MayLeap(double Now, float Gap) const { return Now - Last >= FMath::Max(Gap, 0.f); }
	void Note(double Now) { Last = Now; }

private:
	double Last = -1.0e9;
};

/** The player read's rules as plain functions the tests check without a level. */
namespace FightPressureRules
{
	/**
	 * Camping: still for StillSeconds or more, or hidden from every fighter for HiddenSeconds or more, while MinFighters or
	 * more fight them (one creature on its own never calls it camping).
	 */
	AI_LOOTER_SHOOTER_API bool IsCamping(float StillFor, float HiddenFor, int32 Fighters, float StillSeconds, float HiddenSeconds,
		int32 MinFighters);

	/** Health and ward together against their most (1 when there is nothing to measure). */
	AI_LOOTER_SHOOTER_API float HealthShare(float Health, float MaxHealth, float Ward, float WardCapacity);

	/** The turns at once with the ease on: one fewer, never under 1. */
	AI_LOOTER_SHOOTER_API int32 EasedTokens(int32 Tokens, bool bEasing);
}
