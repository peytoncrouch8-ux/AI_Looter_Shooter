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
	TestTrue(TEXT("Ellis's lantern swings free as it did (2, 1.2, nothing carried)"), FMath::IsNearlyEqual(Lantern.Stiffness, 2.f)
		&& FMath::IsNearlyEqual(Lantern.Damping, 1.2f) && Lantern.Carry == 0.f);

	// By what a chain hangs from: a lantern on the chest is a hook (Crane's), hair from the neck long hair (Crane's).
	TestEqual(TEXT("A lantern chain on the thigh is a lantern"), FBodySwing::KindOf(TEXT("lantern"), TEXT("thigh_r")), FString(TEXT("lantern")));
	TestEqual(TEXT("...on the chest a hook"), FBodySwing::KindOf(TEXT("lantern"), TEXT("spine_05")), FString(TEXT("hook")));
	TestEqual(TEXT("Hair from the head is hair"), FBodySwing::KindOf(TEXT("hair"), TEXT("head")), FString(TEXT("hair")));
	TestEqual(TEXT("...from the neck long hair, its front locks too"), FBodySwing::KindOf(TEXT("hair_fl"), TEXT("neck_01")), FString(TEXT("longhair")));
	TestEqual(TEXT("Anything else by its name"), FBodySwing::KindOf(TEXT("cape_bm"), TEXT("spine_05")), FString(TEXT("cape_bm")));
	TestEqual(TEXT("A coat's skirt is a skirt"), FBodySwing::KindOf(TEXT("skirt_bl"), TEXT("pelvis")), FString(TEXT("skirt_bl")));
	TestEqual(TEXT("...a tailcoat's, tails"), FBodySwing::KindOf(TEXT("skirt_bl"), TEXT("pelvis"), true), FString(TEXT("tails")));
	const FBodySwingSettings Tails = FBodySwing::SettingsFor(TEXT("tails"));
	const FBodySwingSettings Duster = FBodySwing::SettingsFor(TEXT("skirt_bl"));
	TestTrue(TEXT("Tails: stiffer than the duster's skirt, carried with the body"), Tails.Stiffness > Duster.Stiffness && Tails.Carry > 0.f
		&& Duster.Carry == 0.f);
	const FBodySwingSettings Hook = FBodySwing::SettingsFor(TEXT("hook"));
	TestTrue(TEXT("The hook: a pendulum (full gravity), stiffer and quicker to settle than the lantern, going partly with him"),
		FMath::IsNearlyEqual(Hook.Gravity, 1.f) && Hook.Stiffness > Lantern.Stiffness && Hook.Damping > Lantern.Damping && Hook.Carry > 0.f);
	TestTrue(TEXT("Long hair goes partly with him too"), FBodySwing::SettingsFor(TEXT("longhair")).Carry > 0.f);
	TestTrue(TEXT("Worn cloth turns with the body; the hook and the lantern, hung off it, swing more of a turn"),
		Duster.TurnCarry >= 0.85f && Tails.TurnCarry >= 0.85f && Cape.TurnCarry >= 0.85f && Hook.TurnCarry < Duster.TurnCarry
		&& Lantern.TurnCarry < Hook.TurnCarry && Lantern.TurnCarry > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingHandTest, "Looter.Player.BodySwing.Hand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingHandTest::RunTest(const FString& Parameters)
{
	// A hand hanging just outside the coat's side (the chain's out side is +X, the hand 3 cm out from its line): pushed off
	// the shortest way the cloth stays inside it; taken for a limb come through, it was flipped out over the hand.
	const FBodySwingSettings Skirt = FBodySwing::SettingsFor(TEXT("skirt_sl"));
	const FVector Anchor(0.0, 0.0, 100.0);
	FBodySwingCapsule Hand;
	Hand.A = Anchor + FVector(3.0, 0.0, -15.0);
	Hand.B = Anchor + FVector(3.0, 0.0, -35.0);
	Hand.Radius = 5.f;
	Hand.BoneRadius = 5.f;
	auto Hang = [&](bool bShortestWay)
	{
		FBodySwingCapsule Limb = Hand;
		Limb.bShortestWay = bShortestWay;
		const TArray<FBodySwingCapsule> Capsules({ Limb, Limb, Limb });
		FBodySwingPoints Sim;
		Sim.Outward = FVector(1.0, 0.0, 0.0);
		for (int32 Frame = 0; Frame < 180; ++Frame)
		{
			Sim.Step(Anchor, SwingTestTargets(Anchor), SwingTestLengths(), Capsules, Skirt, 1.f / 60.f);
		}
		return Sim.Points;
	};
	const TArray<FVector> Inside = Hang(true);
	const TArray<FVector> Flipped = Hang(false);
	TestTrue(FString::Printf(TEXT("Pushed the shortest way, the cloth stays on its side of the hand (x %.1f)"), Inside[0].X), Inside[0].X < 0.0);
	TestTrue(FString::Printf(TEXT("...near its pose (the hem %.1f cm off)"), FVector::Dist(Inside.Last(), Anchor + FVector(0.0, 0.0, -60.0))),
		FVector::Dist(Inside.Last(), Anchor + FVector(0.0, 0.0, -60.0)) < 8.0);
	TestTrue(FString::Printf(TEXT("(Taken for come through it went over the hand: x %.1f)"), Flipped[0].X), Flipped[0].X > Inside[0].X);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingRestTest, "Looter.Player.BodySwing.Rest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingRestTest::RunTest(const FString& Parameters)
{
	// A cape's back held against the body (a vertical limb just behind its line, its pose a little inside the limb's
	// clearance and off to one side), nudged once: it comes to rest on the limb instead of sliding round it back and forth
	// (it kept the speed of its fall into the limb and wagged side to side at a standstill).
	const FBodySwingSettings Cape = FBodySwing::SettingsFor(TEXT("cape_bm"));
	FBodySwingCapsule Back;
	const FVector Anchor(0.0, 0.0, 100.0);
	Back.A = Anchor + FVector(-6.0, 0.0, 10.0);
	Back.B = Anchor + FVector(-6.0, 0.0, -60.0);
	Back.Radius = 10.f;
	const TArray<FBodySwingCapsule> Capsules({ Back, Back, Back });
	auto Targets = [](const FVector& At)
	{
		TArray<FVector> Out;
		for (int32 Index = 1; Index <= SwingTestCount; ++Index)
		{
			Out.Add(At + FVector(0.0, 2.0, -SwingTestLength * Index));
		}
		return Out;
	};
	FBodySwingPoints Sim;
	FVector Low = FVector(UE_BIG_NUMBER);
	FVector High = FVector(-UE_BIG_NUMBER);
	const float Fps = 60.f;
	for (int32 Frame = 0; Frame < 240; ++Frame)
	{
		const float Time = static_cast<float>(Frame) / Fps;
		// A sideways nudge of the body at half a second (a breath, a shift of weight).
		const FVector At = Anchor + FVector(0.0, Time >= 0.5f ? 4.0 : 0.0, 0.0);
		Sim.Step(At, Targets(At), SwingTestLengths(), Capsules, Cape, 1.f / Fps);
		if (Time >= 3.f)
		{
			Low = Low.ComponentMin(Sim.Points.Last());
			High = High.ComponentMax(Sim.Points.Last());
		}
	}
	const float Wander = static_cast<float>((High - Low).Size());
	TestTrue(FString::Printf(TEXT("At rest against the limb it lies still (%.2f cm of wander in its last second)"), Wander), Wander < 0.3f);
	const FVector Last = Sim.Points.Last();
	TestTrue(TEXT("...outside it"), FVector::Dist(Last, FMath::ClosestPointOnSegment(Last, Back.A, Back.B)) > Back.Radius - 1.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingTurnTest, "Looter.Player.BodySwing.Turn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingTurnTest::RunTest(const FString& Parameters)
{
	// A skirt chain on the hip, 15 cm off the body's turning axis (the origin), leaning 20 degrees out, settles; then the
	// body yaws 90 degrees in one frame (a mouse flick, the hero stand spun) and holds. Turned with the body (TurnCarry),
	// the cloth keeps near its turned rest pose; left to the springs, it whipped out wide and lagged.
	const FBodySwingSettings Skirt = FBodySwing::SettingsFor(TEXT("skirt_sl"));
	FBodySwingSettings Free = Skirt;
	Free.TurnCarry = 0.f;
	TestTrue(FString::Printf(TEXT("The duster's skirt turns with the body (TurnCarry %.2f)"), Skirt.TurnCarry), Skirt.TurnCarry >= 0.85f);
	const FVector Hip(15.0, 0.0, 100.0);
	const double LeanRadians = FMath::DegreesToRadians(20.0);
	const FVector Lean(FMath::Sin(LeanRadians), 0.0, -FMath::Cos(LeanRadians));
	struct FTurnSwing
	{
		/** The worst bone's angle off its turned rest (degrees): the most after the turn, and the most from 0.5 s to 1 s on. */
		float Peak = 0.f;
		float Settled = 0.f;
	};
	auto Turn = [&](const FBodySwingSettings& Settings)
	{
		FTurnSwing Result;
		FBodySwingPoints Sim;
		const float Fps = 60.f;
		const FQuat Turned(FVector::UpVector, UE_HALF_PI);
		TArray<FVector> RestWays;
		for (int32 Frame = 0; Frame < 90; ++Frame)
		{
			const bool bTurned = Frame >= 30;
			const FQuat Body = bTurned ? Turned : FQuat::Identity;
			const FVector Anchor = Body.RotateVector(Hip);
			TArray<FVector> Targets;
			for (int32 Index = 1; Index <= SwingTestCount; ++Index)
			{
				Targets.Add(Anchor + Body.RotateVector(Lean) * (SwingTestLength * Index));
			}
			Sim.BodyRotation = Body;
			Sim.BodyPivot = FVector::ZeroVector;
			Sim.Step(Anchor, Targets, SwingTestLengths(), {}, Settings, 1.f / Fps);
			// Its rest: where it settled before the turn (gravity's share sags it a little off its targets), turned.
			FVector Head = Anchor;
			if (Frame == 29)
			{
				for (int32 Index = 0; Index < SwingTestCount; ++Index)
				{
					RestWays.Add((Sim.Points[Index] - Head).GetSafeNormal());
					Head = Sim.Points[Index];
				}
			}
			else if (bTurned && RestWays.Num() == SwingTestCount)
			{
				float Worst = 0.f;
				for (int32 Index = 0; Index < SwingTestCount; ++Index)
				{
					const FVector Want = Turned.RotateVector(RestWays[Index]);
					const FVector Has = (Sim.Points[Index] - Head).GetSafeNormal();
					const float Degrees = FMath::RadiansToDegrees(static_cast<float>(FMath::Acos(FMath::Clamp(FVector::DotProduct(Want, Has), -1.0, 1.0))));
					Worst = FMath::Max(Worst, Degrees);
					Head = Sim.Points[Index];
				}
				Result.Peak = FMath::Max(Result.Peak, Worst);
				if (Frame >= 60)
				{
					Result.Settled = FMath::Max(Result.Settled, Worst);
				}
			}
		}
		return Result;
	};
	const FTurnSwing With = Turn(Skirt);
	const FTurnSwing Without = Turn(Free);
	TestTrue(FString::Printf(TEXT("Half a second after the turn it's near its turned rest (%.1f degrees off at most)"), With.Settled), With.Settled < 6.f);
	TestTrue(FString::Printf(TEXT("...and swung far less than left to the springs (%.1f degrees at most, against %.1f)"), With.Peak, Without.Peak),
		With.Peak < 20.f && With.Peak < Without.Peak * 0.25f);
	TestTrue(FString::Printf(TEXT("(Left to the springs it whipped out: %.1f degrees, still %.1f half a second on)"), Without.Peak, Without.Settled),
		Without.Settled > With.Settled * 2.f);

	// Turned all the way with the body (TurnCarry 1), the cloth hardly moves on it: the turn alone flings nothing.
	FBodySwingSettings Worn = Skirt;
	Worn.TurnCarry = 1.f;
	const FTurnSwing Rigid = Turn(Worn);
	TestTrue(FString::Printf(TEXT("Carried whole, a turn barely moves it (%.2f degrees)"), Rigid.Peak), Rigid.Peak < 3.f);
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
	// The hands reach the fingers' last joints (ended at the knuckles, the fingers hanging down the coat sank into it);
	// the boots run from foot to ball (heel to toe, FBodySwing::FootLine).
	TestTrue(TEXT("The hands' limbs reach the fingers (hand to middle_03)"), Swing.HasLimb(TEXT("hand_l"), TEXT("middle_03_l"))
		&& Swing.HasLimb(TEXT("hand_r"), TEXT("middle_03_r")));
	TestTrue(TEXT("The boots are limbs (foot to ball)"), Swing.HasLimb(TEXT("foot_l"), TEXT("ball_l")) && Swing.HasLimb(TEXT("foot_r"), TEXT("ball_r")));
	FBodyForearmTwist Twist;
	Twist.Bind(Ellis->GetRefSkeleton());
	TestEqual(TEXT("Two twist bones down each forearm roll it with the hand (the wrists tore open without)"), Twist.NumTwistBones(), 4);
	FBodyHatTip Tip;
	Tip.Bind(Ellis->GetRefSkeleton());
	TestTrue(TEXT("A hat bone to tip"), Tip.HasHat());

	// Which coats are tailcoats (FBodySwing::KindOf): Odessa's skirt has no front chains, the long coats all do.
	const TCHAR* const Coats[][2] = { { TEXT("Ellis"), TEXT("1") }, { TEXT("Crane"), TEXT("1") }, { TEXT("Pike"), TEXT("1") }, { TEXT("Odessa"), TEXT("0") } };
	for (const auto& Coat : Coats)
	{
		const USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Art/Characters/SK_%s.SK_%s"), Coat[0], Coat[0]));
		if (!Body)
		{
			continue;
		}
		const bool bFront = Body->GetRefSkeleton().FindBoneIndex(TEXT("skirt_fl_01")) != INDEX_NONE;
		TestEqual(FString::Printf(TEXT("SK_%s: %s"), Coat[0], bFront ? TEXT("a long coat, its skirt's front chained") : TEXT("a tailcoat, tails only")),
			bFront, FCString::Atoi(Coat[1]) != 0);
	}
	return true;
}

// The hero fit pass's leftovers (2026-10-28, Docs/Pipeline/CharacterModels.md "Clipping and fit pass"): a sprint's back
// boot through the duster's back, a run's front thigh through the cloth between two skirt chains, a hand lying where the
// coat hangs. In these tests +X is the coat's outside (behind her for the back panel, ahead for the front), +Z up.

namespace
{
	/** V turned Degrees about the side-to-side axis, +X toward +Z (a leg swung back past its hip, a thigh raised forward). */
	FVector SwingTestTurn(const FVector& V, float Degrees)
	{
		return FQuat(FVector(0.0, -1.0, 0.0), FMath::DegreesToRadians(Degrees)).RotateVector(V);
	}

	/** The nearest the bones from Anchor through Points come to the segment A-B (cm). */
	double SwingTestBoneGap(const FVector& Anchor, TConstArrayView<FVector> Points, const FVector& A, const FVector& B)
	{
		double Nearest = UE_BIG_NUMBER;
		FVector Head = Anchor;
		for (const FVector& Point : Points)
		{
			FVector OnBone;
			FVector OnLimb;
			FMath::SegmentDistToSegmentSafe(Head, Point, A, B, OnBone, OnLimb);
			Nearest = FMath::Min(Nearest, FVector::Dist(OnBone, OnLimb));
			Head = Point;
		}
		return Nearest;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingBackBootTest, "Looter.Player.BodySwing.BackBoot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingBackBootTest::RunTest(const FString& Parameters)
{
	// Ellis's duster's back panel behind her right leg (skirt_br at rest, from the belt to the hem) and the leg (hip, knee,
	// ankle, ball), the limbs' clearances as Bind takes them (rest distance x 0.95, at most the tuning table's). The leg
	// swings back over a fifth of a second and holds; the coat's bones must keep off the boot (8 cm round the shin, the
	// hit capsule's, and 5.5 round its heel-to-toe line).
	const FBodySwingSettings Skirt = FBodySwing::SettingsFor(TEXT("skirt_br"));
	const FVector Anchor(13.0, 0.0, 97.5);
	const TArray<FVector> Targets({ FVector(13.6, 0.0, 74.0), FVector(15.7, 0.0, 53.0), FVector(20.4, 0.0, 21.7) });
	TArray<float> Lengths;
	{
		FVector Head = Anchor;
		for (const FVector& Target : Targets)
		{
			Lengths.Add(static_cast<float>(FVector::Dist(Head, Target)));
			Head = Target;
		}
	}
	const FVector Hip(-0.6, 0.0, 95.0);
	const FVector Thigh(-3.4, 0.0, -42.0);
	const FVector Shin(1.6, 0.0, -43.1);
	const FVector Foot(-14.0, 0.0, -4.8);
	FVector RestHeel;
	FVector RestToe;
	FBodySwing::FootLine(Hip + Thigh + Shin, Hip + Thigh + Shin + Foot, RestHeel, RestToe);
	const FVector HeelOnAnkle = RestHeel - (Hip + Thigh + Shin);
	const FVector ToeOnBall = RestToe - (Hip + Thigh + Shin + Foot);
	TestTrue(FString::Printf(TEXT("The boot's line starts behind the ankle joint, low (the heel %.1f cm back, %.1f up)"), HeelOnAnkle.X, RestHeel.Z),
		HeelOnAnkle.X > 3.0 && RestHeel.Z < 6.0);

	struct FLegNow
	{
		FVector Knee;
		FVector Ankle;
		FVector Heel;
		FVector Toe;
		/** Thigh, shin, boot: each limb's ends and its cloth side (the coat's outside, turned with it). */
		FBodySwingCapsule Limbs[3];
	};
	const float Caps[3] = { FBodySwing::MaxClearanceOf(TEXT("thigh_r")), FBodySwing::MaxClearanceOf(TEXT("calf_r")),
		FBodySwing::MaxClearanceOf(TEXT("foot_r")) };
	auto LegAt = [&](float ThighDegrees, float ShinDegrees, float FootDegrees, bool bCarried)
	{
		FLegNow Leg;
		Leg.Knee = Hip + SwingTestTurn(Thigh, ThighDegrees);
		Leg.Ankle = Leg.Knee + SwingTestTurn(Shin, ShinDegrees);
		Leg.Heel = Leg.Ankle + SwingTestTurn(HeelOnAnkle, FootDegrees);
		Leg.Toe = Leg.Ankle + SwingTestTurn(Foot + ToeOnBall, FootDegrees);
		const FVector Ends[3][2] = { { Hip, Leg.Knee }, { Leg.Knee, Leg.Ankle }, { Leg.Heel, Leg.Toe } };
		const float Turns[3] = { ThighDegrees, ShinDegrees, FootDegrees };
		for (int32 Limb = 0; Limb < 3; ++Limb)
		{
			Leg.Limbs[Limb].A = Ends[Limb][0];
			Leg.Limbs[Limb].B = Ends[Limb][1];
			// As FBodySwing::Apply: the chain's way out, carried in the leg's own frame, and a leg may lift the cloth.
			Leg.Limbs[Limb].Side = bCarried ? SwingTestTurn(FVector(1.0, 0.0, 0.0), Turns[Limb]) : FVector::ZeroVector;
			Leg.Limbs[Limb].bLifts = bCarried;
		}
		return Leg;
	};
	// The clearances at rest, per point and limb (Bind).
	const FLegNow Rest = LegAt(0.f, 0.f, 0.f, false);
	TArray<float> Clearances;
	TArray<float> BoneClearances;
	for (int32 Index = 0; Index < Targets.Num(); ++Index)
	{
		const FVector Head = Index > 0 ? Targets[Index - 1] : Anchor;
		for (int32 Limb = 0; Limb < 3; ++Limb)
		{
			const FBodySwingCapsule& At = Rest.Limbs[Limb];
			const FVector& Tip = Targets[Index];
			Clearances.Add(FMath::Min(static_cast<float>(FVector::Dist(Tip, FMath::ClosestPointOnSegment(Tip, At.A, At.B))) * FBodySwing::ClearanceShare, Caps[Limb]));
			FVector OnBone;
			FVector OnLimb;
			FMath::SegmentDistToSegmentSafe(Head, Tip, At.A, At.B, OnBone, OnLimb);
			BoneClearances.Add(FMath::Min(static_cast<float>(FVector::Dist(OnBone, OnLimb)) * FBodySwing::ClearanceShare, Caps[Limb]));
		}
	}
	// The worst gap (cm) between the coat's bones and the boot (ShinRadius round the shin, 5.5 round heel to toe) once
	// the leg has held its pose a fifth of a second.
	auto Swing = [&](float ThighDegrees, float ShinDegrees, float FootDegrees, bool bCarried, double ShinRadius)
	{
		FBodySwingPoints Sim;
		Sim.Outward = FVector(1.0, 0.0, 0.0);
		double Worst = UE_BIG_NUMBER;
		const float Fps = 60.f;
		for (int32 Frame = 0; Frame <= 72; ++Frame)
		{
			const float Time = static_cast<float>(Frame) / Fps;
			const float Share = FMath::Clamp((Time - 0.5f) / 0.2f, 0.f, 1.f);
			const FLegNow Leg = LegAt(ThighDegrees * Share, ShinDegrees * Share, FootDegrees * Share, bCarried);
			TArray<FBodySwingCapsule> Capsules;
			for (int32 Index = 0; Index < Targets.Num(); ++Index)
			{
				for (int32 Limb = 0; Limb < 3; ++Limb)
				{
					FBodySwingCapsule Capsule = Leg.Limbs[Limb];
					Capsule.Radius = Clearances[Index * 3 + Limb];
					Capsule.BoneRadius = BoneClearances[Index * 3 + Limb];
					Capsules.Add(Capsule);
				}
			}
			Sim.Step(Anchor, Targets, Lengths, Capsules, Skirt, 1.f / Fps);
			if (Time >= 0.9f)
			{
				Worst = FMath::Min(Worst, SwingTestBoneGap(Anchor, Sim.Points, Leg.Knee, Leg.Ankle) - ShinRadius);
				Worst = FMath::Min(Worst, SwingTestBoneGap(Anchor, Sim.Points, Leg.Heel, Leg.Toe) - 5.5);
			}
		}
		return Worst;
	};

	// A sprint's back kick (the fit audit's run: the thigh 28 degrees back, the knee bent 85 more, the foot -25): the shin
	// rises past level behind her. The coat lies over it, lifted by the heel.
	const double Sprint = Swing(28.f, 113.f, 88.f, true, 8.0);
	TestTrue(FString::Printf(TEXT("A sprint's back kick: the coat keeps off the boot (%.1f cm at the nearest)"), Sprint), Sprint > 1.0);
	// The chain's way out squared to the shin flipped under it past level: the coat went into the knee's fold, the boot
	// through it.
	const double Flipped = Swing(28.f, 113.f, 88.f, false, 8.0);
	TestTrue(FString::Printf(TEXT("(With the chain's own way out the boot came through: %.1f cm)"), Flipped), Flipped < -3.0);

	// The stride's end (the audit's stride: the thigh 20 back, the shin 35, the foot 15), Odessa's riding boots (10 cm
	// round the shin, Pike's 9.7): the shin's clearance (12) leaves the coat room; at 10 it touched.
	const double Stride = Swing(20.f, 35.f, 15.f, true, 10.0);
	TestTrue(FString::Printf(TEXT("The stride's end, the thickest boots: the coat keeps off them (%.1f cm)"), Stride), Stride > 1.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingRunBandTest, "Looter.Player.BodySwing.RunBand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingRunBandTest::RunTest(const FString& Parameters)
{
	// A run's front leg: the thigh swings from 30 degrees back to 70 forward, up under the cloth between a skirt's front
	// and side chains (Pike's front flap over his thigh: the ring's places there, at rest a hand's width off it). Each
	// place keeps its clearance (rest distance x 0.95, at most the thigh's) all through the swing.
	const FVector Hip(0.0, 0.0, 92.0);
	const FVector Thigh(0.0, 0.0, -41.0);
	const float Cap = FBodySwing::MaxClearanceOf(TEXT("thigh_l"));
	const FVector Outward(1.0, 0.0, 0.0);
	const FVector Places[] = { FVector(12.0, 0.0, 80.0), FVector(10.0, 4.0, 85.0), FVector(14.0, -3.0, 74.0), FVector(9.0, 6.0, 70.0),
		FVector(16.0, 0.0, 88.0) };
	for (const FVector& Place : Places)
	{
		const float Radius = FMath::Min(static_cast<float>(FVector::Dist(Place, FMath::ClosestPointOnSegment(Place, Hip, Hip + Thigh)))
			* FBodySwing::ClearanceShare, Cap);
		double Worst = UE_BIG_NUMBER;
		double WorstLevel = UE_BIG_NUMBER;
		for (int32 Degrees = -30; Degrees <= 70; ++Degrees)
		{
			FBodySwingCapsule Leg;
			Leg.A = Hip;
			Leg.B = Hip + SwingTestTurn(Thigh, static_cast<float>(Degrees));
			Leg.Radius = Radius;
			// The cloth's side of the thigh, turned with it (FBodySwing::Apply).
			Leg.Side = SwingTestTurn(Outward, static_cast<float>(Degrees));
			auto Gap = [&](const FVector& Push)
			{
				const FVector Moved = Place + Push;
				return FVector::Dist(Moved, FMath::ClosestPointOnSegment(Moved, Leg.A, Leg.B)) - Radius;
			};
			Worst = FMath::Min(Worst, Gap(FBodySwingPoints::PushBandOut(Place, Leg, Outward)));
			// As the ring pushed before: the chain's way out, kept level.
			FBodySwingCapsule Before = Leg;
			Before.Side = FVector::ZeroVector;
			FVector Level = FBodySwingPoints::PushPlaceOut(Place, Before, Outward);
			Level.Z = 0.0;
			WorstLevel = FMath::Min(WorstLevel, Gap(Level));
		}
		TestTrue(FString::Printf(TEXT("Place (%.0f, %.0f, %.0f): the cloth keeps off the swinging thigh (%.2f cm short at worst)"), Place.X, Place.Y,
			Place.Z, -FMath::Min(Worst, 0.0)), Worst > -0.05);
		TestTrue(FString::Printf(TEXT("...(pushed level, the thigh stayed %.1f cm into it)"), -WorstLevel), WorstLevel < Worst);
	}
	FBodySwingCapsule Over;
	Over.A = FVector(-20.0, 0.0, 53.0);
	Over.B = FVector(20.0, 0.0, 53.0);
	Over.Radius = 6.f;
	TestTrue(TEXT("Never pulled down (a limb over the cloth)"), FBodySwingPoints::PushBandOut(FVector(0.0, 0.0, 50.0), Over, Outward).Z >= 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBodySwingHandThicknessTest, "Looter.Player.BodySwing.HandThickness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FBodySwingHandThicknessTest::RunTest(const FString& Parameters)
{
	// The idle's hand lying where the coat's side hangs (its line half a centimetre outside the cloth's, palm and fingers
	// down its first two bones): the cloth keeps off the glove's own thickness (Body 4 cm from the hand's line), not only
	// HandGive past where the animation has it (3.5 here: the glove sank into the coat).
	const FBodySwingSettings Skirt = FBodySwing::SettingsFor(TEXT("skirt_sl"));
	const FVector Anchor(0.0, 0.0, 100.0);
	const TArray<FVector> Targets = SwingTestTargets(Anchor);
	const float Body = 4.f;
	const float RestClearance = 5.f;
	FBodySwingCapsule Hand;
	Hand.A = Anchor + FVector(0.5, 0.0, -12.0);
	Hand.B = Anchor + FVector(0.5, 0.0, -34.0);
	Hand.bShortestWay = true;
	auto Hang = [&](float HandBody)
	{
		TArray<FBodySwingCapsule> Capsules;
		for (int32 Index = 0; Index < SwingTestCount; ++Index)
		{
			// As FBodySwing::Apply: the point's and its bone's distance from the hand as the animation has them.
			const FVector& Tip = Targets[Index];
			const FVector Head = Index > 0 ? Targets[Index - 1] : Anchor;
			FVector OnBone;
			FVector OnLimb;
			FMath::SegmentDistToSegmentSafe(Head, Tip, Hand.A, Hand.B, OnBone, OnLimb);
			FBodySwingCapsule Capsule = Hand;
			Capsule.Radius = FBodySwing::HandRadius(RestClearance, FVector::Dist(Tip, FMath::ClosestPointOnSegment(Tip, Hand.A, Hand.B)), HandBody, 1.0);
			Capsule.BoneRadius = FBodySwing::HandRadius(RestClearance, FVector::Dist(OnBone, OnLimb), HandBody, 1.0);
			Capsules.Add(Capsule);
		}
		FBodySwingPoints Sim;
		Sim.Outward = FVector(1.0, 0.0, 0.0);
		for (int32 Frame = 0; Frame < 180; ++Frame)
		{
			Sim.Step(Anchor, Targets, SwingTestLengths(), Capsules, Skirt, 1.f / 60.f);
		}
		return SwingTestBoneGap(Anchor, Sim.Points, Hand.A, Hand.B);
	};
	const double Kept = Hang(Body);
	TestTrue(FString::Printf(TEXT("The cloth keeps off the glove (%.2f cm from the hand's line)"), Kept), Kept > Body - 0.25);
	const double GiveOnly = Hang(0.f);
	TestTrue(FString::Printf(TEXT("(With the give alone: %.2f cm, into the glove)"), GiveOnly), GiveOnly < Body - 0.25);
	TestTrue(TEXT("A hand far from the cloth keeps its rest clearance"), FMath::IsNearlyEqual(FBodySwing::HandRadius(RestClearance, 20.0, Body, 1.0), RestClearance));
	return true;
}

#endif
