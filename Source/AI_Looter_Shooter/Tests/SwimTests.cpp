#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/FirstPersonHandsSwim.h"
#include "Player/PlayerLocomotionComponent.h"
#include "Player/PlayerSize.h"
#include "Tests/LocomotionTestWorld.h"
#include "World/SwimWater.h"
#include "World/SwimWaterRules.h"
#include "World/SwimWaterSubsystem.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// Swimming (round 4; Looter.Swim.*): the plain rules (when water swims and when the bed takes the body back, the float, the dive's
// dead zone, the breath that never hurts), the waters' outlines, the hands' stroke, and the real character swimming in a test
// level: in over the chest, not in a shallow pool, back on foot over a shallow bed, and creatures' deep water.

using namespace LocomotionTestWorld;

namespace
{
	/** A square of half-width Half round Center (world XY). */
	TArray<FVector2D> Square(const FVector2D& Center, double Half)
	{
		return { Center + FVector2D(-Half, -Half), Center + FVector2D(Half, -Half), Center + FVector2D(Half, Half), Center + FVector2D(-Half, Half) };
	}

	/** A water at Surface over Center, its shore Half across and deep within DeepHalf (0: a wading pool), begun. */
	ASwimWater* SpawnWater(UWorld* World, const FVector2D& Center, float Surface, double Half, double DeepHalf)
	{
		ASwimWater* Water = World->SpawnActor<ASwimWater>(ASwimWater::StaticClass(), FTransform(FVector(Center.X, Center.Y, Surface)));
		if (!Water)
		{
			return nullptr;
		}
		Water->Shore = Square(Center, Half);
		if (DeepHalf > 0.0)
		{
			Water->DeepZone = Square(Center, DeepHalf);
		}
		Water->WaterId = TEXT("TestPond");
		Water->DispatchBeginPlay();
		return Water;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSwimRulesTest, "Looter.Swim.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSwimRulesTest::RunTest(const FString& Parameters)
{
	const float Scale = LooterPlayerSize::Scale;
	const float Start = SwimRules::StartDepth(Scale);
	const float Stand = SwimRules::StandDepth(Scale);
	TestEqual(TEXT("Chest deep on the full-size body (135 cm), at the player's size"), Start, 135.f * Scale, 0.01f);
	TestTrue(TEXT("Deeper than that swims"), SwimRules::ShouldSwim(Start + 1.f, Scale));
	TestFalse(TEXT("...shallower wades"), SwimRules::ShouldSwim(Start - 1.f, Scale));
	TestTrue(TEXT("The bed takes the body back a little shallower than the swim starts (no flicker at a pool's edge)"), Stand < Start - 1.f);
	TestTrue(TEXT("...where it's shallower than that"), SwimRules::CanStand(Stand - 1.f, Scale) && !SwimRules::CanStand(Stand + 1.f, Scale));

	// Floating: the eye just over the surface; the spring heads there, gently up from under, firmer down from a jump in.
	const float Target = SwimRules::FloatFeetZ(100.f, 140.f, 1.f);
	TestEqual(TEXT("Floating, the eye rides 12 cm over the surface"), Target, 100.f + 12.f - 140.f, 0.01f);
	const float Up = SwimRules::FloatSpeed(Target - 300.f, Target, 1.f);
	const float Down = SwimRules::FloatSpeed(Target + 300.f, Target, 1.f);
	TestTrue(FString::Printf(TEXT("From deep under it drifts up gently (%.0f cm/s)"), Up), Up > 0.f && Up <= 70.f + 0.01f);
	TestTrue(FString::Printf(TEXT("From over it, it sinks (%.0f cm/s)"), Down), Down < 0.f && Down >= -300.f - 0.01f);
	TestEqual(TEXT("At the float height, it holds"), SwimRules::FloatSpeed(Target, Target, 1.f), 0.f, 0.01f);

	// The dive: a glance down never dives; looking well down (or up) goes that way.
	TestEqual(TEXT("Level: no dive"), SwimRules::DiveShare(0.f), 0.f, 0.001f);
	TestEqual(TEXT("A glance down (20 degrees): no dive"), SwimRules::DiveShare(-20.f), 0.f, 0.001f);
	TestEqual(TEXT("Looking straight down: all the way down"), SwimRules::DiveShare(-90.f), -1.f, 0.001f);
	TestEqual(TEXT("Looking straight up: all the way up"), SwimRules::DiveShare(90.f), 1.f, 0.001f);
	TestTrue(TEXT("...and more the further down"), SwimRules::DiveShare(-50.f) < SwimRules::DiveShare(-35.f) && SwimRules::DiveShare(-35.f) < 0.f);

	// Pace: slower than walking, faster with the sprint key; the stroke quicker the faster the swim.
	const float Walk = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::MoveScale;
	TestTrue(TEXT("Swimming is slower than walking, faster with the sprint key"),
		SwimRules::SwimSpeed(Walk, false) < SwimRules::SwimSpeed(Walk, true) && SwimRules::SwimSpeed(Walk, true) < Walk);
	TestTrue(TEXT("Strokes come quicker swimming hard than treading water"), SwimRules::StrokeRate(1.f) > SwimRules::StrokeRate(0.f));

	// Breath: about 22 s under, 3.5 s to fill again; out of it nothing hurts (the body is carried up); a gasp when short.
	float Breath = 1.f;
	float Under = 0.f;
	while (!SwimRules::OutOfBreath(Breath) && Under < 60.f)
	{
		Breath = SwimRules::BreathAfter(Breath, 0.1f, true);
		Under += 0.1f;
	}
	TestTrue(FString::Printf(TEXT("A full breath lasts about 22 s under (%.1f s)"), Under), FMath::IsNearlyEqual(Under, 22.f, 0.2f));
	float Air = 0.f;
	while (Breath < 1.f && Air < 60.f)
	{
		Breath = SwimRules::BreathAfter(Breath, 0.1f, false);
		Air += 0.1f;
	}
	TestTrue(FString::Printf(TEXT("...and fills again in about 3.5 s in the air (%.1f s)"), Air), FMath::IsNearlyEqual(Air, 3.5f, 0.2f));
	TestTrue(TEXT("Surfacing short of breath gasps"), SwimRules::GaspOnSurfacing(0.3f) && !SwimRules::GaspOnSurfacing(0.9f));
	TestTrue(TEXT("Out of breath, the dive comes back part-filled"), SwimRules::BreathBack() > 0.f && SwimRules::BreathBack() < 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSwimShoreTest, "Looter.Swim.Shore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSwimShoreTest::RunTest(const FString& Parameters)
{
	const TArray<FVector2D> Pond = SwimShore::Ellipse(FVector2D(20000.0, -2000.0), FVector2D(600.0, 900.0), 32);
	TestEqual(TEXT("An ellipse's outline has its corners"), Pond.Num(), 32);
	TestTrue(TEXT("Its middle is inside"), SwimShore::Contains(Pond, FVector2D(20000.0, -2000.0)));
	TestTrue(TEXT("...and a point near its long end"), SwimShore::Contains(Pond, FVector2D(20000.0, -2000.0 + 850.0)));
	TestFalse(TEXT("...not past its short side"), SwimShore::Contains(Pond, FVector2D(20000.0 + 650.0, -2000.0)));
	const FBox2D Box = SwimShore::BoundsOf(Pond);
	TestTrue(TEXT("Its box holds it"), Box.bIsValid && FMath::IsNearlyEqual(Box.Max.X, 20600.0, 1.0) && FMath::IsNearlyEqual(Box.Min.Y, -2900.0, 1.0));
	TestFalse(TEXT("Fewer than three corners hold nothing"), SwimShore::Contains({ FVector2D(0.0, 0.0), FVector2D(10.0, 0.0) }, FVector2D(5.0, 0.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSwimHandsTest, "Looter.Swim.Hands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSwimHandsTest::RunTest(const FString& Parameters)
{
	// The stroke goes round without a jump, the hands mirror each other, and stay ahead of the eye and under the crosshair.
	const FHandPlacement Begin = FirstPersonHandsSwim::Stroke(EFirstPersonHand::Right, 0.f);
	const FHandPlacement End = FirstPersonHandsSwim::Stroke(EFirstPersonHand::Right, 0.9999f);
	TestTrue(FString::Printf(TEXT("The stroke comes round to where it began (%.2f cm off)"), FVector::Dist(Begin.Hold, End.Hold)),
		FVector::Dist(Begin.Hold, End.Hold) < 0.5f);
	float Farthest = 0.f;
	FVector Last = Begin.Hold;
	float BiggestStep = 0.f;
	for (int32 Sample = 0; Sample <= 100; ++Sample)
	{
		const float Phase = Sample / 100.f;
		const FHandPlacement Right = FirstPersonHandsSwim::Stroke(EFirstPersonHand::Right, Phase);
		const FHandPlacement Left = FirstPersonHandsSwim::Stroke(EFirstPersonHand::Left, Phase);
		TestTrue(FString::Printf(TEXT("At %.2f the hands mirror each other"), Phase),
			FMath::IsNearlyEqual(Left.Hold.Y, -Right.Hold.Y, 0.01) && FMath::IsNearlyEqual(Left.Hold.X, Right.Hold.X, 0.01));
		TestTrue(FString::Printf(TEXT("At %.2f the hand is ahead of the eye and under the crosshair (%.0f, %.0f, %.0f)"), Phase,
			Right.Hold.X, Right.Hold.Y, Right.Hold.Z), Right.Hold.X > 10.0 && Right.Hold.Z < -5.0);
		Farthest = FMath::Max(Farthest, static_cast<float>(Right.Hold.X));
		BiggestStep = FMath::Max(BiggestStep, static_cast<float>(FVector::Dist(Right.Hold, Last)));
		Last = Right.Hold;
	}
	TestTrue(FString::Printf(TEXT("The stroke reaches out ahead (%.0f cm)"), Farthest), Farthest >= 40.f);
	TestTrue(FString::Printf(TEXT("...smoothly (at most %.1f cm a hundredth of a stroke)"), BiggestStep), BiggestStep < 4.f);
	const FHandPlacement Tread = FirstPersonHandsSwim::Tread(EFirstPersonHand::Right, 1.f);
	const FHandPlacement Still = FirstPersonHandsSwim::Pose(EFirstPersonHand::Right, 0.4f, 0.f, 1.f);
	TestTrue(TEXT("No effort: treading water"), FVector::Dist(Tread.Hold, Still.Hold) < 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSwimPlayTest, "Looter.Swim.Play",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSwimPlayTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	// A pond 2 m deep (its bed's top at -200) with a shallow shelf in it (top at -60), and a wading pool beside it (0.8 m).
	const AStaticMeshActor* Bed = SpawnBlock(World, FVector(0.0, 0.0, -250.0), FVector(4000.0, 4000.0, 100.0));
	const AStaticMeshActor* Shelf = SpawnBlock(World, FVector(1200.0, 0.0, -110.0), FVector(400.0, 400.0, 100.0));
	const AStaticMeshActor* PoolBed = SpawnBlock(World, FVector(5000.0, 0.0, -130.0), FVector(2000.0, 2000.0, 100.0));
	ASwimWater* Pond = SpawnWater(World, FVector2D(0.0, 0.0), 0.f, 1600.0, 1000.0);
	ASwimWater* Pool = SpawnWater(World, FVector2D(5000.0, 0.0), 0.f, 900.0, 0.0);
	UPlayerLocomotionComponent* Swimmer = SpawnPlayer(World, FVector(0.0, 0.0, -200.0));
	UPlayerLocomotionComponent* Wader = SpawnPlayer(World, FVector(5000.0, 0.0, -80.0));
	if (!Bed || !Shelf || !PoolBed || !Pond || !Pool || !Swimmer || !Wader)
	{
		AddError(TEXT("The level wasn't made."));
		return false;
	}
	const USwimWaterSubsystem* Waters = USwimWaterSubsystem::Get(World);
	TestTrue(TEXT("The waters registered as play began"), Waters && Waters->NumWaters() == 2);

	// On the bed two metres down: over the chest, it swims.
	Step(Swimmer);
	ACharacter* SwimmerBody = BodyOf(Swimmer);
	UCharacterMovementComponent* Movement = SwimmerBody->GetCharacterMovement();
	TestTrue(TEXT("Two metres of water over the feet: swimming"), Swimmer->IsSwimming() && Swimmer->GetSwimWater() == Pond);
	TestTrue(TEXT("...the movement flies (no gravity) meanwhile"), Movement->MovementMode == MOVE_Flying);
	TestTrue(TEXT("...and asked from outside, the player is swimming (no gun, strike, throw or cast)"),
		UPlayerLocomotionComponent::IsSwimmingActor(SwimmerBody));
	TestTrue(TEXT("...at the swim's pace, slower than the walk"), Movement->MaxFlySpeed < Movement->MaxWalkSpeed + 1.f);

	// A wading pool 0.8 m deep: never a swim.
	Step(Wader);
	TestFalse(TEXT("Eighty centimetres: wading, not swimming"), Wader->IsSwimming());

	// Over the shelf (60 cm under the surface): the bed takes the body back.
	SwimmerBody->SetActorLocation(FVector(1200.0, 0.0, -60.0 + SwimmerBody->GetDefaultHalfHeight() + 2.0), false, nullptr, ETeleportType::TeleportPhysics);
	for (int32 Tick = 0; Tick < 12 && Swimmer->IsSwimming(); ++Tick)
	{
		Step(Swimmer);
	}
	TestFalse(TEXT("Over a shallow bed: back on foot"), Swimmer->IsSwimming());
	TestTrue(TEXT("...the movement falls onto it (no more flying)"), Movement->MovementMode != MOVE_Flying);

	// Creatures' deep water: in the pond's deep middle (even an actor's middle a little over the surface), not its edge, not the pool.
	TestTrue(TEXT("The pond's middle is deep water"), USwimWaterSubsystem::IsDeepAt(World, FVector(0.0, 0.0, -50.0)));
	TestTrue(TEXT("...a swimmer's middle just over the surface too"), USwimWaterSubsystem::IsDeepAt(World, FVector(0.0, 0.0, 40.0)));
	TestFalse(TEXT("...but not high over it (a cliff above)"), USwimWaterSubsystem::IsDeepAt(World, FVector(0.0, 0.0, 500.0)));
	TestFalse(TEXT("...nor the pond's shallow edge"), USwimWaterSubsystem::IsDeepAt(World, FVector(1400.0, 0.0, -50.0)));
	TestFalse(TEXT("...nor the wading pool"), USwimWaterSubsystem::IsDeepAt(World, FVector(5000.0, 0.0, -50.0)));
	return true;
}

#endif
