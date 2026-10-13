// FRoostVeil: Hob's crows over the screen as plain math, gathering, covering, holding and bursting away.

#include "World/RoostVeil.h"
#include "World/FaunaRules.h"

namespace
{
	/** Each crow draws its choices from this many numbers of its own (RoostVeil's Rand slots). */
	constexpr int32 SlotsPerCrow = 24;

	// What each slot decides: a gathering crow's start, delay, turn speed and way, its band at the edge; every crow's size
	// and wings; a covering crow's start (bias, angle, how far off) and end (bias, angle, how far off), delay, time across,
	// curl and swell; the hold's and the reveal's own.
	constexpr int32 SlotGatherAngle = 0;
	constexpr int32 SlotGatherDelay = 1;
	constexpr int32 SlotGatherSpeed = 2;
	constexpr int32 SlotGatherWay = 3;
	constexpr int32 SlotGatherBand = 4;
	constexpr int32 SlotSize = 5;
	constexpr int32 SlotWingRate = 6;
	constexpr int32 SlotWingOffset = 7;
	constexpr int32 SlotFromBias = 8;
	constexpr int32 SlotFromAngle = 9;
	constexpr int32 SlotFromScale = 10;
	constexpr int32 SlotToBias = 11;
	constexpr int32 SlotToAngle = 12;
	constexpr int32 SlotToScale = 13;
	constexpr int32 SlotCoverDelay = 14;
	constexpr int32 SlotCoverSpan = 15;
	constexpr int32 SlotCurl = 16;
	constexpr int32 SlotSwell = 17;
	constexpr int32 SlotLateAngle = 18;
	constexpr int32 SlotLateStart = 19;
	constexpr int32 SlotLateSpread = 20;
	constexpr int32 SlotLateScale = 21;
	constexpr int32 SlotLateDelay = 22;
	constexpr int32 SlotLateSpan = 23;

	/** A death's gathering takes no more than this share of the time to black. */
	constexpr float GatherShare = 0.55f;
	/** A gathering crow flies in over the edge in this long (seconds). */
	constexpr float EntrySeconds = 0.35f;
	/** A death's screen is already a little dark as the flock covers it (the gather's own dimming carried on). */
	constexpr float DeathDim = 0.15f;
	/** The shortest cover or reveal worth easing (seconds). */
	constexpr float ShortestPhase = 0.05f;

	float Angle2D(const FVector2f& Direction)
	{
		return Direction.IsNearlyZero() ? 0.f : FMath::Atan2(Direction.Y, Direction.X);
	}
}

// ---------------------------------------------------------------------------
// The timeline
// ---------------------------------------------------------------------------

void FRoostVeil::Start(ERoostVeilKind InKind, float CoverIn, int32 InSeed)
{
	Kind = InKind;
	Seed = InSeed;
	Clock = 0.f;
	Lifetime = 0.f;
	RevealFrom = 1.f;
	RevealLength = RevealSeconds;
	const float Total = FMath::Max(CoverIn, 0.f);
	GatherLength = Kind == ERoostVeilKind::Death ? FMath::Min(GatherSeconds, Total * GatherShare) : 0.f;
	CoverLength = Total - GatherLength;
	Phase = GatherLength > 0.f ? ERoostVeilPhase::Gather : CoverLength > 0.f ? ERoostVeilPhase::Cover : ERoostVeilPhase::Hold;
}

void FRoostVeil::Reveal(float Seconds)
{
	if (Phase == ERoostVeilPhase::Idle)
	{
		return;
	}
	RevealFrom = GetShade();
	RevealLength = FMath::Max(Seconds, ShortestPhase);
	Phase = ERoostVeilPhase::Reveal;
	Clock = 0.f;
}

void FRoostVeil::Stop()
{
	Phase = ERoostVeilPhase::Idle;
	Clock = 0.f;
}

