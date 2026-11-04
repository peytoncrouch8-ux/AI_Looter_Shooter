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
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "Tests/BargeTestWorld.h"

// Main 6's barge (the Wardens story, lane MAIN67's W3): her kit on the jumboed Tripo hull, her decks as zones (the well
// deck's arena, the forecastle's high ground, the walks' flanks, the stern deck), the berth and the deck fight's plan, each
// in a test level with no story (ForceStep plays her as if Main 6 were on a step). With SM_DredgeBarge not imported she
// stands on the stand-in box and the kit's own deck plates: the same tests hold. Below the deck and the hull's own
// collision: BargeBoardingHallTests.cpp; her beats: BargeBoardingBeatsTests.cpp.

using namespace BargeTestWorld;

namespace
{
	/** The CoverPieces entry a cover part was laid for (its name: Cover<entry>, then _<barrel> or _Up). */
	int32 CoverEntry(const UPrimitiveComponent& Part)
	{
		FString Rest = Part.GetName().RightChop(5);
		int32 Cut = INDEX_NONE;
		if (Rest.FindChar(TEXT('_'), Cut))
		{
			Rest.LeftInline(Cut);
		}
		return FCString::Atoi(*Rest);
	}

	/** The flat boxes the deck's own pieces stand in (not the cover: the tests read the props' real bounds), named. */
	TArray<TPair<FString, FBox2D>> DeckSolids()
	{
		using namespace BargeRules;
		TArray<TPair<FString, FBox2D>> Solids;
		Solids.Emplace(TEXT("the gantry"), Around(DrumOnDeck, DrumGantryHalf.X, DrumGantryHalf.Y));
		Solids.Emplace(TEXT("the stack's foot"), Around(FallingStackFoot, StackFootHalf, StackFootHalf));
		Solids.Emplace(TEXT("the coaming"), HatchCoaming());
		Solids.Emplace(TEXT("the landing"), Around(FVector(BerthX, BerthSide * (WellDeckHalfBeam - BoardingInset), 0.0), LandingApron, LandingApron));
		for (int32 Index = 0; Index < NumForecastleStairs; ++Index)
		{
			const FStairRun Stair = ForecastleStair(Index);
			Solids.Emplace(FString::Printf(TEXT("forecastle stair %d"), Index),
				FBox2D(FVector2D(Stair.To.X, Stair.To.Y - ForecastleStairHalfWidth), FVector2D(Stair.From.X, Stair.From.Y + ForecastleStairHalfWidth)));
		}
		for (int32 Index = 0; Index < NumVents; ++Index)
		{
			Solids.Emplace(FString::Printf(TEXT("vent %d"), Index), Around(VentSpots[Index], VentHalf, VentHalf));
		}
		return Solids;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeKitTest, "Looter.World.Barge.Kit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeKitTest::RunTest(const FString& Parameters)
{
	using namespace BargeRules;
	// The hall below the deck, smaller (the user's call), fits her beam with a hull wall all round.
	TestTrue(TEXT("The hall fits her beam, at least the contract's radius"), HallFits(HallRadius) && HallRadius >= HallLeastRadius);
	// The berth: on the well deck's port side, its gap within the deck's length; the plank runs over the gap's sill, her bag
	// clears the rail alongside and, at the jump, with the barge listed toward her.
	const FDeckZone* BerthDeck = DeckZoneAt(FVector2D(BerthX, BerthSide * (WellDeckHalfBeam - BoardingInset)));
	TestTrue(TEXT("The berth is on the well deck, its gap in the deck's length, the rail outside the deck"), BerthDeck
		&& BerthDeck->Kind == EDeckZone::WellDeck && BerthX - GapHalf > WellDeckAft && BerthX + GapHalf < WellDeckFore
		&& BulwarkHalfBeam - BulwarkThick >= WellDeckHalfBeam);
	TestTrue(TEXT("The plank clears the gap's sill"), PlankSillClearance() > 0.f);
	TestTrue(TEXT("Her bag clears the rail at the berth"), BerthBagClearance() > 0.f);
	TestTrue(TEXT("...and at the jump, the barge listed toward her"), JumpBagClearance() > 0.f);
	TestTrue(TEXT("The ship's numbers are the ship's"), FMath::IsNearlyEqual(ShipDeckOverKeel, ALamplighterShip::DeckTop)
		&& FMath::IsNearlyEqual(ShipRailOut, ALamplighterShip::RailOut));

	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, DeckStep);
	if (!Barge)
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	TestTrue(TEXT("Shown as the deck fight's step"), Barge->IsPresent() && Barge->GetState() == EBargeState::Hanging);
	TestTrue(TEXT("Her furnace, three valves, the drum"), Barge->GetFurnace() && Barge->GetValves().Num() == NumValves && Barge->GetHoseDrum());
	// The rail walls as the rules lay them out, each where its footprint says, as tall as the rules have them over its deck.
	const TArray<FRailWall> Layout = RailWallLayout();
	bool bWallsLaid = Barge->GetRailWalls().Num() == Layout.Num();
	for (int32 Index = 0; bWallsLaid && Index < Layout.Num(); ++Index)
	{
		const UBoxComponent* Wall = Barge->GetRailWalls()[Index];
		const FVector Middle = Wall ? Wall->GetRelativeLocation() : FVector::ZeroVector;
		const FVector Half = Wall ? Wall->GetUnscaledBoxExtent() : FVector::ZeroVector;
		bWallsLaid &= Wall && Wall->GetFName() == Layout[Index].Name && FVector2D(Middle.X, Middle.Y).Equals(Layout[Index].Footprint.GetCenter(), 0.5)
			&& FVector2D(Half.X, Half.Y).Equals(Layout[Index].Footprint.GetExtent(), 0.5) && FMath::IsNearlyEqual(Half.Z * 2.0, static_cast<double>(RailWallHeight), 0.5)
			&& FMath::IsNearlyEqual(Middle.Z, Layout[Index].Z + RailWallHeight * 0.5, 0.5);
	}
	TestTrue(TEXT("Rail walls along her decks' drops, as the rules lay them out"), bWallsLaid);
	double Floor = 0.0;
	const FVector Landing(BerthX, BerthSide * (WellDeckHalfBeam - BoardingInset), WellDeckZ);
	TestTrue(TEXT("The plank's landing is on the well deck"), FloorUnder(World, *Barge, Landing + FVector(0.0, 0.0, 150.0), Floor)
		&& FMath::IsNearlyEqual(Floor, static_cast<double>(WellDeckZ), 5.0)
		&& Barge->GetBoardingPoint().Equals(Barge->GetActorTransform().TransformPosition(Landing), 1.0));
	TestTrue(TEXT("The hall's floor is under the grate ring"), FloorUnder(World, *Barge, HallPoint(30.f, (GrateInner + GrateOuter) * 0.5f, 150.f), Floor)
		&& FMath::IsNearlyEqual(Floor, static_cast<double>(HallFloor), 5.0));
	// Every deck fight spot stands on its deck; every hall spot on the hall's floor, inside its wall.
	FString OffDeck;
	for (const FVector& Spot : DeckFightSpots)
	{
		const FVector Over = Spot + FVector(0.0, 0.0, SpotUp + 100.0);
		if (!FloorUnder(World, *Barge, Over, Floor) || !FMath::IsNearlyEqual(Floor, Spot.Z, 5.0))
		{
			OffDeck += FString::Printf(TEXT("(%.0f, %.0f): %s; "), Spot.X, Spot.Y, *FloorWhat(World, *Barge, Over));
		}
	}
	TestTrue(FString::Printf(TEXT("Nine deck fight spots, each on its deck (%s)"), *OffDeck),
		Barge->GetFightSpots(ABargeBoarding::DeckFight).Num() == static_cast<int32>(UE_ARRAY_COUNT(DeckFightSpots)) && UE_ARRAY_COUNT(DeckFightSpots) == 9
		&& OffDeck.IsEmpty());
	bool bHallSpots = true;
	for (const FVector& Spot : Barge->GetFightSpots(ABargeBoarding::HallFight))
	{
		const FVector OnHer = Barge->GetActorTransform().InverseTransformPosition(Spot);
		bHallSpots &= FVector2D(OnHer.X - HallCenter.X, OnHer.Y - HallCenter.Y).Size() < HallRadius
			&& FloorUnder(World, *Barge, OnHer + FVector(0.0, 0.0, 100.0), Floor) && FMath::IsNearlyEqual(Floor, static_cast<double>(HallFloor), 5.0);
	}
	TestTrue(TEXT("Eight hall spots, each on the hall's floor"), Barge->GetFightSpots(ABargeBoarding::HallFight).Num() == HallFightSpots && HallFightSpots == 8 && bHallSpots);
	// The berth: her starboard to the barge's port side, level, her deck by BerthUnderDeck off the barge's.
	const FTransform Berth = Barge->GetBerth();
	TestTrue(TEXT("The berth is off her port side, level"), Berth.GetLocation().Y < -BulwarkHalfBeam
		&& FMath::IsNearlyZero(static_cast<float>(Berth.Rotator().Pitch)) && FMath::IsNearlyZero(static_cast<float>(Berth.Rotator().Roll)));
	TestTrue(TEXT("...her deck off the barge's by the plank's fall"), FMath::IsNearlyEqual(
		static_cast<float>(Berth.GetLocation().Z + ShipDeckOverKeel), static_cast<float>(Barge->GetBoardingPoint().Z - BerthUnderDeck), 1.f));
	const FVector BiteOnHer = Barge->GetActorTransform().InverseTransformPosition(Barge->GetGrappleAnchor(0));
	TestTrue(TEXT("The grapples bite her port rail fore and aft"), Barge->GetGrappleAnchor(0).X > Barge->GetGrappleAnchor(1).X
		&& FMath::IsNearlyEqual(static_cast<float>(-BiteOnHer.Y), BulwarkHalfBeam - BulwarkThick * 0.5f, 1.f)
		&& FMath::IsNearlyEqual(static_cast<float>(BiteOnHer.Z), WellDeckZ + RailHeight, 1.f));
	TestTrue(TEXT("Her horns and the office's desk"), Barge->GetHornSpots().Num() == static_cast<int32>(UE_ARRAY_COUNT(HornSpots))
		&& !Barge->GetOfficeSpot().IsNearlyZero());
	Barge->ForceStep(INDEX_NONE);
	TestTrue(TEXT("With no story she isn't there"), !Barge->IsPresent() && Barge->IsHidden());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeDeckTest, "Looter.World.Barge.Deck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeDeckTest::RunTest(const FString& Parameters)
{
	using namespace BargeRules;
	// Her zones: inside her hull, each wide enough for a man (the walks' boxes are the strips clear at every station; the
	// walks are wider); inside her rail line (the walls stand outside them); the forecastle over the well deck, its two
	// stairs walkable from deck to deck.
	bool bZones = true;
	for (const FDeckZone& Zone : DeckZones)
	{
		bZones &= Zone.XMin < Zone.XMax && Zone.YMax - Zone.YMin >= 2.0 * Shoulders && Zone.XMin >= HullAft && Zone.XMax <= HullFore
			&& FMath::Max(FMath::Abs(Zone.YMin), FMath::Abs(Zone.YMax)) < HullHalfBeam;
	}
	FString Walled;
	for (const FRailWall& Wall : RailWallLayout())
	{
		for (const FDeckZone& Zone : DeckZones)
		{
			if (Wall.Footprint.Intersect(Zone.Box().ExpandBy(-1.0)))
			{
				Walled += Wall.Name.ToString() + TEXT("/") + Zone.Name + TEXT(" ");
			}
		}
	}
	TestTrue(FString::Printf(TEXT("No rail wall stands in a zone (%s)"), *Walled), Walled.IsEmpty());
	TestTrue(TEXT("Her zones inside her hull, each a man wide"), bZones
		&& DeckZones[0].Kind == EDeckZone::WellDeck && DeckZones[NumDeckZones - 1].Kind == EDeckZone::Stern);
	bool bStairs = ForecastleZ > WellDeckZ;
	for (int32 Index = 0; Index < NumForecastleStairs; ++Index)
	{
		const FStairRun Stair = ForecastleStair(Index);
		const FDeckZone* Top = DeckZoneAt(FVector2D(Stair.From.X + 1.0, Stair.From.Y));
		const FDeckZone* Foot = DeckZoneAt(FVector2D(Stair.To.X - 1.0, Stair.To.Y));
		bStairs &= StairSlope(Stair) <= StairMostSlope && Top && Top->Kind == EDeckZone::Forecastle && FMath::IsNearlyEqual(Stair.From.Z, static_cast<double>(Top->Z))
			&& Foot && Foot->Kind == EDeckZone::WellDeck && FMath::IsNearlyEqual(Stair.To.Z, static_cast<double>(Foot->Z));
	}
	TestTrue(TEXT("The forecastle is the high ground, its stairs walkable from the well deck"), bStairs);
	// Every spot on her decks stands on open deck at its zone's height (the roof's fires on the casing's roof).
	bool bSpots = true;
	const auto Check = [&bSpots](const FVector& Spot)
	{
		bSpots &= IsOpenDeck(FVector2D(Spot.X, Spot.Y)) && FMath::IsNearlyEqual(OnDeck(FVector2D(Spot.X, Spot.Y)).Z, Spot.Z);
	};
	for (const FVector& Spot : DeckFightSpots) { Check(Spot); }
	for (const FVector& Spot : VentSpots) { Check(Spot); }
	for (const FVector& Spot : HornSpots) { Check(Spot); }
	for (const FCoverPiece& Piece : CoverPieces) { Check(FVector(Piece.X, Piece.Y, Piece.Z)); }
	Check(DrumOnDeck);
	Check(FallingStackFoot);
	Check(FVector(BerthX, BerthSide * (WellDeckHalfBeam - BoardingInset), WellDeckZ));
	TestTrue(TEXT("Every deck spot on open deck at its zone's height"), bSpots);
	// The furnace's vents are the deck fight's hazards: her flues run fore from the hall (aft, under the casing) under the
	// house to them, so each is on the well deck, none over the hall.
	bool bVentsOnWell = true;
	for (const FVector& Vent : VentSpots)
	{
		const FDeckZone* Zone = DeckZoneAt(FVector2D(Vent.X, Vent.Y));
		bVentsOnWell &= Zone && Zone->Kind == EDeckZone::WellDeck;
	}
	TestTrue(TEXT("The vents breathe up through the well deck: each on it"), bVentsOnWell);
	bool bFires = true;
	for (const FVector& Spot : SternFireSpots)
	{
		const FVector2D Flat2(Spot.X, Spot.Y);
		const FDeckZone* Zone = DeckZoneAt(Flat2);
		bFires &= Zone && (Zone->Kind == EDeckZone::Stern || Zone->Kind == EDeckZone::Walk) ? FMath::IsNearlyEqual(static_cast<double>(Zone->Z), Spot.Z)
			: FMath::IsNearlyEqual(static_cast<float>(Spot.Z), CasingRoofZ) && CasingRoof().IsInside(Flat2);
	}
	TestTrue(TEXT("Her fires on the stern deck, the walks or the casing's roof"), bFires);
	// The plan: three on the forecastle's high ground, one in each walk's mouth, the rest on the well deck.
	int32 OnForecastle = 0;
	int32 InWalks = 0;
	int32 OnWell = 0;
	for (const FVector& Spot : DeckFightSpots)
	{
		const FDeckZone* Zone = DeckZoneAt(FVector2D(Spot.X, Spot.Y));
		OnForecastle += Zone && Zone->Kind == EDeckZone::Forecastle ? 1 : 0;
		InWalks += Zone && Zone->Kind == EDeckZone::Walk && Spot.X > Zone->XMax - WalkMouth ? 1 : 0;
		OnWell += Zone && Zone->Kind == EDeckZone::WellDeck ? 1 : 0;
	}
	TestTrue(TEXT("The nine: the forecastle's high ground, the walks' mouths, the well deck"), OnForecastle >= 3 && InWalks == 2 && OnWell >= 3);
	// The deck's own pieces keep apart (the gantry, the stack, the coaming, the stairs, the vents, the landing's apron), no
	// fight spot inside one; the stack, fallen, lies on the well deck clear of the hatch and the landing.
	const TArray<TPair<FString, FBox2D>> Solids = DeckSolids();
	FString Clash;
	for (int32 Index = 0; Index < Solids.Num(); ++Index)
	{
		for (int32 Other = Index + 1; Other < Solids.Num(); ++Other)
		{
			if (Solids[Index].Value.Intersect(Solids[Other].Value))
			{
				Clash += Solids[Index].Key + TEXT("/") + Solids[Other].Key + TEXT(" ");
			}
		}
		for (const FVector& Spot : DeckFightSpots)
		{
			if (Solids[Index].Value.ExpandBy(Shoulders).IsInside(FVector2D(Spot.X, Spot.Y)))
			{
				Clash += FString::Printf(TEXT("spot (%.0f, %.0f) in %s "), Spot.X, Spot.Y, *Solids[Index].Key);
			}
		}
	}
	TestTrue(FString::Printf(TEXT("The deck's pieces keep apart (%s)"), *Clash), Clash.IsEmpty());
	// Fallen, it lies from the flange's edge it turned about out its length.
	const FVector Pivot = StackFallPivot();
	const FVector Tip = StackTip();
	const FBox2D Lying = Around((Pivot + Tip) * 0.5, FMath::Abs(Tip.X - Pivot.X) * 0.5 + StackRadius, FMath::Abs(Tip.Y - Pivot.Y) * 0.5 + StackRadius);
	const FBox2D Apron = Around(FVector(BerthX, BerthSide * (WellDeckHalfBeam - BoardingInset), 0.0), LandingApron, LandingApron);
	TestTrue(TEXT("The stack falls across the well deck, clear of the hatch and the landing"), DeckZoneAt(FVector2D(Tip.X, Tip.Y))
		&& !Lying.Intersect(HatchCoaming()) && !Lying.Intersect(Apron));

	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, DeckStep);
	if (!Barge)
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	const bool bHull = Barge->HasHull();
	// The hull walks and shoots as it is (its own collision); the stand-in box doesn't, and the kit floors each zone on it.
	TestTrue(TEXT("The hull collides only when it's ours"), (Barge->Hull->GetCollisionEnabled() != ECollisionEnabled::NoCollision) == bHull);
	int32 Pieces = 0;
	for (const FDeckZone& Zone : DeckZones)
	{
		Pieces += CutAround(Zone.Box(), HatchHole()).Num();
	}
	bool bPlates = Barge->GetDeckPlates().Num() == (bHull ? 0 : Pieces);
	for (const UPrimitiveComponent* Part : Barge->GetDeckPlates())
	{
		const FBox Box = BoxOnHer(*Barge, *Part);
		const FDeckZone* Zone = DeckZoneAt(FVector2D(Box.GetCenter().X, Box.GetCenter().Y));
		bPlates &= Zone && FMath::IsNearlyEqual(Box.Max.Z, static_cast<double>(Zone->Z), 2.0) && Part->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
	}
	TestTrue(TEXT("The kit's plates: one per zone over the stand-in, none over the hull"), bPlates);
	// The cover as laid (the props' real bounds): each entry there, standing on its deck inside the zones, apart from the
	// other entries and the deck's pieces, no rifleman's spot inside one.
	TArray<int32> Laid;
	Laid.Init(0, NumCoverPieces);
	FString Bad;
	const TArray<TObjectPtr<UStaticMeshComponent>>& Parts = Barge->GetCoverParts();
	for (int32 Index = 0; Index < Parts.Num(); ++Index)
	{
		const UStaticMeshComponent* Part = Parts[Index];
		const int32 Entry = Part ? CoverEntry(*Part) : INDEX_NONE;
		if (!Part || Entry < 0 || Entry >= NumCoverPieces)
		{
			Bad += TEXT("unnamed ");
			continue;
		}
		++Laid[Entry];
		const FBox Box = BoxOnHer(*Barge, *Part);
		const FBox2D Footprint = Flat(Box);
		const bool bOnDeck = IsOpenDeck(Footprint.Min) && IsOpenDeck(Footprint.Max) && IsOpenDeck(FVector2D(Footprint.Min.X, Footprint.Max.Y))
			&& IsOpenDeck(FVector2D(Footprint.Max.X, Footprint.Min.Y));
		// On its deck (a stacked course stands on the one under it).
		const bool bStands = Part->GetName().EndsWith(TEXT("_Up")) || FMath::IsNearlyEqual(Box.Min.Z, static_cast<double>(CoverPieces[Entry].Z), 3.0);
		if (!bOnDeck || !bStands)
		{
			Bad += Part->GetName() + TEXT(" off its deck ");
		}
		for (int32 Other = Index + 1; Other < Parts.Num(); ++Other)
		{
			if (Parts[Other] && CoverEntry(*Parts[Other]) != Entry && Footprint.Intersect(Flat(BoxOnHer(*Barge, *Parts[Other]))))
			{
				Bad += Part->GetName() + TEXT("/") + Parts[Other]->GetName() + TEXT(" ");
			}
		}
		for (const TPair<FString, FBox2D>& Solid : Solids)
		{
			if (Footprint.Intersect(Solid.Value))
			{
				Bad += Part->GetName() + TEXT(" on ") + Solid.Key + TEXT(" ");
			}
		}
		for (const FVector& Spot : DeckFightSpots)
		{
			if (Footprint.ExpandBy(Shoulders).IsInside(FVector2D(Spot.X, Spot.Y)))
			{
				Bad += FString::Printf(TEXT("spot (%.0f, %.0f) in %s "), Spot.X, Spot.Y, *Part->GetName());
			}
		}
	}
	for (int32 Entry = 0; Entry < NumCoverPieces; ++Entry)
	{
		if (Laid[Entry] == 0)
		{
			Bad += FString::Printf(TEXT("cover %d missing "), Entry);
		}
	}
	TestTrue(FString::Printf(TEXT("The cover stands on her decks, apart (%s)"), *Bad), Bad.IsEmpty());
	// The work lamps: lit, shadowless, inside her beam, their lanterns over no rifleman's head (a man's shoulders and 2 m).
	const TArray<TObjectPtr<UPointLightComponent>>& Lamps = Barge->GetDeckLamps();
	bool bLamps = Lamps.Num() == static_cast<int32>(UE_ARRAY_COUNT(DeckLampSpots)) + NumWorkLamps;
	for (const UPointLightComponent* Lamp : Lamps)
	{
		const FVector At = Lamp ? Lamp->GetRelativeLocation() : FVector::ZeroVector;
		bLamps &= Lamp && Lamp->IsVisible() && !Lamp->CastShadows && FMath::Abs(At.Y) < HullHalfBeam && At.X > HullAft && At.X < HullFore;
	}
	for (const FWorkLamp& Lamp : WorkLamps)
	{
		const FVector Seat = WorkLampSeat(Lamp);
		for (const FVector& Spot : DeckFightSpots)
		{
			bLamps &= FVector2D::Distance(FVector2D(Seat.X, Seat.Y), FVector2D(Spot.X, Spot.Y)) > Shoulders + 20.0 || Seat.Z - Spot.Z > 200.0;
		}
	}
	TestTrue(TEXT("The work lamps lit, shadowless, on her, over nobody's head"), bLamps);
	// The player's lines into the fight over the cover: from the plank's landing, and from each forecastle stair's top.
	const auto Lines = [World, Barge](const FVector& Eye)
	{
		int32 Seen = 0;
		for (const FVector& Spot : DeckFightSpots)
		{
			Seen += SeesClear(World, *Barge, Eye, Spot + FVector(0.0, 0.0, SpotUp + ChestUp)) ? 1 : 0;
		}
		return Seen;
	};
	const int32 FromPlank = Lines(FVector(BerthX, BerthSide * (WellDeckHalfBeam - BoardingInset), WellDeckZ + EyeUp));
	bool bFromStairs = true;
	for (int32 Index = 0; Index < NumForecastleStairs; ++Index)
	{
		bFromStairs &= Lines(ForecastleStair(Index).From + FVector(0.0, 0.0, EyeUp)) >= StairSightLines;
	}
	TestTrue(FString::Printf(TEXT("Lines into the fight from the plank (%d of 9) and the forecastle's stairs"), FromPlank), FromPlank >= PlankSightLines && bFromStairs);
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
		const FBox Box = BoxOnHer(*Barge, *Prim);
		if (FMath::Max(FMath::Abs(Box.Min.Y), FMath::Abs(Box.Max.Y)) > HullHalfBeam)
		{
			bInside = false;
			Out += Prim->GetName() + TEXT(" ");
		}
	}
	TestTrue(FString::Printf(TEXT("Nothing she draws pokes out of her beam (%s)"), *Out), bInside);
	return true;
}

#endif
