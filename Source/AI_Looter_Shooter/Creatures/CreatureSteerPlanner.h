#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"
#include "Templates/Function.h"

/**
 * A creature's way round obstacles from looks ahead alone (no navmesh), like a bug that keeps a feeler on the wall. It
 * heads straight for its goal while the way looks clear. Meeting something, it picks a side: the way along it that heads
 * more toward the goal, a quick look either side when it met it head-on, and the side it took last for a few seconds (so
 * it doesn't flip-flop along a row of fences). Then it follows the wall: each update it first tries turning back in toward
 * it (which rounds a fence's end), then straight on, then away from it as little as it must. It leaves the wall once the
 * way to the goal is clear and it's nearer the goal than where it met the wall, or has rounded the wall's end with the goal
 * on its open side; a wall followed for MaxFollowDistance is tried the other way round.
 *
 * What the looks miss (a rail under them, a prop's odd collision) is remembered where the body bumped into it or pressed
 * without getting anywhere (felt walls, a few seconds each), and the looks see those too, so it goes round them instead
 * of walking back into them.
 *
 * Plain logic: the creature does the looking (FCreatureSteerProbe, ACreatureBase::ChooseDirection), and the tests a flat
 * world of their own. All vectors are flat (Z ignored); distances are cm at the creature's size.
 */
struct AI_LOOTER_SHOOTER_API FCreatureSteerPlanner
{
	/** What one look along a direction found. */
	struct FLookResult
	{
		/** Nothing in the way as far as it looked, and ground all along. */
		bool bClear = true;
		/** How far it can go before it meets something (cm); as far as it looked when clear. */
		float FreeDistance = 0.f;
		/** The face it met, flat and pointing back at the creature; zero for a drop or nothing. */
		FVector WallNormal = FVector::ZeroVector;
		/** It stopped at a drop, not at something solid. */
		bool bLedge = false;

		static FLookResult Clear(float Distance)
		{
			FLookResult Result;
			Result.FreeDistance = FMath::Max(Distance, 0.f);
			return Result;
		}

		/** Blocked FreeDistance on, by a face whose normal is Normal (flattened here). */
		static FLookResult Blocked(float Distance, const FVector& Normal, bool bAtLedge = false)
		{
			FLookResult Result;
			Result.bClear = false;
			Result.FreeDistance = FMath::Max(Distance, 0.f);
			Result.WallNormal = FVector(Normal.X, Normal.Y, 0.0).GetSafeNormal();
			Result.bLedge = bAtLedge;
			return Result;
		}
	};

	/** Looks along a direction (flat, unit) for up to a distance (cm). */
	using FLookFunction = TFunctionRef<FLookResult(const FVector& Direction, float Distance)>;

	/** One update's question. */
	struct FRequest
	{
		/** Where it stands, and the goal it's after (a moved goal or a jump starts its way afresh). */
		FVector Here = FVector::ZeroVector;
		FVector Goal = FVector::ZeroVector;
		/** Toward the goal (flat, unit), already bent round its pack. */
		FVector Desired = FVector::ForwardVector;
		float GoalDistance = 0.f;
		/** How far a look reaches, and how wide the body it sweeps is (its radius). */
		float LookLength = 220.f;
		float LookRadius = 50.f;
		/** Its speed (cm/s): how far it has followed a wall, and how far it can have moved since the last update. */
		float Speed = 400.f;
		/** Its size: the distances the planner keeps grow with it. */
		float SizeScale = 1.f;
		/** Up close to a player: two more looks to pick the side round a wall met head-on, and the whole fan. */
		bool bNear = true;
	};

	// --- Tuning (degrees, seconds; distances in cm at size 1) ---

	/** Following a wall, it first tries turning this far back in toward it: it hugs the wall and rounds its end. */
	static constexpr float HugDegrees = 25.f;
	/** The side it took round a wall is kept this long for the next one, unless a wall clearly asks for the other. */
	static constexpr float SideHoldSeconds = 4.f;
	static constexpr float SideMemoryBias = 0.6f;
	/** Sides scoring closer than this met the wall head-on: up close it looks either side before it picks. */
	static constexpr float HeadOnScore = 0.35f;
	/** Turned back in toward the wall this much in all (degrees): it has rounded an end, and may leave with the goal on its open side. */
	static constexpr float RoundedEndTurn = -45.f;
	/** It leaves a wall once this much nearer its goal than where it met it. */
	static constexpr float LeaveMargin = 50.f;
	/** A wall followed this far is tried the other way round (a cliff it can't get round this way). */
	static constexpr float MaxFollowDistance = 4500.f;
	/** Nothing in the fan clear: it takes the longest free way if it's at least this share of a look. */
	static constexpr float MinFreeShare = 0.05f;
	/** Felt walls: how many, how long each is kept, and how near (cm) a new touch is to an old one to count as the same. */
	static constexpr int32 MaxFeltWalls = 6;
	static constexpr float FeltWallSeconds = 6.f;
	static constexpr float FeltMergeDistance = 40.f;
	/** A wall felt by pressing into it without getting anywhere reaches this many body radii either side. */
	static constexpr float StuckWallReach = 2.4f;
	/** Stuck this many times within StuckWindowSeconds while following a wall: it turns back and goes round the other way. */
	static constexpr int32 StucksToTurnBack = 3;
	static constexpr float StuckWindowSeconds = 5.f;
	/** A touch is checked against the looks at most this often (s): each check is a look. */
	static constexpr float ContactCheckSeconds = 0.2f;
	/** Moved farther than its speed allows plus this, or its goal moved this far: its way starts afresh. */
	static constexpr float JumpDistance = 150.f;
	static constexpr float NewGoalDistance = 300.f;
	/** Looks a wall-following update takes at most: up close, and far away. */
	static constexpr int32 NearFanLooks = 8;
	static constexpr int32 FarFanLooks = 4;

