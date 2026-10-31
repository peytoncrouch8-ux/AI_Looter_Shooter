// StoneSentinelMoves: the Stone Sentinel's choice of attack, its boulder's arc and lead and splash, its shockwave's front, gaps
// and whom it catches, as plain rules (StoneSentinelMoveRules.h). Its poses for them are StoneSentinelPoses.cpp's.

#include "Creatures/StoneSentinelMoveRules.h"
#include "Creatures/SpitterRules.h"

namespace
{
	/** Degrees from A to B the short way round (0-180). */
	float AngleApart(float A, float B)
	{
		return FMath::Abs(FMath::FindDeltaAngleDegrees(A, B));
	}
}

namespace StoneSentinelMoves
{
	EStoneSentinelMove Choose(const FStoneSentinelMoveAsk& Ask, const FSentinelShockTuning& Shock, const FSentinelBoulderTuning& Boulder)
	{
		const float Size = FMath::Max(Ask.Size, 0.05f);
		const float SlamStart = StoneSentinelRules::AttackRange * Size;
		const bool bShockLevel = FMath::Abs(Ask.Rise) <= Shock.MaxRise * FMath::Max(Size, 1.f);
		const bool bShockOpen = Ask.bShockReady && bShockLevel;
		// Within its fists' reach: the slam, its melee. Hugged, or after a run of slams, a ready shockwave throws them off
		// instead: standing in its face is never safe for long.
		if (Ask.Distance <= SlamStart)
		{
			const bool bHugged = Ask.Distance <= Shock.HugDistance * Size;
			const bool bRunOfSlams = Ask.Last == EStoneSentinelMove::Slam && Ask.Streak >= FMath::Max(Shock.SlamRunBreak, 1);
			if (bShockOpen && (bHugged || bRunOfSlams))
			{
				return EStoneSentinelMove::Shockwave;
			}
			// Its slam reaches a little up and down; a target over that is the shockwave's or nothing.
			const FStoneSentinelTuning SlamNumbers;
			if (FMath::Abs(Ask.Rise) <= SlamNumbers.SlamMaxRise * FMath::Max(Size, 1.f))
			{
				return EStoneSentinelMove::Slam;
			}
			return bShockOpen ? EStoneSentinelMove::Shockwave : EStoneSentinelMove::None;
		}
		// Near: the shockwave rolls out to them.
		if (bShockOpen && Ask.Distance <= Shock.MaxStart * Size)
		{
			return EStoneSentinelMove::Shockwave;
		}
		// Out in the open: a rock at them, if the arc to them is clear.
		if (Ask.bBoulderReady && Ask.bLobClear && Ask.Distance >= Boulder.MinStart && Ask.Distance <= Boulder.MaxStart
			&& Ask.Rise <= Boulder.MaxRise)
		{
			return EStoneSentinelMove::Boulder;
		}
		return EStoneSentinelMove::None;
	}

	float StartRange(EStoneSentinelMove Move, const FSentinelShockTuning& Shock, const FSentinelBoulderTuning& Boulder, float Size)
	{
		const float Scale = FMath::Max(Size, 0.05f);
		switch (Move)
		{
		case EStoneSentinelMove::Shockwave: return FMath::Max(Shock.MaxStart * Scale, StoneSentinelRules::AttackRange * Scale);
		case EStoneSentinelMove::Boulder: return Boulder.MaxStart;
		default: return StoneSentinelRules::AttackRange * Scale;
		}
	}

	EFightRole TokenRoleFor(EStoneSentinelMove Move)
	{
		return Move == EStoneSentinelMove::Boulder ? EFightRole::Ranged : EFightRole::Melee;
	}

	// ---------------------------------------------------------------------------
	// The boulder
	// ---------------------------------------------------------------------------

	float FlightSeconds(float Distance, const FSentinelBoulderTuning& Boulder)
	{
		const float Span = FMath::Max(Boulder.MaxStart - Boulder.MinStart, 1.f);
		const float Share = FMath::Clamp((Distance - Boulder.MinStart) / Span, 0.f, 1.f);
		return FMath::Lerp(Boulder.FlightNear, Boulder.FlightFar, Share);
	}

