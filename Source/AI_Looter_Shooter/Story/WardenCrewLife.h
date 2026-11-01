#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/** What a story person takes in this frame, for where they look (all world space). */
struct FWardenLookSense
{
	/** Their eyes, and the way their body faces (flat). */
	FVector Eye = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector;
	/** The player's eyes, when there's a player. */
	bool bHasPlayer = false;
	FVector Player = FVector::ZeroVector;
	/** Whom they're talking to while their lines play: looked in the eye, nothing else. */
	bool bHasListener = false;
	FVector Listener = FVector::ZeroVector;
	/** What their work keeps them watching (the sky ahead of the wheel, the lantern's flame, the engine's works, a target). */
	bool bHasFocus = false;
	FVector Focus = FVector::ZeroVector;
	/** How firmly the focus holds their eyes (0-1) when nothing else does. */
	float FocusWeight = 0.6f;
	/** The focus is all they see (Delia on a target): no glances, no turn to the player. */
	bool bFocusOnly = false;
	/** The player nearer than this (cm) and somewhere in front of them is looked at, a while at a time. */
	float NoticeRadius = 500.f;
};

/**
 * What keeps a story person alive between their lines, apart from the world (tests run it on plain numbers): where they
 * look (the one they're talking to; the player coming near, a few seconds at a time, then away and back; glances round the
 * deck or the yard; their work), eased as a head turns; and their brace when the ship lurches (a quick crouch held a while,
 * and a sway that rings out).
 */
struct AI_LOOTER_SHOOTER_API FWardenLife
{
	explicit FWardenLife(int32 Seed = 0);

	/** Moves the eyes on by DeltaSeconds toward what Sense makes worth a look. */
	void UpdateLook(float DeltaSeconds, const FWardenLookSense& Sense);

	/** Where the head turns now and how much (WardenCrewPose's LookAt and LookWeight). */
	FVector GetLookAt() const { return LookEye + LookWay * LookDistance; }
	float GetLookWeight() const { return LookWeight; }
	/** Their eyes are on the player now (a turn or a nod at them). */
	bool IsWatchingPlayer() const { return bWatchingPlayer; }

	/** The ship lurched: brace for Seconds (held, then eased off), the body swaying as it rings out. */
	void Brace(float Seconds);
	void UpdateBrace(float DeltaSeconds);
	/** How braced (0-1, eased), and the sway's roll to the right and pitch forward now (degrees). */
	float GetBrace() const { return BraceAlpha; }
	float GetSwayRoll() const { return SwayRoll; }
	float GetSwayPitch() const { return SwayPitch; }
	bool IsBracing() const { return BraceLeft > 0.f; }

	/** Player is near enough (Radius) to Eye and somewhere in front of Forward (within NoticeHalfAngle either side). */
	static bool Notices(const FVector& Eye, const FVector& Forward, const FVector& Player, float Radius);

	/** Front of a person that notices someone coming: either side of straight ahead (degrees). */
	static constexpr float NoticeHalfAngle = 110.f;
	/** A beat before the head comes round to someone new (s). */
	static constexpr float NoticeDelay = 0.35f;
	/** The player is watched this long at a time (s, a roll between), then the eyes go elsewhere this long. */
	static constexpr float WatchMin = 3.5f;
	static constexpr float WatchMax = 6.5f;
	static constexpr float AwayMin = 1.2f;
	static constexpr float AwayMax = 2.6f;
	/** Glances: one every this long (s, a roll between), held this long, this far off the body's way (degrees). */
	static constexpr float GlanceGapMin = 3.5f;
	static constexpr float GlanceGapMax = 8.f;
	static constexpr float GlanceHoldMin = 0.8f;
	static constexpr float GlanceHoldMax = 2.f;
	static constexpr float GlanceYawMin = 20.f;
	static constexpr float GlanceYawMax = 55.f;
	/** The head's turn: how quickly it closes on its target (1/s) and how fast at most (degrees a second). */
	static constexpr float TurnRate = 7.f;
	static constexpr float MaxTurnSpeed = 300.f;
	/** A brace comes on this fast and goes off this fast (shares a second); its sway rings out at this rate (1/s). */
	static constexpr float BraceOnRate = 7.f;
	static constexpr float BraceOffRate = 1.8f;
	static constexpr float SwayDecay = 1.6f;
	static constexpr float SwayRollDegrees = 7.f;
	static constexpr float SwayPitchDegrees = 3.f;

private:
	FRandomStream Random;

	/** The eyes' way (unit, world), the head's spot as last seen, how far off what they look at is, how much they look. */
	FVector LookWay = FVector::ForwardVector;
	FVector LookEye = FVector::ZeroVector;
	float LookDistance = 300.f;
	float LookWeight = 0.f;
	bool bLookStarted = false;

	/** The player: watched how long, looked away for how long more, and the beat before the head comes round. */
	bool bWatchingPlayer = false;
	float WatchedFor = 0.f;
	float WatchHold = 5.f;
	float AwayLeft = 0.f;
	float NoticeLeft = 0.f;
	bool bPlayerNear = false;

	/** The glance under way (its way off the body's, in degrees) and the wait for the next. */
	float GlanceLeft = 0.f;
	float GlanceYaw = 0.f;
	float GlancePitch = 0.f;
	float NextGlance = 4.f;

	/** The brace: held how much longer, how much now, and the sway's clock and side. */
	float BraceLeft = 0.f;
	float BraceAlpha = 0.f;
	float SwayClock = 100.f;
	float SwaySide = 1.f;
	float SwayRoll = 0.f;
	float SwayPitch = 0.f;
};
