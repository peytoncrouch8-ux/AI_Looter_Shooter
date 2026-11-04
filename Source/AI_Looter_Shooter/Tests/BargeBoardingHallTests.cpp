#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/BargeBoarding.h"
#include "World/BargeRules.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "Tests/BargeTestWorld.h"

// Main 6's barge below the deck (the user's "below deck, smaller", 2026-11-04): the hall in the hull's hollow under the
// casing, the hatch's stair from the cross walk down onto its catwalk, the stoker's corner there, the stairs down to the
// floor, each walked by a capsule; then the jumboed Tripo hull's own collision against the layout the kit is fitted to.

using namespace BargeTestWorld;

namespace
{
	/** A capsule walks a stair from its foot to its top a step at a time: the stair under its feet, Headroom clear over them.
	 *  Why (if given) gets where it first stopped and on what. */
	bool ClimbClear(const UWorld* World, const ABargeBoarding& Barge, const BargeRules::FStairRun& Stair, double Headroom, FString* Why = nullptr)
	{
		constexpr double Step = 50.0;
		const FVector Up = Stair.From - Stair.To;
		const double Length = Up.Size();
		bool bClear = true;
		double Floor = 0.0;
		for (double Along = 0.0; Along <= Length; Along += Step)
		{
			const FVector Feet = Stair.To + Up * (Along / Length);
			const FVector Over = Feet + FVector(0.0, 0.0, Step);
			const bool bFloored = FloorUnder(World, Barge, Over, Floor) && FMath::IsNearlyEqual(Floor, Feet.Z, 5.0);
			const bool bHead = HeadroomClear(World, Barge, Feet, Headroom);
			if ((!bFloored || !bHead) && Why && Why->IsEmpty())
			{
				const FString Under = bFloored ? FString() : TEXT(" floor: ") + FloorWhat(World, Barge, Over);
				const FString Overhead = bHead ? FString() : TEXT(" head in: ") + HeadroomWhat(World, Barge, Feet, Headroom);
				*Why = FString::Printf(TEXT("first at (%.0f, %.0f, %.0f)%s%s"), Feet.X, Feet.Y, Feet.Z, *Under, *Overhead);
			}
			bClear &= bFloored && bHead;
		}
		return bClear;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeHallTest, "Looter.World.Barge.Hall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeHallTest::RunTest(const FString& Parameters)
{
	using namespace BargeRules;
	// Below the deck (under the casing): the floor, 9 m of wall, the ceiling closing the hollow; inside her skin.
	TestTrue(TEXT("The hall is below the deck it's reached from, its walls the contract's 9 m"), HallCeiling < HatchDeckZ()
		&& HallFloor < HallCeiling && HallHeight >= HallLeastHeight);
	TestTrue(TEXT("...the catwalk under its ceiling with a man's headroom"), CatwalkUp + StairHeadroom <= HallHeight);
	const FBox2D Ceiling = HallCeilingArea();
	TestTrue(TEXT("The hall inside her skin, under her decks"), HallFits(HallRadius) && Ceiling.Max.Y <= HallSkinHalfBeam
		&& Ceiling.Min.Y >= -HallSkinHalfBeam && Ceiling.Min.X > HullAft && Ceiling.Max.X < HullFore);
	TestTrue(TEXT("The alcoves fit inside her skin, still past the hall's wall"), HallRadius + AlcoveDepth + AlcoveWall + SkinGap <= HallSkinHalfBeam + 0.01f
		&& AlcoveDepth > WallThickness);
	// The hatch: in the cross walk (or a deck over the hall), its stair along her beam from the hole's edge, flush with the
	// deck, down onto the catwalk at no more than the stairs' slope; through the hole at its top, inside the hall's wall all
	// the way, over the catwalk ring; the hole long enough for a head to clear the ceiling past it.
	const FBox2D Hole = HatchHole();
	const FStairRun Hatch = HatchStair();
	const FVector2D Way = HatchWay.GetSafeNormal();
	const FDeckZone* HatchDeck = DeckZoneAt(Hole.GetCenter());
	TestTrue(TEXT("The hatch is in the cross walk, its stair along her beam"), HatchDeck && HatchDeck->Kind == EDeckZone::CrossWalk
		&& FMath::IsNearlyZero(Way.X));
	TestTrue(TEXT("...its stair walkable, flush with the deck, down onto the catwalk, the hole long enough"), StairSlope(Hatch) <= StairMostSlope
		&& FMath::IsNearlyEqual(Hatch.From.Z, static_cast<double>(HatchDeckZ())) && FMath::IsNearlyEqual(Hatch.To.Z, static_cast<double>(CatwalkFloor))
		&& Hole.GetSize().Y >= HatchHoleLeast());
	const FVector Across(-Way.Y, Way.X, 0.0);
	bool bInHall = Hatch.From.X - HatchStairWidth * 0.5 >= Hole.Min.X && Hatch.From.X + HatchStairWidth * 0.5 <= Hole.Max.X;
	for (const FVector& End : { Hatch.From, Hatch.To })
	{
		for (const double Sign : { -1.0, 1.0 })
		{
			const FVector Corner = End + Across * (Sign * HatchStairWidth * 0.5) - HallCenter;
			const double Out = FVector2D(Corner.X, Corner.Y).Size();
			bInHall &= Out < HallRadius - SkinGap && Out > CatwalkInner;
		}
	}
	TestTrue(TEXT("...through the hole at its top, over the catwalk inside the hall's wall all the way"), bInHall);
	// Nothing at the hall's wall under the hatch's stair (no valve, stokehold or floor stair: the stair comes down there).
	float ArcFoot = 0.f;
	float ArcTop = 0.f;
	HatchStairArc(ArcFoot, ArcTop);
	const auto UnderHatch = [ArcFoot, ArcTop](float Angle)
	{
		const float Unwound = FMath::UnwindDegrees(Angle);
		return Unwound > FMath::Min(ArcFoot, ArcTop) - DoorGapDegrees && Unwound < FMath::Max(ArcFoot, ArcTop) + DoorGapDegrees;
	};
	bool bUnderClear = true;
	for (const float Angle : ValveAngles) { bUnderClear &= !UnderHatch(Angle); }
	for (const float Angle : StokeholdAngles) { bUnderClear &= !UnderHatch(Angle); }
	// The two floor stairs: walkable, from the catwalk to the floor, their feet off the grate ring and inside the wall; the
	// catwalk open over them and under the hatch's stair, laid past their tops, ended past their feet.
	bool bFloorStairs = true;
	int32 OpenSpans = 0;
	int32 OverStairs = 0;
	const double SpanDegrees = 360.0 / CatwalkSpans;
	for (int32 Index = 0; Index < CatwalkSpans; ++Index)
	{
		OpenSpans += IsCatwalkSpanLaid(Index) ? 0 : 1;
		// Over a stair: the span's middle over the hatch stair's arc or a floor stair's.
		const float SpanMiddle = CatwalkSpanAngle(Index);
		const float Unwound = FMath::UnwindDegrees(SpanMiddle);
		bool bOver = Unwound > FMath::Min(ArcFoot, ArcTop) && Unwound < FMath::Max(ArcFoot, ArcTop);
		for (int32 StairIndex = 0; StairIndex < NumFloorStairs; ++StairIndex)
		{
			const float StairTop = FloorStairAngles[StairIndex];
			const float StairFoot = StairTop + FloorStairWays[StairIndex] * FloorStairSpan;
			bOver |= SpanMiddle > FMath::Min(StairTop, StairFoot) && SpanMiddle < FMath::Max(StairTop, StairFoot);
		}
		OverStairs += bOver ? 1 : 0;
	}
	for (int32 Index = 0; Index < NumFloorStairs; ++Index)
	{
		const FStairRun Stair = FloorStair(Index);
		const FVector Run = (Stair.To - Stair.From).GetSafeNormal2D();
		const FVector Side(-Run.Y, Run.X, 0.0);
		const double Top = FloorStairAngles[Index];
		const double FootAngle = Top + FloorStairWays[Index] * FloorStairSpan;
		bUnderClear &= !UnderHatch(static_cast<float>(Top)) && !UnderHatch(static_cast<float>(FootAngle));
		for (const double Sign : { -1.0, 1.0 })
		{
			const FVector Corner = Stair.To + Side * (Sign * StairWidth * 0.5) - HallCenter;
			bFloorStairs &= FVector2D(Corner.X, Corner.Y).Size() > GrateOuter && FVector2D(Corner.X, Corner.Y).Size() < HallRadius;
		}
		bFloorStairs &= StairSlope(Stair) <= StairMostSlope && FMath::IsNearlyEqual(static_cast<float>(Stair.To.Z), HallFloor, 1.f)
			&& FMath::IsNearlyEqual(static_cast<float>(Stair.From.Z), HallFloor + CatwalkUp, 1.f);
		// The landing span past the top laid; the ring past the foot laid only from where the stair under it is a man's
		// headroom below the catwalk's underside (a foot mid-span: the ring runs on over the stair's low end).
		const double Landing = Top - FloorStairWays[Index] * SpanDegrees * 0.5;
		const double EndDown = FMath::Clamp((CatwalkEndAngle(Index) - Top) * FloorStairWays[Index] / FloorStairSpan, 0.0, 1.0);
		bFloorStairs &= IsCatwalkSpanLaid(FMath::FloorToInt32(Landing / SpanDegrees))
			&& CatwalkUp * (1.0 - EndDown) + StairHeadroom <= CatwalkUp - CatwalkThick;
	}
	TestTrue(TEXT("Nothing at the wall under the hatch's stair"), bUnderClear);
	TestTrue(TEXT("The floor stairs walkable, their feet off the grates; the catwalk open over them and under the hatch's stair"),
		bFloorStairs && OpenSpans == OverStairs);
	// The stoker's corner on the catwalk: the desk and the safe on laid spans against the wall, off every stair, the
	// walkway past them; the radio on the desk.
	const auto OnLaidRing = [SpanDegrees](float Angle)
	{
		return IsCatwalkSpanLaid(FMath::FloorToInt32(FMath::Fmod(Angle + 360.f, 360.f) / SpanDegrees));
	};
	const FVector Desk = DeskTop();
	const FVector Radio = RadioSpot(Desk);
	const FVector DeskOut = Desk - HallCenter;
	TestTrue(TEXT("The stoker's desk and safe on the catwalk by the wall, off the stairs, a walkway inside them"), OnLaidRing(DeskAngle)
		&& OnLaidRing(SafeAngle) && !UnderHatch(DeskAngle) && !UnderHatch(SafeAngle)
		&& FVector2D(DeskOut.X, DeskOut.Y).Size() - DeskFallbackHalf.X - CatwalkInner >= 2.0 * Shoulders
		&& FMath::IsNearlyEqual(static_cast<float>(Desk.Z), CatwalkFloor + DeskFallbackHalf.Z * 2.f, 1.f));
	TestTrue(TEXT("...the radio on the desk"), FVector2D(Radio.X - Desk.X, Radio.Y - Desk.Y).Size() < DeskFallbackHalf.Y && Radio.Z > Desk.Z);

	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, ManifestStep);
	if (!Barge)
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	const double Keel = Barge->GetActorLocation().Z;
	const FTransform Her = Barge->GetActorTransform();
	const double Under = HallCeiling - CeilingThickness;
	// The ceiling over the hall.
	FVector Hit;
	double Floor = 0.0;
	TestTrue(TEXT("The hall has its ceiling closing the hollow"), IronOver(World, Her.TransformPosition(HallPoint(135.f, CatwalkInner - 100.f, 100.f)), 2000.0, Hit)
		&& FMath::IsNearlyEqual(Hit.Z, Keel + Under, 5.0));
	// The way in, a capsule all the way: down the hatch's stair from the cross walk (headroom past the hole) onto the
	// catwalk, along the ring past the stoker's desk to the port floor stair's top, floored.
	const double Head = StairHeadroom - OverFeet;
	FString HatchWhy;
	const bool bHatchClimb = ClimbClear(World, *Barge, Hatch, Head, &HatchWhy);
	TestTrue(FString::Printf(TEXT("Down the hatch's stair, its floor under every step, headroom all the way (%s)"), *HatchWhy), bHatchClimb);
	const FVector OffStair = Hatch.To + FVector(Way, 0.0) * 50.0;
	const FVector ByDesk = HallPoint(DeskAngle, CatwalkInner + Shoulders + 10.f, CatwalkUp);
	const FVector AtStair = HallPoint(FloorStairAngles[1] + 5.f, CatwalkMiddle, CatwalkUp);
	const FVector Chest(0.0, 0.0, 100.0);
	bool bWay = WalkClear(World, *Barge, OffStair + Chest, ByDesk + Chest) && WalkClear(World, *Barge, ByDesk + Chest, AtStair + Chest);
	for (const FVector& At : { OffStair, ByDesk, AtStair })
	{
		bWay &= FloorUnder(World, *Barge, At + FVector(0.0, 0.0, 50.0), Floor) && FMath::IsNearlyEqual(Floor, static_cast<double>(CatwalkFloor), 2.0)
			&& HeadroomClear(World, *Barge, At, Head);
	}
	TestTrue(TEXT("...along the catwalk past the stoker's desk to the floor stair, floored"), bWay);
	// Down each floor stair from the catwalk, the catwalk past its top, the ring's ends walled (past each stair's foot and
	// past the hatch stair's top), so nobody walks off it into the drop.
	bool bClimb = true;
	bool bEnds = true;
	const auto EndBlocks = [World, &Her](float End, float RingWay)
	{
		const FVector OnRing = Her.TransformPosition(HallPoint(End + RingWay * 10.f, CatwalkMiddle, CatwalkUp + StandUp));
		const FVector OverDrop = Her.TransformPosition(HallPoint(End - RingWay * 10.f, CatwalkMiddle, CatwalkUp + StandUp));
		return World->LineTraceTestByChannel(OnRing, OverDrop, ECC_Pawn);
	};
	for (int32 Index = 0; Index < NumFloorStairs; ++Index)
	{
		const FStairRun Stair = FloorStair(Index);
		bClimb &= ClimbClear(World, *Barge, Stair, Head);
		const FVector Beyond = Stair.From + (Stair.From - Stair.To).GetSafeNormal2D() * 50.0;
		bClimb &= FloorUnder(World, *Barge, Beyond + FVector(0.0, 0.0, 50.0), Floor) && FMath::IsNearlyEqual(Floor, static_cast<double>(CatwalkFloor), 2.0);
		bEnds &= EndBlocks(CatwalkEndAngle(Index), FloorStairWays[Index]);
	}
	bEnds &= EndBlocks(HatchCatwalkEnd(), ArcTop > ArcFoot ? 1.f : -1.f);
	TestTrue(TEXT("Down each floor stair from the catwalk, headroom all the way"), bClimb);
	TestTrue(TEXT("...and the ring's ends walled"), bEnds);
	// After the manifest she puts a player back by the hatch, on the cross walk behind its stair.
	const FVector StartOnHer = Her.InverseTransformPosition(Barge->GetPlayerStart().GetLocation());
	const bool bStartOpen = IsOpenDeck(FVector2D(StartOnHer.X, StartOnHer.Y));
	const bool bStartFloored = FloorUnder(World, *Barge, StartOnHer, Floor) && FMath::IsNearlyEqual(Floor, static_cast<double>(HatchDeckZ()), 5.0);
	TestTrue(FString::Printf(TEXT("Put back by the hatch: on open deck (at (%.0f, %.0f, %.0f)%s, floor: %s)"), StartOnHer.X, StartOnHer.Y, StartOnHer.Z,
		bStartOpen ? TEXT("") : TEXT(", off the open deck"), *FloorWhat(World, *Barge, StartOnHer)), bStartOpen && bStartFloored);
	// The desk as laid (the bench at its size) on the catwalk inside the wall; the manifest's spot on its top.
	const FBox DeskBox = Barge->Desk ? BoxOnHer(*Barge, *Barge->Desk) : FBox(ForceInit);
	const FVector OfficeSpot = Her.InverseTransformPosition(Barge->GetOfficeSpot());
	const FVector DeskMiddle = DeskBox.GetCenter() - HallCenter;
	TestTrue(TEXT("The desk on the catwalk inside the wall; the manifest on its top"), Barge->Desk
		&& FVector2D(DeskMiddle.X, DeskMiddle.Y).Size() < HallRadius && FVector2D(DeskMiddle.X, DeskMiddle.Y).Size() > CatwalkInner
		&& FMath::IsNearlyEqual(DeskBox.Min.Z, static_cast<double>(CatwalkFloor), 2.0)
		&& FMath::IsNearlyEqual(OfficeSpot.Z, DeskBox.Max.Z, 1.0) && OfficeSpot.Z > CatwalkFloor + 50.0);
	// The lanterns' lights: the hall's four and the one over the desk, each inside the hall.
	bool bLit = Barge->GetLanterns().Num() == static_cast<int32>(UE_ARRAY_COUNT(HallLanternAngles)) + 1;
	for (const UPointLightComponent* Light : Barge->GetLanterns())
	{
		const FVector At = Light ? Light->GetRelativeLocation() : FVector::ZeroVector;
		bLit &= Light && !Light->CastShadows && FVector2D(At.X - HallCenter.X, At.Y - HallCenter.Y).Size() <= HallRadius + 1.0
			&& At.Z > HallFloor && At.Z < Under;
	}
	TestTrue(TEXT("Lanterns light the hall, shadowless"), bLit);
	// Nothing of the alcoves' iron pokes past her skin (the barge stands unturned: her Y is the world's).
	bool bAlcovesIn = true;
	int32 AlcoveParts = 0;
	for (UActorComponent* Part : Barge->GetComponents())
	{
		const UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Part);
		if (Prim && Prim->GetName().StartsWith(TEXT("Alcove")))
		{
			const FBox Box = BoxOf(*Prim);
			bAlcovesIn &= FMath::Max(FMath::Abs(Box.Min.Y), FMath::Abs(Box.Max.Y)) <= HallSkinHalfBeam - SkinGap + 1.0;
			++AlcoveParts;
		}
	}
	TestTrue(TEXT("The alcoves' iron is inside her skin"), AlcoveParts > 0 && bAlcovesIn);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeHullTest, "Looter.World.Barge.Hull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeHullTest::RunTest(const FString& Parameters)
{
	// The jumboed Tripo hull's own collision against the layout the kit is fitted to (BARGE_LAYOUT.md): what breaks when the
	// model is reworked or exported again. Traced against the hull alone (no kit piece answers).
	using namespace BargeRules;
	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, DeckStep);
	if (!Barge)
	{
		return false;
	}
	if (!Barge->HasHull())
	{
		AddInfo(FString::Printf(TEXT("%s isn't imported: the hull's collision isn't checked (she stands on the stand-in box)."), ABargeBoarding::HullPath));
		return true;
	}
	const FTransform Her = Barge->GetActorTransform();
	UPrimitiveComponent* Hull = Barge->Hull.Get();
	FHitResult Hit;
	const auto HitsHull = [Hull, &Her, &Hit](const FVector& From, const FVector& To)
	{
		return Hull->LineTraceComponent(Hit, Her.TransformPosition(From), Her.TransformPosition(To), FCollisionQueryParams(SCENE_QUERY_STAT(BargeHullTest), false));
	};
	const auto HitZ = [&Her, &Hit]() { return Her.InverseTransformPosition(Hit.ImpactPoint).Z; };
	constexpr double Hand = 20.0;
	TestTrue(TEXT("Over the hull the kit lays no deck plate"), Barge->GetDeckPlates().Num() == 0);
	// Each zone's deck at its height: its corners a hand in, and its middle.
	// To the hand every floor check holds (the zones' heights sit between the hull's tops and the level's drops: BargeRules'
	// "The decks' heights"); the info lines give both at each zone's first point, the failures the hull's top.
	constexpr double DeckHand = 5.0;
	UWorld* World = TestLevel.GetTestWorld();
	FString Off;
	for (const FDeckZone& Zone : DeckZones)
	{
		const FBox2D Box = Zone.Box().ExpandBy(-Hand);
		bool bTold = false;
		for (const FVector2D& At : { Box.Min, Box.Max, FVector2D(Box.Min.X, Box.Max.Y), FVector2D(Box.Max.X, Box.Min.Y), Box.GetCenter() })
		{
			if (HatchCoaming().IsInside(At))
			{
				continue;
			}
			const bool bTop = HitsHull(FVector(At, Zone.Z + 200.0), FVector(At, Zone.Z - 200.0));
			const double Top = bTop ? HitZ() : 0.0;
			if (!bTop || !FMath::IsNearlyEqual(Top, static_cast<double>(Zone.Z), DeckHand))
			{
				Off += bTop ? FString::Printf(TEXT("%s (%.0f, %.0f) at %.1f "), Zone.Name, At.X, At.Y, Top) : FString::Printf(TEXT("%s (%.0f, %.0f) none "), Zone.Name, At.X, At.Y);
			}
			if (!bTold)
			{
				double Drop = 0.0;
				const bool bDrop = FloorUnder(World, *Barge, FVector(At, Zone.Z + 200.0), Drop);
				AddInfo(FString::Printf(TEXT("%s at (%.0f, %.0f): the hull's top %.1f, the level's drop %.1f%s"), Zone.Name, At.X, At.Y, Top, Drop,
					bDrop ? TEXT("") : TEXT(" (none)")));
				bTold = true;
			}
		}
	}
	TestTrue(FString::Printf(TEXT("Her decks at their zones' heights (%s)"), *Off), Off.IsEmpty());
	// The rail's gap at the berth: open a hand inside its posts, closed a hand outside them (across the rail at half its height).
	const double Waist = WellDeckZ + RailHeight * 0.5;
	const auto AcrossRail = [&](double X)
	{
		return HitsHull(FVector(X, BerthSide * (WellDeckHalfBeam - Hand), Waist), FVector(X, BerthSide * (BulwarkHalfBeam + Hand), Waist));
	};
	TestTrue(TEXT("The rail's gap is open at the berth"), !AcrossRail(BerthX - GapHalf + Hand) && !AcrossRail(BerthX + GapHalf - Hand));
	TestTrue(TEXT("...and the rail closed past it"), AcrossRail(BerthX - GapHalf - Hand) && AcrossRail(BerthX + GapHalf + Hand));
	// The hatch: its hole open from the deck down into the hall; its coaming round three sides, open where the stair starts.
	const FBox2D Hole = HatchHole();
	const double Deck = HatchDeckZ();
	TestTrue(TEXT("The hatch's hole is open down into the hall"), !HitsHull(FVector(Hole.GetCenter(), Deck + 100.0), FVector(Hole.GetCenter(), CatwalkFloor + 20.0)));
	const FVector2D Open = -HatchWay.GetSafeNormal();
	bool bCoaming = true;
	for (const FVector2D& Out : { FVector2D(1.0, 0.0), FVector2D(-1.0, 0.0), FVector2D(0.0, 1.0), FVector2D(0.0, -1.0) })
	{
		const FVector2D Edge = Hole.GetCenter() + Out * (Hole.GetExtent() * Out.GetAbs()).Size();
		const bool bRim = HitsHull(FVector(Edge - Out * Hand, Deck + HatchCoamingUp * 0.5), FVector(Edge + Out * (HatchCoamingThick + Hand), Deck + HatchCoamingUp * 0.5));
		bCoaming &= (Out - Open).IsNearlyZero(0.01) ? !bRim : bRim;
	}
	TestTrue(TEXT("...its coaming round it, open on the stair's side"), bCoaming);
	// The hollow: nothing of the hull in the hall (across it and along it, low, at the catwalk, under the ceiling, and on
	// the diagonals) or over the hatch's stair; no hull surface over the hall's floor (the kit's ceiling closes its top).
	const double In = HallRadius - Hand;
	bool bHollow = true;
	for (const double Up : { 100.0, double(CatwalkUp + StandUp), double(HallHeight - 50.f) })
	{
		for (const float Angle : { 0.f, 45.f, 90.f, 135.f })
		{
			bHollow &= !HitsHull(HallPoint(Angle, In, Up), HallPoint(Angle + 180.f, In, Up));
		}
	}
	const FStairRun Hatch = HatchStair();
	bHollow &= !HitsHull(Hatch.To + FVector(0.0, 0.0, StandUp), Hatch.From + FVector(0.0, 0.0, StandUp - (Deck - (HallCeiling - CeilingThickness)) - Hand));
	TestTrue(TEXT("Nothing of the hull in the hall or over the hatch's stair"), bHollow);
	TestFalse(TEXT("...and nothing of it over the hall's floor"), HitsHull(HallPoint(30.f, In * 0.5f, 50.f), HallPoint(30.f, In * 0.5f, -2.f)));
	// The walks clear end to end, along their middles and a hand in from each side (the flanks).
	bool bWalks = true;
	for (const FDeckZone& Zone : DeckZones)
	{
		if (Zone.Kind != EDeckZone::Walk)
		{
			continue;
		}
		for (const double Y : { Zone.YMin + Hand, (Zone.YMin + Zone.YMax) * 0.5, Zone.YMax - Hand })
		{
			bWalks &= !HitsHull(FVector(Zone.XMin + Hand, Y, Zone.Z + StandUp), FVector(Zone.XMax - Hand, Y, Zone.Z + StandUp));
		}
	}
	TestTrue(TEXT("The house walks are clear end to end"), bWalks);
	return true;
}

#endif
