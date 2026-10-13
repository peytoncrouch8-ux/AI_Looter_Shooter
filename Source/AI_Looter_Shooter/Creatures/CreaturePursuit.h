#pragma once

#include "CoreMinimal.h"

/** Where a hunt stands after one of the creature's looks (FCreaturePursuit::Look). */
enum class EPursuitStep : uint8
{
	/** After its target: in sight, or out of it for a moment (it heard where they went). */
	Hunt,
	/** Lost sight of them: going to where it last knew them to be. */
	Track,
	/** At that spot, nosing round it. */
	Sniff,
	/** It can't get to them (off its ground, up out of its reach): waiting as near as it gets, pacing and watching. */
	Prowl,
	/** The hunt is over (GetEnd says why): it walks home. */
	GiveUp,
};

/** Why a hunt ended. */
enum class EPursuitEnd : uint8
{
	None,
	/** Dead, gone from the level, or farther than it ever follows (a fast travel). */
	Gone,
	/** In a safe zone that's on (Delia's salt line). */
	Sheltered,
	/** A scene holds the player. */
	Scene,
	/** Unseen and far for long, its search done. */
	Lost,
	/** Off its ground (its group's level of ground) past its patience. */
	OffGround,
	/** Up where it can't reach them, or getting no nearer, past its patience. */
	OutOfReach,
};

/** The reason's name, for the log and the tests. */
AI_LOOTER_SHOOTER_API const TCHAR* LexToString(EPursuitEnd End);

/**
 * How a creature keeps after the player once it hunts them, as plain rules (the user's call, 2026-10-12: creatures follow
 * until there's a real reason to stop). ACreatureBase hands it a look at its target at its senses' pace (five times a
 * second); the step it gets back is what it does until the next look:
 *  - Hunt: after them. Out of sight it keeps coming for UnseenGrace (it heard where they went), then
 *  - Track: goes to where it last knew them to be, and
 *  - Sniff: noses round there. Seen again at any point, the hunt is on again. Their scent still near (within its lock-on
 *    radius), it tracks them again once its sniff is done; unseen and farther than that for LostSeconds, its sniff done,
 *    it has lost them, and unseen for MaxUnseenSeconds it has lost them wherever they are.
 *  - Prowl: it can't get to them: they're off its ground, or near but above or below its reach (OverheadSeconds), or it has
 *    made its way toward them without getting nearer while they stood (StuckSeconds). It waits there watching until they
 *    come back within reach, or its patience runs out (ProwlSeconds a hunt; reaching them or a hit fills it again).
 *  - GiveUp: the hunt is over, for one of the real reasons (EPursuitEnd). Seen far off, alive, on its ground, it never is.
 * Bosses keep their fights' own rules and don't use it. All distances are cm at size 1, grown with the creature's size.
 */
struct AI_LOOTER_SHOOTER_API FCreaturePursuit
{
	/** The numbers it goes by (UEncounterSettings' "Pursuit" gives the tunable ones: MakePursuitRules). */
	struct FRules
	{
		float UnseenGrace = 2.f;
		/** It goes to the last known spot for this long (s) at most before it sniffs where it got to. */
		float TrackSeconds = 8.f;
		/** This near the spot (flat) it's there. */
		float ArriveRadius = 150.f;
		float SniffSeconds = 5.f;
		float LostSeconds = 13.f;
		float MaxUnseenSeconds = 30.f;
		float ProwlSeconds = 10.f;
		float StuckSeconds = 10.f;
		float OverheadSeconds = 4.f;
		/** Getting this much nearer (3D) counts as getting somewhere. */
		float ProgressStep = 100.f;
		/** The target moving this far starts its measure of getting somewhere afresh (they're not standing still). */
		float TargetShift = 300.f;
		/** Out of reach, it tries again once the target has moved this far from where it gave up reaching them. */
		float ResumeShift = 600.f;
	};

