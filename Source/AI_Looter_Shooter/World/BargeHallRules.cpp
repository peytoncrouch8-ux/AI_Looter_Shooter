// BargeHallRules: the second half of the boarding stage's math (Main 6, "The Pilot Light"): the hall below the deck, the
// hatch's stair down onto its catwalk and the stoker's corner there, the grate ring, the vents' clock, the hose, her dying
// and fall.

#include "World/BargeHallRules.h"

namespace BargeRules
{
	namespace
	{
		FVector2D Flat(const FVector& At)
		{
			return FVector2D(At.X, At.Y);
		}

		/** A point's hall angle (degrees from OfficeAngle, -180..180). */
		float HallAngleOf(const FVector& At)
		{
			const float Barge = FMath::RadiansToDegrees(FMath::Atan2(At.Y - HallCenter.Y, At.X - HallCenter.X));
			return FMath::UnwindDegrees(Barge - OfficeAngle);
		}

		/** The hall's outward and along-the-ring ways at a hall angle (flat, the barge's frame). */
		FVector OutAt(float AngleDegrees)
		{
			return HallPoint(AngleDegrees, 1.f) - HallPoint(AngleDegrees, 0.f);
		}

		FVector AlongAt(float AngleDegrees)
		{
			const FVector Out = OutAt(AngleDegrees);
			return FVector(-Out.Y, Out.X, 0.0);
		}
	}

	bool HallFits(float Radius)
	{
		return Radius > 0.f && Radius + WallThickness + HullWall <= HallSkinHalfBeam;
	}

	float HallDoorHalf()
	{
		// A door's gap is a panel's width either side of its middle: its half chord at the wall's inner face.
		return HallRadius * FMath::Sin(FMath::DegreesToRadians(DoorGapDegrees));
	}

	float AlcoveHalfWidth()
	{
		return HallDoorHalf();
	}

	FBox2D HallCeilingArea()
	{
		const double Out = HallRadius + WallThickness;
		return FBox2D(FVector2D(HallCenter.X - Out, HallCenter.Y - Out), FVector2D(HallCenter.X + Out, HallCenter.Y + Out));
	}

	FBox2D HatchHole()
	{
		return FBox2D(FVector2D(HatchHoleXMin, HatchHoleYMin), FVector2D(HatchHoleXMax, HatchHoleYMax));
	}

	FBox2D HatchCoaming()
	{
		return HatchHole().ExpandBy(HatchCoamingThick);
	}

	float HatchDeckZ()
	{
		return static_cast<float>(OnDeck(HatchHole().GetCenter()).Z);
	}

	FVector HatchSpot()
	{
		return HatchStair().From;
	}

	FVector HallPoint(float AngleDegrees, float Radius, float Up)
	{
		const float Turn = FMath::DegreesToRadians(OfficeAngle + AngleDegrees);
		return HallCenter + FVector(FMath::Cos(Turn) * Radius, FMath::Sin(Turn) * Radius, HallFloor + Up);
	}

	float CatwalkSpanAngle(int32 Index)
	{
		return (Index + 0.5f) * 360.f / CatwalkSpans;
	}

	bool IsCatwalkSpanLaid(int32 Index)
	{
		// A span whose middle lies over a floor stair's arc would roof the climb (the stairs run under the ring's middle), and
		// one under the hatch's stair would stand in the way down it: open there. The floor stairs don't cross 0 degrees.
		const float Middle = CatwalkSpanAngle(Index);
		for (int32 Stair = 0; Stair < NumFloorStairs; ++Stair)
		{
			const float Top = FloorStairAngles[Stair];
			const float Foot = Top + FloorStairWays[Stair] * FloorStairSpan;
			if (Middle > FMath::Min(Top, Foot) && Middle < FMath::Max(Top, Foot))
			{
				return false;
			}
		}
		float HatchFoot = 0.f;
		float HatchTop = 0.f;
		HatchStairArc(HatchFoot, HatchTop);
		const float Unwound = FMath::UnwindDegrees(Middle);
		return !(Unwound > FMath::Min(HatchFoot, HatchTop) && Unwound < FMath::Max(HatchFoot, HatchTop));
	}

	float CatwalkEndAngle(int32 Stair)
	{
		// The first laid span past the foot, the stair's way round: its near edge.
		const int32 At = FMath::Clamp(Stair, 0, NumFloorStairs - 1);
		const float Way = FloorStairWays[At];
		const float Foot = FloorStairAngles[At] + Way * FloorStairSpan;
		const float Half = 180.f / CatwalkSpans * CatwalkSpanStretch;
		float Nearest = Way > 0.f ? 360.f : 0.f;
		for (int32 Index = 0; Index < CatwalkSpans; ++Index)
		{
			const float Middle = CatwalkSpanAngle(Index);
			if (IsCatwalkSpanLaid(Index) && (Way > 0.f ? Middle >= Foot && Middle < Nearest : Middle <= Foot && Middle > Nearest))
			{
				Nearest = Middle;
			}
		}
		return Nearest - Way * Half;
	}