void FRoostVeil::Advance(float DeltaSeconds)
{
	if (Phase == ERoostVeilPhase::Idle)
	{
		return;
	}
	const float Step = FMath::Max(DeltaSeconds, 0.f);
	Clock += Step;
	Lifetime += Step;
	// A long step carries over into the next phase rather than losing its time.
	for (;;)
	{
		if (Phase == ERoostVeilPhase::Gather && Clock >= GatherLength)
		{
			Clock -= GatherLength;
			Phase = CoverLength > 0.f ? ERoostVeilPhase::Cover : ERoostVeilPhase::Hold;
			continue;
		}
		if (Phase == ERoostVeilPhase::Cover && Clock >= CoverLength)
		{
			Clock -= CoverLength;
			Phase = ERoostVeilPhase::Hold;
			continue;
		}
		if (Phase == ERoostVeilPhase::Reveal && Clock >= RevealLength)
		{
			Stop();
		}
		break;
	}
}

float FRoostVeil::GetPhaseLength() const
{
	switch (Phase)
	{
	case ERoostVeilPhase::Gather: return GatherLength;
	case ERoostVeilPhase::Cover:  return CoverLength;
	case ERoostVeilPhase::Reveal: return RevealLength;
	default:                      return 0.f;
	}
}

float FRoostVeil::GetShade() const
{
	const float Dim = Kind == ERoostVeilKind::Death ? DeathDim : 0.f;
	switch (Phase)
	{
	case ERoostVeilPhase::Gather:
		// Dimming a little as they gather (the death's own red fade does the rest under it).
		return Dim * FMath::SmoothStep(0.f, FMath::Max(GatherLength, ShortestPhase), Clock);
	case ERoostVeilPhase::Cover:
	{
		// Black only as the flock is thickest: the first crows cross a world still seen.
		const float Through = Clock / FMath::Max(CoverLength, ShortestPhase);
		return Dim + (1.f - Dim) * FMath::SmoothStep(0.2f, 1.f, Through);
	}
	case ERoostVeilPhase::Hold:
		return 1.f;
	case ERoostVeilPhase::Reveal:
		// The world is back within the first half, as the crows tear away off it.
		return RevealFrom * (1.f - FMath::SmoothStep(0.f, 0.5f, Clock / RevealLength));
	default:
		return 0.f;
	}
}

// ---------------------------------------------------------------------------
// The crows
// ---------------------------------------------------------------------------

float FRoostVeil::Rand(int32 Index, int32 Slot) const
{
	return FaunaRules::Random01(static_cast<uint32>(Seed), static_cast<uint32>(Index * SlotsPerCrow + Slot + 1));
}

FVector2f FRoostVeil::EdgePoint(float Angle, float Aspect, float Scale)
{
	const FVector2f Direction(FMath::Cos(Angle), FMath::Sin(Angle));
	const float HalfWidth = FMath::Max(Aspect, 0.1f) * 0.5f;
	const float AcrossX = FMath::Abs(Direction.X) > UE_KINDA_SMALL_NUMBER ? HalfWidth / FMath::Abs(Direction.X) : TNumericLimits<float>::Max();
	const float AcrossY = FMath::Abs(Direction.Y) > UE_KINDA_SMALL_NUMBER ? 0.5f / FMath::Abs(Direction.Y) : TNumericLimits<float>::Max();
	return Direction * (FMath::Min(AcrossX, AcrossY) * Scale);
}

int32 FRoostVeil::WingFrame(float Time, float Rate, float Offset)
{
	// Raised, level, lowered, level: a beat in four steps reads as flapping at these sizes and speeds.
	static const int32 Cycle[] = { 0, 1, 2, 1 };
	const int32 Step = FMath::FloorToInt32((Time * Rate + Offset) * 4.f);
	return Cycle[((Step % 4) + 4) % 4];
}

bool FRoostVeil::GatherCrow(int32 Index, float Aspect, float Time, FRoostVeilCrow& Out) const
{
	if (Index >= GatherCrows || GatherLength <= 0.f)
	{
		return false;
	}
	const float Delay = Rand(Index, SlotGatherDelay) * GatherShare * GatherLength;
	const float Since = Time - Delay;
	if (Since < 0.f)
	{
		return false;
	}
	// Each comes in over the edge where it is, then circles along it, this way or that, at its own pace.
	const float Start = UE_TWO_PI * (static_cast<float>(Index) + 0.6f * Rand(Index, SlotGatherAngle)) / static_cast<float>(GatherCrows);
	const float Turn = (0.35f + 0.45f * Rand(Index, SlotGatherSpeed)) * (Rand(Index, SlotGatherWay) < 0.5f ? -1.f : 1.f);
	const float Band = 0.84f + 0.16f * Rand(Index, SlotGatherBand);
	auto Where = [&](float At)
	{
		const float Entry = FMath::Clamp(At / EntrySeconds, 0.f, 1.f);
		const float In = 1.f - FMath::Square(1.f - Entry);
		return EdgePoint(Start + Turn * At, Aspect, FMath::Lerp(1.35f, Band, In));
	};
	Out.Position = Where(Since);
	Out.Angle = Angle2D(Where(Since + 0.03f) - Out.Position);
	Out.Size = 0.05f + 0.035f * Rand(Index, SlotSize);
	Out.Frame = WingFrame(Lifetime, 6.f + 3.f * Rand(Index, SlotWingRate), Rand(Index, SlotWingOffset));
	Out.Opacity = 1.f;
	return true;
}

