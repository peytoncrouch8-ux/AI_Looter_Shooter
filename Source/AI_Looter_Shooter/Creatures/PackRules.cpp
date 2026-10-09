#include "Creatures/PackRules.h"

namespace
{
	/** A leg shorter than this (cm) has no direction of its own. */
	constexpr float ShortestLeg = 1.f;
}

// ---------------------------------------------------------------------------
// Flanking
// ---------------------------------------------------------------------------

float PackRules::BearingAround(const FVector& Target, const FVector& Me)
{
	return FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(Me.Y - Target.Y, Me.X - Target.X)));
}

float PackRules::FlankHalfSpread(int32 PackSize, float StepDegrees, float MaxDegrees)
{
	if (PackSize <= 1)
	{
		return 0.f;
	}
	return FMath::Clamp(FMath::Max(StepDegrees, 0.f) * static_cast<float>(PackSize - 1), 0.f, FMath::Max(MaxDegrees, 0.f));
}

TArray<float> PackRules::FlankOffsets(const TArray<float>& BearingsDegrees, const TArray<uint32>& TieBreaks, float StepDegrees,
	float MaxDegrees)
{
	const int32 Count = BearingsDegrees.Num();
	TArray<float> Offsets;
	Offsets.Init(0.f, Count);
	const float Half = FlankHalfSpread(Count, StepDegrees, MaxDegrees);
	if (Count <= 1 || Half <= 0.f)
	{
		return Offsets;
	}
	// Ranked about the pack's mean bearing (a circular mean, so a pack astride south ranks as one line, not two ends).
	double SumSin = 0.0;
	double SumCos = 0.0;
	for (const float Bearing : BearingsDegrees)
	{
		SumSin += FMath::Sin(FMath::DegreesToRadians(static_cast<double>(Bearing)));
		SumCos += FMath::Cos(FMath::DegreesToRadians(static_cast<double>(Bearing)));
	}
	const float Mean = FMath::Abs(SumSin) + FMath::Abs(SumCos) < UE_KINDA_SMALL_NUMBER
		? 0.f : FMath::RadiansToDegrees(static_cast<float>(FMath::Atan2(SumSin, SumCos)));
	TArray<int32> Order;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Order.Add(Index);
	}
	Order.Sort([&BearingsDegrees, &TieBreaks, Mean](int32 A, int32 B)
	{
		const float RelativeA = FMath::UnwindDegrees(BearingsDegrees[A] - Mean);
		const float RelativeB = FMath::UnwindDegrees(BearingsDegrees[B] - Mean);
		if (!FMath::IsNearlyEqual(RelativeA, RelativeB, 0.01f))
		{
			return RelativeA < RelativeB;
		}
		const uint32 TieA = TieBreaks.IsValidIndex(A) ? TieBreaks[A] : static_cast<uint32>(A);
		const uint32 TieB = TieBreaks.IsValidIndex(B) ? TieBreaks[B] : static_cast<uint32>(B);
		return TieA < TieB;
	});
	// The low end swings further low, the high end further high: the pack opens out round its prey.
	for (int32 Rank = 0; Rank < Count; ++Rank)
	{
		Offsets[Order[Rank]] = -Half + 2.f * Half * static_cast<float>(Rank) / static_cast<float>(Count - 1);
	}
	return Offsets;
}

FVector PackRules::FlankDirection(const FVector& Me, const FVector& Target, float FlankDegrees, float ReleaseDistance)
{
	const FVector Out = FVector(Me.X - Target.X, Me.Y - Target.Y, 0.0);
	const double Distance = Out.Size();
	if (Distance < UE_KINDA_SMALL_NUMBER)
	{
		return FVector::ZeroVector;
	}
	const FVector Outward = Out / Distance;
	if (FMath::IsNearlyZero(FlankDegrees) || Distance <= ReleaseDistance)
	{
		return -Outward;
	}
	// Inward, turned toward the way its bearing grows (that bearing's tangent): cos of the flank in, sin of it round.
	const FVector Round(-Outward.Y, Outward.X, 0.0);
	const double Radians = FMath::DegreesToRadians(static_cast<double>(FlankDegrees));
	return (-Outward * FMath::Cos(Radians) + Round * FMath::Sin(Radians)).GetSafeNormal2D();
}

