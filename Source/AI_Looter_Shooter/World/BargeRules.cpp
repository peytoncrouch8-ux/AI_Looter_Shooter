// BargeRules: the boarding stage's numbers as plain math (Main 6, "The Pilot Light").

#include "World/BargeRules.h"

namespace BargeRules
{
	FName Mission()
	{
		static const FName Name(TEXT("Main6"));
		return Name;
	}

	FName DrumJammedFlag()
	{
		static const FName Name(TEXT("Pilot.DrumJammed"));
		return Name;
	}

	FName BargeFellFlag()
	{
		static const FName Name(TEXT("Pilot.BargeFell"));
		return Name;
	}

	FBox2D VestibuleArea()
	{
		// From its aft wall to the lining just inside the house's fore wall, between its side walls.
		return FBox2D(FVector2D(VestibuleAft, -VestibuleHalfWidth), FVector2D(HouseFore - HouseWallThick - VestibuleWall, VestibuleHalfWidth));
	}

	TArray<FRailWall> RailWallLayout()
	{
		const auto Box = [](double XMin, double XMax, double YA, double YB)
		{
			return FBox2D(FVector2D(XMin, FMath::Min(YA, YB)), FVector2D(XMax, FMath::Max(YA, YB)));
		};
		// On the bulwark itself (its inner face to its outer), standing over its 110 cm; the far side whole, the berth side
		// either side of its gap; then across the bow. The house is the full beam: nothing to close beside it.
		const double In = BulwarkHalfBeam - BulwarkThick;
		const double Out = BulwarkHalfBeam;
		const double Side = BerthSide;
		return {
			{ TEXT("RailWallFar"), Box(HouseFore, DeckFore, -Side * In, -Side * Out) },
			{ TEXT("RailWallBerthFore"), Box(BerthX + GapHalf, DeckFore, Side * In, Side * Out) },
			{ TEXT("RailWallBerthAft"), Box(HouseFore, BerthX - GapHalf, Side * In, Side * Out) },
			{ TEXT("RailWallBow"), Box(DeckFore, DeckFore + RailWallThick, -Out, Out) },
		};
	}

	bool IsOpenDeck(const FVector2D& At)
	{
		if (At.X < DeckAft || At.X > DeckFore || FMath::Abs(At.Y) > DeckHalfBeam)
		{
			return false;
		}
		for (const FDeckBlock& Block : DeckBlocks())
		{
			if (At.X >= Block.XMin && At.X <= Block.XMax && At.Y >= Block.YMin && At.Y <= Block.YMax)
			{
				return false;
			}
		}
		return true;
	}

	float PlankSillClearance()
	{
		// Her hinge (her deck's edge at her rail) to the landing (BoardingInset in from the arena's edge), measured outward
		// from the barge's middle; the plank's floor where it crosses the bulwark's inner face, over the gap's sill.
		const float HingeOut = BerthOut - ShipRailOut;
		const float HingeUp = DeckTop - BerthUnderDeck;
		const float LandOut = DeckHalfBeam - BoardingInset;
		const float CrossOut = BulwarkHalfBeam - BulwarkThick;
		const float Share = (CrossOut - LandOut) / FMath::Max(HingeOut - LandOut, 1.f);
		const float Line = FMath::Lerp(DeckTop, HingeUp, Share);
		return Line - PlankUnderside - (DeckTop + SillUp);
	}

	float BerthBagClearance()
	{
		return BerthOut - ShipBagHalfBeam - BulwarkHalfBeam;
	}

	float JumpBagClearance()
	{
		// The bulwark's top on her side, from her keel at the jump berth (both in the barge's frame), turned by the full list
		// (the barge's turn about her +X; her berth side goes down) into her level frame: how far inboard of her keel line it
		// ends up, less her bag's half beam.
		const FVector2D Bulwark(BerthSide * BulwarkHalfBeam, DeckTop + RailHeight);
		const FVector2D Keel(BerthSide * JumpOut, DeckTop - JumpUnderDeck - ShipDeckOverKeel);
		const FVector2D Rel = Bulwark - Keel;
		const float Turn = FMath::DegreesToRadians(-BerthSide * ListDegrees);
		const float Across = Rel.X * FMath::Cos(Turn) - Rel.Y * FMath::Sin(Turn);
		return -BerthSide * Across - ShipBagHalfBeam;
	}

	bool HallFits(float Radius)
	{
		return Radius > 0.f && Radius + WallThickness + HullWall <= HallSkinHalfBeam;
	}

	FVector HallPoint(float AngleDegrees, float Radius, float Up)
	{
		const float Turn = FMath::DegreesToRadians(AngleDegrees);
		return HallCenter + FVector(FMath::Cos(Turn) * Radius, FMath::Sin(Turn) * Radius, HallFloor + Up);
	}

	FBox2D HallCeilingArea()
	{
		// The hall to its wall's outer face (the office and the vestibule have their own roofs).
		const double Out = HallRadius + WallThickness;
		return FBox2D(FVector2D(HallCenter.X - Out, HallCenter.Y - Out), FVector2D(HallCenter.X + Out, HallCenter.Y + Out));
	}

	FBox2D OfficeArea()
	{
		return FBox2D(FVector2D(HallCenter.X + HallRadius, HallCenter.Y - OfficeHalfWidth),
			FVector2D(HallCenter.X + HallRadius + OfficeDepth, HallCenter.Y + OfficeHalfWidth));
	}

	FStairRun FloorStair(int32 Index)
	{
		// Its top on the catwalk, its foot on the hall's floor's top (a hair over the deck).
		const int32 At = FMath::Clamp(Index, 0, NumFloorStairs - 1);
		const float Top = FloorStairAngles[At];
		return { HallPoint(Top, CatwalkMiddle, CatwalkUp), HallPoint(Top + FloorStairWays[At] * FloorStairSpan, CatwalkMiddle, InteriorFloorLift) };
	}

	float CatwalkSpanAngle(int32 Index)
	{
		return (Index + 0.5f) * 360.f / CatwalkSpans;
	}

	bool IsCatwalkSpanLaid(int32 Index)
	{
		// A span whose middle lies over a stair's arc would roof the climb (the stairs run under the ring's middle): open
		// there. The stairs don't cross 0 degrees.
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
		return true;
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

	float StairSlope(const FStairRun& Stair)
	{
		const FVector Run = Stair.To - Stair.From;
		return FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(Run.Z), FVector2D(Run.X, Run.Y).Size()));
	}

	FVector HatchSpot()
	{
		return FVector(HouseFore, 0.0, DeckTop);
	}

	FVector DeskTop()
	{
		// Out from the hall's wall into the office (on the hall's floor's level), then across it toward the port end.
		const float Turn = FMath::DegreesToRadians(OfficeAngle);
		const FVector Across(-FMath::Sin(Turn), FMath::Cos(Turn), 0.0);
		return HallPoint(OfficeAngle, HallRadius + DeskOut, InteriorFloorLift + DeskTopUp) + Across * DeskAcross;
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
		// The whole of her: the house (and the hall in it) aft, the arena and the bow fore, out to her bulwark.
		return BargeLocal.X >= HouseAft - 100.f && BargeLocal.X <= DeckFore + 150.f && FMath::Abs(BargeLocal.Y) <= BulwarkHalfBeam + 60.f
			&& BargeLocal.Z >= HallFloor - 150.f && BargeLocal.Z <= HouseRoof + AboardOverRoof;
	}
}
