#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/BulletSubsystem.h"
#include "Player/FootfallDust.h"
#include "Player/PlayerFootfallComponent.h"
#include "Tests/LocomotionTestWorld.h"
#include "Weapons/WaterRings.h"
#include "Weapons/WeaponFX.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// Rings and wakes on the water (the water concept's point 5; Looter.FX.WaterRings.*): the rules (how far a ring spreads, how
// it fades, how many a hit leaves, how far apart a walk's wakes are), the pool (a budget, a cap, the dashes it draws, the
// camera range), the effects (a shot or a footfall on flat water leaves rings, a waterfall's face none) and the real
// character walking through a water-tagged floor and a dirt one.

namespace
{
	/** Close enough (floats that went through a Lerp). */
	bool Near(float A, float B, float Tolerance = 0.001f)
	{
		return FMath::Abs(A - B) <= Tolerance;
	}

	/** The farthest and the nearest a dash's middle is from Center, along the water (cm). */
	void DashReach(const TArray<FTransform>& Dashes, const FVector& Center, float& OutNearest, float& OutFarthest)
	{
		OutNearest = 0.f;
		OutFarthest = 0.f;
		bool bAny = false;
		for (const FTransform& Dash : Dashes)
		{
			const float Reach = static_cast<float>(FVector::Dist2D(Dash.GetLocation(), Center));
			OutNearest = bAny ? FMath::Min(OutNearest, Reach) : Reach;
			OutFarthest = FMath::Max(OutFarthest, Reach);
			bAny = true;
		}
	}

