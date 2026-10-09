#include "Creatures/CreatureSteerPlanner.h"

// FCreatureSteerPlanner: a creature's way round obstacles (see the header). Tuned in a flat dry run of fence fields, a
// U-shaped pen, a gate, a post field, a 60 m cliff and rails the looks can't see, for every creature's size and speed:
// with the wall-following below none of them turned back on itself more than twice, where the old fan-and-detour
// steering ping-ponged along long fences.

namespace
{
	/** Flat and unit (zero when there's nothing flat about it). */
	FVector FlatUnit(const FVector& Vector)
	{
		return FVector(Vector.X, Vector.Y, 0.0).GetSafeNormal();
	}

	/** Flat, any length. */
	FVector Planar(const FVector& Vector)
	{
		return FVector(Vector.X, Vector.Y, 0.0);
	}

	/** Turned about the up axis (positive angles one way round, negative the other). */
	FVector Turn(const FVector& Vector, float Degrees)
	{
		return Vector.RotateAngleAxis(Degrees, FVector::UpVector);
	}

	/** The fan along a wall: straight on, then turning away from it (degrees); HugDegrees back in toward it comes first. */
	constexpr float FanDegrees[] = { 0.f, 30.f, 60.f, 90.f, 120.f, 150.f, 180.f };
}

FVector FCreatureSteerPlanner::Plan(const FRequest& Request, FLookFunction Look)
{
	// Put somewhere else (a phase-step, a reset home) or after a goal somewhere else: the wall it was following no longer
	// applies. A jump also leaves its felt walls behind.
	if (bHasPlanned)
	{
		const float Allowed = Request.Speed * SinceLastPlan * 1.5f + JumpDistance * Request.SizeScale;
		const bool bJumped = FVector::DistSquared2D(Request.Here, LastHere) > FMath::Square(Allowed);
		if (bJumped || FVector::DistSquared2D(Request.Goal, LastGoal) > FMath::Square(NewGoalDistance * Request.SizeScale))
		{
			Forget(bJumped);
		}
	}
	bHasPlanned = true;
	LastHere = Request.Here;
	LastGoal = Request.Goal;
	SinceLastPlan = 0.f;

	const FVector Desired = FlatUnit(Request.Desired);
	if (Desired.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}
	// Toward the goal it looks no farther than the goal: a wall behind a player isn't in its way.
	const float Reach = FMath::Min(Request.LookLength, Request.GoalDistance);
	const FLookResult GoalLook = LookWithFelt(Look, Request, Desired, Reach);
	if (bFollowing)
	{
		if (GoalLook.bClear && ShouldLeave(Request))
		{
			bFollowing = false;
			return Commit(Desired);
		}
		if (FollowTime * Request.Speed > MaxFollowDistance * Request.SizeScale)
		{
			// A long way along it and still not round: the other way, then.
			SetSide(-Side);
			SideMemory = SideHoldSeconds;
			FollowTime = 0.f;
			FollowStartDistance = Request.GoalDistance;
			Heading = -Heading;
			Turned = 0.f;
		}
		return Commit(FollowFan(Request, Look, false));
	}
	if (GoalLook.bClear)
	{
		return Commit(Desired);
	}
	BeginFollowing(Request, Request.bNear ? &Look : nullptr, GoalLook);
	return Commit(FollowFan(Request, Look, true));
}

void FCreatureSteerPlanner::Advance(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	SinceLastPlan += DeltaSeconds;
	ContactWait = FMath::Max(0.f, ContactWait - DeltaSeconds);
	SideMemory = FMath::Max(0.f, SideMemory - DeltaSeconds);
	if (bFollowing)
	{
		FollowTime += DeltaSeconds;
	}
	for (int32 Index = FeltWalls.Num() - 1; Index >= 0; --Index)
	{
		FeltWalls[Index].SecondsLeft -= DeltaSeconds;
		if (FeltWalls[Index].SecondsLeft <= 0.f)
		{
			FeltWalls.RemoveAt(Index);
		}
	}
}

bool FCreatureSteerPlanner::NoteContact(const FVector& Point, const FVector& Normal, const FVector& Going, float HalfLength, float SizeScale)
{
	const FVector Facing = FlatUnit(Normal);
	const FVector Way = FlatUnit(Going);
	// Brushing along something isn't news; only pressing into it.
	if (Facing.IsNearlyZero() || Way.IsNearlyZero() || FVector::DotProduct(Way, Facing) > -0.5)
	{
		return false;
	}
	for (FFeltWall& Wall : FeltWalls)
	{
		if (FVector::DistSquared2D(Wall.Center, Point) < FMath::Square(FeltMergeDistance * SizeScale)
			&& FVector::DotProduct(Wall.Normal, Facing) > 0.9)
		{
			Wall.SecondsLeft = FeltWallSeconds;
			return false;
		}
	}
	AddFeltWall(Point, Facing, HalfLength);
	return true;
}

