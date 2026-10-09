#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerSize.h"
#include "Player/PlayerTraversal.h"
#include "Player/TraversalCurve.h"
#include "Player/TraversalRules.h"

// The traversal's rules and planning, without a world: the curves the moves ride, a mantle's and a vault's paths (the
// body clear of the obstacle, the eye smooth and over it, the run's speed kept). Played out on the real character in a
// test level: LocomotionTraversalPlayTests.cpp.

namespace
{
	constexpr float Frame = 1.f / 60.f;
	/** The player's capsule at its size: the full-size 42 x 96 scaled by 0.85 (the tests on the real one read its own). */
	constexpr float Radius = 42.f * LooterPlayerSize::Scale;
	constexpr float HalfHeight = 96.f * LooterPlayerSize::Scale;
	constexpr float StandingEye = 140.f;

	/**
	 * The eye's limits at 60 fps through a move. A mantle lifts the view a metre in about 0.4 s, so its steps are bigger
	 * than a slide's 8 cm (Looter.Locomotion.Slide.View): at most 10 cm a frame (600 cm/s). How much a step changes from
	 * one frame to the next, the jolt the slide's bug was, keeps the slide's 1.5 cm limit.
	 */
	constexpr float EyeStepLimit = 10.f;
	constexpr float EyeStepChangeLimit = 1.5f;

	/** A setup at the player's size, standing on the ground at the origin, facing +X. */
	FTraversalSetup PlayerSetup(ETraversalKind Kind)
	{
		FTraversalSetup Setup;
		Setup.Kind = Kind;
		Setup.Origin = FVector(0.0, 0.0, HalfHeight);
		Setup.Direction = FVector::ForwardVector;
		Setup.Radius = Radius;
		Setup.HalfHeight = HalfHeight;
		Setup.EyeZ = StandingEye;
		Setup.EyeForward = 15.f;
		return Setup;
	}

	/**
	 * The eye's worst step and step change at 60 fps over the move, and the slowest speed along the way. The frame before
	 * the move is the eye's own motion as it began (Setup's speed and acceleration: a fall's, when a ledge is caught), so
	 * the first step is judged against the step the eye was already making, not against standing still.
	 */
	void SampleEye(const FPlayerTraversal& Move, const FTraversalSetup& Setup, float& OutStep, float& OutChange, float& OutSlowest)
	{
		OutStep = 0.f;
		OutChange = 0.f;
		OutSlowest = UE_BIG_NUMBER;
		double LastEye = Move.EyeAt(0.0);
		double LastStep = Setup.EyeSpeed * Frame - 0.5 * Setup.EyeAcceleration * Frame * Frame;
		const int32 Frames = FMath::CeilToInt(Move.GetDuration() / Frame) + 2;
		for (int32 Index = 1; Index <= Frames; ++Index)
		{
			const double At = Index * Frame;
			const double Eye = Move.EyeAt(At);
			const double Step = Eye - LastEye;
			OutStep = FMath::Max(OutStep, static_cast<float>(FMath::Abs(Step)));
			OutChange = FMath::Max(OutChange, static_cast<float>(FMath::Abs(Step - LastStep)));
			LastEye = Eye;
			LastStep = Step;
			if (At <= Move.GetDuration())
			{
				OutSlowest = FMath::Min(OutSlowest, static_cast<float>(Move.AlongSpeedAt(At)));
			}
		}
	}