	/** The strongest opacity among the dashes (the fourth float of each). */
	float Strongest(const TArray<float>& SmokeData)
	{
		float Most = 0.f;
		for (int32 Index = 3; Index < SmokeData.Num(); Index += 4)
		{
			Most = FMath::Max(Most, SmokeData[Index]);
		}
		return Most;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterRingsRulesTest, "Looter.FX.WaterRings.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWaterRingsRulesTest::RunTest(const FString& Parameters)
{
	// How far a ring spreads: 0.6 m for the smallest hit, 1.2 m for the largest, wider for a bigger one.
	TestTrue(TEXT("A rifle shot's ring reaches between 0.6 and 1.2 m"), FWaterRings::MaxRadiusFor(1.f) > 60.f && FWaterRings::MaxRadiusFor(1.f) < 120.f);
	TestTrue(TEXT("...a wading step's the smallest, a hard landing's the largest"),
		Near(FWaterRings::MaxRadiusFor(0.5f), 60.f) && Near(FWaterRings::MaxRadiusFor(1.5f), 120.f));
	TestTrue(TEXT("...a bigger hit spreads wider"), FWaterRings::MaxRadiusFor(1.25f) > FWaterRings::MaxRadiusFor(1.f));
	TestTrue(TEXT("...and sizes out of range stay within it"), FWaterRings::MaxRadiusFor(0.f) >= 60.f && FWaterRings::MaxRadiusFor(9.f) <= 120.f);
	TestTrue(TEXT("A small ring lives 0.8 s, a large one 1.2 s"),
		Near(FWaterRings::LifeFor(60.f), 0.8f) && Near(FWaterRings::LifeFor(120.f), 1.2f));

	// Growth: starts at its small radius, ends at the largest, never shrinks, quick at first.
	float Previous = FWaterRings::RadiusAt(0.f, 6.f, 90.f);
	TestTrue(TEXT("A ring is born small"), Near(Previous, 6.f));
	bool bGrows = true;
	for (int32 Step = 1; Step <= 20; ++Step)
	{
		const float Radius = FWaterRings::RadiusAt(Step / 20.f, 6.f, 90.f);
		bGrows &= Radius >= Previous;
		Previous = Radius;
	}
	TestTrue(TEXT("...only ever spreads"), bGrows);
	TestTrue(TEXT("...to its full radius"), Near(FWaterRings::RadiusAt(1.f, 6.f, 90.f), 90.f));
	TestTrue(TEXT("...and is past halfway in half its life"), FWaterRings::RadiusAt(0.5f, 6.f, 90.f) > 48.f);

	// Fade: nothing at birth, a peak early, thinning to nothing.
	TestTrue(TEXT("A ring fades in from nothing"), FWaterRings::OpacityAt(0.f) == 0.f);
	float Peak = 0.f;
	float PeakAt = 0.f;
	for (int32 Step = 0; Step <= 50; ++Step)
	{
		const float Opacity = FWaterRings::OpacityAt(Step / 50.f);
		if (Opacity > Peak)
		{
			Peak = Opacity;
			PeakAt = Step / 50.f;
		}
	}
	TestTrue(FString::Printf(TEXT("...peaks early and no stronger than 1 (%.2f at %.2f)"), Peak, PeakAt), Peak > 0.5f && Peak <= 1.f && PeakAt < 0.2f);
	bool bThins = true;
	for (int32 Step = 4; Step < 10; ++Step)
	{
		bThins &= FWaterRings::OpacityAt((Step + 1) / 10.f) < FWaterRings::OpacityAt(Step / 10.f);
	}
	TestTrue(TEXT("...thins out from there"), bThins);
	TestTrue(TEXT("...to nothing at the end"), FWaterRings::OpacityAt(1.f) == 0.f);

	// How many a hit leaves: two for a shot, one for a pellet or a step.
	TestEqual(TEXT("A rifle shot: two rings"), FWaterRings::RingsFor(1.f), 2);
	TestEqual(TEXT("A revolver's: two"), FWaterRings::RingsFor(1.25f), 2);
	TestEqual(TEXT("A shotgun pellet: one"), FWaterRings::RingsFor(0.6f), 1);
	TestEqual(TEXT("A wading step (0.6 of a 0.85 body): one"), FWaterRings::RingsFor(0.6f * 0.85f), 1);

	// Only flat water holds rings.
	TestTrue(TEXT("Still water holds rings"), FWaterRings::LiesFlat(FVector::UpVector) && FWaterRings::LiesFlat(FVector(0.3, 0.0, 0.95).GetSafeNormal()));
	TestFalse(TEXT("A waterfall's face doesn't"), FWaterRings::LiesFlat(FVector::ForwardVector));
	TestFalse(TEXT("...nor a steep slope"), FWaterRings::LiesFlat(FVector(0.0, 0.0, 0.5)));

	// Dashes: ten for the smallest ring, eighteen for the largest.
	TestTrue(TEXT("A ring has 10 to 18 dashes"), FWaterRings::SegmentsFor(60.f) == 10 && FWaterRings::SegmentsFor(120.f) == 18
		&& FWaterRings::SegmentsFor(0.f) == 10 && FWaterRings::SegmentsFor(999.f) == 18);

	// The wake: one every 75 cm at the player's size, none creeping.
	TestFalse(TEXT("No wake before a stride's worth"), FWaterRings::WakeDue(74.f, 459.f, 1.f));
	TestTrue(TEXT("...one at 75 cm"), FWaterRings::WakeDue(75.f, 459.f, 1.f));
	TestFalse(TEXT("...none when barely moving, however far"), FWaterRings::WakeDue(500.f, 100.f, 1.f));
	TestTrue(TEXT("...a smaller body's spacing is smaller"), FWaterRings::WakeDue(64.f, 459.f, 0.85f) && !FWaterRings::WakeDue(60.f, 459.f, 0.85f));
	TestTrue(TEXT("A faster walk leaves a stronger wake"), FWaterRings::WakeOpacityFor(711.f) > FWaterRings::WakeOpacityFor(459.f)
		&& FWaterRings::WakeOpacityFor(459.f) > FWaterRings::WakeOpacityFor(150.f));
	TestTrue(TEXT("...between 0.45 and 1"), Near(FWaterRings::WakeOpacityFor(0.f), 0.45f) && Near(FWaterRings::WakeOpacityFor(2000.f), 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterRingsPoolTest, "Looter.FX.WaterRings.Pool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWaterRingsPoolTest::RunTest(const FString& Parameters)
{
	const FVector Center(100.0, 200.0, 50.0);
	const FVector Up = FVector::UpVector;
	const FLinearColor Foam = FWaterRings::FoamAlbedo();

	// A shot: two concentric rings, the second a beat behind, so only the first is drawn at first.
	FWaterRings Pool;
	TestTrue(TEXT("The pool starts empty with a full budget"), Pool.IsEmpty() && Near(Pool.GetBudget(), FWaterRings::BudgetCap));
	TestEqual(TEXT("A shot leaves two rings"), Pool.AddRings(Center, Up, 1.f, FWaterRings::RingsFor(1.f), Foam), 2);
	TestEqual(TEXT("...both in the pool"), Pool.NumRings(), 2);
	TestTrue(TEXT("...paid for from the budget"), Near(Pool.GetBudget(), FWaterRings::BudgetCap - 2.f * FWaterRings::RingCost));
	TArray<FTransform> Dashes;
	TArray<float> Data;
	Pool.Draw(FVector(0.0, 0.0, 300.0), Dashes, Data);
	const int32 FirstDashes = Dashes.Num();
	TestTrue(FString::Printf(TEXT("Only the first ring is drawn at first (%d dashes)"), FirstDashes),
		FirstDashes > 0 && FirstDashes <= FWaterRings::SegmentsFor(FWaterRings::MaxRadiusFor(1.f)));
	TestEqual(TEXT("...four floats a dash (colour and opacity)"), Data.Num(), FirstDashes * 4);
	Pool.Tick(0.15f);
	Dashes.Reset();
	Data.Reset();
	Pool.Draw(FVector(0.0, 0.0, 300.0), Dashes, Data);
	TestTrue(FString::Printf(TEXT("A beat later the second is drawn too (%d dashes)"), Dashes.Num()), Dashes.Num() > FirstDashes);

	// One ring's geometry: dashes lie flat on the water, on a circle that spreads and fades.
	FWaterRings One;
	One.AddRings(Center, Up, 1.f, 1, Foam);
	float EarlyNearest = 0.f, EarlyFarthest = 0.f, LateNearest = 0.f, LateFarthest = 0.f, EndFarthest = 0.f, Unused = 0.f;
	float EarlyStrength = 0.f, LateStrength = 0.f;
	bool bFlat = true;
	for (int32 Frame = 1; Frame <= 9; ++Frame)
	{
		One.Tick(0.1f);
		if (Frame != 1 && Frame != 5 && Frame != 9)
		{
			continue;
		}
		Dashes.Reset();
		Data.Reset();
		One.Draw(FVector(0.0, 0.0, 300.0), Dashes, Data);
		for (const FTransform& Dash : Dashes)
		{
			bFlat &= FVector::DotProduct(Dash.GetRotation().GetUpVector(), Up) > 0.999 && FMath::Abs(Dash.GetLocation().Z - Center.Z) < 1.0;
		}
		if (Frame == 1)
		{
			DashReach(Dashes, Center, EarlyNearest, EarlyFarthest);
			EarlyStrength = Strongest(Data);
		}
		else if (Frame == 5)
		{
			DashReach(Dashes, Center, LateNearest, LateFarthest);
		}
		else
		{
			DashReach(Dashes, Center, Unused, EndFarthest);
			LateStrength = Strongest(Data);
		}
	}
	TestTrue(TEXT("Every dash lies flat on the water"), bFlat);
	TestTrue(FString::Printf(TEXT("A ring's dashes are one circle (%.1f to %.1f cm)"), EarlyNearest, EarlyFarthest), EarlyFarthest - EarlyNearest < 0.5f);
	TestTrue(FString::Printf(TEXT("...that spreads (%.0f, %.0f, %.0f cm)"), EarlyFarthest, LateFarthest, EndFarthest),
		EarlyFarthest < LateFarthest && LateFarthest < EndFarthest);
	TestTrue(FString::Printf(TEXT("...the radius the rule says (%.1f cm)"), LateFarthest),
		Near(LateFarthest, FWaterRings::RadiusAt(0.5f, 5.f, FWaterRings::MaxRadiusFor(1.f)), 1.f));
	TestTrue(FString::Printf(TEXT("...and fades (%.2f, then %.3f)"), EarlyStrength, LateStrength), EarlyStrength > 0.2f && LateStrength < EarlyStrength * 0.2f);

	// A burst into the water thins to the budget; a moment later there are rings again.
	FWaterRings Burst;
	int32 Made = 0;
	for (int32 Shot = 0; Shot < 20; ++Shot)
	{
		Made += Burst.AddRings(Center, Up, 1.f, 2, Foam);
	}
	TestEqual(TEXT("Twenty shots at once make as many rings as the budget pays"), Made, FMath::FloorToInt32(FWaterRings::BudgetCap / FWaterRings::RingCost));
	TestEqual(TEXT("...and the next is refused"), Burst.AddRings(Center, Up, 1.f, 2, Foam), 0);
	for (int32 Frame = 0; Frame < 10; ++Frame)
	{
		Burst.Tick(0.1f);
	}
	TestTrue(TEXT("A second on the budget is back"), Near(Burst.GetBudget(), FWaterRings::BudgetCap));
	TestEqual(TEXT("...and rings come again"), Burst.AddRings(Center, Up, 1.f, 2, Foam), 2);

	// Hammered for a long time, the pool never passes its cap.
	FWaterRings Hammered;
	int32 MostRings = 0;
	for (int32 Frame = 0; Frame < 300; ++Frame)
	{
		Hammered.AddRings(Center, Up, 1.f, 2, Foam);
		Hammered.Tick(0.03f);
		MostRings = FMath::Max(MostRings, Hammered.NumRings());
	}
	TestTrue(FString::Printf(TEXT("Never more than %d rings alive (%d)"), FWaterRings::MaxRings, MostRings), MostRings > 4 && MostRings <= FWaterRings::MaxRings);

	// Not on a waterfall's face; not out of the camera's range; and a pool with no camera draws it all.
	FWaterRings Edge;
	TestEqual(TEXT("A vertical face holds no rings"), Edge.AddRings(Center, FVector::ForwardVector, 1.f, 2, Foam), 0);
	TestTrue(TEXT("...and nothing was spent"), Edge.IsEmpty() && Near(Edge.GetBudget(), FWaterRings::BudgetCap));
	const FVector Far(9000.0, 0.0, 50.0);
	Edge.AddRings(Far, Up, 1.f, 1, Foam);
	Edge.Tick(0.1f);
	Dashes.Reset();
	Data.Reset();
	Edge.Draw(FVector(0.0, 0.0, 1.0), Dashes, Data);
	TestEqual(TEXT("Rings 90 m from the camera aren't drawn"), Dashes.Num(), 0);
	Edge.Draw(Far + FVector(500.0, 0.0, 200.0), Dashes, Data);
	TestTrue(TEXT("...near it they are"), Dashes.Num() > 0);
	Dashes.Reset();
	Data.Reset();
	Edge.Draw(FVector::ZeroVector, Dashes, Data);
	TestTrue(TEXT("...and with no camera known they are"), Dashes.Num() > 0);

	// They end: a shot's rings are gone in a second and a bit.
	for (int32 Frame = 0; Frame < 12; ++Frame)
	{
		One.Tick(0.1f);
		Pool.Tick(0.1f);
	}
	TestTrue(TEXT("A second and a bit on, every ring is gone"), One.IsEmpty() && Pool.IsEmpty());

	// The wake: a V trailing back from the way the body goes, its two arms on either side of the track.
	FWaterRings Wake;
	const FVector Going(1.0, 0.0, 0.0);
	TestTrue(TEXT("A wake on flat water"), Wake.AddWake(Center, Up, Going, 459.f, 0.85f, Foam));
	TestEqual(TEXT("...is one in the pool"), Wake.NumWakes(), 1);
	Dashes.Reset();
	Data.Reset();
	Wake.Draw(FVector(0.0, 0.0, 300.0), Dashes, Data);
	if (TestEqual(TEXT("...of two arms"), Dashes.Num(), 2))
	{
		const FVector A = Dashes[0].GetLocation() - Center;
		const FVector B = Dashes[1].GetLocation() - Center;
		TestTrue(TEXT("...both trailing back from the track"), A.X < -5.0 && B.X < -5.0);
		TestTrue(TEXT("...one each side of it"), A.Y * B.Y < 0.0);
		TestTrue(TEXT("...lying flat"), FVector::DotProduct(Dashes[0].GetRotation().GetUpVector(), Up) > 0.999);
	}
	TestFalse(TEXT("No direction, no wake"), Wake.AddWake(Center, Up, FVector::ZeroVector, 459.f, 1.f, Foam));
	TestFalse(TEXT("A wake going straight up the water's face is none"), Wake.AddWake(Center, Up, Up, 459.f, 1.f, Foam));
	TestFalse(TEXT("None on a vertical face"), Wake.AddWake(Center, FVector::ForwardVector, Going, 459.f, 1.f, Foam));
	for (int32 Index = 0; Index < 30; ++Index)
	{
		Wake.AddWake(Center + Going * (80.0 * Index), Up, Going, 459.f, 1.f, Foam);
	}
	TestEqual(TEXT("Thirty wakes in a row: the cap holds"), Wake.NumWakes(), FWaterRings::MaxWakes);
	for (int32 Frame = 0; Frame < 12; ++Frame)
	{
		Wake.Tick(0.1f);
	}
	TestTrue(TEXT("A second and a bit on, the wakes are gone"), Wake.IsEmpty());

	Pool.AddRings(Center, Up, 1.f, 1, Foam);
	Pool.Reset();
	TestTrue(TEXT("Reset empties the pool and refills the budget"), Pool.IsEmpty() && Near(Pool.GetBudget(), FWaterRings::BudgetCap));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterRingsEffectsTest, "Looter.FX.WaterRings.Effects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWaterRingsEffectsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	FWeaponFX Effects;
	Effects.Initialize(World);
	auto Run = [&Effects](float Seconds)
	{
		for (float Time = 0.f; Time < Seconds; Time += 1.f / 60.f)
		{
			Effects.Tick(1.f / 60.f, FVector(-500.0, 0.0, 170.0));
		}
	};
	const FVector Surface(0.0, 0.0, 0.0);
	const FVector Shot = FVector(1.0, 0.0, -0.3).GetSafeNormal();

	// A rifle shot into flat water: two rings, with the splash.
	Effects.SpawnSurfaceImpact(Surface, FVector::UpVector, Shot, EImpactMaterial::Water, 1.f, false);
	TestEqual(TEXT("A rifle shot into the water leaves two rings"), Effects.NumWaterRings(), 2);
	TestTrue(TEXT("...with its splash"), Effects.NumParticles() > 0);
	Run(0.3f);
	TestFalse(TEXT("...and the effects keep ticking while they spread"), Effects.IsIdle());
	Run(2.5f);
	TestEqual(TEXT("A few seconds on the rings are gone"), Effects.NumWaterRings(), 0);
	Effects.Tick(1.f / 60.f, FVector::ZeroVector);
	TestTrue(TEXT("...and nothing is left to tick"), Effects.IsIdle());

	// A shotgun pellet: one. A dirt hit and a waterfall's face: none.
	Effects.SpawnSurfaceImpact(Surface, FVector::UpVector, Shot, EImpactMaterial::Water, 0.6f, false);
	TestEqual(TEXT("A pellet leaves one ring"), Effects.NumWaterRings(), 1);
	Run(2.f);
	Effects.SpawnSurfaceImpact(Surface, FVector::UpVector, Shot, EImpactMaterial::Dirt, 1.f, false);
	TestEqual(TEXT("Dirt leaves none"), Effects.NumWaterRings(), 0);
	Effects.SpawnSurfaceImpact(Surface, FVector::ForwardVector, FVector(-1.0, 0.0, 0.0), EImpactMaterial::Water, 1.f, false);
	TestEqual(TEXT("A waterfall's face leaves none"), Effects.NumWaterRings(), 0);
	TestTrue(TEXT("...though it splashes"), Effects.NumParticles() > 0);
	Run(2.5f);

	// A wake, and the footfall's splash: one ring for a step, two for a hard landing.
	TestTrue(TEXT("A wake on flat water"), Effects.SpawnWake(Surface, FVector::UpVector, FVector::ForwardVector, 459.f, 0.85f));
	TestEqual(TEXT("...is one in the pool"), Effects.NumWakes(), 1);
	TestFalse(TEXT("None on a waterfall's face"), Effects.SpawnWake(Surface, FVector::ForwardVector, FVector::UpVector, 459.f, 0.85f));
	Run(2.5f);
	TestEqual(TEXT("A second and a bit on the wake is gone"), Effects.NumWakes(), 0);

	FFootfallSpot Spot;
	Spot.Scale = 0.85f;
	Spot.Along = FVector::ForwardVector;
	Spot.Speed = 600.f;
	FFootfallDust Wet;
	Spot.Hardness = 0.f;
	Wet.Throw(Effects, EFootfall::Step, EImpactMaterial::Water, Spot, FLinearColor::White);
	TestEqual(TEXT("A sprinting step in the water: one ring"), Effects.NumWaterRings(), 1);
	Run(2.f);
	Spot.Hardness = 1.f;
	FFootfallDust Wetter;
	Wetter.Throw(Effects, EFootfall::Land, EImpactMaterial::Water, Spot, FLinearColor::White);
	TestEqual(TEXT("A hard landing in it: two"), Effects.NumWaterRings(), 2);
	Effects.Shutdown();
	TestEqual(TEXT("Shut down, the rings go with it"), Effects.NumWaterRings(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterRingsWadeTest, "Looter.FX.WaterRings.Wade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWaterRingsWadeTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	FEditorScriptExecutionGuard RunActorEvents;
	UWorld* World = TestLevel.GetTestWorld();
	UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
	if (!TestNotNull(TEXT("The bullets (and their effects) are there"), Bullets))
	{
		return false;
	}

	struct FWadeGround
	{
		const TCHAR* Name;
		const TCHAR* Tag;
		FVector Feet;
		bool bWet;
	};
	const FWadeGround Grounds[] = {
		{ TEXT("Water"), TEXT("Surface.Water"), FVector::ZeroVector, true },
		{ TEXT("Dirt"), TEXT("Surface.Dirt"), FVector(0.0, 4000.0, 0.0), false },
	};
	for (const FWadeGround& Ground : Grounds)
	{
		AStaticMeshActor* Block = LocomotionTestWorld::SpawnBlock(World, Ground.Feet - FVector(0.0, 0.0, 50.0), FVector(1500.0, 1500.0, 100.0));
		if (!TestNotNull(FString::Printf(TEXT("%s: the ground"), Ground.Name), Block))
		{
			return false;
		}
		Block->Tags.Add(FName(Ground.Tag));
		UPlayerLocomotionComponent* Locomotion = LocomotionTestWorld::SpawnPlayer(World, Ground.Feet);
		ACharacter* Body = Locomotion ? LocomotionTestWorld::BodyOf(Locomotion) : nullptr;
		UPlayerFootfallComponent* Feet = Body ? Body->FindComponentByClass<UPlayerFootfallComponent>() : nullptr;
		if (!TestNotNull(FString::Printf(TEXT("%s: the player's feet"), Ground.Name), Feet))
		{
			return false;
		}
		// A walk at the player's pace for a second and a half: no sprint, so only the wading acts.
		Body->GetCharacterMovement()->Velocity = FVector(460.0, 0.0, 0.0);
		FWeaponFX& Effects = Bullets->GetEffects();
		const int32 RingsBefore = Effects.NumWaterRings();
		const int32 WakesBefore = Effects.NumWakes();
		for (int32 Frame = 0; Frame < 90; ++Frame)
		{
			Feet->TickComponent(LocomotionTestWorld::Frame, LEVELTICK_All, nullptr);
		}
		const int32 Rings = Effects.NumWaterRings() - RingsBefore;
		const int32 Wakes = Effects.NumWakes() - WakesBefore;
		AddInfo(FString::Printf(TEXT("%s: %d rings, %d wakes"), Ground.Name, Rings, Wakes));
		if (Ground.bWet)
		{
			TestTrue(FString::Printf(TEXT("%s: a ring at the steps (%d)"), Ground.Name, Rings), Rings >= 2);
			TestTrue(FString::Printf(TEXT("%s: a wake along the walk (%d), within the cap"), Ground.Name, Wakes), Wakes >= 3 && Wakes <= FWaterRings::MaxWakes);
		}
		else
		{
			TestTrue(FString::Printf(TEXT("%s: dry ground leaves nothing (%d rings, %d wakes)"), Ground.Name, Rings, Wakes), Rings == 0 && Wakes == 0);
		}
	}
	return true;
}

#endif
