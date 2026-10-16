#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/Animation/BodyForearmTwist.h"
#include "Player/Animation/BodyHatTip.h"
#include "Player/Animation/BodySwing.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"

// Ellis's swinging cloth as plain math (FBodySwingPoints): a chain of three points hanging from an anchor settles on its
// animated pose, lags when the body moves and comes back, keeps its lengths, stays out of a limb, swings the same at 30
// and 120 frames a second, and starts over after a teleport.

namespace
{
	constexpr int32 SwingTestCount = 3;
	constexpr float SwingTestLength = 20.f;

	/** A chain hanging straight down from Anchor, SwingTestLength between points: where the animation has it. */
	TArray<FVector> SwingTestTargets(const FVector& Anchor)
	{
		TArray<FVector> Targets;
		for (int32 Index = 1; Index <= SwingTestCount; ++Index)
		{
			Targets.Add(Anchor + FVector(0.0, 0.0, -SwingTestLength * Index));
		}
		return Targets;
	}

	TArray<float> SwingTestLengths()
	{
		return TArray<float>({ SwingTestLength, SwingTestLength, SwingTestLength });
	}

	/** Steps the chain Seconds long at Fps with its anchor at Anchor, no limbs. */
	void SwingTestRun(FBodySwingPoints& Sim, const FVector& Anchor, float Seconds, float Fps, const FBodySwingSettings& Settings)
	{
		const int32 Frames = FMath::RoundToInt(Seconds * Fps);
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			Sim.Step(Anchor, SwingTestTargets(Anchor), SwingTestLengths(), {}, Settings, 1.f / Fps);
		}
	}

	float SwingTestOff(const FBodySwingPoints& Sim, const FVector& Anchor)
	{
		const TArray<FVector> Targets = SwingTestTargets(Anchor);
		float Worst = 0.f;
		for (int32 Index = 0; Index < SwingTestCount; ++Index)
		{
			Worst = FMath::Max(Worst, static_cast<float>(FVector::Dist(Sim.Points[Index], Targets[Index])));
		}
		return Worst;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingSettleTest, "Looter.Player.BodySwing.Settle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingSettleTest::RunTest(const FString& Parameters)
{
	const FBodySwingSettings Skirt = FBodySwing::SettingsFor(TEXT("skirt_fl"));
	FBodySwingPoints Sim;
	const FVector Start(0.0, 0.0, 100.0);
	SwingTestRun(Sim, Start, 0.1f, 60.f, Skirt);
	TestTrue(TEXT("Starts on its animated pose"), Sim.bStarted && SwingTestOff(Sim, Start) < 3.f);

	// The body steps 40 cm ahead in one frame: the cloth trails behind it, then comes back over the next seconds.
	const FVector Moved = Start + FVector(40.0, 0.0, 0.0);
	SwingTestRun(Sim, Moved, 1.f / 60.f, 60.f, Skirt);
	TestTrue(FString::Printf(TEXT("It lags behind a step (the tip %.1f cm behind)"), Moved.X - Sim.Points.Last().X),
		Sim.Points.Last().X < Moved.X - 5.0);
	SwingTestRun(Sim, Moved, 3.f, 60.f, Skirt);
	const float Off = SwingTestOff(Sim, Moved);
	TestTrue(FString::Printf(TEXT("...and settles back (%.2f cm off its pose; gravity's share sags it a little)"), Off), Off < 3.f);

	// Lengths: each point its length from the one before, the first from the anchor.
	FVector Head = Moved;
	for (int32 Index = 0; Index < SwingTestCount; ++Index)
	{
		TestNearlyEqual(FString::Printf(TEXT("Point %d keeps its length"), Index), static_cast<float>(FVector::Dist(Sim.Points[Index], Head)),
			SwingTestLength, 0.05f);
		Head = Sim.Points[Index];
	}

	// A teleport (a respawn) starts it over on its pose instead of whipping it across the map.
	const FVector Far = Moved + FVector(0.0, 900.0, 0.0);
	SwingTestRun(Sim, Far, 1.f / 60.f, 60.f, Skirt);
	TestTrue(TEXT("A teleport starts it over on its pose"), SwingTestOff(Sim, Far) < 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingRateTest, "Looter.Player.BodySwing.FrameRate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingRateTest::RunTest(const FString& Parameters)
{
	// The body runs at 3 m/s for a fifth of a second and stops (as play moves it: a little every frame, not a jump,
	// which would fling the chain by as much as the frame is short), stepped at 30 and at 120 frames a second. The
	// skirt's settings: three links as loose as the lantern's are a triple pendulum, chaotic, apart at any two rates.
	const FBodySwingSettings Lantern = FBodySwing::SettingsFor(TEXT("skirt_fl"));
	const FVector Start(0.0, 0.0, 100.0);
	const FVector Run(300.0, 100.0, 0.0);
	auto Play = [&](float Fps)
	{
		FBodySwingPoints Sim;
		const int32 Frames = FMath::RoundToInt(0.7f * Fps);
		for (int32 Frame = 0; Frame <= Frames; ++Frame)
		{
			const float Time = static_cast<float>(Frame) / Fps;
			const FVector Anchor = Start + Run * FMath::Min(Time, 0.2f);
			Sim.Step(Anchor, SwingTestTargets(Anchor), SwingTestLengths(), {}, Lantern, 1.f / Fps);
		}
		return Sim.Points.Last();
	};
	const FVector Slow = Play(30.f);
	const FVector Fast = Play(120.f);
	const float Apart = static_cast<float>(FVector::Dist(Slow, Fast));
	TestTrue(FString::Printf(TEXT("30 and 120 fps swing alike (%.2f cm apart half a second after a stop)"), Apart), Apart < 3.f);
	const FVector Rest = Start + Run * 0.2f + FVector(0.0, 0.0, -SwingTestLength * SwingTestCount);
	TestTrue(FString::Printf(TEXT("...and it still swings then (%.1f cm off its rest)"), FVector::Dist(Fast, Rest)), FVector::Dist(Fast, Rest) > 1.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingLimbTest, "Looter.Player.BodySwing.Limbs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingLimbTest::RunTest(const FString& Parameters)
{
	const FBodySwingSettings Skirt = FBodySwing::SettingsFor(TEXT("skirt_fl"));
	const FVector Anchor(0.0, 0.0, 100.0);
	// A thigh swung forward through where the chain hangs: a vertical segment 5 cm in front of it, 12 cm kept from it.
	FBodySwingCapsule Thigh;
	Thigh.A = Anchor + FVector(5.0, 0.0, -10.0);
	Thigh.B = Anchor + FVector(5.0, 0.0, -70.0);
	Thigh.Radius = 12.f;
	TArray<FBodySwingCapsule> Capsules;
	for (int32 Index = 0; Index < SwingTestCount; ++Index)
	{
		Capsules.Add(Thigh);
	}
	FBodySwingPoints Sim;
	for (int32 Frame = 0; Frame < 120; ++Frame)
	{
		Sim.Step(Anchor, SwingTestTargets(Anchor), SwingTestLengths(), Capsules, Skirt, 1.f / 60.f);
	}
	for (int32 Index = 0; Index < SwingTestCount; ++Index)
	{
		const FVector P = Sim.Points[Index];
		const float Clear = static_cast<float>(FVector::Dist(P, FMath::ClosestPointOnSegment(P, Thigh.A, Thigh.B)));
		// The first point hangs from the anchor, which is inside the limb's reach: its length wins over the last few mm.
		TestTrue(FString::Printf(TEXT("Point %d stays out of the limb (%.1f cm from its middle)"), Index, Clear), Clear > Thigh.Radius - 1.5f);
	}
	TestTrue(TEXT("Pushed out the far side: behind the thigh"), Sim.Points.Last().X < 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingBoneTest, "Looter.Player.BodySwing.Bones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingBoneTest::RunTest(const FString& Parameters)
{
	// A shin swung back across the cloth between two points (a stride's back leg under the coat's back): a level
	// segment through the second bone, clear of both its ends' points; 2 cm inside the cloth, then right through it.
	// The coat's outside is +X: the cloth goes out round the shin's back, not through it or round its side.
	const FBodySwingSettings Skirt = FBodySwing::SettingsFor(TEXT("skirt_bl"));
	const FVector Anchor(0.0, 0.0, 100.0);
	for (const double Inside : { 2.0, 0.0 })
	{
		FBodySwingCapsule Shin;
		Shin.A = Anchor + FVector(-Inside, -30.0, -SwingTestLength * 1.5);
		Shin.B = Anchor + FVector(-Inside, 30.0, -SwingTestLength * 1.5);
		Shin.Radius = 6.f;
		Shin.BoneRadius = 6.f;
		TArray<FBodySwingCapsule> Capsules;
		for (int32 Index = 0; Index < SwingTestCount; ++Index)
		{
			Capsules.Add(Shin);
		}
		FBodySwingPoints Sim;
		Sim.Outward = FVector(1.0, 0.0, 0.0);
		for (int32 Frame = 0; Frame < 120; ++Frame)
		{
			Sim.Step(Anchor, SwingTestTargets(Anchor), SwingTestLengths(), Capsules, Skirt, 1.f / 60.f);
		}
		FVector Head = Anchor;
		for (int32 Index = 0; Index < SwingTestCount; ++Index)
		{
			FVector OnBone;
			FVector OnLimb;
			FMath::SegmentDistToSegmentSafe(Head, Sim.Points[Index], Shin.A, Shin.B, OnBone, OnLimb);
			const float Clear = static_cast<float>(FVector::Dist(OnBone, OnLimb));
			TestTrue(FString::Printf(TEXT("Shin %.0f cm in: bone %d stays out of it (%.1f cm)"), Inside, Index, Clear), Clear > Shin.BoneRadius - 1.5f);
			Head = Sim.Points[Index];
		}
		TestTrue(FString::Printf(TEXT("Shin %.0f cm in: the cloth goes out round its back (x %.1f, y %.1f)"), Inside, Sim.Points[1].X, Sim.Points[1].Y),
			Sim.Points[1].X > 0.0 && FMath::Abs(Sim.Points[1].Y) < 1.0);
	}

	// A place on the cloth between two chains (the ring): pushed straight off a limb it's near, out the body's side of
	// one that has come through it, left alone when clear.
	FBodySwingCapsule Shin;
	Shin.A = FVector(0.0, -30.0, 50.0);
	Shin.B = FVector(0.0, 30.0, 50.0);
	Shin.Radius = 6.f;
	const FVector Outward(1.0, 0.0, 0.0);
	TestTrue(TEXT("Clear: left alone"), FBodySwingPoints::PushPlaceOut(FVector(8.0, 0.0, 50.0), Shin, Outward).IsNearlyZero());
	TestTrue(TEXT("Near, outside: pushed out to its clearance"),
		FBodySwingPoints::PushPlaceOut(FVector(4.0, 0.0, 50.0), Shin, Outward).Equals(FVector(2.0, 0.0, 0.0), 0.01));
	TestTrue(TEXT("Come through (inside): out across the limb"),
		FBodySwingPoints::PushPlaceOut(FVector(-3.0, 0.0, 50.0), Shin, Outward).Equals(FVector(9.0, 0.0, 0.0), 0.01));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingFloorTest, "Looter.Player.BodySwing.Floor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingFloorTest::RunTest(const FString& Parameters)
{
	// A crouch: the chain's animated pose reaches 15 cm under the ground; it lies on it instead, its lengths kept.
	const FBodySwingSettings Skirt = FBodySwing::SettingsFor(TEXT("skirt_bl"));
	const FVector Anchor(0.0, 0.0, 100.0);
	FBodySwingFloor Floor;
	Floor.Z = Anchor.Z - SwingTestLength * SwingTestCount + 15.0;
	auto Run = [&](FBodySwingPoints& Sim)
	{
		for (int32 Frame = 0; Frame < 120; ++Frame)
		{
			Sim.Step(Anchor, SwingTestTargets(Anchor), SwingTestLengths(), {}, Skirt, 1.f / 60.f, Floor);
		}
	};
	FBodySwingPoints Rigid;
	Run(Rigid);
	FVector Head = Anchor;
	for (int32 Index = 0; Index < SwingTestCount; ++Index)
	{
		TestTrue(FString::Printf(TEXT("Point %d over the floor (%.2f cm)"), Index, Rigid.Shown[Index].Z - Floor.Z), Rigid.Shown[Index].Z >= Floor.Z - 0.01);
		TestNearlyEqual(FString::Printf(TEXT("Point %d keeps its length"), Index), static_cast<float>(FVector::Dist(Rigid.Shown[Index], Head)),
			SwingTestLength, 0.05f);
		Head = Rigid.Shown[Index];
	}

	// Gathered: the last point lies on the ground no more than a third of its length out from its head, so its bone is
	// shorter (the hem bunches up instead of jutting out); the others keep theirs.
	Floor.LastReach = 1.f / 3.f;
	FBodySwingPoints Gathered;
	Run(Gathered);
	const FVector LastHead = Gathered.Shown[SwingTestCount - 2];
	const FVector Last = Gathered.Shown.Last();
	const float Out = static_cast<float>(FVector::Dist2D(Last, LastHead));
	TestTrue(FString::Printf(TEXT("The last point on the floor (%.2f cm over)"), Last.Z - Floor.Z), Last.Z >= Floor.Z - 0.01 && Last.Z < Floor.Z + 0.5);
	TestTrue(FString::Printf(TEXT("...a third of its length out at most (%.1f cm)"), Out), Out <= SwingTestLength / 3.f + 0.05f);
	TestTrue(TEXT("...so its bone is shorter"), FVector::Dist(Last, LastHead) < SwingTestLength - 1.0);
	TestNearlyEqual(TEXT("The point before keeps its length"), static_cast<float>(FVector::Dist(LastHead, Gathered.Shown[0])), SwingTestLength, 0.05f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingKindsTest, "Looter.Player.BodySwing.Kinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingKindsTest::RunTest(const FString& Parameters)
{
	const FBodySwingSettings Skirt = FBodySwing::SettingsFor(TEXT("skirt_bl"));
	const FBodySwingSettings Cape = FBodySwing::SettingsFor(TEXT("cape_fr"));
	const FBodySwingSettings Lantern = FBodySwing::SettingsFor(TEXT("lantern"));
	TestTrue(TEXT("The cape is held stiffer than the duster's skirt"), Cape.Stiffness > Skirt.Stiffness);
	TestTrue(TEXT("The lantern hangs loosest, under full gravity"), Lantern.Stiffness < Skirt.Stiffness && FMath::IsNearlyEqual(Lantern.Gravity, 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingHatTipTest, "Looter.Player.BodySwing.HatTip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingHatTipTest::RunTest(const FString& Parameters)
{
	TestNearlyEqual(TEXT("No tip with the gun lowered"), FBodyHatTip::DegreesFor(0.f), 0.f, 1e-4f);
	TestNearlyEqual(TEXT("The full tip at the sight"), FBodyHatTip::DegreesFor(1.f), FBodyHatTip::MaxDegrees, 1e-4f);
	bool bRising = true;
	for (int32 Step = 1; Step <= 20; ++Step)
	{
		bRising &= FBodyHatTip::DegreesFor(Step / 20.f) >= FBodyHatTip::DegreesFor((Step - 1) / 20.f);
	}
	TestTrue(TEXT("It tips steadily as the sight comes up"), bRising);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingForearmTwistTest, "Looter.Player.BodySwing.ForearmTwist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingForearmTwistTest::RunTest(const FString& Parameters)
{
	// A hand's turn split into its roll about the forearm (which the forearm's twist bones share) and the rest (the
	// wrist's bend, which they don't): the left hand rolls about 100 degrees onto a foregrip.
	const FVector Axis = FVector::ForwardVector;
	const float Roll = FMath::DegreesToRadians(100.f);
	const FQuat Twist(Axis, Roll);
	const FQuat Bend(FVector::UpVector, FMath::DegreesToRadians(25.f));
	TestNearlyEqual(TEXT("A pure roll is all twist"), FBodyForearmTwist::TwistAbout(Twist, Axis), Roll, 1e-3f);
	TestNearlyEqual(TEXT("...the other way round too"), FBodyForearmTwist::TwistAbout(Twist.Inverse(), Axis), -Roll, 1e-3f);
	TestNearlyEqual(TEXT("A bend of the wrist is none"), FBodyForearmTwist::TwistAbout(Bend, Axis), 0.f, 1e-3f);
	TestNearlyEqual(TEXT("A bent, rolled hand: the roll"), FBodyForearmTwist::TwistAbout(Bend * Twist, Axis), Roll, 1e-3f);
	TestNearlyEqual(TEXT("The same turn as its negative quaternion"), FBodyForearmTwist::TwistAbout(Twist * -1.0, Axis), Roll, 1e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingEllisTest, "Looter.Player.BodySwing.Ellis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingEllisTest::RunTest(const FString& Parameters)
{
	// SK_Ellis (Art/Models/Characters/Ellis.py) carries the swing chains and the hat bone the code finds by name.
	const USkeletalMesh* Ellis = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Art/Characters/SK_Ellis.SK_Ellis"));
	if (!TestNotNull(TEXT("SK_Ellis"), Ellis))
	{
		return false;
	}
	FBodySwing Swing;
	Swing.Bind(Ellis->GetRefSkeleton());
	TestEqual(TEXT("21 swing chains: 9 skirt (five across the back), 9 cape (four a side from the shoulder, one down the back), the bandana, the hair, the lantern"),
		Swing.NumChains(), 21);
	// Each ends in an unweighted `<chain>_end` where its cloth ends (the springs' last tip; a guess fell short of the hem).
	const TCHAR* const ChainNames[] = { TEXT("skirt_fl"), TEXT("skirt_sl"), TEXT("skirt_bl"), TEXT("skirt_bli"), TEXT("skirt_bm"),
		TEXT("skirt_bri"), TEXT("skirt_br"), TEXT("skirt_sr"), TEXT("skirt_fr"), TEXT("cape_fl"), TEXT("cape_al"), TEXT("cape_sl"),
		TEXT("cape_bl"), TEXT("cape_bm"), TEXT("cape_br"), TEXT("cape_sr"), TEXT("cape_ar"), TEXT("cape_fr"), TEXT("bandana"),
		TEXT("hair"), TEXT("lantern") };
	// The arms as they hang at the idle, for the cape's rest on them and its clearances (the bind pose's arms are raised).
	TestTrue(TEXT("cape_arm_l and cape_arm_r"), Ellis->GetRefSkeleton().FindBoneIndex(TEXT("cape_arm_l")) != INDEX_NONE
		&& Ellis->GetRefSkeleton().FindBoneIndex(TEXT("cape_arm_r")) != INDEX_NONE);
	for (const TCHAR* Chain : ChainNames)
	{
		TestTrue(FString::Printf(TEXT("%s ends in %s_end"), Chain, Chain),
			Ellis->GetRefSkeleton().FindBoneIndex(FName(*FString::Printf(TEXT("%s_end"), Chain))) != INDEX_NONE);
	}
	TestEqual(TEXT("The cloth between the 9 skirt chains keeps off the legs: 8 pairs round the coat (its front open)"), Swing.NumRingPairs(), 8);
	FBodyForearmTwist Twist;
	Twist.Bind(Ellis->GetRefSkeleton());
	TestEqual(TEXT("Two twist bones down each forearm roll it with the hand (the wrists tore open without)"), Twist.NumTwistBones(), 4);
	FBodyHatTip Tip;
	Tip.Bind(Ellis->GetRefSkeleton());
	TestTrue(TEXT("A hat bone to tip"), Tip.HasHat());
	return true;
}

#endif