bool FCreatureSteerPlanner::TakeContactCheck()
{
	if (ContactWait > 0.f)
	{
		return false;
	}
	ContactWait = ContactCheckSeconds;
	return true;
}

void FCreatureSteerPlanner::NoteStuck(const FRequest& Request, const FVector& Going, float BodyRadius)
{
	FVector Way = FlatUnit(Going);
	if (Way.IsNearlyZero())
	{
		Way = FlatUnit(Request.Desired);
	}
	if (Way.IsNearlyZero())
	{
		return;
	}
	// Whatever stops it is right ahead, whether the looks see it or not.
	AddFeltWall(Request.Here + Way * BodyRadius, -Way, BodyRadius * StuckWallReach);
	RecentStucks.RemoveAll([this](float When) { return Clock - When >= StuckWindowSeconds; });
	if (RecentStucks.Num() >= StucksToTurnBack)
	{
		RecentStucks.RemoveAt(0);
	}
	RecentStucks.Add(Clock);
	if (!bFollowing)
	{
		// Along the felt wall, picking its side without looking (it's stuck: no time for a careful look round).
		FRequest Pressed = Request;
		Pressed.Desired = Way;
		BeginFollowing(Pressed, nullptr, FLookResult::Blocked(0.f, -Way));
	}
	else if (RecentStucks.Num() >= StucksToTurnBack)
	{
		// Stuck again and again going round this way: a corner it can't see out of. Back the other way.
		SetSide(-Side);
		SideMemory = SideHoldSeconds;
		Heading = -Heading;
		Turned = 0.f;
		RecentStucks.Reset();
	}
}

void FCreatureSteerPlanner::Forget(bool bForgetFeltWalls)
{
	bFollowing = false;
	Turned = 0.f;
	FollowTime = 0.f;
	LastDirection = FVector::ZeroVector;
	RecentStucks.Reset();
	if (bForgetFeltWalls)
	{
		FeltWalls.Reset();
	}
}

void FCreatureSteerPlanner::SeedSide(float InSide)
{
	if (!bHasPlanned)
	{
		Side = InSide >= 0.f ? 1.f : -1.f;
	}
}

FCreatureSteerPlanner::FLookResult FCreatureSteerPlanner::LookWithFelt(FLookFunction Look, const FRequest& Request,
	const FVector& Direction, float Distance) const
{
	FLookResult Result = Look(Direction, Distance);
	// A felt wall is a line through where it was felt: the look stops where its body (LookRadius wide) would reach it.
	for (const FFeltWall& Wall : FeltWalls)
	{
		const double Into = FVector::DotProduct(Direction, Wall.Normal);
		if (Into >= -1.e-3)
		{
			continue;
		}
		const double Gap = FVector::DotProduct(Planar(Request.Here - Wall.Center), Wall.Normal) - Request.LookRadius;
		const double At = FMath::Max(0.0, Gap / -Into);
		const double Limit = Result.bClear ? Distance : FMath::Min<double>(Distance, Result.FreeDistance);
		if (At > Limit)
		{
			continue;
		}
		const FVector Reached = Request.Here + Direction * At;
		const FVector Along(-Wall.Normal.Y, Wall.Normal.X, 0.0);
		if (FMath::Abs(FVector::DotProduct(Planar(Reached - Wall.Center), Along)) > Wall.HalfLength + Request.LookRadius)
		{
			continue;
		}
		Result = FLookResult::Blocked(static_cast<float>(At), Wall.Normal);
	}
	return Result;
}

bool FCreatureSteerPlanner::ShouldLeave(const FRequest& Request) const
{
	// Nearer the goal than where it met the wall (it can't go round in circles), or round the wall's end with the goal on
	// its open side. Turning back in toward the wall is how it rounds an end; turning away (a pen's corner) doesn't count.
	const FVector Away = Turn(Heading, Side * 90.f);
	return Request.GoalDistance < FollowStartDistance - LeaveMargin * Request.SizeScale
		|| (FVector::DotProduct(FlatUnit(Request.Desired), Away) >= 0.0 && Turned <= RoundedEndTurn);
}