	/** The capsule's bottom's worst dip under what it must stay out of (sampled finely; 0 when clear). */
	float WorstCornerMiss(const FPlayerTraversal& Move, const FTraversalSetup& Setup)
	{
		float Worst = 0.f;
		for (double At = 0.0; At <= Move.GetDuration(); At += 1.0 / 240.0)
		{
			Worst = FMath::Max(Worst, static_cast<float>(FPlayerTraversal::RequiredFeet(Setup, Move.AlongAt(At)) - Move.FeetAt(At)));
		}
		return Worst;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTraversalCurveTest, "Looter.Locomotion.Traversal.Curve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTraversalCurveTest::RunTest(const FString& Parameters)
{
	// The quintics a move rides: each piece arrives where it was asked, at the speed and acceleration asked, and the next
	// carries on from there, so nothing riding them ever jumps.
	FTraversalCurve Curve;
	Curve.Begin(0.0, 100.0, -980.0);
	Curve.Add(0.3, 50.0, 20.0, 5.0);
	Curve.Add(0.5, 60.0);
	TestTrue(TEXT("Starts where it began, at its speed and acceleration"), FMath::IsNearlyEqual(Curve.Value(0.0), 0.0, 1.e-6)
		&& FMath::IsNearlyEqual(Curve.Speed(0.0), 100.0, 1.e-6) && FMath::IsNearlyEqual(Curve.Acceleration(0.0), -980.0, 1.e-6));
	TestTrue(FString::Printf(TEXT("Arrives as asked (%.3f, %.3f cm/s, %.3f cm/s^2)"), Curve.Value(0.3), Curve.Speed(0.3), Curve.Acceleration(0.3)),
		FMath::IsNearlyEqual(Curve.Value(0.3), 50.0, 1.e-3) && FMath::IsNearlyEqual(Curve.Speed(0.3), 20.0, 1.e-3)
		&& FMath::IsNearlyEqual(Curve.Acceleration(0.3), 5.0, 1.e-2));
	const double Before = 0.3 - 1.e-7;
	const double After = 0.3 + 1.e-7;
	TestTrue(TEXT("...and the next piece carries on from there"), FMath::IsNearlyEqual(Curve.Value(Before), Curve.Value(After), 1.e-4)
		&& FMath::IsNearlyEqual(Curve.Speed(Before), Curve.Speed(After), 1.e-3) && FMath::IsNearlyEqual(Curve.Acceleration(Before), Curve.Acceleration(After), 0.1));
	TestTrue(TEXT("Ends at rest where asked"), FMath::IsNearlyEqual(Curve.Value(0.5), 60.0, 1.e-3) && FMath::IsNearlyZero(Curve.Speed(0.5), 1.e-3));
	TestTrue(TEXT("...and stays there"), FMath::IsNearlyEqual(Curve.Value(2.0), 60.0, 1.e-3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTraversalRulesTest, "Looter.Locomotion.Traversal.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTraversalRulesTest::RunTest(const FString& Parameters)
{
	// The user's numbers (2026-10-08): ledges to about 1.5 m are climbed, fences and low walls to about 1.1 m vaulted, a
	// climb takes 0.3 to 0.5 s; a late jump off an edge and an early one before landing still count.
	TestEqual(TEXT("Ledges climbed to 1.5 m"), LooterTraversal::MaxMantleHeight, 150.f);
	TestEqual(TEXT("Obstacles vaulted to 1.1 m"), LooterTraversal::MaxVaultHeight, 110.f);
	TestTrue(TEXT("Coyote time 0.1-0.15 s"), LooterTraversal::CoyoteSeconds >= 0.1f && LooterTraversal::CoyoteSeconds <= 0.15f);
	TestTrue(TEXT("Jump buffer 0.1-0.2 s"), LooterTraversal::JumpBufferSeconds >= 0.1f && LooterTraversal::JumpBufferSeconds <= 0.2f);
	TestTrue(TEXT("A 1.2 m mantle takes 0.3-0.5 s"), LooterTraversal::MantleSeconds(120.f) >= 0.3f && LooterTraversal::MantleSeconds(120.f) <= 0.5f);
	TestTrue(TEXT("...a lower one less"), LooterTraversal::MantleSeconds(50.f) < LooterTraversal::MantleSeconds(120.f));
	TestTrue(TEXT("A vault over a 1 m fence 0.3-0.5 s"), LooterTraversal::VaultSeconds(73.f) >= 0.3f && LooterTraversal::VaultSeconds(73.f) <= 0.5f);

	// A mantle from a stand at a 1.2 m ledge 10 cm away, stepping on at a walk.
	{
		FTraversalSetup Setup = PlayerSetup(ETraversalKind::Mantle);
		Setup.NearFace = Radius + 10.f;
		Setup.TopZ = 120.f;
		Setup.EndAlong = Setup.NearFace + Radius + 8.f;
		Setup.EndFeetZ = 122.15f;
		Setup.ExitSpeed = 230.f;
		Setup.Duration = LooterTraversal::MantleSeconds(Setup.EndFeetZ);
		Setup.EyeEndHeight = StandingEye - LooterTraversal::MantleEyeDip;
		FPlayerTraversal Move;
		if (!TestTrue(TEXT("Mantle: planned"), Move.Plan(Setup)))
		{
			return false;
		}
		TestTrue(FString::Printf(TEXT("Mantle: quick (%.2f s)"), Move.GetDuration()), Move.GetDuration() >= 0.3f && Move.GetDuration() <= 0.5f);
		TestTrue(TEXT("Mantle: starts where the body is, still"), Move.CenterAt(0.0).Equals(Setup.Origin, 0.01) && Move.VelocityAt(0.0).IsNearlyZero(0.01));
		const FVector End = Move.CenterAt(Move.GetDuration());
		TestTrue(FString::Printf(TEXT("Mantle: ends on the top, past the edge (%s)"), *End.ToCompactString()),
			FMath::IsNearlyEqual(End.X, Setup.EndAlong, 0.01) && FMath::IsNearlyEqual(End.Z - HalfHeight, Setup.EndFeetZ, 0.01));
		const FVector Exit = Move.VelocityAt(Move.GetDuration());
		TestTrue(FString::Printf(TEXT("Mantle: steps on at its exit speed, not rising (%s)"), *Exit.ToCompactString()),
			FMath::IsNearlyEqual(Exit.X, Setup.ExitSpeed, 0.01) && FMath::IsNearlyZero(Exit.Z, 0.01));
		const float Miss = WorstCornerMiss(Move, Setup);
		TestTrue(FString::Printf(TEXT("Mantle: the body's bottom stays clear of the edge (%.2f cm in at worst)"), Miss), Miss <= 1.5f);
		float Step = 0.f;
		float Change = 0.f;
		float Slowest = 0.f;
		SampleEye(Move, Setup, Step, Change, Slowest);
		TestTrue(FString::Printf(TEXT("Mantle: the eye rises smoothly (%.2f cm a frame at most, changing by %.2f)"), Step, Change),
			Step <= EyeStepLimit && Change <= EyeStepChangeLimit);
		float Corner = 0.f;
		float EyeOver = 0.f;
		float EyeLow = 0.f;
		float EyeHigh = 0.f;
		float EyeAcceleration = 0.f;
		Move.Measure(Setup, Move.GetDuration(), Corner, EyeOver, EyeLow, EyeHigh, EyeAcceleration);
		TestTrue(FString::Printf(TEXT("Mantle: the eye stays over the top and in the body (%.2f, %.2f, %.2f)"), EyeOver, EyeLow, EyeHigh),
			EyeOver <= 1.f && EyeLow <= 1.f && EyeHigh <= 1.f);
		TestTrue(FString::Printf(TEXT("Mantle: ...turning no harder than %.0f cm/s^2 (%.0f)"), FPlayerTraversal::MaxEyeAcceleration, EyeAcceleration),
			EyeAcceleration <= FPlayerTraversal::MaxEyeAcceleration);
		TestTrue(TEXT("Mantle: the eye ends a little under standing (it rises back after)"),
			FMath::IsNearlyEqual(Move.EyeAt(Move.GetDuration()), Setup.EndFeetZ + Setup.EyeEndHeight, 0.01));
	}

	// Caught in the air: falling at 300 cm/s and coming in at 500 cm/s toward a 1.4 m ledge. The move sets off at the
	// speed and acceleration the body had (no jolt), and stops short of the face before coming over.
	{
		FTraversalSetup Setup = PlayerSetup(ETraversalKind::Mantle);
		Setup.Origin.Z += 60.0;
		Setup.Velocity = FVector(500.0, 0.0, -300.0);
		Setup.GravityZ = -980.f;
		Setup.EyeZ = 60.f + StandingEye;
		Setup.EyeSpeed = -300.f;
		Setup.EyeAcceleration = -980.f;
		Setup.NearFace = Radius + 25.f;
		Setup.TopZ = 140.f;
		Setup.EndAlong = Setup.NearFace + Radius + 8.f;
		Setup.EndFeetZ = 142.15f;
		Setup.Duration = LooterTraversal::MantleSeconds(82.f);
		Setup.EyeEndHeight = StandingEye - LooterTraversal::MantleEyeDip;
		FPlayerTraversal Move;
		if (TestTrue(TEXT("Caught: planned"), Move.Plan(Setup)))
		{
			TestTrue(TEXT("Caught: sets off at the body's own velocity"), Move.VelocityAt(0.0).Equals(Setup.Velocity, 0.01));
			TestTrue(TEXT("Caught: ...the eye too"), FMath::IsNearlyEqual(Move.EyeAt(0.0), Setup.EyeZ, 0.01));
			// The run comes to a stop at the face (the body touching it at most) before the body comes over the top.
			bool bStopped = false;
			for (double At = 0.0; At <= Move.GetDuration() && !bStopped; At += 1.0 / 240.0)
			{
				bStopped = FMath::Abs(Move.AlongSpeedAt(At)) <= 5.0 && Move.AlongAt(At) <= Setup.NearFace - Radius + 1.0;
			}
			TestTrue(TEXT("Caught: the run stops at the face before coming over"), bStopped);
			const float Miss = WorstCornerMiss(Move, Setup);
			TestTrue(FString::Printf(TEXT("Caught: clear of the edge (%.2f cm in at worst)"), Miss), Miss <= 1.5f);
			float Step = 0.f;
			float Change = 0.f;
			float Slowest = 0.f;
			SampleEye(Move, Setup, Step, Change, Slowest);
			TestTrue(FString::Printf(TEXT("Caught: the eye smooth (%.2f cm a frame, changing by %.2f)"), Step, Change),
				Step <= EyeStepLimit && Change <= EyeStepChangeLimit);
		}
	}

	// A vault at a sprint over a 1 m rail 10 cm thick, 60 cm off: the hop clears the rail (legs tucked), the eye clears it
	// well, the run's speed is kept through it and out of it, and it comes down on the floor beyond.
	{
		const float Sprint = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale * 1.55f;
		FTraversalSetup Setup = PlayerSetup(ETraversalKind::Vault);
		Setup.Velocity = FVector(Sprint, 0.0, 0.0);
		Setup.NearFace = Radius + 60.f;
		Setup.FarFace = Setup.NearFace + 10.f;
		Setup.TopZ = 100.f;
		Setup.Tuck = LooterTraversal::VaultTuck;
		Setup.Duration = LooterTraversal::VaultSeconds(Setup.TopZ - Setup.Tuck + 3.f);
		Setup.EndAlong = FMath::Max(0.9f * Sprint * Setup.Duration, Setup.FarFace + Radius + 10.f);
		Setup.EndFeetZ = 2.15f;
		Setup.ExitSpeed = Sprint;
		Setup.EyeEndHeight = StandingEye - LooterTraversal::VaultEyeDip;
		FPlayerTraversal Move;
		if (TestTrue(TEXT("Vault: planned"), Move.Plan(Setup)))
		{
			const float Miss = WorstCornerMiss(Move, Setup);
			TestTrue(FString::Printf(TEXT("Vault: over the rail, legs tucked (%.2f cm in at worst)"), Miss), Miss <= 1.5f);
			float Step = 0.f;
			float Change = 0.f;
			float Slowest = 0.f;
			SampleEye(Move, Setup, Step, Change, Slowest);
			TestTrue(FString::Printf(TEXT("Vault: the eye smooth (%.2f cm a frame, changing by %.2f)"), Step, Change),
				Step <= EyeStepLimit && Change <= EyeStepChangeLimit);
			TestTrue(FString::Printf(TEXT("Vault: the run kept (%.0f cm/s at its slowest of %.0f)"), Slowest, Sprint), Slowest >= Sprint * 0.7f);
			TestTrue(TEXT("Vault: ...and out of it"), FMath::IsNearlyEqual(Move.VelocityAt(Move.GetDuration()).X, Sprint, 0.5));
			float Corner = 0.f;
			float EyeOver = 0.f;
			float EyeLow = 0.f;
			float EyeHigh = 0.f;
			float EyeAcceleration = 0.f;
			Move.Measure(Setup, Move.GetDuration(), Corner, EyeOver, EyeLow, EyeHigh, EyeAcceleration);
			TestTrue(FString::Printf(TEXT("Vault: the eye over the rail and in the body (%.2f, %.2f, %.2f)"), EyeOver, EyeLow, EyeHigh),
				EyeOver <= 1.f && EyeLow <= 1.f && EyeHigh <= 1.f);
			const FVector End = Move.CenterAt(Move.GetDuration());
			TestTrue(FString::Printf(TEXT("Vault: down on the floor beyond (%s)"), *End.ToCompactString()),
				End.X > Setup.FarFace + Radius && FMath::IsNearlyEqual(End.Z - HalfHeight, Setup.EndFeetZ, 0.01));
		}
	}
	return true;
}

#endif