bool FRoostVeil::CoverCrow(int32 Index, float Aspect, FRoostVeilCrow& Out) const
{
	if (Index >= CoverCrows)
	{
		return false;
	}
	const bool bTravel = Kind == ERoostVeilKind::Travel;
	// A death's gatherers set off from where they circled; the rest come in from off the screen (a travel's mostly from
	// below and the lower sides, as if the crows were lifting Ellis up into them).
	const bool bGatherer = !bTravel && Index < GatherCrows && GatherLength > 0.f;
	FVector2f From;
	FRoostVeilCrow Gathered;
	if (bGatherer && GatherCrow(Index, Aspect, GatherLength, Gathered))
	{
		From = Gathered.Position;
	}
	else
	{
		const float Angle = bTravel && Rand(Index, SlotFromBias) < 0.7f ? UE_HALF_PI + (Rand(Index, SlotFromAngle) - 0.5f) * 2.6f
			: UE_TWO_PI * Rand(Index, SlotFromAngle);
		From = EdgePoint(Angle, Aspect, 1.2f + 0.25f * Rand(Index, SlotFromScale));
	}
	// Out across the screen past its middle, a travel's mostly away upward.
	const float ToAngle = bTravel && Rand(Index, SlotToBias) < 0.7f ? -UE_HALF_PI + (Rand(Index, SlotToAngle) - 0.5f) * 2.4f
		: Angle2D(From) + UE_PI + (Rand(Index, SlotToAngle) - 0.5f) * 1.3f;
	const FVector2f To = EdgePoint(ToAngle, Aspect, 1.45f + 0.2f * Rand(Index, SlotToScale));
	const float Delay = (bGatherer ? 0.15f : 0.5f) * Rand(Index, SlotCoverDelay) * CoverLength;
	const float Span = FMath::Max((0.45f + 0.4f * Rand(Index, SlotCoverSpan)) * CoverLength, ShortestPhase);
	const float Across = (Clock - Delay) / Span;
	if (Across >= 1.f || (Across <= 0.f && !bGatherer))
	{
		return false;
	}
	const float Alpha = FMath::Max(Across, 0.f);
	const FVector2f Line = To - From;
	const FVector2f Side = FVector2f(-Line.Y, Line.X).GetSafeNormal();
	const float Curl = (Rand(Index, SlotCurl) - 0.5f) * 0.4f;
	Out.Position = From + Line * Alpha + Side * (FMath::Sin(UE_PI * Alpha) * Curl);
	Out.Angle = Angle2D(Line + Side * (UE_PI * FMath::Cos(UE_PI * Alpha) * Curl));
	// Swelling as it comes past the camera: most a little, a few filling a quarter of the screen.
	const float Swell = 0.06f + 0.34f * FMath::Square(Rand(Index, SlotSwell));
	Out.Size = 0.05f + 0.035f * Rand(Index, SlotSize) + Swell * FMath::Pow(Alpha, 1.6f);
	Out.Frame = WingFrame(Lifetime, 8.f + 4.f * Rand(Index, SlotWingRate), Rand(Index, SlotWingOffset));
	Out.Opacity = 1.f;
	return true;
}

