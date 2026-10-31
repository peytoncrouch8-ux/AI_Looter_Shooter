#include "Creatures/UnpaidRiseRules.h"

// How the Unpaid rise out of the ground, as plain rules (2026-10-31): the phases and their timings, how deep it is through each,
// and the surround's slots (bearing, distance, stagger, candidates, spacing). UnpaidCreatureRise.cpp plays them.

namespace
{
	float EaseOut(float Alpha)
	{
		return FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	float EaseInOut(float Alpha)
	{
		return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	/**
	 * The surround's pattern, off the player's view (degrees): ahead (they see the first hand come up), behind (they hear it and
	 * turn), their right and left, then the gaps. Never two in a row on one side, so the stagger walks round them.
	 */
	constexpr float Pattern[UnpaidRiseRules::PatternSlots] = { 0.f, 180.f, -100.f, 100.f, -45.f, 45.f, -140.f, 140.f };
	/** Every other slot this share farther out; each time round the pattern this share farther again, half a gap round. */
	constexpr float OddFarther = 0.18f;
	constexpr float RoundFarther = 0.3f;
	constexpr float RoundTurn = 22.5f;
	/** A slot's jitter: up to this share of the gap later. */
	constexpr float StaggerJitter = 0.35f;

	/** Where a slot tries, best first: degrees round from its own bearing and a share of its distance. */
	constexpr float Tries[][2] = { { 0.f, 1.f }, { 20.f, 1.f }, { -20.f, 1.f }, { 40.f, 1.f }, { -40.f, 1.f }, { 0.f, 0.8f }, { 0.f, 1.2f },
		{ 25.f, 0.85f }, { -25.f, 0.85f } };

	// --- How deep (cm under its hang) ---
	constexpr float BreachTop = 180.f;
	constexpr float FirstHeave = 120.f;
	constexpr float SecondHeave = 40.f;
	constexpr float QuickBreachTop = 125.f;
	constexpr float QuickFirstHeave = 80.f;
	constexpr float QuickSecondHeave = 25.f;
	/** The stand overshoots its hang this far (cm) and settles. */
	constexpr float StandOvershoot = 5.f;
	/** The haul's first heave over this share of it, a gather, then the second from GatherEnd on. */
	constexpr float FirstHeaveEnd = 0.45f;
	constexpr float GatherEnd = 0.55f;
	constexpr float OvershootAt = 0.6f;
}

FUnpaidRiseTiming UnpaidRiseRules::Timing(EUnpaidRiseStyle Style, float Delay, float Rumble)
{
	FUnpaidRiseTiming Timing;
	if (Style == EUnpaidRiseStyle::Ambush)
	{
		Timing.Buried = FMath::Max(Delay, 0.f);
		Timing.Rumble = FMath::Clamp(Rumble, 0.f, Timing.Buried);
		Timing.Breach = AmbushBreach;
		Timing.Haul = AmbushHaul;
		Timing.Stand = AmbushStand;
		return Timing;
	}
	Timing.Breach = QuickBreach;
	Timing.Haul = QuickHaul;
	Timing.Stand = QuickStand;
	return Timing;
}

EUnpaidRisePhase UnpaidRiseRules::PhaseAt(float Time, const FUnpaidRiseTiming& Timing, float& OutInto)
{
	OutInto = 0.f;
	if (Time < 0.f)
	{
		return EUnpaidRisePhase::None;
	}
	float Start = 0.f;
	const struct { EUnpaidRisePhase Phase; float Length; } Phases[] = { { EUnpaidRisePhase::Buried, Timing.Buried },
		{ EUnpaidRisePhase::Breach, Timing.Breach }, { EUnpaidRisePhase::Haul, Timing.Haul }, { EUnpaidRisePhase::Stand, Timing.Stand } };
	for (const auto& Each : Phases)
	{
		if (Each.Length > 0.f && Time < Start + Each.Length)
		{
			OutInto = FMath::Clamp((Time - Start) / Each.Length, 0.f, 1.f);
			return Each.Phase;
		}
		Start += FMath::Max(Each.Length, 0.f);
	}
	return EUnpaidRisePhase::None;
}

bool UnpaidRiseRules::IsRumbling(float Time, const FUnpaidRiseTiming& Timing)
{
	return Timing.Rumble > 0.f && Time >= Timing.Buried - Timing.Rumble && Time < Timing.Buried;
}

float UnpaidRiseRules::Sink(EUnpaidRiseStyle Style, EUnpaidRisePhase Phase, float Into)
{
	const bool bQuick = Style != EUnpaidRiseStyle::Ambush;
	const float Deep = bQuick ? QuickDepth : BuriedDepth;
	const float Top = bQuick ? QuickBreachTop : BreachTop;
	const float First = bQuick ? QuickFirstHeave : FirstHeave;
	const float Second = bQuick ? QuickSecondHeave : SecondHeave;
	switch (Phase)
	{
	case EUnpaidRisePhase::Buried:
		return -Deep;
	case EUnpaidRisePhase::Breach:
		// Its arm thrusts up out of the dirt and the body jerks up after it a little.
		return -FMath::Lerp(Deep, Top, EaseOut(Into));
	case EUnpaidRisePhase::Haul:
		// Two heaves: up, a gather on its planted hands, up again and out.
		if (Into < FirstHeaveEnd)
		{
			return -FMath::Lerp(Top, First, EaseInOut(Into / FirstHeaveEnd));
		}
		if (Into < GatherEnd)
		{
			return -First;
		}
		return -FMath::Lerp(First, Second, EaseOut((Into - GatherEnd) / (1.f - GatherEnd)));
	case EUnpaidRisePhase::Stand:
		// Straightening: a touch past its hang as it throws its head back, then settled.
		if (Into < OvershootAt)
		{
			return FMath::Lerp(-Second, StandOvershoot, EaseOut(Into / OvershootAt));
		}
		return FMath::Lerp(StandOvershoot, 0.f, EaseInOut((Into - OvershootAt) / (1.f - OvershootAt)));
	default:
		return 0.f;
	}
}

float UnpaidRiseRules::PatternOffset(int32 Slot)
{
	const int32 Safe = FMath::Max(Slot, 0);
	const int32 Round = Safe / PatternSlots;
	return FMath::UnwindDegrees(Pattern[Safe % PatternSlots] + Round * RoundTurn);
}

float UnpaidRiseRules::SurroundBearing(int32 Slot, float ViewYaw, float Turn)
{
	return FMath::UnwindDegrees(ViewYaw + PatternOffset(Slot) + Turn);
}

float UnpaidRiseRules::SurroundRadius(int32 Slot, float Radius)
{
	const int32 Safe = FMath::Max(Slot, 0);
	return Radius * (1.f + (Safe % 2) * OddFarther + (Safe / PatternSlots) * RoundFarther);
}

float UnpaidRiseRules::StaggerDelay(int32 Slot, float First, float Gap, float Roll)
{
	const float Step = FMath::Max(Gap, 0.f);
	return FMath::Max(First, 0.f) + FMath::Max(Slot, 0) * Step + FMath::Clamp(Roll, 0.f, 1.f) * StaggerJitter * Step;
}

TArray<FVector> UnpaidRiseRules::SurroundCandidates(const FVector& Center, float Bearing, float Radius)
{
	TArray<FVector> Candidates;
	for (const float* Try : Tries)
	{
		const float Radians = FMath::DegreesToRadians(Bearing + Try[0]);
		const float Out = Radius * Try[1];
		Candidates.Add(FVector(Center.X + FMath::Cos(Radians) * Out, Center.Y + FMath::Sin(Radians) * Out, Center.Z));
	}
	return Candidates;
}

bool UnpaidRiseRules::IsClear(const FVector& Spot, const TArray<FVector>& Taken, float Spacing, const FVector& Center, float ClearOfPlayer)
{
	if (FVector::DistSquared2D(Spot, Center) < FMath::Square(static_cast<double>(ClearOfPlayer)))
	{
		return false;
	}
	const double Apart = FMath::Square(static_cast<double>(Spacing));
	return !Taken.ContainsByPredicate([&Spot, Apart](const FVector& Other) { return FVector::DistSquared2D(Spot, Other) < Apart; });
}

int32 UnpaidRiseRules::ClaimSlot(FUnpaidRiseBurst& Burst, double Now, float ViewYaw, float Window, float TurnRoll, float MaxTurn)
{
	if (Now - Burst.Last > FMath::Max(Window, 0.f))
	{
		// A fresh burst: round where they look now, turned a little so no two bursts come up the same way.
		Burst.Start = Now;
		Burst.Given = 0;
		Burst.ViewYaw = ViewYaw;
		Burst.Turn = (FMath::Clamp(TurnRoll, 0.f, 1.f) * 2.f - 1.f) * MaxTurn;
		Burst.Spots.Reset();
	}
	Burst.Last = Now;
	return Burst.Given++;
}