// ---------------------------------------------------------------------------
// Breaking off
// ---------------------------------------------------------------------------

bool PackRules::WantsRetreatRoll(const FRetreatAsk& Ask)
{
	return Ask.Rank == ECreatureRank::Basic && Ask.bAllowedKind && Ask.Chance > 0.f && Ask.HealthShare > 0.f
		&& Ask.HealthShare <= Ask.HealthBelow && Ask.bHadPack && Ask.PackmatesLeft <= 0 && !Ask.bAlreadyRolled;
}

bool PackRules::ShouldRetreat(const FRetreatAsk& Ask, float Roll)
{
	return WantsRetreatRoll(Ask) && Roll < Ask.Chance;
}

FVector PackRules::RetreatGoal(const FVector& Me, const FVector& Target, const FVector& Home, float Distance,
	TFunctionRef<bool(const FVector&)> IsOnGround)
{
	const FVector ToHome = FVector(Home.X - Me.X, Home.Y - Me.Y, 0.0).GetSafeNormal();
	FVector Away = FVector(Me.X - Target.X, Me.Y - Target.Y, 0.0).GetSafeNormal();
	if (Away.IsNearlyZero())
	{
		Away = ToHome.IsNearlyZero() ? FVector::ForwardVector : ToHome;
	}
	const FVector Straight = Me + Away * Distance;
	if (IsOnGround(Straight) || ToHome.IsNearlyZero())
	{
		return FVector(Straight.X, Straight.Y, Me.Z);
	}
	// Away would take it off its ground: away and homeward at once, or straight home if even that leaves it.
	const FVector Bent = (Away + ToHome).GetSafeNormal();
	const FVector Between = Me + (Bent.IsNearlyZero() ? ToHome : Bent) * Distance;
	const FVector Goal = IsOnGround(Between) ? Between : Me + ToHome * Distance;
	return FVector(Goal.X, Goal.Y, Me.Z);
}

// ---------------------------------------------------------------------------
// The rank sting
// ---------------------------------------------------------------------------

bool PackRules::ShouldRankSting(ECreatureRank Rank, bool bShowsTag, bool bAlreadyStung)
{
	return Rank != ECreatureRank::Basic && Rank != ECreatureRank::Boss && bShowsTag && !bAlreadyStung;
}

// ---------------------------------------------------------------------------
// Patrols
// ---------------------------------------------------------------------------

float PackRules::RouteLength(const TArray<FVector>& Route, bool bLoop)
{
	const int32 Count = Route.Num();
	float Length = 0.f;
	for (int32 Index = 0; Index + 1 < Count; ++Index)
	{
		Length += static_cast<float>(FVector::Dist2D(Route[Index], Route[Index + 1]));
	}
	if (bLoop && Count > 2)
	{
		Length += static_cast<float>(FVector::Dist2D(Route.Last(), Route[0]));
	}
	return Length;
}

FVector PackRules::PointAlong(const TArray<FVector>& Route, bool bLoop, float Along, FVector* OutLegDirection)
{
	const int32 Count = Route.Num();
	if (OutLegDirection)
	{
		*OutLegDirection = FVector::ForwardVector;
	}
	if (Count == 0)
	{
		return FVector::ZeroVector;
	}
	if (Count == 1)
	{
		return Route[0];
	}
	const bool bClosed = bLoop && Count > 2;
	const float Length = RouteLength(Route, bLoop);
	float Left = bClosed && Length > 0.f ? FMath::Fmod(FMath::Fmod(Along, Length) + Length, Length) : FMath::Clamp(Along, 0.f, Length);
	const int32 Legs = bClosed ? Count : Count - 1;
	for (int32 Leg = 0; Leg < Legs; ++Leg)
	{
		const FVector& From = Route[Leg];
		const FVector& To = Route[(Leg + 1) % Count];
		const float LegLength = static_cast<float>(FVector::Dist2D(From, To));
		if (Left <= LegLength || Leg == Legs - 1)
		{
			if (OutLegDirection && LegLength >= ShortestLeg)
			{
				*OutLegDirection = FVector(To.X - From.X, To.Y - From.Y, 0.0) / LegLength;
			}
			return FMath::Lerp(From, To, LegLength >= ShortestLeg ? FMath::Clamp(Left / LegLength, 0.f, 1.f) : 1.f);
		}
		Left -= LegLength;
	}
	return Route.Last();
}

