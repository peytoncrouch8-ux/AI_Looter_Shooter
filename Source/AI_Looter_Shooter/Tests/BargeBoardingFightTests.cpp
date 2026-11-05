#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/RappelSpawner.h"
#include "World/BargeBoarding.h"
#include "World/BargeFurnace.h"
#include "World/BargeRules.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "Tests/BargeTestWorld.h"

// Main 6's deck fight, bigger (the user's "Bigger", 2026-11-04: BargeFightRules.h): three pushes (the deck's own, the
// droppers off the house's roof and the bow's scoop, the stern's up the walks), the pacing between them, every push's spots on
// their zones, the droppers' ropes and landings clear, the stern push's way to the well deck open.

using namespace BargeTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeFightTest, "Looter.World.Barge.Fight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeFightTest::RunTest(const FString& Parameters)
{
	using namespace BargeRules;
	// The pushes: three, each with its encounters, the Dredgers they bring.
	bool bPushes = true;
	int32 Dredgers = 0;
	for (int32 Push = 0; Push < NumDeckPushes; ++Push)
	{
		bPushes &= !ABargeBoarding::PushFights(Push).IsEmpty() && DeckPushDredgers[Push] > 0;
		Dredgers += DeckPushDredgers[Push];
	}
	TestTrue(FString::Printf(TEXT("Three pushes, each its encounters and its Dredgers (%d in all)"), Dredgers), NumDeckPushes == 3 && bPushes && Dredgers > DeckPushDredgers[0] && ABargeBoarding::PushFights(NumDeckPushes).IsEmpty()
		&& ABargeBoarding::PushFights(0)[0] == ABargeBoarding::DeckFight);
	// The pacing: the next when the last is mostly down a beat, or after its time; not before.
	TestTrue(TEXT("The next push when the last is mostly down a beat, or on long enough"), IsNextPushDue(true, PushBeatSeconds, 1.f)
		&& !IsNextPushDue(true, PushBeatSeconds * 0.5f, 1.f) && !IsNextPushDue(false, 0.f, PushNextSeconds - 1.f) && IsNextPushDue(false, 0.f, PushNextSeconds));
	// Every push's spots on open deck at their zone's height: the droppers' landings on the well deck and the forecastle,
	// the stern's on the stern deck.
	FString OffZone;
	const auto OnZone = [&OffZone](const FVector& Spot, EDeckZone Kind)
	{
		const FDeckZone* Zone = DeckZoneAt(FVector2D(Spot.X, Spot.Y));
		if (!Zone || Zone->Kind != Kind || !IsOpenDeck(FVector2D(Spot.X, Spot.Y)) || !FMath::IsNearlyEqual(static_cast<float>(Spot.Z), Zone->Z))
		{
			OffZone += FString::Printf(TEXT("(%.0f, %.0f) "), Spot.X, Spot.Y);
		}
	};
	for (const FDropRope& Rope : RoofDrops) { OnZone(Rope.Feet, EDeckZone::WellDeck); }
	for (const FDropRope& Rope : TowerDrops) { OnZone(Rope.Feet, EDeckZone::Forecastle); }
	for (const FVector& Spot : SternFightSpots) { OnZone(Spot, EDeckZone::CrossWalk); }
	// ...each at least the spawner's spacing (250) from the next of its push.
	bool bApart = true;
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(SternFightSpots)); ++Index)
	{
		for (int32 Other = Index + 1; Other < static_cast<int32>(UE_ARRAY_COUNT(SternFightSpots)); ++Other)
		{
			bApart &= FVector::Dist2D(SternFightSpots[Index], SternFightSpots[Other]) >= 250.0;
		}
	}
	TestTrue(TEXT("The stern push's spots a spawner's spacing apart"), bApart);
	// A dropper's rope: the nearest to his landing for her droppers (a skipped spot can't send him across a face), in turn
	// for Mains 4-5's.
	TArray<FVector> Tops;
	for (const FDropRope& Rope : RoofDrops) { Tops.Add(Rope.Top); }
	bool bNearest = true;
	for (int32 Index = 0; Index < Tops.Num(); ++Index)
	{
		bNearest &= ARappelSpawner::PickRope(Tops, RoofDrops[Index].Feet, (Index + 1) % Tops.Num(), true) == Index;
	}
	TestTrue(TEXT("Each dropper takes the rope over his own landing, whatever his turn"), bNearest);
	TestTrue(TEXT("...and Mains 4-5's ropes go in turn"), ARappelSpawner::PickRope(Tops, RoofDrops[0].Feet, 4, false) == 4 % Tops.Num()
		&& ARappelSpawner::PickRope(TArray<FVector>(), FVector::ZeroVector, 0, true) == INDEX_NONE);
	TestTrue(FString::Printf(TEXT("Every push's spots on their zones (%s)"), *OffZone), OffZone.IsEmpty());

	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, DeckStep);
	if (!Barge)
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	const FTransform Her = Barge->GetActorTransform();
	// Her getters for the build script: each push's spots and the droppers' rope tops, one a landing.
	TestTrue(TEXT("The build script's spots: the droppers' landings and ropes, the stern's"),
		Barge->GetFightSpots(ABargeBoarding::RoofFight).Num() == static_cast<int32>(UE_ARRAY_COUNT(RoofDrops))
		&& Barge->GetDropRopeTops(ABargeBoarding::RoofFight).Num() == static_cast<int32>(UE_ARRAY_COUNT(RoofDrops))
		&& Barge->GetFightSpots(ABargeBoarding::TowerFight).Num() == static_cast<int32>(UE_ARRAY_COUNT(TowerDrops))
		&& Barge->GetDropRopeTops(ABargeBoarding::TowerFight).Num() == static_cast<int32>(UE_ARRAY_COUNT(TowerDrops))
		&& Barge->GetFightSpots(ABargeBoarding::SternFight).Num() == static_cast<int32>(UE_ARRAY_COUNT(SternFightSpots))
		&& Barge->GetPushesStarted() == 1);
	// The droppers: each rope's top by the face he comes over (the hull's, a hand behind it), the rope clear from top to
	// landing, the landing floored and clear for a Dredger.
	constexpr double DredgerRadius = 40.0;
	constexpr double DredgerTall = 190.0;
	FString Drops;
	// (Her getter's tops, as the build script sets the spawners': it finds each face on her hull.)
	const TArray<FVector> RoofTops = Barge->GetDropRopeTops(ABargeBoarding::RoofFight);
	const TArray<FVector> TowerTops = Barge->GetDropRopeTops(ABargeBoarding::TowerFight);
	const auto CheckDrop = [&](const FDropRope& Rope, const FVector& Top, double FloorZ)
	{
		FHitResult Hit;
		// Her face within DropTopOff behind the top, at its height or where he hangs under it.
		const double FaceWay = DropFaceWay(Rope);
		const FVector TopOnHer = Her.InverseTransformPosition(Top);
		bool bFace = false;
		for (int32 Try = 0; Barge->Hull && !bFace && Try < DropFaceTries; ++Try)
		{
			const FVector At = TopOnHer - FVector(0.0, 0.0, Try * DropFaceStepDown);
			bFace = Barge->Hull->LineTraceComponent(Hit, Her.TransformPosition(At), Her.TransformPosition(At + FVector(FaceWay * DropTopOff, 0.0, 0.0)),
				FCollisionQueryParams(SCENE_QUERY_STAT(BargeFightTest), false));
		}
		const bool bRope = !World->LineTraceTestByChannel(Top, Her.TransformPosition(Rope.Feet + FVector(0.0, 0.0, DredgerTall)), ECC_Visibility);
		double Floor = 0.0;
		const bool bFloor = FloorUnder(World, *Barge, Rope.Feet + FVector(0.0, 0.0, 120.0), Floor) && FMath::IsNearlyEqual(Floor, FloorZ, 5.0);
		FString Blocker;
		const bool bClear = CapsuleClear(World, *Barge, FVector(Rope.Feet.X, Rope.Feet.Y, bFloor ? Floor : FloorZ), DredgerRadius, DredgerTall, &Blocker);
		if (!bFace || !bRope || !bFloor || !bClear)
		{
			const FString NoFace = bFace ? FString() : FString::Printf(TEXT(" no face by its top (%.0f, %.0f, %.0f)"), TopOnHer.X, TopOnHer.Y, TopOnHer.Z);
			Drops += FString::Printf(TEXT("(%.0f, %.0f):%s%s%s%s "), Rope.Feet.X, Rope.Feet.Y, *NoFace,
				bRope ? TEXT("") : TEXT(" the rope blocked"), bFloor ? TEXT("") : *(TEXT(" floor ") + FloorWhat(World, *Barge, Rope.Feet + FVector(0.0, 0.0, 120.0))),
				bClear ? TEXT("") : *(TEXT(" in ") + Blocker));
		}
	};
	for (int32 Index = 0; Index < RoofTops.Num(); ++Index) { CheckDrop(RoofDrops[Index], RoofTops[Index], WellDeckZ); }
	for (int32 Index = 0; Index < TowerTops.Num(); ++Index) { CheckDrop(TowerDrops[Index], TowerTops[Index], ForecastleZ); }
	TestTrue(FString::Printf(TEXT("The droppers' ropes by their faces and clear, their landings floored and clear (%s)"), *Drops), Drops.IsEmpty());
	// The stern push's way: from the cross walk forward up each house walk to the well deck, a man's way the whole way (one
	// search each side, from that side's first spot).
	FString Route;
	for (const double Side : { -1.0, 1.0 })
	{
		const FVector& From = SternFightSpots[Side < 0.0 ? 0 : 1];
		FString Why;
		// A Dredger's body (DredgerCreature.cpp: 36 cm round).
		if (!RouteOpen(World, *Barge, FVector2D(From.X, From.Y), LaneAftX, Side < 0.0 ? -HullHalfBeam : 0.0, Side < 0.0 ? 0.0 : HullHalfBeam,
			CrossWalkZ, 36.0, &Why))
		{
			Route += FString::Printf(TEXT("%s: %s; "), Side < 0.0 ? TEXT("port") : TEXT("starboard"), *Why);
		}
	}
	TestTrue(FString::Printf(TEXT("The stern push's way up both walks to the well deck is open (%s)"), *Route), Route.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeLightsTest, "Looter.World.Barge.Lights",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeLightsTest::RunTest(const FString& Parameters)
{
	// Her lights by where the player is (the perf lane's 1.65 ms): on her deck or near, the deck's alone; below it, the hall's
	// and the cage's alone; far, none. Counted per view.
	using namespace BargeRules;
	TestTrue(TEXT("The player's view: on the well deck, on the Lamplighter alongside, under her on the ground near: the deck's"),
		ABargeBoarding::LightViewAt(DeckMiddle() + FVector(0.0, 0.0, 100.0)) == 0
		&& ABargeBoarding::LightViewAt(FVector(BerthX, BerthSide * BerthOut, WellDeckZ - 200.0)) == 0
		&& ABargeBoarding::LightViewAt(FVector(BerthX, 0.0, -2500.0)) == 0);
	TestTrue(TEXT("...in the hall, on the hatch's stair: below"), ABargeBoarding::LightViewAt(HallPoint(0.f, 300.f, 100.f)) == 1
		&& ABargeBoarding::LightViewAt((HatchStair().From + HatchStair().To) * 0.5 + FVector(0.0, 0.0, 100.0)) == 1);
	TestTrue(TEXT("...far off: none"), ABargeBoarding::LightViewAt(DeckMiddle() + FVector(LampsNear + 100.0, 0.0, 0.0)) == 2);

	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, DeckStep);
	if (!Barge)
	{
		return false;
	}
	const auto Count = [](const TArray<TObjectPtr<UPointLightComponent>>& Lights)
	{
		int32 Shown = 0;
		for (const UPointLightComponent* Light : Lights)
		{
			Shown += Light && Light->IsVisible() ? 1 : 0;
		}
		return Shown;
	};
	const int32 DeckCount = static_cast<int32>(UE_ARRAY_COUNT(DeckLampSpots)) + NumWorkLamps;
	const int32 HallCount = Barge->GetLanterns().Num();
	FString Views;
	for (int32 View = 0; View < 3; ++View)
	{
		Barge->ShowLightsFor(View);
		const int32 OnDeck = Count(Barge->GetDeckLamps());
		const int32 Below = Count(Barge->GetLanterns()) + (Barge->GetFurnace() && Barge->GetFurnace()->IsCageLightOn() ? 1 : 0);
		const int32 WantDeck = View == 0 ? DeckCount : 0;
		const int32 WantBelow = View == 1 ? HallCount + 1 : 0;
		if (OnDeck != WantDeck || Below != WantBelow)
		{
			Views += FString::Printf(TEXT("view %d: %d on deck, %d below; "), View, OnDeck, Below);
		}
	}
	TestTrue(FString::Printf(TEXT("Her lights per view: the deck's %d on deck, the hall's %d and the cage's below, none far (%s)"), DeckCount, HallCount, *Views),
		Views.IsEmpty());
	// Their reach, local pools (PERF.md).
	bool bPools = true;
	for (const UPointLightComponent* Lamp : Barge->GetDeckLamps())
	{
		bPools &= Lamp && Lamp->AttenuationRadius <= DeckLampReach;
	}
	TestTrue(TEXT("...the deck's in pools, none past the house lamp's reach"), bPools && WorkLampReach < DeckLampReach);
	return true;
}

#endif