void FCreatureSteerPlanner::BeginFollowing(const FRequest& Request, const FLookFunction* Look, const FLookResult& GoalLook)
{
	const FVector Desired = FlatUnit(Request.Desired);
	FVector Normal = FlatUnit(GoalLook.WallNormal);
	// A drop, or a touch that doesn't face it: as if it met the wall head-on.
	if (Normal.IsNearlyZero() || FVector::DotProduct(Normal, Desired) > -0.05)
	{
		Normal = -Desired;
	}
	// Each side's way along the wall, scored by how much it heads toward the goal, and the side it took last while it
	// remembers it.
	const float Sides[2] = { 1.f, -1.f };
	float Score[2];
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Score[Index] = static_cast<float>(FVector::DotProduct(Turn(Normal, -Sides[Index] * 90.f), Desired));
		if (SideMemory > 0.f && Sides[Index] == Side)
		{
			Score[Index] += SideMemoryBias;
		}
	}
	// Met head-on: up close it looks either side, and goes the way that's clear or open farther.
	if (Look && FMath::Abs(Score[0] - Score[1]) < HeadOnScore)
	{
		for (int32 Index = 0; Index < 2; ++Index)
		{
			const FLookResult Aside = LookWithFelt(*Look, Request, Turn(Desired, Sides[Index] * 45.f), Request.LookLength);
			Score[Index] += (Aside.bClear ? 0.5f : 0.f) + 0.5f * Aside.FreeDistance / FMath::Max(Request.LookLength, 1.f);
		}
	}
	const float Chosen = FMath::Abs(Score[0] - Score[1]) < 1.e-3f ? Side : (Score[0] > Score[1] ? 1.f : -1.f);
	SetSide(Chosen);
	SideMemory = SideHoldSeconds;
	Heading = Desired;
	bFollowing = true;
	++WallFollows;
	FollowStartDistance = Request.GoalDistance;
	FollowTime = 0.f;
	Turned = 0.f;
}

FVector FCreatureSteerPlanner::FollowFan(const FRequest& Request, FLookFunction Look, bool bEntering)
{
	// Back in toward the wall first (it hugs the wall and rounds its end), then straight on, then away from it as little as
	// it must. Just met, its way straight on is the way that was blocked, and turning in would be the other side.
	TArray<float, TInlineAllocator<8>> Angles;
	if (!bEntering)
	{
		Angles.Add(-HugDegrees);
	}
	for (int32 Index = bEntering ? 1 : 0; Index < UE_ARRAY_COUNT(FanDegrees); ++Index)
	{
		Angles.Add(FanDegrees[Index]);
	}
	const int32 MaxLooks = Request.bNear ? NearFanLooks : FarFanLooks;
	FVector Best = FVector::ZeroVector;
	float BestFree = -1.f;
	float BestAngle = 0.f;
	int32 Looked = 0;
	for (const float Angle : Angles)
	{
		if (Looked >= MaxLooks)
		{
			break;
		}
		++Looked;
		const FVector Candidate = Turn(Heading, Side * Angle);
		const FLookResult Result = LookWithFelt(Look, Request, Candidate, Request.LookLength);
		if (Result.bClear)
		{
			Heading = Candidate;
			Turned += Angle;
			return Candidate;
		}
		// Never toward a drop: where it ends is anyone's guess (a deck's edge, an island's).
		if (!Result.bLedge && Result.FreeDistance > BestFree)
		{
			Best = Candidate;
			BestFree = Result.FreeDistance;
			BestAngle = Angle;
		}
	}
	// Nothing clear: the way open farthest, if any is open at all...
	if (!Best.IsNearlyZero() && BestFree > MinFreeShare * Request.LookLength)
	{
		Heading = Best;
		Turned += BestAngle;
		return Best;
	}
	// ...or, hemmed in, it stands this update and turns well away from the wall (right round when even that was blocked)
	// to look again from there. Standing still a while, its unstick feels what's ahead and goes round (NoteStuck).
	const float LastAngle = Angles[FMath::Min(Looked, Angles.Num()) - 1];
	const float Swing = LastAngle >= 180.f ? 180.f : 90.f;
	Heading = Turn(Heading, Side * Swing);
	Turned += Swing;
	return FVector::ZeroVector;
}

FVector FCreatureSteerPlanner::Commit(const FVector& Direction)
{
	if (Direction.IsNearlyZero())
	{
		return Direction;
	}
	if (!LastDirection.IsNearlyZero() && FVector::DotProduct(Direction, LastDirection) < -0.5)
	{
		++Reversals;
	}
	LastDirection = Direction;
	return Direction;
}

void FCreatureSteerPlanner::SetSide(float NewSide)
{
	if (NewSide != Side)
	{
		++SideFlips;
		Side = NewSide;
	}
}

void FCreatureSteerPlanner::AddFeltWall(const FVector& Center, const FVector& Normal, float HalfLength)
{
	if (FeltWalls.Num() >= MaxFeltWalls)
	{
		FeltWalls.RemoveAt(0);
	}
	FFeltWall& Wall = FeltWalls.AddDefaulted_GetRef();
	Wall.Center = Planar(Center);
	Wall.Normal = FlatUnit(Normal);
	Wall.HalfLength = HalfLength;
	Wall.SecondsLeft = FeltWallSeconds;
}
