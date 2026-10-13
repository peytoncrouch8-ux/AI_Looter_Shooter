#pragma once

#include "CoreMinimal.h"

/** What brings Hob's crows over the screen. */
enum class ERoostVeilKind : uint8
{
	/** Fast travel: the crows lift Ellis from one roost and carry them to another. */
	Travel,
	/** A death: the crows gather round the dying and drag them back to the nearest roost. */
	Death,
};

/** Where the crows over the screen are. */
enum class ERoostVeilPhase : uint8
{
	Idle,
	/** A death's crows circling at the screen's edges, more and more of them. */
	Gather,
	/** The flock pouring across the screen as the dark under it rises to black. */
	Cover,
	/** Black, a few wings flickering in it, until the player has been moved (Reveal). */
	Hold,
	/** The crows bursting off the screen and the world coming back. */
	Reveal,
};

/**
 * One crow over the screen at a moment, in the veil's own frame: the screen's middle at (0, 0) and its height 1, x to the
 * right and y down, so a screen W wide and H high spans x from -W/(2H) to W/(2H) and y from -0.5 to 0.5.
 */
struct FRoostVeilCrow
{
	FVector2f Position = FVector2f::ZeroVector;
	/** The way it flies (radians, clockwise from the screen's right, as Slate turns things). */
	float Angle = 0.f;
	/** Its wingspan, as a share of the screen's height. */
	float Size = 0.1f;
	/** Its wings: 0 raised, 1 level, 2 lowered. */
	int32 Frame = 1;
	float Opacity = 1.f;
};

/**
 * Hob's crows over the screen as plain math, the carry's look (the user's call, 2026-10-13: Hob and his crows carry Ellis
 * between roosts and drag them back to the nearest after a death): URoostCarrySubsystem plays it, UHudCrowVeilWidget draws
 * it, and the tests drive it without a screen.
 *  - Gather (a death only): crows come in over the screen's edges and circle there, more and more, round the dying.
 *  - Cover: the flock pours across the screen from every side (a fast travel's from below and the sides, lifting), each
 *    crow swelling as it flies past the camera, while the dark under them (the shade) rises to black.
 *  - Hold: black, a few wings flickering in it, until Reveal (the player has been moved).
 *  - Reveal: the crows burst outward from the middle and away off the screen's edges; the shade is gone in the first half.
 * Every crow's flight comes from the seed, so a veil plays the same from the same seed.
 */
struct AI_LOOTER_SHOOTER_API FRoostVeil
{
	/** How many crows each phase draws at most. */
	static constexpr int32 GatherCrows = 26;
	static constexpr int32 CoverCrows = 64;
	static constexpr int32 HoldCrows = 10;
	static constexpr int32 RevealCrows = 56;

	/** A death's gathering at the screen's edges, at most (seconds; the rest of the cover time covers). */
	static constexpr float GatherSeconds = 1.1f;
	/** The burst off the screen (seconds). */
	static constexpr float RevealSeconds = 0.9f;

	/**
	 * Starts it: the screen black CoverIn seconds from now. A death's crows gather at the edges for the first part (at most
	 * GatherSeconds, never more than 55% of it), then cover; a travel's cover over all of it. Seed picks every crow's flight.
	 */
	void Start(ERoostVeilKind InKind, float CoverIn, int32 InSeed);

	/** The crows burst off the screen over Seconds, from wherever it has got to (held black, or still covering). */
	void Reveal(float Seconds = RevealSeconds);

	/** Gone at once (a level ending). */
	void Stop();

	/** Moves it on by DeltaSeconds: Gather into Cover into Hold, which lasts until Reveal; Reveal into Idle. */
	void Advance(float DeltaSeconds);

	bool IsActive() const { return Phase != ERoostVeilPhase::Idle; }
	ERoostVeilPhase GetPhase() const { return Phase; }
	ERoostVeilKind GetKind() const { return Kind; }
	/** Seconds into the phase. */
	float GetPhaseTime() const { return Clock; }
	/** The phase's length (Hold's is open: 0). */
	float GetPhaseLength() const;

	/** The dark under the crows (0 the world clear, 1 black). */
	float GetShade() const;

	/** The crows to draw now on a screen Aspect wide (its width over its height), back to front: the smallest first. */
	void GetCrows(float Aspect, TArray<FRoostVeilCrow>& OutCrows) const;

	/** Where a point at Angle (radians) from the middle meets the screen's edge, times Scale (1 on the edge, more off it). */
	static FVector2f EdgePoint(float Angle, float Aspect, float Scale);

private:
	/** A number in [0, 1) for a crow (Index) and one of its choices (Slot), the same for the same seed. */
	float Rand(int32 Index, int32 Slot) const;

	/** A gathering crow at Time seconds into the gather: where, which way it flies, and whether it has come in yet. */
	bool GatherCrow(int32 Index, float Aspect, float Time, FRoostVeilCrow& Out) const;
	bool CoverCrow(int32 Index, float Aspect, FRoostVeilCrow& Out) const;
	bool HoldCrow(int32 Index, float Aspect, FRoostVeilCrow& Out) const;
	bool RevealCrow(int32 Index, float Aspect, FRoostVeilCrow& Out) const;

	/** Its wings at Time for a beat of Rate a second from Phase (0 to 1): raised, level, lowered, level. */
	static int32 WingFrame(float Time, float Rate, float Offset);

	ERoostVeilKind Kind = ERoostVeilKind::Travel;
	ERoostVeilPhase Phase = ERoostVeilPhase::Idle;
	float Clock = 0.f;
	/** Seconds since it started: the wings beat on across phases. */
	float Lifetime = 0.f;
	float GatherLength = 0.f;
	float CoverLength = 0.f;
	float RevealLength = RevealSeconds;
	/** The shade as the reveal began (it falls from there). */
	float RevealFrom = 1.f;
	int32 Seed = 1;
};