	FVector AimPoint(const FVector& Feet, const FVector& Velocity, float Seconds, const FSentinelBoulderTuning& Boulder)
	{
		FVector Lead = FVector(Velocity.X, Velocity.Y, 0.0) * (FMath::Max(Seconds, 0.f) * FMath::Clamp(Boulder.LeadShare, 0.f, 1.f));
		const double Most = FMath::Max(Boulder.MaxLead, 0.f);
		if (Lead.SizeSquared() > Most * Most)
		{
			Lead = Lead.GetSafeNormal() * Most;
		}
		return Feet + Lead;
	}

	FVector LobVelocity(const FVector& Start, const FVector& Target, float Seconds, const FSentinelBoulderTuning& Boulder)
	{
		return SpitterRules::LobVelocity(Start, Target, Seconds, Boulder.Gravity);
	}

	FVector LobPoint(const FVector& Start, const FVector& Velocity, float After, const FSentinelBoulderTuning& Boulder)
	{
		return SpitterRules::LobPoint(Start, Velocity, After, Boulder.Gravity);
	}

	float SplashShare(float Side, const FSentinelBoulderTuning& Boulder)
	{
		const float Radius = FMath::Max(Boulder.SplashRadius, 1.f);
		if (Side > Radius)
		{
			return 0.f;
		}
		return FMath::Lerp(1.f, FMath::Clamp(Boulder.EdgeShare, 0.f, 1.f), FMath::Clamp(Side, 0.f, Radius) / Radius);
	}

	// ---------------------------------------------------------------------------
	// The shockwave
	// ---------------------------------------------------------------------------

	float FrontRadius(float Age, const FSentinelShockTuning& Shock, float Size)
	{
		const float Scale = FMath::Max(Size, 0.05f);
		const float Radius = Shock.StartRadius + Shock.Speed * FMath::Max(Age, 0.f);
		return FMath::Min(Radius, Shock.MaxRadius) * Scale;
	}

	float FrontSeconds(const FSentinelShockTuning& Shock)
	{
		return FMath::Max(Shock.MaxRadius - Shock.StartRadius, 0.f) / FMath::Max(Shock.Speed, 1.f);
	}

	TArray<float> MakeGaps(float TargetBearing, const FSentinelShockTuning& Shock, int32 Seed)
	{
		TArray<float> Middles;
		const int32 Count = FMath::Clamp(Shock.Gaps, 0, 6);
		if (Count <= 0 || Shock.GapDegrees <= 0.f)
		{
			return Middles;
		}
		const float Step = 360.f / static_cast<float>(Count);
		// The target mid-way between two gaps (as far from both as can be), the ring turned a little either way by the seed
		// within what still keeps KeepClear between them and the nearer gap's edge.
		const float Room = FMath::Max(0.f, Step * 0.5f - Shock.GapDegrees * 0.5f - Shock.KeepClear);
		FRandomStream Random(Seed);
		const float Wobble = Room > 0.f ? Random.FRandRange(-Room, Room) : 0.f;
		const float First = TargetBearing + Step * 0.5f + Wobble;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Middles.Add(static_cast<float>(FRotator::NormalizeAxis(First + Step * static_cast<float>(Index))));
		}
		return Middles;
	}

	bool IsInGap(float Bearing, TConstArrayView<float> Gaps, float GapDegrees)
	{
		for (const float Middle : Gaps)
		{
			if (AngleApart(Bearing, Middle) <= GapDegrees * 0.5f)
			{
				return true;
			}
		}
		return false;
	}

	bool FrontCrosses(float Distance, float BodyRadius, float FromRadius, float ToRadius, float Band)
	{
		// The front's hurting band swept [From - Band, To] this moment; the body spans [Distance - Radius, Distance + Radius].
		const float Near = Distance - FMath::Max(BodyRadius, 0.f);
		const float Far = Distance + FMath::Max(BodyRadius, 0.f);
		return Near <= ToRadius && Far >= FromRadius - FMath::Max(Band, 0.f);
	}

	bool Catches(bool bFalling, float FeetAbove, const FSentinelShockTuning& Shock)
	{
		if (FMath::Abs(FeetAbove) > Shock.BodyMaxRise)
		{
			// On a ledge over it, or down a step below: the front runs along its own ground.
			return false;
		}
		// Jumped: in the air and clear of the spikes' feet as the front passes under.
		return !(bFalling && FeetAbove >= Shock.JumpClear);
	}
}