	/** What one look found. */
	struct FLook
	{
		/** Seconds since its last look. */
		float Interval = 0.f;
		/** Where it stands and where its target stands (world). */
		FVector Here = FVector::ZeroVector;
		FVector Prey = FVector::ZeroVector;
		float SizeScale = 1.f;
		/** Dead, gone from the level, or farther than it ever follows. */
		bool bGone = false;
		/** In a safe zone that's on. */
		bool bSheltered = false;
		/** A scene holds the player. */
		bool bInScene = false;
		/** Off its hunting ground. */
		bool bOffGround = false;
		/** In its sight, or near enough to be heard. */
		bool bSeen = false;
		/** Farther than its lock-on radius: past its nose. */
		bool bBeyondSenses = false;
		/** How near (flat) it closes in: a creature that hangs back (the Spitter, the Bell-Ringer) tracks no nearer either. */
		float HoldDistance = 0.f;
		/** Farther off (flat) than that: it's making its way toward them. */
		bool bFarOff = false;
		/** Breaking off for a moment (its pack gone, hurt): running, not hunting. */
		bool bBreakingOff = false;
		/** Near (within its hold or a few meters), but above or below its reach: up on a rock or a roof over it. */
		bool bOverhead = false;
	};

	/** A new hunt after a target standing at Prey. */
	void Begin(const FVector& Prey);

	/** One look at its target: the step it's at now. Once given up, it stays given up until the next Begin. */
	EPursuitStep Look(const FLook& InLook, const FRules& Rules);

	/** Its target was heard of from elsewhere (a pack's call): it knows where they are, and a search turns into the hunt. */
	void Reacquire(const FVector& Prey);

	/** Its target hit it from From: as Reacquire, and its patience fills again (shot at, it watches on). */
	void NoteHit(const FVector& From);

	/** It reached them (an attack began): its patience fills again, and its measure of getting somewhere starts afresh. */
	void NoteReached();

	/** Ends the hunt now (its target went away between looks). */
	void Stop(EPursuitEnd Why);

	EPursuitStep GetStep() const { return Step; }
	EPursuitEnd GetEnd() const { return End; }
	/** Where it last knew its target to be: where it tracks to and sniffs round. */
	const FVector& GetLastKnown() const { return LastKnown; }
	/** Seconds since it last saw (or heard) them. */
	float GetUnseen() const { return Unseen; }
	/** Seconds into its step, counted by its looks. */
	float GetStepTime() const { return StepTime; }
	/** Seconds of its patience spent waiting this hunt. */
	float GetPatienceUsed() const { return Patience; }
	/** Waiting at its ground's edge (rather than under a target out of its reach). */
	bool IsProwlingOffGround() const { return Step == EPursuitStep::Prowl && bProwlOffGround; }

	/**
	 * Given up on a target out of its reach (PawnId, standing at Where): it doesn't take them up again by sight until Until
	 * (world seconds), unless they've moved from there by ResumeShift. A hit or a pack's call still does.
	 */
	void Spurn(uint32 PawnId, const FVector& Where, double Until);
	bool IsSpurned(uint32 PawnId, const FVector& Where, double Now, float ResumeShift) const;

	/** The creature's own walking in a step (a sniff's spot, the prowl's anchor and pacing), cleared as each step starts. */
	struct FWalk
	{
		FVector Goal = FVector::ZeroVector;
		bool bHasGoal = false;
		float Clock = 0.f;
		FVector Anchor = FVector::ZeroVector;
		bool bAnchored = false;
		/** Seconds it has walked up toward its edge, and how far off its target was once it stopped there. */
		float ApproachTime = 0.f;
		float AnchorReach = 0.f;
		bool bPacing = false;
		float PaceSign = 1.f;
	};
	FWalk Walk;

private:
	void SetStep(EPursuitStep NewStep);
	EPursuitStep Finish(EPursuitEnd Why);
	/** Waiting where it can't get to them: its patience spent by Interval, and the hunt over once it's gone. */
	EPursuitStep Wait(float Interval, const FRules& Rules, EPursuitEnd Why);
	void ResetProgress(float Reach, const FVector& Prey);

	EPursuitStep Step = EPursuitStep::GiveUp;
	EPursuitEnd End = EPursuitEnd::None;
	FVector LastKnown = FVector::ZeroVector;
	float Unseen = 0.f;
	float StepTime = 0.f;
	float Patience = 0.f;
	/** Prowling at its ground's edge, or (false) under a target out of its reach, who stood at ProwlFrom then. */
	bool bProwlOffGround = false;
	FVector ProwlFrom = FVector::ZeroVector;
	/** Getting somewhere: its nearest (3D) since the target stood at ProgressPrey, and how long since it got nearer. */
	float BestReach = TNumericLimits<float>::Max();
	FVector ProgressPrey = FVector::ZeroVector;
	float Stalled = 0.f;
	/** How long its target has stood near but above or below its reach. */
	float Overhead = 0.f;

	uint32 SpurnedId = 0;
	FVector SpurnedAt = FVector::ZeroVector;
	double SpurnedUntil = -1.0;
};