bool PackRules::AdvancePatrol(const TArray<FVector>& Route, bool bLoop, float Speed, float DeltaSeconds, float PauseSeconds,
	FPatrolWalk& Walk)
{
	const float Length = RouteLength(Route, bLoop);
	if (Route.Num() < 2 || Length < ShortestLeg)
	{
		return false;
	}
	if (Walk.PauseLeft > 0.f)
	{
		Walk.PauseLeft = FMath::Max(Walk.PauseLeft - DeltaSeconds, 0.f);
		return false;
	}
	const float Step = FMath::Max(Speed, 0.f) * FMath::Max(DeltaSeconds, 0.f);
	if (Step <= 0.f)
	{
		return false;
	}
	Walk.Along += (Walk.Direction >= 0.f ? 1.f : -1.f) * Step;
	if (bLoop && Route.Num() > 2)
	{
		// Round and round, resting at its first point each time it passes it.
		if (Walk.Along >= Length)
		{
			Walk.Along = FMath::Fmod(Walk.Along, Length);
			Walk.PauseLeft = FMath::Max(PauseSeconds, 0.f);
		}
		else if (Walk.Along < 0.f)
		{
			Walk.Along += Length;
		}
		Walk.Direction = 1.f;
		return true;
	}
	// There and back, resting at each end before it turns.
	if (Walk.Along >= Length)
	{
		Walk.Along = Length;
		Walk.Direction = -1.f;
		Walk.PauseLeft = FMath::Max(PauseSeconds, 0.f);
	}
	else if (Walk.Along <= 0.f)
	{
		Walk.Along = 0.f;
		Walk.Direction = 1.f;
		Walk.PauseLeft = FMath::Max(PauseSeconds, 0.f);
	}
	return true;
}

FVector PackRules::PatrolHeading(const TArray<FVector>& Route, bool bLoop, const FPatrolWalk& Walk)
{
	FVector Leg = FVector::ForwardVector;
	PointAlong(Route, bLoop, Walk.Along, &Leg);
	return Walk.Direction >= 0.f ? Leg : -Leg;
}

FVector PackRules::FormationSpot(const FVector& Point, const FVector& Heading, int32 Index, float Spacing)
{
	if (Index <= 0)
	{
		return Point;
	}
	const FVector Ahead = FVector(Heading.X, Heading.Y, 0.0).GetSafeNormal();
	const FVector Forward = Ahead.IsNearlyZero() ? FVector::ForwardVector : Ahead;
	// Its right, seen from above (X forward, Y right): the file staggers right, left, right behind the leader.
	const FVector Right(-Forward.Y, Forward.X, 0.0);
	const int32 Row = (Index + 1) / 2;
	const float Side = Index % 2 == 1 ? 1.f : -1.f;
	return Point - Forward * (Spacing * Row) + Right * (Side * Spacing * 0.5f);
}

// ---------------------------------------------------------------------------
// Ambushes
// ---------------------------------------------------------------------------

bool PackRules::IsWalkIn(bool bSeenBefore, bool bWasInside, bool bInside, const FVector& Before, const FVector& Now, float MaxStep)
{
	return bSeenBefore && !bWasInside && bInside && FVector::Dist2D(Before, Now) <= MaxStep;
}
