#pragma once

#include "CoreMinimal.h"

/**
 * When a creature is stuck, and the glide that frees it, as plain rules: ACreatureBase feeds it every frame it makes its
 * way somewhere (CreatureBaseUnstick.cpp), and the tests feed it frames of their own. Three ways of being stuck:
 * - blocked (its own sense of it, ACreatureBase::IsStuck: barely moving for its speed, or a slime's hops cut short) for
 *   BlockedSeconds: its steering feels a wall ahead and goes round it (FCreatureSteerPlanner::NoteStuck);
 * - wedged: pushed on by its movement and blocked, against something solid, without getting ProgressRadius anywhere for
 *   WedgedSeconds (a fence's corner, a rail it can't step);
 * - hung: in the air without moving for HungSeconds (balanced on a prop's edge, caught between rocks).
 * Wedged or hung, it glides to the nearest free spot with ground under it (FCreatureSteerProbe::FindFreeSpot) as the
 * player's unstick does: a short glide, not a pop, and the fall takes it the last little way.
 */
struct AI_LOOTER_SHOOTER_API FCreatureUnstick
{
	enum class EAction : uint8
	{
		None,
		/** Blocked a while: go round what's ahead. */
		GoRound,
		/** Wedged against something: glide free. */
		NudgeWedged,
		/** Hung in the air: glide free. */
		NudgeHung
	};

	static constexpr float BlockedSeconds = 0.5f;
	/** Getting this far (cm, at size 1) from where it last got anywhere counts as progress. */
	static constexpr float ProgressRadius = 15.f;
	static constexpr float WedgedSeconds = 1.5f;
	/** Its movement was stopped by something solid this lately (s): it's against it. */
	static constexpr float TouchSeconds = 0.5f;
	static constexpr float HungSeconds = 0.5f;
	/** No free spot near: it looks again after this long. */
	static constexpr float RetrySeconds = 1.f;
	/** The glide to a free spot takes this long, as the player's (LooterTraversal::UnstickSeconds). */
	static constexpr float GlideSeconds = 0.25f;

	/** One frame of a creature making its way. */
	struct FFrame
	{
		FVector Here = FVector::ZeroVector;
		/** Its own sense of being blocked (ACreatureBase::IsStuck). */
		bool bBlocked = false;
		/** Its movement is pushing it along its steering (not standing still for something else: a phase-step's fade, a boss's moment). */
		bool bPressing = false;
		/** In the air. */
		bool bFalling = false;
		float SizeScale = 1.f;
		float DeltaSeconds = 0.f;
	};

	/** What to do this frame. Nothing while it glides. */
	EAction Update(const FFrame& Frame);

	/** Its movement was stopped by something solid (not a creature or a player): ACreatureBase::MoveBlockedBy. */
	void NoteBlockedMove() { SinceBlockedMove = 0.f; }

	/** Starts afresh where it stands (a new state, the end of a glide); a glide under way is dropped. */
	void Reset(const FVector& Here);

	/** No free spot near: it waits RetrySeconds before it looks again. */
	void NudgeFailed();

	void StartGlide(const FVector& From, const FVector& To);
	bool IsGliding() const { return bGliding; }

	/** Moves the glide on by DeltaSeconds: where the body is now, eased in and out; bOutDone at its end (then it starts afresh there). */
	FVector StepGlide(float DeltaSeconds, bool& bOutDone);

private:
	FVector Anchor = FVector::ZeroVector;
	bool bAnchored = false;
	float StillTime = 0.f;
	float HungTime = 0.f;
	float BlockedTime = 0.f;
	float RetryIn = 0.f;
	float SinceBlockedMove = 1000.f;
	bool bGliding = false;
	FVector GlideFrom = FVector::ZeroVector;
	FVector GlideTo = FVector::ZeroVector;
	float GlideTime = 0.f;
};
