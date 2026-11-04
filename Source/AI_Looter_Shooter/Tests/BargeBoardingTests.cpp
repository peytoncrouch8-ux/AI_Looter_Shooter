#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/BargeBoarding.h"
#include "World/BargeFurnace.h"
#include "World/BargeHoseDrum.h"
#include "World/BargeRules.h"
#include "World/FurnaceDraftValve.h"
#include "World/LamplighterShip.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "Tests/BargeTestWorld.h"

// Main 6's barge (the Wardens story, lane MAIN67's W3): her kit on the reworked Tripo hull (the deck's arena, the berth, the
// vestibule, the office and the hall at deck level in the house), each in a test level with no story (ForceStep plays her as
// if Main 6 were on a step). With SM_DredgeBarge not imported she stands on the stand-in box and the kit's own deck plate:
// the same tests hold (but .Hull, which skips).
// Her beats (the grates, valves, cage, dying, fall): BargeBoardingBeatsTests.cpp.

using BargeTestWorld::GroundUnder;
using BargeTestWorld::SpawnBarge;

namespace
{
	/** Where a straight climb from From meets world-static iron within Reach, or false. */
	bool IronOver(const UWorld* World, const FVector& From, double Reach, FVector& OutHit)
	{
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, From, From + FVector(0.0, 0.0, Reach), FCollisionObjectQueryParams(ECC_WorldStatic)))
		{
			OutHit = Hit.ImpactPoint;
			return true;
		}
		return false;
	}

	/** True when a pawn's way from From to To (her frame, at a walker's chest height over the deck) is clear. */
	bool WalkClear(const UWorld* World, const ABargeBoarding& Barge, const FVector2D& From, const FVector2D& To)
	{
		const double Chest = BargeRules::DeckTop + 100.0;
		const FVector A = Barge.GetActorTransform().TransformPosition(FVector(From.X, From.Y, Chest));
		const FVector B = Barge.GetActorTransform().TransformPosition(FVector(To.X, To.Y, Chest));
		return !World->LineTraceTestByChannel(A, B, ECC_Pawn);
	}

	/** True when a walker with his feet at Feet (her frame, on a surface) has Headroom over him: a capsule of a player's
	 *  shoulders, from a little over his feet up Headroom, meets nothing a pawn bumps into. */
	bool HeadroomClear(const UWorld* World, const ABargeBoarding& Barge, const FVector& Feet, double Headroom)
	{
		// The player's capsule (34 x 0.85) to the nearest hand; clear of the step under him and of a 33 degree slope.
		constexpr float Shoulders = 30.f;
		constexpr double OverFeet = 30.0;
		const float Half = static_cast<float>(Headroom * 0.5);
		const FVector Middle = Barge.GetActorTransform().TransformPosition(Feet + FVector(0.0, 0.0, OverFeet + Half));
		return !World->OverlapAnyTestByChannel(Middle, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Shoulders, Half));
	}

	/** A component's box now (computed, not the cached bounds: instanced looks laid this frame count). */
	FBox BoxOf(const UPrimitiveComponent& Part)
	{
		return Part.CalcBounds(Part.GetComponentTransform()).GetBox();
	}

	/** A flat box round a deck piece (centre, half size). */
	FBox2D Around(const FVector& Spot, double HalfX, double HalfY)
	{
		return FBox2D(FVector2D(Spot.X - HalfX, Spot.Y - HalfY), FVector2D(Spot.X + HalfX, Spot.Y + HalfY));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeKitTest, "Looter.World.Barge.Kit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeKitTest::RunTest(const FString& Parameters)
{
	// The design's 26 m hall doesn't fit the 24.75 m hull; the 22 m one does, with the hull's wall all round.
	TestFalse(TEXT("A 26 m hall wouldn't fit the hull"), BargeRules::HallFits(BargeRules::DesignHallRadius));
	TestTrue(TEXT("The 22 m hall fits"), BargeRules::HallFits(BargeRules::HallRadius));
	TestTrue(TEXT("The deck's arena lies fore of the house, inside her bulwark"), BargeRules::DeckAft < BargeRules::DeckFore
		&& BargeRules::DeckAft >= BargeRules::HouseFore && BargeRules::DeckFore < BargeRules::HullLength * 0.5f
		&& BargeRules::DeckHalfBeam < BargeRules::BulwarkHalfBeam - BargeRules::BulwarkThick);
	// The berth against the hull's bulwark (110 high, its gap's sill at the berth): the plank runs over the sill, her bag clears
	// the bulwark alongside and, at the jump, with the barge listed toward her.
	TestTrue(TEXT("The plank clears the bulwark gap's sill"), BargeRules::PlankSillClearance() > 0.f);
	TestTrue(TEXT("Her bag clears the bulwark at the berth"), BargeRules::BerthBagClearance() > 0.f);
	TestTrue(TEXT("...and at the jump, the barge listed toward her"), BargeRules::JumpBagClearance() > 0.f);
	TestTrue(TEXT("The ship's numbers are the ship's"), FMath::IsNearlyEqual(BargeRules::ShipDeckOverKeel, ALamplighterShip::DeckTop)
		&& FMath::IsNearlyEqual(BargeRules::ShipRailOut, ALamplighterShip::RailOut));

	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, BargeRules::DeckStep);
	if (!Barge)
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	TestTrue(TEXT("Shown as the deck fight's step"), Barge->IsPresent() && Barge->GetState() == EBargeState::Hanging);
	TestTrue(TEXT("Her furnace, three valves, the drum"), Barge->GetFurnace() && Barge->GetValves().Num() == BargeRules::NumValves && Barge->GetHoseDrum());
	// The rail walls as the rules lay them out, each where its footprint says, as tall as the rules have them.
	const TArray<BargeRules::FRailWall> Layout = BargeRules::RailWallLayout();
	bool bWallsLaid = Barge->GetRailWalls().Num() == Layout.Num();
	for (int32 Index = 0; bWallsLaid && Index < Layout.Num(); ++Index)
	{
		const UBoxComponent* Wall = Barge->GetRailWalls()[Index];
		const FVector Middle = Wall ? Wall->GetRelativeLocation() : FVector::ZeroVector;
		const FVector Half = Wall ? Wall->GetUnscaledBoxExtent() : FVector::ZeroVector;
		bWallsLaid &= Wall && Wall->GetName() == Layout[Index].Name && FVector2D(Middle.X, Middle.Y).Equals(Layout[Index].Footprint.GetCenter(), 0.5)
			&& FVector2D(Half.X, Half.Y).Equals(Layout[Index].Footprint.GetExtent(), 0.5) && FMath::IsNearlyEqual(Half.Z * 2.0, static_cast<double>(BargeRules::RailWallHeight), 0.5);
	}
	TestTrue(TEXT("Rail walls on her bulwark and across the bow, as the rules lay them out"), bWallsLaid);
	FVector Hit;
	TestTrue(TEXT("The plank's landing is on her deck"), GroundUnder(World, Barge->GetBoardingPoint() + FVector(0.0, 0.0, 150.0), Hit)
		&& FMath::IsNearlyEqual(static_cast<float>(Hit.Z), static_cast<float>(Barge->GetBoardingPoint().Z), 5.f));
	TestTrue(TEXT("The hall's floor is under the grate ring (past the furnace)"), GroundUnder(World, Barge->GetHallMiddle() + FVector(650.0, 0.0, 150.0), Hit)
		&& FMath::IsNearlyEqual(static_cast<float>(Hit.Z), static_cast<float>(Barge->GetActorLocation().Z + BargeRules::HallFloor), 5.f));
	// Every deck fight spot stands on her deck; every hall spot on the hall's floor (inside its wall).
	bool bDeckSpots = true;
	for (const FVector& Spot : Barge->GetFightSpots(ABargeBoarding::DeckFight))
	{
		bDeckSpots &= GroundUnder(World, Spot + FVector(0.0, 0.0, 100.0), Hit) && FMath::Abs(Hit.Z - (Barge->GetActorLocation().Z + BargeRules::DeckTop)) < 5.0;
	}
	TestTrue(TEXT("One deck fight spot per rules spot, each on the deck"), Barge->GetFightSpots(ABargeBoarding::DeckFight).Num() == static_cast<int32>(UE_ARRAY_COUNT(BargeRules::DeckFightSpots)) && bDeckSpots);
	bool bHallSpots = true;
	for (const FVector& Spot : Barge->GetFightSpots(ABargeBoarding::HallFight))
	{
		const FVector Flat = Spot - Barge->GetHallMiddle();
		bHallSpots &= FVector2D(Flat.X, Flat.Y).Size() < BargeRules::HallRadius && GroundUnder(World, Spot + FVector(0.0, 0.0, 100.0), Hit);
	}
	TestTrue(TEXT("The hall's spots, each on the hall's floor"), Barge->GetFightSpots(ABargeBoarding::HallFight).Num() == BargeRules::HallFightSpots && bHallSpots);
	// The berth: her starboard to the barge's port side, level, her deck by BerthUnderDeck off the barge's.
	const FTransform Berth = Barge->GetBerth();
	TestTrue(TEXT("The berth is off her port side, level"), Berth.GetLocation().Y < -BargeRules::HullHalfBeam
		&& FMath::IsNearlyZero(static_cast<float>(Berth.Rotator().Pitch)) && FMath::IsNearlyZero(static_cast<float>(Berth.Rotator().Roll)));
	TestTrue(TEXT("...her deck off the barge's by the plank's fall"), FMath::IsNearlyEqual(
		static_cast<float>(Berth.GetLocation().Z + BargeRules::ShipDeckOverKeel), static_cast<float>(Barge->GetBoardingPoint().Z - BargeRules::BerthUnderDeck), 1.f));
	const FVector BiteOnHer = Barge->GetActorTransform().InverseTransformPosition(Barge->GetGrappleAnchor(0));
	TestTrue(TEXT("The grapples bite her port bulwark fore and aft"), Barge->GetGrappleAnchor(0).X > Barge->GetGrappleAnchor(1).X
		&& FMath::IsNearlyEqual(static_cast<float>(-BiteOnHer.Y), BargeRules::BulwarkHalfBeam - BargeRules::BulwarkThick * 0.5f, 1.f)
		&& FMath::IsNearlyEqual(static_cast<float>(BiteOnHer.Z), BargeRules::DeckTop + BargeRules::RailHeight, 1.f));
	TestTrue(TEXT("Her horns and the office's desk"), Barge->GetHornSpots().Num() == static_cast<int32>(UE_ARRAY_COUNT(BargeRules::HornSpots))
		&& !Barge->GetOfficeSpot().IsNearlyZero());
	Barge->ForceStep(INDEX_NONE);
	TestTrue(TEXT("With no story she isn't there"), !Barge->IsPresent() && Barge->IsHidden());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeHallTest, "Looter.World.Barge.Hall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeHallTest::RunTest(const FString& Parameters)
{
	using namespace BargeRules;
	// The hall at deck level in the house: its floor the deck's level, its walls and ceiling under the house's roof, its round
	// wall inside the house's walls.
	TestEqual(TEXT("The hall's floor is the deck's level"), HallFloor, DeckTop);
	TestTrue(TEXT("Its ceiling under the house's roof"), CeilingThickness > 0.f && HallFloor + HallHeight + CeilingThickness < HouseRoof);
	const FBox2D Ceiling = HallCeilingArea();
	TestTrue(TEXT("The hall stands inside the house"), Ceiling.Min.X >= HouseAft + HouseWallThick && Ceiling.Max.X <= HouseFore - HouseWallThick
		&& Ceiling.Max.Y <= HallSkinHalfBeam && Ceiling.Min.Y >= -HallSkinHalfBeam);
	// Fore of it, on the same level: the office from the hall's rim to the vestibule, the vestibule from there to the door.
	const FBox2D Vestibule = VestibuleArea();
	const FBox2D Office = OfficeArea();
	TestTrue(TEXT("The office runs from the hall's rim to the vestibule's aft wall"), FMath::IsNearlyEqual(Office.Min.X, HallCenter.X + HallRadius, 1.0)
		&& FMath::IsNearlyEqual(Office.Max.X, static_cast<double>(VestibuleAft - VestibuleWall), 1.0));
	TestTrue(TEXT("...the vestibule from there to the lining inside the door"), FMath::IsNearlyEqual(Vestibule.Max.X, static_cast<double>(HouseFore - HouseWallThick - VestibuleWall), 1.0)
		&& Vestibule.Max.Y >= DoorHalfWidth && Office.Max.Y >= Vestibule.Max.Y && Office.Max.Y + OfficeWall <= HallSkinHalfBeam);
	// The desk in the office, out of the way from door to door; the radio on its top.
	const FVector Desk = DeskTop();
	const FBox2D DeskBox = Around(Desk, 45.0, 90.0);
	const FVector Radio = Desk + RadioOnDesk;
	TestTrue(TEXT("The desk is in the office, out of the way through it"), Office.IsInside(DeskBox.Min) && Office.IsInside(DeskBox.Max)
		&& DeskBox.Max.Y < -DoorHalfWidth);
	TestTrue(TEXT("...the radio on it"), DeskBox.IsInside(FVector2D(Radio.X, Radio.Y)) && Radio.Z > Desk.Z && Radio.Z < Desk.Z + 30.0);
	// The two stairs from the floor up to the catwalk; the catwalk open over them (two of its spans each), its span past each
	// top laid (the landing), its end past each foot short of the stair's foot.
	bool bFloorStairs = true;
	for (int32 Index = 0; Index < NumFloorStairs; ++Index)
	{
		const FStairRun Stair = FloorStair(Index);
		bFloorStairs &= StairSlope(Stair) <= StairMostSlope && FMath::IsNearlyEqual(static_cast<float>(Stair.To.Z), HallFloor + InteriorFloorLift, 1.f)
			&& FMath::IsNearlyEqual(static_cast<float>(Stair.From.Z), HallFloor + CatwalkUp, 1.f);
	}
	TestTrue(TEXT("The stairs up to the catwalk are walkable"), bFloorStairs);
	int32 Open = 0;
	for (int32 Index = 0; Index < CatwalkSpans; ++Index)
	{
		Open += IsCatwalkSpanLaid(Index) ? 0 : 1;
	}
	bool bLandings = Open == 2 * NumFloorStairs;
	const double SpanDegrees = 360.0 / CatwalkSpans;
	for (int32 Index = 0; Index < NumFloorStairs; ++Index)
	{
		const double Landing = FloorStairAngles[Index] - FloorStairWays[Index] * SpanDegrees * 0.5;
		const double Foot = FloorStairAngles[Index] + FloorStairWays[Index] * FloorStairSpan;
		bLandings &= IsCatwalkSpanLaid(FMath::FloorToInt32(Landing / SpanDegrees))
			&& (CatwalkEndAngle(Index) - Foot) * FloorStairWays[Index] > 0.0;
	}
	TestTrue(TEXT("The catwalk is open over the stairs, laid past their tops, ended past their feet"), bLandings);
	// The hatch as the story knows it: the house's door; a player put back after the manifest stands on the deck before it.
	TestTrue(TEXT("The hatch's spot is the house's door"), HatchSpot().Equals(FVector(HouseFore, 0.0, DeckTop), 1.0)
		&& IsOpenDeck(FVector2D(HouseFore + HatchStartOut, 0.0)));
	// The stokehold alcoves stay inside the house's walls, and still reach past the hall's wall.
	TestTrue(TEXT("The alcoves fit inside the house's walls"), HallRadius + AlcoveDepth + AlcoveWall + SkinGap <= HallSkinHalfBeam + 0.01f
		&& AlcoveDepth > WallThickness);

	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, ManifestStep);
	if (!Barge)
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	const double Keel = Barge->GetActorLocation().Z;
	const FTransform Her = Barge->GetActorTransform();
	// The ceiling over the hall (between the furnace and the catwalk); the vestibule and the office roofed.
	FVector Hit;
	TestTrue(TEXT("The hall has a ceiling at its walls' top"), IronOver(World, Her.TransformPosition(HallPoint(45.f, 700.f, 100.f)), 2000.0, Hit)
		&& FMath::IsNearlyEqual(Hit.Z, Keel + HallFloor + HallHeight, 5.0));
	TestTrue(TEXT("The vestibule is roofed"), IronOver(World, Her.TransformPosition(FVector(Vestibule.GetCenter().X, 0.0, DeckTop + StandUp)), 1000.0, Hit)
		&& FMath::IsNearlyEqual(Hit.Z, Keel + DeckTop + VestibuleHeight, 5.0));
	TestTrue(TEXT("...and the office"), IronOver(World, Her.TransformPosition(FVector(Office.GetCenter().X, 0.0, DeckTop + StandUp)), 1000.0, Hit)
		&& FMath::IsNearlyEqual(Hit.Z, Keel + DeckTop + OfficeHeight, 5.0));
	// The way in, on one level: from the deck through the door, the vestibule and the office onto the hall's floor, a floor
	// under every step of it.
	const double InHall = HallCenter.X + HallRadius - HallSpotIn;
	TestTrue(TEXT("Through the door, the vestibule and the office into the hall"), WalkClear(World, *Barge, FVector2D(HouseFore + HatchStartOut, 0.0), FVector2D(InHall, 0.0)));
	bool bFloored = true;
	for (const double X : { Vestibule.GetCenter().X, Office.GetCenter().X, InHall })
	{
		bFloored &= GroundUnder(World, Her.TransformPosition(FVector(X, 0.0, DeckTop + StandUp)), Hit)
			&& FMath::IsNearlyEqual(Hit.Z, Keel + DeckTop + InteriorFloorLift, 1.0);
	}
	TestTrue(TEXT("...floored all the way, a hair over the deck"), bFloored);
	// Up each stair to the catwalk, a step every 50 cm from its foot to its top: the stair under the feet, 2.1 m clear over the
	// head (round 4's stairs ran under the ring and failed this halfway up), then the catwalk past the top; and the ring's end
	// past the foot walled, so nobody walks off it into the drop.
	constexpr double Step = 50.0;
	constexpr double Head = 210.0;
	bool bClimb = true;
	bool bEnds = true;
	for (int32 Index = 0; Index < NumFloorStairs; ++Index)
	{
		const FStairRun Stair = FloorStair(Index);
		const FVector Up = Stair.From - Stair.To;
		const double Length = Up.Size();
		for (double Along = 0.0; Along <= Length; Along += Step)
		{
			const FVector Feet = Stair.To + Up * (Along / Length);
			bClimb &= GroundUnder(World, Her.TransformPosition(Feet + FVector(0.0, 0.0, Step)), Hit) && FMath::IsNearlyEqual(Hit.Z, Keel + Feet.Z, 5.0)
				&& HeadroomClear(World, *Barge, Feet, Head);
		}
		const FVector Beyond = Stair.From + Up.GetSafeNormal2D() * Step;
		bClimb &= GroundUnder(World, Her.TransformPosition(Beyond + FVector(0.0, 0.0, Step)), Hit)
			&& FMath::IsNearlyEqual(Hit.Z, Keel + HallFloor + CatwalkUp, 2.0);
		const double Way = FloorStairWays[Index];
		const FVector OnRing = Her.TransformPosition(HallPoint(CatwalkEndAngle(Index) + Way * SpanDegrees * 0.5, CatwalkMiddle, CatwalkUp + StandUp));
		const FVector OverDrop = Her.TransformPosition(HallPoint(CatwalkEndAngle(Index) - Way * SpanDegrees * 0.5, CatwalkMiddle, CatwalkUp + StandUp));
		bEnds &= World->LineTraceTestByChannel(OnRing, OverDrop, ECC_Pawn);
	}
	TestTrue(TEXT("Up each stair from the floor onto the catwalk, headroom all the way"), bClimb);
	TestTrue(TEXT("...and the ring's end past each stair's foot walled"), bEnds);
	// After the manifest she puts a player back before the house's door, on the deck.
	const FVector Start = Barge->GetPlayerStart().GetLocation();
	const FVector StartOnHer = Her.InverseTransformPosition(Start);
	TestTrue(TEXT("Put back before the door: on the open deck"), IsOpenDeck(FVector2D(StartOnHer.X, StartOnHer.Y))
		&& GroundUnder(World, Start, Hit) && FMath::IsNearlyEqual(Hit.Z, Keel + DeckTop, 5.0));
	// Nothing of the alcoves' iron pokes past the house's walls (the barge stands unturned: her Y is the world's).
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
	TestTrue(TEXT("The alcoves' iron is inside the house's walls"), AlcoveParts > 0 && bAlcovesIn);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeDeckTest, "Looter.World.Barge.Deck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeDeckTest::RunTest(const FString& Parameters)
{
	using namespace BargeRules;
	// Every spot on her deck stands in the arena (and out of any blocks); the roof's fires on the roof. The hatch's hole is
	// in the vestibule, aft of the arena (the Hall test holds it there).
	bool bSpots = true;
	const auto Check = [&bSpots](const FVector& Spot) { bSpots &= IsOpenDeck(FVector2D(Spot.X, Spot.Y)); };
	for (const FVector& Spot : DeckFightSpots) { Check(Spot); }
	for (const FVector& Spot : CoverSpots) { Check(Spot); }
	for (const FVector& Spot : VentSpots) { Check(Spot); }
	for (const FVector& Spot : HornSpots) { Check(Spot); }
	Check(DrumOnDeck);
	Check(FallingStackFoot);
	Check(FVector(BerthX, BerthSide * (DeckHalfBeam - BoardingInset), 0.0));
	TestTrue(TEXT("Every deck spot is in the deck's arena"), bSpots);
	bool bFires = true;
	const FBox2D Roof(FVector2D(HouseAft, -HouseHalfBeam), FVector2D(HouseFore, HouseHalfBeam));
	for (const FVector& Spot : SternFireSpots)
	{
		const FVector2D Flat(Spot.X, Spot.Y);
		const float Up = static_cast<float>(Spot.Z);
		bFires &= FMath::IsNearlyEqual(Up, DeckTop) ? IsOpenDeck(Flat) : FMath::IsNearlyEqual(Up, HouseRoof) && Roof.IsInside(Flat);
	}
	TestTrue(TEXT("Her fires on the after deck or on the house's roof"), bFires);
	// The pieces keep apart: the solid ones (crates, the falling stack's foot, the drum's gantry) never
	// overlap each other or a vent, and no fight spot stands inside one or on a vent.
	TArray<FBox2D> Solids;
	for (const FVector& Crate : CoverSpots)
	{
		Solids.Add(Around(Crate, CoverHalf.X, CoverHalf.Y));
	}
	Solids.Add(Around(FallingStackFoot, StackRadius, StackRadius));
	Solids.Add(Around(DrumOnDeck, DrumGantryHalf.X, DrumGantryHalf.Y));
	TArray<FBox2D> Vents;
	for (const FVector& Vent : VentSpots)
	{
		Vents.Add(Around(Vent, VentHalf, VentHalf));
	}
	bool bApart = true;
	for (int32 Index = 0; Index < Solids.Num(); ++Index)
	{
		for (int32 Other = Index + 1; Other < Solids.Num(); ++Other)
		{
			bApart &= !Solids[Index].Intersect(Solids[Other]);
		}
		for (const FBox2D& Vent : Vents)
		{
			bApart &= !Solids[Index].Intersect(Vent);
		}
	}
	for (const FVector& Spot : DeckFightSpots)
	{
		const FVector2D Flat(Spot.X, Spot.Y);
		for (const FBox2D& Piece : Solids)
		{
			bApart &= !Piece.IsInside(Flat);
		}
		for (const FBox2D& Vent : Vents)
		{
			bApart &= !Vent.IsInside(Flat);
		}
	}
	TestTrue(TEXT("Crates, the stack, the gantry, vents and fight spots keep apart"), bApart);

	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, DeckStep);
	if (!Barge)
	{
		return false;
	}
	const double Keel = Barge->GetActorLocation().Z;
	const bool bHull = Barge->HasHull();
	// The hull walks and shoots as it is (its own collision); the stand-in box doesn't.
	TestTrue(TEXT("The hull collides only when it's ours"), (Barge->Hull->GetCollisionEnabled() != ECollisionEnabled::NoCollision) == bHull);
	// The plates: the vestibule's always (a hair over the deck), the open deck's only over the stand-in, drawn.
	const TArray<TObjectPtr<UPrimitiveComponent>>& Parts = Barge->GetDeckPlates();
	int32 Next = 0;
	bool bPlates = true;
	for (const FDeckPlate& Plate : DeckPlates)
	{
		if (Plate.bOnHullDeck && bHull)
		{
			continue;
		}
		const UPrimitiveComponent* Part = Parts.IsValidIndex(Next) ? Parts[Next].Get() : nullptr;
		const double PlateTop = Keel + DeckTop + (Plate.bOnHullDeck ? 0.0 : static_cast<double>(InteriorFloorLift));
		bPlates &= Part && Part->IsA<UStaticMeshComponent>() && Part->IsVisible() && !Part->bHiddenInGame
			&& FMath::IsNearlyEqual(BoxOf(*Part).Max.Z, PlateTop, 2.0) && Part->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
		++Next;
	}
	TestTrue(TEXT("The kit's plates: the vestibule's, and the open deck's over the stand-in only"), bPlates && Next == Parts.Num());
	// No superstructure walled off (the hull has its own).
	TestTrue(TEXT("No blocks"), Barge->GetDeckBlocks().Num() == DeckBlocks().Num());
	// Nothing she draws pokes out of her hull's beam (the hose and the coal hang in the world's frame: not hers).
	bool bInside = true;
	FString Out;
	for (UActorComponent* Part : Barge->GetComponents())
	{
		const UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Part);
		if (!Prim || !Prim->IsVisible() || Prim->bHiddenInGame || Prim == Barge->Hull.Get() || Prim == Barge->HoseSections.Get()
			|| Prim == Barge->CoalRain.Get())
		{
			continue;
		}
		const FBox Box = BoxOf(*Prim);
		if (FMath::Max(FMath::Abs(Box.Min.Y), FMath::Abs(Box.Max.Y)) > HullHalfBeam)
		{
			bInside = false;
			Out += Prim->GetName() + TEXT(" ");
		}
	}
	TestTrue(FString::Printf(TEXT("Nothing she draws pokes out of her beam (%s)"), *Out), bInside);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeHullTest, "Looter.World.Barge.Hull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeHullTest::RunTest(const FString& Parameters)
{
	// The reworked Tripo hull's own collision, which is what breaks when the model is reworked or exported again: traced
	// against the hull alone (no kit piece answers), where the kit counts on it.
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
	const auto HitsHull = [Hull, &Her](const FVector& From, const FVector& To, FHitResult& Hit)
	{
		return Hull->LineTraceComponent(Hit, Her.TransformPosition(From), Her.TransformPosition(To), FCollisionQueryParams(SCENE_QUERY_STAT(BargeHullTest), false));
	};
	FHitResult Hit;
	// The kit lays no plate over the arena: every one of its plates is the vestibule's, inside the house.
	bool bNoPlates = true;
	for (const UPrimitiveComponent* Part : Barge->GetDeckPlates())
	{
		bNoPlates &= Part && Her.InverseTransformPosition(BoxOf(*Part).Max).X <= HouseFore + 1.0;
	}
	TestTrue(TEXT("Over the hull the kit lays no plate on the open deck"), bNoPlates);
	// The deck's top at the arena's four corners.
	bool bDeck = true;
	for (const float X : { DeckAft, DeckFore })
	{
		for (const float Y : { -DeckHalfBeam, DeckHalfBeam })
		{
			bDeck &= HitsHull(FVector(X, Y, DeckTop + 200.0), FVector(X, Y, DeckTop - 200.0), Hit)
				&& FMath::IsNearlyEqual(Her.InverseTransformPosition(Hit.ImpactPoint).Z, static_cast<double>(DeckTop), 2.0);
		}
	}
	TestTrue(TEXT("Her deck's top at the arena's corners"), bDeck);
	// The bulwark's gap at the berth: open a hand inside its posts, closed a hand outside them (across the bulwark, at half
	// its height).
	constexpr double Hand = 20.0;
	const double Waist = DeckTop + RailHeight * 0.5;
	const auto AcrossBulwark = [&](double X)
	{
		return HitsHull(FVector(X, BerthSide * (DeckHalfBeam - Hand), Waist), FVector(X, BerthSide * (BulwarkHalfBeam + Hand), Waist), Hit);
	};
	TestTrue(TEXT("The bulwark's gap is open at the berth"), !AcrossBulwark(BerthX - GapHalf + Hand) && !AcrossBulwark(BerthX + GapHalf - Hand));
	TestTrue(TEXT("...and the bulwark closed past it"), AcrossBulwark(BerthX - GapHalf - Hand) && AcrossBulwark(BerthX + GapHalf + Hand));
	// The house's door: open at its middle, its wall either side of it.
	const double Chest = DeckTop + DoorHeight * 0.5;
	const auto ThroughFore = [&](double Y)
	{
		return HitsHull(FVector(HouseFore + 100.0, Y, Chest), FVector(HouseFore - HouseWallThick - 100.0, Y, Chest), Hit);
	};
	TestTrue(TEXT("The house's door is open"), !ThroughFore(0.0));
	TestTrue(TEXT("...its wall either side"), ThroughFore(DoorHalfWidth + Hand) && ThroughFore(-(DoorHalfWidth + Hand)));
	// Nothing solid inside the house (the kit's hall stands in it), at a walker's height and just under its roof (where the
	// stacks and the tower must stop).
	const double InsideFore = HouseFore - HouseWallThick - Hand;
	const double InsideAft = HouseAft + HouseWallThick + Hand;
	const double InsideSide = HallSkinHalfBeam - Hand;
	bool bHollow = true;
	for (const double Up : { double(DeckTop + StandUp), double(HouseRoof - 2.0 * StandUp) })
	{
		bHollow &= !HitsHull(FVector(InsideFore, 0.0, Up), FVector(InsideAft, 0.0, Up), Hit)
			&& !HitsHull(FVector((InsideFore + InsideAft) * 0.5, -InsideSide, Up), FVector((InsideFore + InsideAft) * 0.5, InsideSide, Up), Hit)
			&& !HitsHull(FVector(HallCenter.X, -InsideSide, Up), FVector(HallCenter.X, InsideSide, Up), Hit);
	}
	TestTrue(TEXT("The house is empty inside, fore to aft and side to side, low and high"), bHollow);
	return true;
}

#endif
