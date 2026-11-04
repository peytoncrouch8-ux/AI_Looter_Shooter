#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Player/AimAssist.h"
#include <limits>

// The gamepad's look (the audit's change 7, 2026-11-03): the stick as degrees a second by the frame's time, its dead zone and
// curve, Invert Y, and the aim assist's friction and magnetism, all as plain math.

namespace
{
	/** Degrees a held stick of Push (along X) turns the view over Seconds, in frames of 1 / Fps, as ALooterCharacter's pad look adds them. */
	float StickDegreesOver(float Push, float Seconds, float Fps)
	{
		const AimAssist::FTuning Tuning;
		const FVector2D Shaped = AimAssist::ShapeStick(FVector2D(Push, 0.0), Tuning.StickDeadZone, Tuning.StickCurve);
		const int32 Frames = FMath::RoundToInt(Seconds * Fps);
		double Degrees = 0.0;
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			Degrees += AimAssist::StickTurn(Shaped, Tuning.YawDegreesPerSecond, Tuning.PitchDegreesPerSecond, 1.f / Fps, Tuning.MaxStepSeconds).X;
		}
		return static_cast<float>(Degrees);
	}

	/** A point Distance away from the origin at Yaw degrees right and Pitch degrees up. */
	FVector PointAt(float Distance, float Yaw, float Pitch)
	{
		return FRotator(Pitch, Yaw, 0.f).Vector() * Distance;
	}

	AimAssist::FTarget Capsule(const FVector& Center, float Radius = 40.f, float HalfHeight = 90.f)
	{
		AimAssist::FTarget Target;
		Target.Center = Center;
		Target.Radius = Radius;
		Target.HalfHeight = HalfHeight;
		return Target;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAimAssistStickCurveTest, "Looter.AimAssist.StickCurve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAimAssistStickCurveTest::RunTest(const FString& Parameters)
{
	const AimAssist::FTuning Tuning;
	TestEqual(TEXT("The dead zone is 0.18"), Tuning.StickDeadZone, 0.18f);
	TestEqual(TEXT("The curve's power is 1.6"), Tuning.StickCurve, 1.6f);
	const float Zone = Tuning.StickDeadZone;
	const float Power = Tuning.StickCurve;

	// Inside the dead zone nothing, however it leans; at its edge still nothing.
	TestTrue(TEXT("Drift inside the dead zone: nothing"), AimAssist::ShapeStick(FVector2D(0.1, 0.1), Zone, Power).IsNearlyZero());
	TestTrue(TEXT("...on the dead zone's edge too"), AimAssist::ShapeStick(FVector2D(Zone, 0.0), Zone, Power).IsNearlyZero());
	TestTrue(TEXT("...and a stick at rest"), AimAssist::ShapeStick(FVector2D::ZeroVector, Zone, Power).IsNearlyZero());
	TestTrue(TEXT("Not a number: nothing"), AimAssist::ShapeStick(FVector2D(std::numeric_limits<double>::quiet_NaN(), 0.5), Zone, Power).IsNearlyZero());

	// Past it the push starts from nothing and a full push is the whole turn.
	TestTrue(TEXT("Just past the dead zone: a whisper"), AimAssist::ShapeStick(FVector2D(Zone + 0.02f, 0.0), Zone, Power).Size() < 0.01);
	TestTrue(TEXT("A full push: 1"), AimAssist::ShapeStick(FVector2D(1.0, 0.0), Zone, Power).Equals(FVector2D(1.0, 0.0), 1.e-4));
	TestTrue(TEXT("Past full (a square gate's corner) is still 1 long"), FMath::IsNearlyEqual(AimAssist::ShapeStick(FVector2D(1.0, 1.0), Zone, Power).Size(), 1.0, 1.e-4));
	TestTrue(TEXT("...at the corner's angle"), FMath::IsNearlyEqual(AimAssist::ShapeStick(FVector2D(1.0, 1.0), Zone, Power).X, AimAssist::ShapeStick(FVector2D(1.0, 1.0), Zone, Power).Y, 1.e-6));

	// Half way between the dead zone and full: 0.5 ^ 1.6 of the turn, finer than a straight line for small pushes.
	const float HalfPush = Zone + 0.5f * (1.f - Zone);
	TestTrue(TEXT("Half way past the dead zone: 0.5 ^ 1.6 = 0.33"), FMath::IsNearlyEqual(AimAssist::ShapeStick(FVector2D(HalfPush, 0.0), Zone, Power).X, 0.3299, 0.001));
	TestTrue(TEXT("A small push turns less than a straight line would"), AimAssist::ShapeStick(FVector2D(0.4, 0.0), Zone, Power).X < 0.4);
	double Last = 0.0;
	bool bRises = true;
	for (float Push = 0.2f; Push <= 1.f; Push += 0.05f)
	{
		const double Turn = AimAssist::ShapeStick(FVector2D(Push, 0.0), Zone, Power).X;
		bRises = bRises && Turn > Last;
		Last = Turn;
	}
	TestTrue(TEXT("A bigger push always turns more"), bRises);

	// Its direction is kept, signs and all (the input mapping sets them): the dead zone is round, not per axis.
	const FVector2D Shaped = AimAssist::ShapeStick(FVector2D(-0.6, 0.8), Zone, Power);
	TestTrue(TEXT("Direction kept: left and the other way up"), Shaped.X < 0.0 && Shaped.Y > 0.0 && FMath::IsNearlyEqual(Shaped.X / Shaped.Y, -0.75, 1.e-4));
	TestTrue(TEXT("A thumb on one axis is not cut by the other"), AimAssist::ShapeStick(FVector2D(0.05, 0.9), Zone, Power).Y > 0.0);

	// A linear curve and no dead zone leave the stick as it is (the switch for a tuner who wants the old response).
	TestTrue(TEXT("Dead zone 0, power 1: unchanged"), AimAssist::ShapeStick(FVector2D(0.3, -0.4), 0.f, 1.f).Equals(FVector2D(0.3, -0.4), 1.e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAimAssistStickTurnTest, "Looter.AimAssist.StickTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAimAssistStickTurnTest::RunTest(const FString& Parameters)
{
	const AimAssist::FTuning Tuning;
	TestEqual(TEXT("A full push turns 220 degrees a second round"), Tuning.YawDegreesPerSecond, 220.f);
	TestEqual(TEXT("...and 160 up and down"), Tuning.PitchDegreesPerSecond, 160.f);

	// The point of the change: the same turn in a second at any frame rate (it doubled at 120 fps when the stick was added per frame).
	for (const float Fps : { 30.f, 60.f, 90.f, 120.f, 144.f, 240.f })
	{
		TestTrue(FString::Printf(TEXT("A full push for a second at %.0f fps: 220 degrees"), Fps), FMath::IsNearlyEqual(StickDegreesOver(1.f, 1.f, Fps), 220.f, 0.01f));
	}
	// A full turn round takes about 1.6 seconds.
	for (const float Fps : { 60.f, 120.f })
	{
		TestTrue(FString::Printf(TEXT("360 degrees in 1.64 s at %.0f fps"), Fps), FMath::IsNearlyEqual(StickDegreesOver(1.f, 360.f / 220.f, Fps), 360.f, 1.5f));
	}
	TestTrue(TEXT("Half a push turns less than a third of that (the curve)"), StickDegreesOver(0.59f, 1.f, 60.f) < 220.f / 3.f + 0.5f);

	// Each axis has its rate, and the stick's signs come through.
	const FVector2D Turn = AimAssist::StickTurn(FVector2D(-1.0, 1.0), 220.f, 160.f, 0.5f, 0.1f);
	TestTrue(TEXT("Up and down turn at their own rate, signs kept"), Turn.Equals(FVector2D(-22.0, 16.0), 1.e-4));
	TestTrue(TEXT("The step is the frame's time"), AimAssist::StickTurn(FVector2D(1.0, 0.0), 220.f, 160.f, 1.f / 60.f, 0.1f).Equals(FVector2D(220.0 / 60.0, 0.0), 1.e-4));

	// A hitch frame must not spin the view; a frame that never happened turns nothing.
	TestTrue(TEXT("A half-second frame turns as a tenth of a second would"), AimAssist::StickTurn(FVector2D(1.0, 0.0), 220.f, 160.f, 0.5f, 0.1f).Equals(FVector2D(22.0, 0.0), 1.e-4));
	TestTrue(TEXT("A negative step: nothing"), AimAssist::StickTurn(FVector2D(1.0, 1.0), 220.f, 160.f, -1.f, 0.1f).IsNearlyZero());
	TestTrue(TEXT("A step that isn't a number: nothing"), AimAssist::StickTurn(FVector2D(1.0, 1.0), 220.f, 160.f, std::numeric_limits<float>::quiet_NaN(), 0.1f).IsNearlyZero());
	TestTrue(TEXT("Rate 0 turns nothing (Looter.Look.PadDegreesPerSecond 0 switches this whole path off in the character)"),
		AimAssist::StickTurn(FVector2D(1.0, 1.0), 0.f, 0.f, 0.1f, 0.1f).IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAimAssistInvertTest, "Looter.AimAssist.InvertY",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAimAssistInvertTest::RunTest(const FString& Parameters)
{
	const FVector2D Look(3.0, -1.5);
	TestTrue(TEXT("Off: the look as it was"), AimAssist::InvertY(Look, false).Equals(Look));
	TestTrue(TEXT("On: up and down reversed, left and right not"), AimAssist::InvertY(Look, true).Equals(FVector2D(3.0, 1.5)));
	TestTrue(TEXT("Twice is none"), AimAssist::InvertY(AimAssist::InvertY(Look, true), true).Equals(Look));
	TestTrue(TEXT("Nothing stays nothing"), AimAssist::InvertY(FVector2D::ZeroVector, true).IsNearlyZero());

	// The engine's legacy scales (yaw 2.5, pitch -2.5 here) are divided out of a stick turn, so a number of degrees is that
	// many degrees: what the controller then makes of it (input x scale) is as long as the turn asked for.
	const FVector2D Scales(2.5, -2.5);
	const FVector2D Degrees(10.0, -6.0);
	const FVector2D Input = AimAssist::DegreesToInput(Degrees, Scales);
	TestTrue(TEXT("The stick's degrees divided by the scales' size, signs as the stick's"), Input.Equals(FVector2D(4.0, -2.4), 1.e-6));
	TestTrue(TEXT("...which the controller makes 10 degrees round and 6 up or down"), FMath::IsNearlyEqual(FMath::Abs(Input.X * Scales.X), 10.0, 1.e-6)
		&& FMath::IsNearlyEqual(FMath::Abs(Input.Y * Scales.Y), 6.0, 1.e-6));
	TestTrue(TEXT("Scales of 1: unchanged"), AimAssist::DegreesToInput(Degrees, FVector2D(1.0, 1.0)).Equals(Degrees));
	TestTrue(TEXT("A scale of 0 can't divide by nothing"), AimAssist::DegreesToInput(Degrees, FVector2D::ZeroVector).Equals(Degrees));

	// A rotation (the magnet's pull: yaw right, pitch up) keeps the scales' signs, so the controller turns it exactly that way.
	const FVector2D Pull(0.5, 0.2);
	const FVector2D PullInput = AimAssist::RotationToInput(Pull, Scales);
	TestTrue(TEXT("A pull divided by the scales"), PullInput.Equals(FVector2D(0.2, -0.08), 1.e-6));
	TestTrue(TEXT("...the controller turns it right 0.5 and up 0.2"), (PullInput * Scales).Equals(Pull, 1.e-6));

	// Whatever the project's own scales are, the controller turns by the degrees asked.
	const APlayerController* Defaults = GetDefault<APlayerController>();
	const FVector2D EngineScales = AimAssist::LegacyInputScales(*Defaults);
	const FVector2D Made = AimAssist::DegreesToInput(Degrees, EngineScales);
	TestTrue(TEXT("With the project's scales: still 10 and 6 degrees"), FMath::IsNearlyEqual(FMath::Abs(Made.X * EngineScales.X), 10.0, 1.e-4)
		&& FMath::IsNearlyEqual(FMath::Abs(Made.Y * EngineScales.Y), 6.0, 1.e-4));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAimAssistFrictionTest, "Looter.AimAssist.Friction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAimAssistFrictionTest::RunTest(const FString& Parameters)
{
	const AimAssist::FTuning Tuning;
	TestEqual(TEXT("Friction keeps 0.6 of the look"), Tuning.Friction, 0.6f);
	TestEqual(TEXT("...within 20 cm of the capsule"), Tuning.MarginCm, 20.f);
	TestEqual(TEXT("...out to 40 m"), Tuning.RangeCm, 4000.f);

	// A creature 10 m ahead: its capsule 40 cm round, 90 cm half height (its axis runs 50 cm either way of the middle).
	const FVector Eye = FVector::ZeroVector;
	const AimAssist::FTarget Creature = Capsule(FVector(1000.0, 0.0, 0.0));
	auto On = [&](const FVector& Aim) { return AimAssist::OnTarget(Eye, Aim, Creature, Tuning.MarginCm, Tuning.RangeCm); };
	TestTrue(TEXT("Through its middle: on it"), On(FVector(1.0, 0.0, 0.0)));
	TestTrue(TEXT("Through its edge (40 cm out): on it"), On(FVector(1000.0, 40.0, 0.0)));
	TestTrue(TEXT("20 cm past its edge, with a little to spare: on it"), On(FVector(1000.0, 55.0, 0.0)));
	TestFalse(TEXT("30 cm past its edge: off it"), On(FVector(1000.0, 72.0, 0.0)));
	TestFalse(TEXT("The other side as well"), On(FVector(1000.0, -72.0, 0.0)));
	TestTrue(TEXT("Over its head, within the margin: on it"), On(FVector(1000.0, 0.0, 100.0)));
	TestFalse(TEXT("Well over its head: off it"), On(FVector(1000.0, 0.0, 130.0)));
	TestTrue(TEXT("The gap is negative through it and grows outside"), AimAssist::RayGap(Eye, FVector(1.0, 0.0, 0.0), Creature, 4000.f) < 0.f
		&& AimAssist::RayGap(Eye, FVector(1000.0, 100.0, 0.0), Creature, 4000.f) > AimAssist::RayGap(Eye, FVector(1000.0, 60.0, 0.0), Creature, 4000.f));

	// Behind the eye and past 40 m are none, however the numbers fall.
	TestFalse(TEXT("A creature behind: not on it"), AimAssist::OnTarget(Eye, FVector(1.0, 0.0, 0.0), Capsule(FVector(-1000.0, 0.0, 0.0)), 20.f, 4000.f));
	TestFalse(TEXT("A creature past 40 m: not on it"), AimAssist::OnTarget(Eye, FVector(1.0, 0.0, 0.0), Capsule(FVector(5000.0, 0.0, 0.0)), 20.f, 4000.f));
	TestTrue(TEXT("...the same creature at 30 m is"), AimAssist::OnTarget(Eye, FVector(1.0, 0.0, 0.0), Capsule(FVector(3000.0, 0.0, 0.0)), 20.f, 4000.f));
	TestFalse(TEXT("A zero aim is on nothing"), AimAssist::OnTarget(Eye, FVector::ZeroVector, Creature, 20.f, 4000.f));
	TestFalse(TEXT("A creature standing at the eye, looked away from: not on it"), AimAssist::OnTarget(Eye, FVector(-1.0, 0.0, 0.0), Capsule(FVector(30.0, 0.0, 0.0)), 20.f, 4000.f));

	// The look kept: 0.6 on a creature, all of it off one, never nothing.
	TestEqual(TEXT("On a creature: 0.6 of the look"), AimAssist::FrictionScale(true, 0.6f), 0.6f);
	TestEqual(TEXT("Off one: all of it"), AimAssist::FrictionScale(false, 0.6f), 1.f);
	TestEqual(TEXT("Friction 1 is none"), AimAssist::FrictionScale(true, 1.f), 1.f);
	TestEqual(TEXT("Friction 0 is held to 0.05, not a frozen stick"), AimAssist::FrictionScale(true, 0.f), 0.05f);
	TestEqual(TEXT("Friction past 1 is held to 1 (it never speeds the look up)"), AimAssist::FrictionScale(true, 2.f), 1.f);

	// Which creatures are assisted at all: alive, not passive, not hidden, solid.
	TestTrue(TEXT("A live, active, visible, solid creature"), AimAssist::CanAssist(false, false, false, true));
	TestFalse(TEXT("A dead one"), AimAssist::CanAssist(true, false, false, true));
	TestFalse(TEXT("A passive one (a boss held back)"), AimAssist::CanAssist(false, true, false, true));
	TestFalse(TEXT("A hidden one (blinked away, not yet risen)"), AimAssist::CanAssist(false, false, true, true));
	TestFalse(TEXT("One with no collision"), AimAssist::CanAssist(false, false, false, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAimAssistMagnetTest, "Looter.AimAssist.Magnetism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAimAssistMagnetTest::RunTest(const FString& Parameters)
{
	const AimAssist::FTuning Tuning;
	TestEqual(TEXT("The magnet pulls 2 degrees a second at most"), Tuning.MagnetDegreesPerSecond, 2.f);
	TestEqual(TEXT("...within 2 degrees of the crosshair"), Tuning.MagnetConeDegrees, 2.f);

	const FVector Eye = FVector::ZeroVector;
	const FVector Ahead(1.0, 0.0, 0.0);
	const float Dps = Tuning.MagnetDegreesPerSecond;
	const float Cone = Tuning.MagnetConeDegrees;
	const float Settle = Tuning.MagnetSettleDegrees;
	const float Frame = 1.f / 60.f;
	auto Pull = [&](const FVector& To, bool bMoving = true, float Dt = 1.f / 60.f)
	{
		return AimAssist::MagnetPull(Eye, Ahead, To, bMoving, Dps, Cone, Settle, Dt);
	};

	// The angle and the turn to a point.
	TestTrue(TEXT("1.5 degrees to the right is 1.5 degrees away"), FMath::IsNearlyEqual(AimAssist::AngleTo(Eye, Ahead, PointAt(1000.f, 1.5f, 0.f)), 1.5f, 0.001f));
	TestTrue(TEXT("...a turn of 1.5 right and none up"), AimAssist::TurnTo(Eye, Ahead, PointAt(1000.f, 1.5f, 0.f)).Equals(FVector2D(1.5, 0.0), 0.001));
	TestTrue(TEXT("1 degree up is a turn of 1 up"), AimAssist::TurnTo(Eye, Ahead, PointAt(1000.f, 0.f, 1.f)).Equals(FVector2D(0.0, 1.0), 0.001));
	TestTrue(TEXT("1 degree left is a turn of 1 left"), AimAssist::TurnTo(Eye, Ahead, PointAt(1000.f, -1.f, 0.f)).Equals(FVector2D(-1.0, 0.0), 0.001));

	// Toward the creature, at 2 degrees a second of the frame: 0.033 at 60 fps, only while the stick moves.
	const FVector2D Right = Pull(PointAt(1000.f, 1.5f, 0.f));
	TestTrue(TEXT("To the right: a pull right of 2 degrees a second x the frame"), Right.Equals(FVector2D(Dps * Frame, 0.0), 1.e-4));
	TestTrue(TEXT("Above: a pull up"), Pull(PointAt(1000.f, 0.f, 1.5f)).Equals(FVector2D(0.0, Dps * Frame), 1.e-4));
	TestTrue(TEXT("To the left: a pull left"), Pull(PointAt(1000.f, -1.5f, 0.f)).X < 0.0);
	TestTrue(TEXT("The stick still: no pull"), Pull(PointAt(1000.f, 1.5f, 0.f), false).IsNearlyZero());
	TestTrue(TEXT("Outside the cone (3 degrees): no pull"), Pull(PointAt(1000.f, 3.f, 0.f)).IsNearlyZero());
	TestTrue(TEXT("Dead on: nothing to pull"), Pull(PointAt(1000.f, 0.f, 0.f)).IsNearlyZero());
	TestTrue(TEXT("No magnet (0 degrees a second): no pull"), AimAssist::MagnetPull(Eye, Ahead, PointAt(1000.f, 1.5f, 0.f), true, 0.f, Cone, Settle, Frame).IsNearlyZero());
	TestTrue(TEXT("No time: no pull"), Pull(PointAt(1000.f, 1.5f, 0.f), true, 0.f).IsNearlyZero());

	// It eases off inside the settle angle and never carries the crosshair past.
	const double Full = Pull(PointAt(1000.f, 1.f, 0.f)).Size();
	const double Half = Pull(PointAt(1000.f, Settle * 0.5f, 0.f)).Size();
	TestTrue(TEXT("Half way to the middle within the settle angle: half the pull"), FMath::IsNearlyEqual(Half / Full, 0.5, 0.01));
	TestTrue(TEXT("A step is never more than the way left"), Pull(PointAt(1000.f, 0.01f, 0.f), true, 1.f).Size() <= 0.0101);

	// Rate, not frames: the pull a second adds is the same at 60 and 120 fps.
	const double PerSecond60 = Pull(PointAt(1000.f, 1.9f, 0.f), true, 1.f / 60.f).Size() * 60.0;
	const double PerSecond120 = Pull(PointAt(1000.f, 1.9f, 0.f), true, 1.f / 120.f).Size() * 120.0;
	TestTrue(TEXT("The same degrees a second at 60 and 120 fps"), FMath::IsNearlyEqual(PerSecond60, PerSecond120, 1.e-3) && FMath::IsNearlyEqual(PerSecond60, Dps, 1.e-3));

	// Followed through: from 1.8 degrees off, the crosshair comes onto the creature and stays, never crossing it, at either rate.
	for (const float Fps : { 60.f, 120.f })
	{
		FRotator Aim(0.f, 0.f, 0.f);
		const FVector Creature = PointAt(1000.f, 1.8f, 0.5f);
		bool bCrossed = false;
		for (int32 Step = 0; Step < static_cast<int32>(Fps * 4.f); ++Step)
		{
			const FVector2D Moved = AimAssist::MagnetPull(Eye, Aim.Vector(), Creature, true, Dps, Cone, Settle, 1.f / Fps);
			Aim.Yaw += Moved.X;
			Aim.Pitch += Moved.Y;
			bCrossed = bCrossed || Aim.Yaw > 1.8f + 0.001f || Aim.Pitch > 0.5f + 0.001f;
		}
		TestFalse(FString::Printf(TEXT("At %.0f fps: never carried past the creature"), Fps), bCrossed);
		TestTrue(FString::Printf(TEXT("At %.0f fps: on it within 0.01 degrees after 4 s (%.4f)"), Fps, AimAssist::AngleTo(Eye, Aim.Vector(), Creature)),
			AimAssist::AngleTo(Eye, Aim.Vector(), Creature) < 0.01f);
	}

	// The creature the magnet picks: the one the crosshair is nearest, inside the cone and the range.
	const AimAssist::FTarget Closer = Capsule(PointAt(1000.f, 0.5f, 0.f));
	const AimAssist::FTarget Wider = Capsule(PointAt(1000.f, 1.5f, 0.f));
	const AimAssist::FTarget Outside = Capsule(PointAt(1000.f, 4.f, 0.f));
	const AimAssist::FTarget Distant = Capsule(PointAt(6000.f, 0.f, 0.f));
	const AimAssist::FTarget Behind = Capsule(PointAt(1000.f, 180.f, 0.f));
	TestEqual(TEXT("Of two inside the cone, the one the crosshair is nearer"), AimAssist::PickMagnetTarget(Eye, Ahead, { Wider, Closer }, Cone, 4000.f), 1);
	TestEqual(TEXT("Outside the cone: none"), AimAssist::PickMagnetTarget(Eye, Ahead, { Outside }, Cone, 4000.f), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("Past the range: none"), AimAssist::PickMagnetTarget(Eye, Ahead, { Distant }, Cone, 4000.f), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("Behind the eye: none"), AimAssist::PickMagnetTarget(Eye, Ahead, { Behind }, Cone, 4000.f), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("A tie goes to the first listed"), AimAssist::PickMagnetTarget(Eye, Ahead, { Closer, Closer }, Cone, 4000.f), 0);
	TestEqual(TEXT("None in view: none"), AimAssist::PickMagnetTarget(Eye, Ahead, {}, Cone, 4000.f), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("The pick skips the ones out of reach"), AimAssist::PickMagnetTarget(Eye, Ahead, { Outside, Distant, Wider }, Cone, 4000.f), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAimAssistRefreshTest, "Looter.AimAssist.Refresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAimAssistRefreshTest::RunTest(const FString& Parameters)
{
	// The creature list is found by walking the world's actors, so it may be found again at most ten times a second.
	const AimAssist::FTuning Tuning;
	TestTrue(TEXT("At most every tenth of a second"), Tuning.RefreshSeconds >= 0.1f);
	for (const double Fps : { 30.0, 60.0, 144.0, 240.0 })
	{
		const double Seconds = 3.0;
		double Last = -1.0e9;
		int32 Refreshes = 0;
		for (int32 Frame = 0; Frame < static_cast<int32>(Seconds * Fps); ++Frame)
		{
			const double Now = 1000.0 + Frame / Fps;
			if (AimAssist::RefreshDue(Now, Last, Tuning.RefreshSeconds))
			{
				Last = Now;
				++Refreshes;
			}
		}
		TestTrue(FString::Printf(TEXT("%.0f fps for 3 s: found %d times, at most 31 (and not never)"), Fps, Refreshes), Refreshes >= 20 && Refreshes <= 31);
	}
	TestTrue(TEXT("Never found: due at once"), AimAssist::RefreshDue(5.0, -1.0e9, 0.1f));
	TestFalse(TEXT("Found a frame ago: not due"), AimAssist::RefreshDue(5.0, 4.99, 0.1f));
	TestTrue(TEXT("A little over a tenth of a second ago: due"), AimAssist::RefreshDue(5.11, 5.0, 0.1f));
	TestTrue(TEXT("A clock that went back: due"), AimAssist::RefreshDue(2.0, 5.0, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAimAssistTuningTest, "Looter.AimAssist.Tuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FAimAssistTuningTest::RunTest(const FString& Parameters)
{
	// The cvars exist (so they can be set live and from perf.ps1's -Exec) and start at the documented numbers.
	for (const TCHAR* Name : { TEXT("Looter.Look.PadDegreesPerSecond"), TEXT("Looter.Look.PadPitchDegreesPerSecond"), TEXT("Looter.Look.PadDeadZone"),
		TEXT("Looter.Look.PadCurve"), TEXT("Looter.AimAssist"), TEXT("Looter.AimAssist.Friction"), TEXT("Looter.AimAssist.Magnetism"),
		TEXT("Looter.AimAssist.Cone"), TEXT("Looter.AimAssist.Margin"), TEXT("Looter.AimAssist.Range") })
	{
		TestNotNull(FString::Printf(TEXT("%s is a cvar"), Name), IConsoleManager::Get().FindConsoleVariable(Name));
	}
	const AimAssist::FTuning Live = AimAssist::LiveTuning();
	const AimAssist::FTuning Made;
	TestTrue(TEXT("On by default"), Live.bEnabled);
	TestEqual(TEXT("Yaw rate"), Live.YawDegreesPerSecond, Made.YawDegreesPerSecond);
	TestEqual(TEXT("Pitch rate"), Live.PitchDegreesPerSecond, Made.PitchDegreesPerSecond);
	TestEqual(TEXT("Dead zone"), Live.StickDeadZone, Made.StickDeadZone);
	TestEqual(TEXT("Curve"), Live.StickCurve, Made.StickCurve);
	TestEqual(TEXT("Friction"), Live.Friction, Made.Friction);
	TestEqual(TEXT("Magnetism"), Live.MagnetDegreesPerSecond, Made.MagnetDegreesPerSecond);
	TestEqual(TEXT("Cone"), Live.MagnetConeDegrees, Made.MagnetConeDegrees);
	TestEqual(TEXT("Margin"), Live.MarginCm, Made.MarginCm);
	TestEqual(TEXT("Range"), Live.RangeCm, Made.RangeCm);

	// A bad number is held in range, so a typo at the console can't spin the view or freeze it.
	IConsoleVariable* Rate = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Look.PadDegreesPerSecond"));
	IConsoleVariable* Friction = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.AimAssist.Friction"));
	if (Rate && Friction)
	{
		const float WasRate = Rate->GetFloat();
		const float WasFriction = Friction->GetFloat();
		Rate->Set(100000.f, ECVF_SetByCode);
		Friction->Set(0.f, ECVF_SetByCode);
		TestEqual(TEXT("A rate of 100000 is held to 1000"), AimAssist::LiveTuning().YawDegreesPerSecond, 1000.f);
		TestEqual(TEXT("A friction of 0 is held to 0.05"), AimAssist::LiveTuning().Friction, 0.05f);
		Rate->Set(WasRate, ECVF_SetByCode);
		Friction->Set(WasFriction, ECVF_SetByCode);
	}
	return true;
}

#endif