	/** The way to go now (flat, unit), from Look's looks. Called every steering update. */
	FVector Plan(const FRequest& Request, FLookFunction Look);

	/** Its timers (side memory, felt walls, the wall followed), every frame it moves. */
	void Advance(float DeltaSeconds);

	/**
	 * The body bumped into something going along Going, which its looks don't see (the caller checked): it's kept as a felt
	 * wall through Point facing Normal, HalfLength either side. True when it should plan again at once.
	 */
	bool NoteContact(const FVector& Point, const FVector& Normal, const FVector& Going, float HalfLength, float SizeScale);

	/** Whether a touch may be checked now (the checks are rationed); starts the next wait when it may. */
	bool TakeContactCheck();

	/**
	 * It pressed on along Going without getting anywhere: a felt wall ahead of it (BodyRadius out), and it follows along
	 * it; stuck again and again while following, it turns back and goes round the other way.
	 */
	void NoteStuck(const FRequest& Request, const FVector& Going, float BodyRadius);

	/** Starts afresh (a new state, a jump): no wall followed; the side it took is kept, and the felt walls unless asked. */
	void Forget(bool bForgetFeltWalls);

	/** The side it goes round by before it has had to pick one (+1 or -1): creatures of a pack split round things. */
	void SeedSide(float InSide);

	bool HasPlanned() const { return bHasPlanned; }
	bool IsFollowingWall() const { return bFollowing; }
	/** +1: it goes round turning by positive angles (the wall on its other hand); -1 the other way. */
	float GetSide() const { return Side; }
	int32 NumFeltWalls() const { return FeltWalls.Num(); }

	/** Counts since it began, for the tests and debugging: side changes, turns of over 120 degrees in one update, walls followed. */
	int32 GetSideFlips() const { return SideFlips; }
	int32 GetReversals() const { return Reversals; }
	int32 GetWallFollows() const { return WallFollows; }

private:
	struct FFeltWall
	{
		FVector Center = FVector::ZeroVector;
		FVector Normal = FVector::ForwardVector;
		float HalfLength = 0.f;
		float SecondsLeft = 0.f;
	};

	/** A look, with the felt walls in it. */
	FLookResult LookWithFelt(FLookFunction Look, const FRequest& Request, const FVector& Direction, float Distance) const;
	bool ShouldLeave(const FRequest& Request) const;
	/** Starts following the wall GoalLook met; with Look, it may look either side first (null: it picks without looking). */
	void BeginFollowing(const FRequest& Request, const FLookFunction* Look, const FLookResult& GoalLook);
	/** Along the wall: back in toward it, straight on, then away from it as little as it must. */
	FVector FollowFan(const FRequest& Request, FLookFunction Look, bool bEntering);
	FVector Commit(const FVector& Direction);
	void SetSide(float NewSide);
	void AddFeltWall(const FVector& Center, const FVector& Normal, float HalfLength);

	bool bFollowing = false;
	float Side = 1.f;
	float SideMemory = 0.f;
	/** Its way along the wall it follows. */
	FVector Heading = FVector::ForwardVector;
	float FollowStartDistance = 0.f;
	float FollowTime = 0.f;
	/** Degrees turned since it began following: away from the wall positive, back in toward it negative. */
	float Turned = 0.f;
	FVector LastDirection = FVector::ZeroVector;
	TArray<FFeltWall, TInlineAllocator<MaxFeltWalls>> FeltWalls;
	TArray<float, TInlineAllocator<StucksToTurnBack>> RecentStucks;
	float Clock = 0.f;
	float ContactWait = 0.f;
	bool bHasPlanned = false;
	FVector LastHere = FVector::ZeroVector;
	FVector LastGoal = FVector::ZeroVector;
	float SinceLastPlan = 0.f;
	int32 SideFlips = 0;
	int32 Reversals = 0;
	int32 WallFollows = 0;
};