	float HatchCatwalkEnd()
	{
		// The first laid span past the hatch stair's top, away from its foot: its near edge (the ring under the stair's top
		// isn't laid, and nothing comes down there).
		float Foot = 0.f;
		float Top = 0.f;
		HatchStairArc(Foot, Top);
		const float Way = Top > Foot ? 1.f : -1.f;
		const float Half = 180.f / CatwalkSpans * CatwalkSpanStretch;
		float Nearest = Top + Way * 360.f;
		for (int32 Index = 0; Index < CatwalkSpans; ++Index)
		{
			const float Middle = FMath::UnwindDegrees(CatwalkSpanAngle(Index));
			if (IsCatwalkSpanLaid(Index) && (Middle - Top) * Way > 0.f && FMath::Abs(Middle - Top) < FMath::Abs(Nearest - Top))
			{
				Nearest = Middle;
			}
		}
		return Nearest - Way * Half;
	}

	FStairRun FloorStair(int32 Index)
	{
		// Its top on the catwalk, its foot on the hall's floor.
		const int32 At = FMath::Clamp(Index, 0, NumFloorStairs - 1);
		const float Top = FloorStairAngles[At];
		return { HallPoint(Top, CatwalkMiddle, CatwalkUp), HallPoint(Top + FloorStairWays[At] * FloorStairSpan, CatwalkMiddle) };
	}

