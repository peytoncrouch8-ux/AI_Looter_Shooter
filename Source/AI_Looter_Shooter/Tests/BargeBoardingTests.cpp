#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/BargeBoarding.h"
#include "World/BargeFurnace.h"
#include "World/BargeHoseDrum.h"
#include "World/BargeRules.h"
#include "World/FurnaceDraftValve.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

// Main 6's barge (the Wardens story, lane MAIN67's W3): her walkable kit, the grate ring's wheel, the valves, the cage, her
// dying and her fall, each in a test level with no story (ForceStep plays her as if Main 6 were on a step).

namespace
{
	/** A barge in a fresh test level at Where, begun and played as if Main 6 were on Step. */
	ABargeBoarding* SpawnBarge(FTestWorldWrapper& TestLevel, FAutomationTestBase& Test, int32 Step, const FVector& Where = FVector(0.0, 0.0, 10000.0))
	{
		if (!Test.TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
		{
			return nullptr;
		}
		UWorld* World = TestLevel.GetTestWorld();
		ABargeBoarding* Barge = World->SpawnActor<ABargeBoarding>(Where, FRotator::ZeroRotator);
		if (!Test.TestNotNull(TEXT("The barge"), Barge))
		{
			return nullptr;
		}
		Barge->DispatchBeginPlay();
		Barge->ForceStep(Step);
		return Barge;
	}

	/** Where a straight drop from From hits world-static ground, or false. */
	bool GroundUnder(const UWorld* World, const FVector& From, FVector& OutHit)
	{
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, From, From - FVector(0.0, 0.0, 3000.0), FCollisionObjectQueryParams(ECC_WorldStatic)))
		{
			OutHit = Hit.ImpactPoint;
			return true;
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeKitTest, "Looter.World.Barge.Kit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeKitTest::RunTest(const FString& Parameters)
{
	// The design's 26 m hall doesn't fit the 24.75 m hull; the 22 m one does, with the hull's wall all round.
	TestFalse(TEXT("A 26 m hall wouldn't fit the hull"), BargeRules::HallFits(BargeRules::DesignHallRadius));
	TestTrue(TEXT("The 22 m hall fits"), BargeRules::HallFits(BargeRules::HallRadius));
	TestTrue(TEXT("The deck is about 70 m"), BargeRules::DeckFore - BargeRules::DeckAft >= 6500.f);

	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, BargeRules::DeckStep);
	if (!Barge)
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	TestTrue(TEXT("Shown as the deck fight's step"), Barge->IsPresent() && Barge->GetState() == EBargeState::Hanging);
	TestTrue(TEXT("Her furnace, three valves, the drum"), Barge->GetFurnace() && Barge->GetValves().Num() == BargeRules::NumValves && Barge->GetHoseDrum());
	TestTrue(TEXT("Rail walls round her deck and the hatch (8)"), Barge->GetRailWalls().Num() == 8);
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
	TestTrue(TEXT("Nine deck fight spots, each on the deck"), Barge->GetFightSpots(ABargeBoarding::DeckFight).Num() == 9 && bDeckSpots);
	bool bHallSpots = true;
	for (const FVector& Spot : Barge->GetFightSpots(ABargeBoarding::HallFight))
	{
		const FVector Flat = Spot - Barge->GetHallMiddle();
		bHallSpots &= FVector2D(Flat.X, Flat.Y).Size() < BargeRules::HallRadius && GroundUnder(World, Spot + FVector(0.0, 0.0, 100.0), Hit);
	}
	TestTrue(TEXT("Eight hall spots, each on the hall's floor"), Barge->GetFightSpots(ABargeBoarding::HallFight).Num() == 8 && bHallSpots);
	// The berth: her starboard to the barge's port side, her deck a little under the barge's.
	const FTransform Berth = Barge->GetBerth();
	TestTrue(TEXT("The berth is off her port side, level"), Berth.GetLocation().Y < -BargeRules::HullHalfBeam
		&& FMath::IsNearlyZero(static_cast<float>(Berth.Rotator().Pitch)) && FMath::IsNearlyZero(static_cast<float>(Berth.Rotator().Roll)));
	TestTrue(TEXT("...her deck under the barge's by the plank's climb"), FMath::IsNearlyEqual(
		static_cast<float>(Berth.GetLocation().Z + BargeRules::ShipDeckOverKeel), static_cast<float>(Barge->GetBoardingPoint().Z - BargeRules::BerthUnderDeck), 1.f));
	TestTrue(TEXT("The grapples bite her port rail fore and aft"), Barge->GetGrappleAnchor(0).X > Barge->GetGrappleAnchor(1).X
		&& Barge->GetGrappleAnchor(0).Y < 0.0 && Barge->GetHornSpots().Num() == 4 && !Barge->GetOfficeSpot().IsNearlyZero());
	Barge->ForceStep(INDEX_NONE);
	TestTrue(TEXT("With no story she isn't there"), !Barge->IsPresent() && Barge->IsHidden());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeGratesTest, "Looter.World.Barge.Grates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeGratesTest::RunTest(const FString& Parameters)
{
	// 4 of 12 hot, spread evenly; the wheel steps a wedge every 3 s; a second's glow, then the burn; each valve cools it.
	const auto Count = [](uint32 Mask) { return static_cast<int32>(FMath::CountBits(Mask)); };
	TestEqual(TEXT("4 of 12 hot with no valve shut"), Count(BargeRules::HotMask(0.f, 0)), 4);
	TestTrue(TEXT("...every third wedge"), BargeRules::HotMask(0.f, 0) == ((1u << 0) | (1u << 3) | (1u << 6) | (1u << 9)));
	TestTrue(TEXT("A turn later the wheel has stepped one wedge"), BargeRules::HotMask(3.1f, 0) == ((1u << 1) | (1u << 4) | (1u << 7) | (1u << 10)));
	TestTrue(TEXT("...the same all through a turn"), BargeRules::HotMask(5.9f, 0) == BargeRules::HotMask(3.1f, 0));
	TestFalse(TEXT("Glowing, not burning, in a turn's first second"), BargeRules::IsBurning(3.5f));
	TestTrue(TEXT("Burning after it"), BargeRules::IsBurning(4.5f));
	TestEqual(TEXT("One valve: 3 hot"), Count(BargeRules::HotMask(0.f, 1)), 3);
	TestEqual(TEXT("Two: 2"), Count(BargeRules::HotMask(0.f, 2)), 2);
	TestTrue(TEXT("Three: cold"), BargeRules::HotMask(0.f, 3) == 0u);
	TestEqual(TEXT("The ring's wedge under a point"), BargeRules::GrateAt(FVector2D(600.0, 10.0)), 0);
	TestEqual(TEXT("...off the ring by the furnace"), BargeRules::GrateAt(FVector2D(350.0, 0.0)), INDEX_NONE);
	TestEqual(TEXT("...and outside it"), BargeRules::GrateAt(FVector2D(950.0, 0.0)), INDEX_NONE);

	// The vents: a second's glow before every burn, never all at once.
	bool bToldFirst = true;
	for (float Time = 0.f; Time < 30.f; Time += 0.05f)
	{
		if (BargeRules::VentPhase(0, Time) == BargeRules::EVentPhase::Burn && BargeRules::VentPhase(0, Time - 0.06f) == BargeRules::EVentPhase::Quiet)
		{
			bToldFirst = false;
		}
	}
	TestTrue(TEXT("A vent glows before it burns"), bToldFirst);
	int32 MostBurning = 0;
	for (float Time = 6.f; Time < 18.f; Time += 0.05f)
	{
		int32 Burning = 0;
		for (int32 Vent = 0; Vent < BargeRules::NumVents; ++Vent)
		{
			Burning += BargeRules::VentPhase(Vent, Time) == BargeRules::EVentPhase::Burn ? 1 : 0;
		}
		MostBurning = FMath::Max(MostBurning, Burning);
	}
	TestTrue(TEXT("...and they run in a pattern, a few at a time"), MostBurning > 0 && MostBurning <= 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeValvesTest, "Looter.World.Barge.Valves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeValvesTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, BargeRules::FurnaceStep);
	ABargeFurnace* Furnace = Barge ? Barge->GetFurnace() : nullptr;
	if (!Furnace)
	{
		return false;
	}
	TestTrue(TEXT("The furnace's step: the ring turns, every valve open"), Furnace->IsRingRunning() && Barge->CountValvesShut() == 0);
	Barge->Advance(1.5f);
	TestTrue(TEXT("...4 wedges burning 1.5 s in"), FMath::CountBits(Furnace->GetHotMask()) == 4 && Furnace->IsRingBurning());
	AFurnaceDraftValve* First = Barge->GetValves()[0];
	TestTrue(TEXT("A valve shuts (a hold)"), First->Shut(nullptr, /*bForce*/ true) && First->IsShut());
	TestFalse(TEXT("...and can't be shut again"), First->Shut(nullptr, true));
	TestTrue(TEXT("...the ring a notch colder"), Furnace->GetValvesShut() == 1 && FMath::CountBits(Furnace->GetHotMask()) == 3);
	for (AFurnaceDraftValve* Valve : Barge->GetValves())
	{
		Valve->Shut(nullptr, true);
	}
	TestTrue(TEXT("All three: the ring cold and still"), Barge->CountValvesShut() == 3 && !Furnace->IsRingRunning() && Furnace->GetHotMask() == 0u);
	First->Tick(2.f);
	TestTrue(TEXT("...the wheel spun down and seated"), FMath::IsNearlyEqual(First->GetTurn(), 1.f));
	// A load on the cage's step finds them all shut; one on the furnace's step finds them open again.
	Barge->ForceStep(BargeRules::CageStep);
	TestEqual(TEXT("Past the furnace's step: all shut"), Barge->CountValvesShut(), 3);
	Barge->ForceStep(BargeRules::FurnaceStep);
	TestEqual(TEXT("On it again (a load): open"), Barge->CountValvesShut(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeCageTest, "Looter.World.Barge.Cage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeCageTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, BargeRules::CageStep);
	ABargeFurnace* Furnace = Barge ? Barge->GetFurnace() : nullptr;
	if (!Furnace)
	{
		return false;
	}
	TestTrue(TEXT("Its step, the ring cold: the cage whole"), !Furnace->IsCageBurst() && Furnace->GetValvesShut() == 3);
	// No campaign here, so no lantern: the cage won't open without Abel's own lantern to carry him in.
	TestFalse(TEXT("Not without Abel's lantern"), Furnace->CanOpenCage());
	TestTrue(TEXT("...the story aside, it could"), Furnace->CanOpenCage(/*bForce*/ true));
	TestTrue(TEXT("The cage's heart faces into the hall"), FVector::Dist(Furnace->GetCageLocation(), Barge->GetHallMiddle()) < BargeRules::FurnaceRadius + 200.f);
	// Its burst's end state (the scene leaves it so): the heart gone, the barge's pilot light out.
	Barge->NoteCageBurst();
	TestTrue(TEXT("Burst: the cage open, the barge without her pilot light"), Furnace->IsCageBurst() && Barge->GetState() == EBargeState::Burst);
	TestFalse(TEXT("...nothing more to open"), Furnace->CanOpenCage(true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeDyingTest, "Looter.World.Barge.Dying",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeDyingTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Her list comes in over its seconds to 12 degrees"), FMath::IsNearlyZero(BargeRules::ListAt(0.f))
		&& FMath::IsNearlyEqual(BargeRules::ListAt(BargeRules::ListSeconds + 1.f), BargeRules::ListDegrees));
	TestTrue(TEXT("Her drift eases in, then holds its speed, never past its most"), BargeRules::DriftAt(1.f) < BargeRules::DriftSpeed
		&& BargeRules::DriftAt(100.f) <= BargeRules::DriftMost);

	FTestWorldWrapper TestLevel;
	const FVector Where(0.0, 0.0, 10000.0);
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, BargeRules::EscapeStep, Where);
	if (!Barge)
	{
		return false;
	}
	TestTrue(TEXT("The escape's step: she's dying"), Barge->IsDying() && Barge->GetFurnace()->IsCageBurst());
	const FVector JumpAtStart = Barge->GetJumpBerth().GetLocation();
	const FVector JumpOnHerAtStart = Barge->GetActorTransform().InverseTransformPosition(JumpAtStart);
	Barge->Advance(BargeRules::StackFallsAt + BargeRules::StackFallSeconds + 0.5f);
	const float Dying = Barge->GetDyingSeconds();
	TestTrue(TEXT("Drifting west"), FMath::IsNearlyEqual(static_cast<float>(Where.Y - Barge->GetActorLocation().Y), BargeRules::DriftAt(Dying), 2.f));
	TestTrue(TEXT("...listing to her berth side"), FMath::Abs(static_cast<float>(Barge->GetActorRotation().Roll)) > 5.f);
	// The berth hangs under the gangway's broken end, so it lists with her as well as drifting: her 12 degrees swing it about
	// a metre further west than the drift alone (it's 16.5 m out and 6 m up her frame). Fixed in her frame, then; west by
	// at least the drift.
	const FVector JumpNow = Barge->GetJumpBerth().GetLocation();
	TestTrue(TEXT("...the jump berth going with her"), Barge->GetActorTransform().InverseTransformPosition(JumpNow).Equals(JumpOnHerAtStart, 1.0)
		&& JumpAtStart.Y - JumpNow.Y >= BargeRules::DriftAt(Dying) - 2.f);
	TestTrue(TEXT("...the ship's berth kept level"), FMath::IsNearlyZero(static_cast<float>(Barge->GetJumpBerth().Rotator().Roll)));
	TestFalse(TEXT("The third stack is down across the port lane"), Barge->FallingStackPivot->GetRelativeRotation().Equals(FRotator::ZeroRotator, 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBargeFallTest, "Looter.World.Barge.Fall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBargeFallTest::RunTest(const FString& Parameters)
{
	const FTransform From(FRotator::ZeroRotator, FVector(0.0, 0.0, 10000.0));
	const FVector To(0.0, -30000.0, 1000.0);
	TestTrue(TEXT("Her fall starts where she was"), BargeRules::FallPose(From, To, 0.f).GetLocation().Equals(From.GetLocation(), 1.0));
	TestTrue(TEXT("...and ends on the canyon's floor"), BargeRules::FallPose(From, To, BargeRules::FallSeconds).GetLocation().Equals(To, 1.0));
	TestTrue(TEXT("...gliding first, high over the brink at the plunge"), BargeRules::FallPose(From, To, BargeRules::PlungeAt).GetLocation().Z
		> (From.GetLocation().Z + To.Z) * 0.5);

	FTestWorldWrapper TestLevel;
	ABargeBoarding* Barge = SpawnBarge(TestLevel, *this, BargeRules::EscapeStep, From.GetLocation());
	if (!Barge)
	{
		return false;
	}
	Barge->FallTarget = To;
	Barge->ApplyFallAt(6.f);
	TestTrue(TEXT("The scene has her: falling, on her way"), Barge->GetState() == EBargeState::Falling && Barge->GetActorLocation().Y < 0.0);
	Barge->ApplyFallAt(BargeRules::FallSeconds);
	TestTrue(TEXT("...down in the canyon"), Barge->GetActorLocation().Equals(To, 50.0));
	Barge->FinishFall();
	TestTrue(TEXT("Its end: gone from the sky, hidden, no collision"), Barge->GetState() == EBargeState::Fallen && Barge->IsHidden()
		&& !Barge->GetActorEnableCollision() && !Barge->IsPresent());
	return true;
}

#endif