bool FRoostVeil::HoldCrow(int32 Index, float Aspect, FRoostVeilCrow& Out) const
{
	if (Index >= HoldCrows)
	{
		return false;
	}
	// Inside the flock: a few big wings drifting across the dark, coming and going.
	const float Period = 1.3f + 0.9f * Rand(Index, SlotLateSpan);
	const float Alpha = FMath::Frac((Clock + Rand(Index, SlotLateDelay) * Period) / Period);
	const float Angle = UE_TWO_PI * Rand(Index, SlotLateAngle);
	const FVector2f From = EdgePoint(Angle, Aspect, 1.15f);
	const FVector2f To = EdgePoint(Angle + UE_PI + (Rand(Index, SlotLateSpread) - 0.5f), Aspect, 1.15f);
	Out.Position = From + (To - From) * Alpha;
	Out.Angle = Angle2D(To - From);
	Out.Size = 0.12f + 0.18f * Rand(Index, SlotLateScale);
	Out.Frame = WingFrame(Lifetime, 5.f + 2.f * Rand(Index, SlotWingRate), Rand(Index, SlotWingOffset));
	Out.Opacity = 0.55f * FMath::Sin(UE_PI * Alpha);
	return Out.Opacity > 0.01f;
}

bool FRoostVeil::RevealCrow(int32 Index, float Aspect, FRoostVeilCrow& Out) const
{
	if (Index >= RevealCrows)
	{
		return false;
	}
	// From about the middle outward and off the screen, fastest at first: a burst (a travel's mostly up and away).
	const float Angle = Kind == ERoostVeilKind::Travel && Rand(Index, SlotToBias) < 0.6f
		? -UE_HALF_PI + (Rand(Index, SlotLateAngle) - 0.5f) * 2.4f : UE_TWO_PI * Rand(Index, SlotLateAngle);
	const float Near = 0.03f + 0.24f * Rand(Index, SlotLateStart);
	const FVector2f From(FMath::Cos(Angle) * Near, FMath::Sin(Angle) * Near);
	const FVector2f To = EdgePoint(Angle + (Rand(Index, SlotLateSpread) - 0.5f) * 0.4f, Aspect, 1.35f + 0.3f * Rand(Index, SlotLateScale));
	const float Delay = 0.22f * Rand(Index, SlotLateDelay) * RevealLength;
	const float Span = FMath::Max((0.42f + 0.33f * Rand(Index, SlotLateSpan)) * RevealLength, ShortestPhase);
	const float Alpha = FMath::Clamp((Clock - Delay) / Span, 0.f, 1.f);
	if (Alpha >= 1.f)
	{
		return false;
	}
	const float Eased = 1.f - FMath::Square(1.f - Alpha);
	Out.Position = From + (To - From) * Eased;
	Out.Angle = Angle2D(To - From);
	Out.Size = (0.1f + 0.12f * Rand(Index, SlotSize)) * FMath::Lerp(1.f, 1.6f + 1.2f * Rand(Index, SlotSwell), Eased);
	Out.Frame = WingFrame(Lifetime, 10.f + 4.f * Rand(Index, SlotWingRate), Rand(Index, SlotWingOffset));
	Out.Opacity = 1.f - FMath::SmoothStep(0.85f, 1.f, Alpha);
	return true;
}

void FRoostVeil::GetCrows(float Aspect, TArray<FRoostVeilCrow>& OutCrows) const
{
	OutCrows.Reset();
	FRoostVeilCrow Crow;
	switch (Phase)
	{
	case ERoostVeilPhase::Gather:
		for (int32 Index = 0; Index < GatherCrows; ++Index)
		{
			if (GatherCrow(Index, Aspect, Clock, Crow))
			{
				OutCrows.Add(Crow);
			}
		}
		break;
	case ERoostVeilPhase::Cover:
		for (int32 Index = 0; Index < CoverCrows; ++Index)
		{
			if (CoverCrow(Index, Aspect, Crow))
			{
				OutCrows.Add(Crow);
			}
		}
		break;
	case ERoostVeilPhase::Hold:
		for (int32 Index = 0; Index < HoldCrows; ++Index)
		{
			if (HoldCrow(Index, Aspect, Crow))
			{
				OutCrows.Add(Crow);
			}
		}
		break;
	case ERoostVeilPhase::Reveal:
		for (int32 Index = 0; Index < RevealCrows; ++Index)
		{
			if (RevealCrow(Index, Aspect, Crow))
			{
				OutCrows.Add(Crow);
			}
		}
		break;
	default:
		break;
	}
	// The far (small) ones first, so the near ones pass over them.
	OutCrows.Sort([](const FRoostVeilCrow& A, const FRoostVeilCrow& B) { return A.Size < B.Size; });
}