	float StairSlope(const FStairRun& Stair)
	{
		const FVector Run = Stair.To - Stair.From;
		return FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(Run.Z), FVector2D(Run.X, Run.Y).Size()));
	}

	FStairRun HatchStair()
	{
		// From the hole's edge behind its way (along Y), flush with the deck, on its line along her, down its way onto the
		// catwalk.
		const FBox2D Hole = HatchHole();
		const FVector2D Way = HatchWay.GetSafeNormal();
		const FVector2D Top(HatchStairX, Way.Y < 0.0 ? Hole.Max.Y : Hole.Min.Y);
		const double Deck = HatchDeckZ();
		const double Run = (Deck - CatwalkFloor) / FMath::Tan(FMath::DegreesToRadians(HatchStairSlope));
		return { FVector(Top, Deck), FVector(Top + Way * Run, CatwalkFloor) };
	}

	float HatchHoleLeast()
	{
		// Past the hole's far edge the ceiling's underside is over the stair: the feet must be StairHeadroom under it there.
		const float Under = HatchDeckZ() - (HallCeiling - CeilingThickness);
		return (StairHeadroom + Under) / FMath::Tan(FMath::DegreesToRadians(HatchStairSlope));
	}

	void HatchStairArc(float& OutFoot, float& OutTop)
	{
		const FStairRun Stair = HatchStair();
		OutFoot = HallAngleOf(Stair.To);
		OutTop = HallAngleOf(Stair.From);
	}

	FVector DeskSpot(float HalfDepth)
	{
		return HallPoint(DeskAngle, HallRadius - DeskGap - HalfDepth, CatwalkUp);
	}

	float DeskYaw()
	{
		// The bench's front (its local +X, where the stoker stands) faces the hall's middle, its tool wall the hall's wall.
		return OfficeAngle + DeskAngle + 180.f;
	}

	FVector DeskTop()
	{
		return DeskSpot(DeskFallbackHalf.X) + FVector(0.0, 0.0, DeskWorkTop);
	}

	FVector RadioSpot(const FVector& OnDesk)
	{
		// On the work top's back corner (toward the tool wall), along the ring: the clear middle stays the manifest's.
		return OnDesk - OutAt(DeskAngle) * RadioOnDesk.X + AlongAt(DeskAngle) * RadioOnDesk.Y + FVector(0.0, 0.0, RadioOnDesk.Z);
	}

	float RadioYaw()
	{
		// Its dials into the hall.
		return OfficeAngle + DeskAngle + 180.f;
	}

	FVector SafeSpot()
	{
		return HallPoint(SafeAngle, HallRadius - SafeGap - SafeFallbackHalf.X, CatwalkUp);
	}

	FVector OfficeLanternSpot()
	{
		// On the hall's wall over the stoker's desk.
		return HallPoint(DeskAngle, HallRadius, CatwalkUp + OfficeLanternUp);
	}

	bool IsNearAngle(float Angle, float Target, float Half)
	{
		const float Apart = FMath::Fmod(FMath::Abs(Angle - Target), 360.f);
		return FMath::Min(Apart, 360.f - Apart) < Half;
	}

	int32 HotGrates(int32 ValvesShut)
	{
		// Each valve shut takes a wedge's heat; the last one takes the rest: the ring goes cold with the furnace.
		if (ValvesShut >= NumValves)
		{
			return 0;
		}
		return FMath::Clamp(4 - FMath::Max(ValvesShut, 0), 0, 4);
	}

	uint32 HotMask(float Time, int32 ValvesShut)
	{
		const int32 Hot = HotGrates(ValvesShut);
		if (Hot <= 0)
		{
			return 0u;
		}
		// The wheel steps one wedge a turn; the hot ones spread evenly round it (4: every third; fewer: as evenly as they go).
		const int32 Turn = FMath::FloorToInt32(FMath::Max(Time, 0.f) / GrateTurnSeconds);
		uint32 Mask = 0u;
		for (int32 Index = 0; Index < Hot; ++Index)
		{
			const int32 Wedge = (Turn + Index * Grates / Hot) % Grates;
			Mask |= 1u << Wedge;
		}
		return Mask;
	}

	bool IsBurning(float Time)
	{
		return FMath::Fmod(FMath::Max(Time, 0.f), GrateTurnSeconds) >= GrateTellSeconds;
	}

	int32 GrateAt(const FVector2D& FromHallMiddle)
	{
		const double Radius = FromHallMiddle.Size();
		if (Radius < GrateInner || Radius > GrateOuter)
		{
			return INDEX_NONE;
		}
		double Angle = FMath::RadiansToDegrees(FMath::Atan2(FromHallMiddle.Y, FromHallMiddle.X));
		if (Angle < 0.0)
		{
			Angle += 360.0;
		}
		return FMath::Clamp(FMath::FloorToInt32(Angle / (360.0 / Grates)), 0, Grates - 1);
	}

	EVentPhase VentPhase(int32 Index, float Time)
	{
		const float Local = Time - Index * VentStagger;
		if (Local < 0.f)
		{
			return EVentPhase::Quiet;
		}
		const float Into = FMath::Fmod(Local, VentEvery);
		// The glow's tell at the end of its quiet, then the burn: so the last second before a burn always glows.
		const float BurnFrom = VentEvery - VentBurnSeconds;
		if (Into >= BurnFrom)
		{
			return EVentPhase::Burn;
		}
		if (Into >= BurnFrom - VentTellSeconds)
		{
			return EVentPhase::Tell;
		}
		return EVentPhase::Quiet;
	}

	float HoseReach(int32 Step, float SecondsInStep)
	{
		// Before the boarding it hangs halfway; it snakes lower while the Warden fights, and stops short of the roof's ridge.
		if (Step < 3)
		{
			return 0.55f;
		}
		const float Base = Step == 3 ? 0.6f : Step == 4 ? 0.75f : 0.85f;
		const float Cap = Step == 3 ? 0.75f : Step == 4 ? 0.85f : 0.94f;
		return FMath::Min(Base + HoseLowerPerSecond * FMath::Max(SecondsInStep, 0.f), Cap);
	}

	float DriftAt(float DyingSeconds)
	{
		// Easing in (the wind takes a dead ship slowly), then a steady drift, never past DriftMost.
		const float T = FMath::Max(DyingSeconds, 0.f);
		const float Eased = T < DriftEaseSeconds ? 0.5f * DriftSpeed * T * T / DriftEaseSeconds : DriftSpeed * (T - 0.5f * DriftEaseSeconds);
		return FMath::Min(Eased, DriftMost);
	}

	float ListAt(float DyingSeconds)
	{
		return ListDegrees * FMath::SmoothStep(0.f, ListSeconds, FMath::Max(DyingSeconds, 0.f));
	}

	FTransform FallPose(const FTransform& From, const FVector& To, float Seconds)
	{
		const float T = FMath::Clamp(Seconds, 0.f, FallSeconds);
		const FVector Start = From.GetLocation();
		// The glide: most of the way across, sinking a little, eased; then the plunge: the rest of the way, falling hard.
		const FVector Brink = FMath::Lerp(Start, To, 0.8) + FVector(0.0, 0.0, (Start.Z - To.Z) * 0.6);
		FVector Where;
		float Nose = 0.f;
		if (T < PlungeAt)
		{
			const float A = FMath::SmoothStep(0.f, 1.f, T / PlungeAt);
			Where = FMath::Lerp(Start, Brink, static_cast<double>(A));
			Nose = 6.f * A;
		}
		else
		{
			const float A = (T - PlungeAt) / (FallSeconds - PlungeAt);
			// Gravity's curve: slow off the brink, fast at the end.
			Where = FMath::Lerp(Brink, To, static_cast<double>(A * A));
			Nose = 6.f + 30.f * A;
		}
		// Nosing down (pitch, bow first) and rolling further onto her berth side as the dead weight goes (From already has
		// the dying list in it). A turn about +X lifts +Y: her berth side (-Y) goes down with a positive turn.
		const FQuat Turn = From.GetRotation() * FQuat(FVector::YAxisVector, FMath::DegreesToRadians(Nose))
			* FQuat(FVector::XAxisVector, FMath::DegreesToRadians(-BerthSide * 1.5f * T));
		return FTransform(Turn, Where, From.GetScale3D());
	}

	bool IsAboard(const FVector& BargeLocal)
	{
		// The whole of her: stern to bow, out to her beam, from under the hall to over her top.
		return BargeLocal.X >= HullAft - AboardMargin && BargeLocal.X <= HullFore + AboardMargin
			&& FMath::Abs(BargeLocal.Y) <= HullHalfBeam + AboardMargin
			&& BargeLocal.Z >= HallFloor - AboardMargin && BargeLocal.Z <= HullTop + AboardOverRoof;
	}
}
