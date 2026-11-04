#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/PlayerCameraMotion.h"
#include "Player/ViewKick.h"
#include "HAL/IConsoleManager.h"

// The camera answering the movement (core-loop audit change 6, Player/PlayerCameraMotion.h): the math only, no world. Each rule
// is driven with the shipped numbers (FTuning's defaults), never a value the player may have set in play; the variables are only
// checked to exist, to be read live and to switch the lot off, and put back as they were.

namespace
{
	using CameraMotion::FTuning;
	namespace Def = CameraMotion::Default;

	/** A console variable, set to Value for the scope and put back after (a variable that doesn't exist is left alone). */
	struct FScopedVariable
	{
		explicit FScopedVariable(const TCHAR* Name, float Value)
			: Variable(IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (Variable)
			{
				Was = Variable->GetFloat();
				Variable->Set(*FString::SanitizeFloat(Value), ECVF_SetByConsole);
			}
		}
		~FScopedVariable()
		{
			if (Variable)
			{
				Variable->Set(*FString::SanitizeFloat(Was), ECVF_SetByConsole);
			}
		}
		IConsoleVariable* Variable = nullptr;
		float Was = 0.f;
	};

	/** One footfall found in the stride clock: when, the clock's unwrapped phase then, and the side asked for. */
	struct FFootfall
	{
		double Time = 0.0;
		double Phase = 0.0;
		float Side = 0.f;
	};

	/**
	 * Runs the gun bob's stride clock the way UpdateViewModel does (StepPhase += Dt x Speed / StepLength x pi, wrapped at 2 pi)
	 * for Seconds at Dt and feeds each frame to a camera motion; returns the footfalls it asked for. OutRate: radians a second.
	 */
	TArray<FFootfall> WalkTheStride(float Speed, float StepLength, double Seconds, float StartPhase, float Dt, float Peak, double& OutRate)
	{
		FCameraMotion Motion;
		TArray<FFootfall> Found;
		OutRate = Speed / StepLength * UE_PI;
		float Phase = StartPhase;
		double Unwrapped = StartPhase;
		float Side = 0.f;
		// The first reading only learns the clock.
		Motion.StepFootfall(Phase, Dt, Peak, Side);
		const int32 Frames = FMath::RoundToInt32(Seconds / Dt);
		for (int32 Frame = 1; Frame <= Frames; ++Frame)
		{
			Phase = FMath::Fmod(Phase + Dt * static_cast<float>(OutRate), 2.f * UE_PI);
			Unwrapped += Dt * OutRate;
			if (Motion.StepFootfall(Phase, Dt, Peak, Side))
			{
				Found.Add({ Frame * static_cast<double>(Dt), Unwrapped, Side });
			}
		}
		return Found;
	}

	/** The pitch a lone kick holds over time: the lowest and the highest the view gets, and when it is settled (s; -1 if not in 2 s). */
	struct FKickTrace
	{
		float Lowest = 0.f;
		float Highest = 0.f;
		float LowestAt = 0.f;
		float SettledAt = -1.f;
	};

	FKickTrace TraceKick(const FViewKick& Kick, float Dt)
	{
		FViewKickStack Stack;
		Stack.Add(Kick);
		FKickTrace Trace;
		for (int32 Frame = 1; Frame <= FMath::RoundToInt32(2.f / Dt); ++Frame)
		{
			Stack.Tick(Dt);
			const float Pitch = Stack.GetRotation().Pitch;
			if (Pitch < Trace.Lowest)
			{
				Trace.Lowest = Pitch;
				Trace.LowestAt = Frame * Dt;
			}
			Trace.Highest = FMath::Max(Trace.Highest, Pitch);
			if (Stack.IsSettled())
			{
				Trace.SettledAt = Frame * Dt;
				break;
			}
		}
		return Trace;
	}
}

// ---------------------------------------------------------------------------
// The variables
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCameraMotionVariablesTest, "Looter.Camera.Motion.Variables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCameraMotionVariablesTest::RunTest(const FString& Parameters)
{
	// The master is Looter.Camera.Motion and every number is Looter.Camera.Motion.<Name> (the audit's contract names).
	TestNotNull(TEXT("Looter.Camera.Motion"), IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Camera.Motion")));
	const TCHAR* Names[] = {
		TEXT("LandMinSpeed"), TEXT("LandFullSpeed"), TEXT("LandPitchMin"), TEXT("LandPitchMax"), TEXT("LandDropMin"), TEXT("LandDropMax"),
		TEXT("LandHertz"), TEXT("LandDamping"), TEXT("LandDropSeconds"), TEXT("JumpPitch"), TEXT("JumpHertz"), TEXT("JumpDamping"),
		TEXT("StepBobWalk"), TEXT("StepBobSprint"), TEXT("StepHertz"), TEXT("StepDamping"), TEXT("SprintSway"), TEXT("StrafeRoll"),
		TEXT("StrafeRate"), TEXT("MantleDip"), TEXT("VaultDip"), TEXT("AimedShare") };
	for (const TCHAR* Name : Names)
	{
		const FString Full = FString(TEXT("Looter.Camera.Motion.")) + Name;
		TestNotNull(*Full, IConsoleManager::Get().FindConsoleVariable(*Full));
	}

	// Read live, never cached: a number set in play is in the next Live().
	{
		FScopedVariable PitchMax(TEXT("Looter.Camera.Motion.LandPitchMax"), 2.5f);
		FScopedVariable Roll(TEXT("Looter.Camera.Motion.StrafeRoll"), 1.5f);
		const FTuning Live = CameraMotion::Live();
		TestEqual(TEXT("LandPitchMax read live"), Live.LandPitchMax, 2.5f);
		TestEqual(TEXT("StrafeRoll read live"), Live.StrafeRoll, 1.5f);
		TestEqual(TEXT("a hard landing dips by the new number"), CameraMotion::LandingPitch(1000.f, Live), 2.5f);
	}
	return true;
}

// ---------------------------------------------------------------------------
// The shipped numbers are the audit's
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCameraMotionShippedTest, "Looter.Camera.Motion.Shipped",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCameraMotionShippedTest::RunTest(const FString& Parameters)
{
	const FTuning Shipped;
	TestEqual(TEXT("on by default"), Shipped.Master, 1.f);
	// Landing dip 0.4-1.6 deg pitch and 1-4 cm eye drop by fall speed; nothing under a step (the footfall's soft landing, 300 cm/s).
	TestEqual(TEXT("dip min"), Shipped.LandPitchMin, 0.4f);
	TestEqual(TEXT("dip max"), Shipped.LandPitchMax, 1.6f);
	TestEqual(TEXT("drop min"), Shipped.LandDropMin, 1.f);
	TestEqual(TEXT("drop max"), Shipped.LandDropMax, 4.f);
	TestEqual(TEXT("under a step"), Shipped.LandMinSpeed, 300.f);
	TestEqual(TEXT("a hard landing"), Shipped.LandFullSpeed, 1000.f);
	// Jump kick 0.3 deg up, step bob 0.15-0.35 deg, sprint sway 0.2 deg roll, strafe roll 0.6 deg.
	TestEqual(TEXT("jump kick"), Shipped.JumpPitch, 0.3f);
	TestEqual(TEXT("walking nod"), Shipped.StepBobWalk, 0.15f);
	TestEqual(TEXT("sprinting nod"), Shipped.StepBobSprint, 0.35f);
	TestEqual(TEXT("sprint sway"), Shipped.SprintSway, 0.2f);
	TestEqual(TEXT("strafe roll"), Shipped.StrafeRoll, 0.6f);
	return true;
}

// ---------------------------------------------------------------------------
// Landing
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCameraMotionLandingTest, "Looter.Camera.Motion.Landing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCameraMotionLandingTest::RunTest(const FString& Parameters)
{
	const FTuning T;

	// Nothing under a step: a curb's hop, a step down, standing still.
	for (const float Slow : { -50.f, 0.f, 150.f, 250.f, 299.9f })
	{
		TestFalse(*FString::Printf(TEXT("%.0f cm/s is not felt"), Slow), CameraMotion::LandingFelt(Slow, T));
		TestEqual(*FString::Printf(TEXT("%.0f cm/s: no dip"), Slow), CameraMotion::LandingPitch(Slow, T), 0.f);
		TestEqual(*FString::Printf(TEXT("%.0f cm/s: no eye drop"), Slow), CameraMotion::LandingDrop(Slow, T), 0.f);
		TestTrue(*FString::Printf(TEXT("%.0f cm/s: no kick"), Slow), CameraMotion::LandingKick(Slow, T).IsNone());
	}

	// The smallest felt landing is 0.4 degrees and 1 cm; a hard one 1.6 and 4, held beyond; between, in proportion.
	TestTrue(TEXT("300 cm/s is felt"), CameraMotion::LandingFelt(300.f, T));
	TestEqual(TEXT("smallest dip"), CameraMotion::LandingPitch(300.f, T), 0.4f);
	TestEqual(TEXT("smallest drop"), CameraMotion::LandingDrop(300.f, T), 1.f);
	TestEqual(TEXT("hard dip"), CameraMotion::LandingPitch(1000.f, T), 1.6f);
	TestEqual(TEXT("hard drop"), CameraMotion::LandingDrop(1000.f, T), 4.f);
	TestEqual(TEXT("a fall past hard dips no more"), CameraMotion::LandingPitch(4000.f, T), 1.6f);
	TestEqual(TEXT("...nor drops the eye more"), CameraMotion::LandingDrop(4000.f, T), 4.f);
	TestEqual(TEXT("halfway dip"), CameraMotion::LandingPitch(650.f, T), 1.f);
	TestEqual(TEXT("halfway drop"), CameraMotion::LandingDrop(650.f, T), 2.5f);
	float LastPitch = 0.f;
	float LastDrop = 0.f;
	bool bMonotonic = true;
	for (float Speed = 300.f; Speed <= 1100.f; Speed += 25.f)
	{
		const float Pitch = CameraMotion::LandingPitch(Speed, T);
		const float Drop = CameraMotion::LandingDrop(Speed, T);
		bMonotonic &= Pitch >= LastPitch && Drop >= LastDrop;
		LastPitch = Pitch;
		LastDrop = Drop;
	}
	TestTrue(TEXT("a harder fall never dips less"), bMonotonic);

	// The kick is a dip (negative pitch, view down), and it plays out as the spring: down in a couple of frames, a small swing
	// back up, settled in under half a second.
	const FViewKick Kick = CameraMotion::LandingKick(650.f, T);
	TestEqual(TEXT("the kick turns the view down by the dip"), Kick.Pitch, -1.f);
	TestEqual(TEXT("...with no roll or yaw"), Kick.Roll + Kick.Yaw, 0.f);
	const FKickTrace Trace = TraceKick(Kick, 1.f / 120.f);
	TestTrue(TEXT("the view reaches the dip"), FMath::IsNearlyEqual(Trace.Lowest, -1.f, 0.03f));
	TestTrue(TEXT("...within 80 ms"), Trace.LowestAt <= 0.08f);
	TestTrue(TEXT("...and comes back up past level by at most a fifth of the dip"), Trace.Highest <= 0.2f);
	TestTrue(TEXT("...settled in under half a second"), Trace.SettledAt > 0.f && Trace.SettledAt <= 0.5f);

	// A hard landing out of a sprint's jump, with every other kick the movement gives, stays inside the stack's turn limit.
	FViewKickStack Stack;
	Stack.Add(CameraMotion::LandingKick(1000.f, T));
	Stack.Add(CameraMotion::StepKick(1.f, 1.f, 1.f, T));
	Stack.Add(CameraMotion::JumpKick(T));
	float Worst = 0.f;
	for (int32 Frame = 0; Frame < 120; ++Frame)
	{
		Stack.Tick(1.f / 120.f);
		Worst = FMath::Max(Worst, FMath::Abs(Stack.GetRotation().Pitch));
	}
	TestTrue(TEXT("a hard landing, a sprint step and a jump together stay under 2.5 degrees"), Worst < 2.5f);
	return true;
}

// ---------------------------------------------------------------------------
// The eye's dip
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCameraMotionEyeDipTest, "Looter.Camera.Motion.EyeDip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCameraMotionEyeDipTest::RunTest(const FString& Parameters)
{
	// The curve: at rest at the start, 1 at the attack time, back to nothing in eight of them, never below the rest height.
	TestEqual(TEXT("starts at 0"), CameraMotion::DipShape(0.f, 0.06f), 0.f);
	TestEqual(TEXT("nothing before the landing"), CameraMotion::DipShape(-0.1f, 0.06f), 0.f);
	TestEqual(TEXT("deepest at the attack time"), CameraMotion::DipShape(0.06f, 0.06f), 1.f);
	TestTrue(TEXT("gone in eight attack times"), CameraMotion::DipShape(0.48f, 0.06f) < 0.001f);
	bool bNeverBelow = true;
	bool bRisesThenFalls = true;
	float Last = 0.f;
	for (int32 Step = 1; Step <= 200; ++Step)
	{
		const float Age = Step * 0.003f;
		const float Shape = CameraMotion::DipShape(Age, 0.06f);
		bNeverBelow &= Shape >= 0.f;
		// The peak step (20 x 0.003 lands a hair over 0.06 in float) counts as the rise.
		bRisesThenFalls &= (Age <= 0.06f + 1.e-4f) ? Shape >= Last - 1.e-6f : Shape <= Last + 1.e-6f;
		Last = Shape;
	}
	TestTrue(TEXT("the eye is never lifted above its rest by a landing"), bNeverBelow);
	TestTrue(TEXT("it goes down once and comes back once"), bRisesThenFalls);

	// A 2.5 cm dip at 60 frames a second: no snap (the first frame is under half way), the deepest point is the depth, nothing left
	// after half a second.
	FCameraMotion Motion;
	Motion.AddEyeDip(2.5f, 0.06f);
	TestEqual(TEXT("no drop in the landing's own frame"), Motion.GetEyeDrop(), 0.f);
	float Deepest = 0.f;
	float PreviousDrop = 0.f;
	float BiggestFrame = 0.f;
	bool bNegative = false;
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		Motion.Advance(1.f / 60.f);
		const float Drop = Motion.GetEyeDrop();
		Deepest = FMath::Max(Deepest, Drop);
		BiggestFrame = FMath::Max(BiggestFrame, FMath::Abs(Drop - PreviousDrop));
		bNegative |= Drop < 0.f;
		PreviousDrop = Drop;
	}
	TestTrue(TEXT("it reaches (nearly) the depth"), FMath::IsNearlyEqual(Deepest, 2.5f, 0.1f));
	TestTrue(TEXT("no frame moves it more than half the depth"), BiggestFrame <= 2.5f * 0.5f);
	TestFalse(TEXT("never negative"), bNegative);
	TestEqual(TEXT("nothing left after half a second"), Motion.GetEyeDrop(), 0.f);

	// Dips stack, held within the limit however many land.
	FCameraMotion Pile;
	for (int32 Each = 0; Each < 10; ++Each)
	{
		Pile.AddEyeDip(4.f, 0.06f);
	}
	for (int32 Frame = 0; Frame < 4; ++Frame)
	{
		Pile.Advance(0.02f);
	}
	TestTrue(TEXT("a pile of dips is held at the limit"), Pile.GetEyeDrop() <= Def::MaxEyeDrop + 1.e-4f);
	Pile.AddEyeDip(-1.f, 0.06f);
	Pile.Reset();
	TestEqual(TEXT("Reset forgets them"), Pile.GetEyeDrop(), 0.f);
	return true;
}

// ---------------------------------------------------------------------------
// The footfall
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCameraMotionStepKickTest, "Looter.Camera.Motion.StepKick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCameraMotionStepKickTest::RunTest(const FString& Parameters)
{
	const FTuning T;

	// A walk nods 0.15 degrees and rolls nothing; a full sprint 0.35 and rolls 0.2 toward the foot's side.
	const FViewKick Walk = CameraMotion::StepKick(0.f, 1.f, 1.f, T);
	TestEqual(TEXT("walking nod"), Walk.Pitch, -0.15f);
	TestEqual(TEXT("no roll walking"), Walk.Roll, 0.f);
	const FViewKick SprintRight = CameraMotion::StepKick(1.f, 1.f, 1.f, T);
	const FViewKick SprintLeft = CameraMotion::StepKick(1.f, 1.f, -1.f, T);
	TestEqual(TEXT("sprinting nod"), SprintRight.Pitch, -0.35f);
	TestEqual(TEXT("sprint sway, right foot"), SprintRight.Roll, 0.2f);
	TestEqual(TEXT("sprint sway, left foot"), SprintLeft.Roll, -0.2f);
	TestEqual(TEXT("the nod is the same either foot"), SprintLeft.Pitch, SprintRight.Pitch);

	// Slower feet nod less; standing nods nothing; the view model's weight past a walk (a sprint's 1.6) is held at the walk's.
	TestEqual(TEXT("a creep nods half"), CameraMotion::StepKick(0.f, 0.5f, 1.f, T).Pitch, -0.075f);
	TestTrue(TEXT("standing still: no nod"), CameraMotion::StepKick(0.f, 0.f, 1.f, T).IsNone());
	TestEqual(TEXT("weight past 1 is held"), CameraMotion::StepKick(1.f, 1.6f, 1.f, T).Pitch, -0.35f);
	TestEqual(TEXT("half a sprint, half way between"), CameraMotion::StepKick(0.5f, 1.f, 1.f, T).Pitch, -0.25f);

	// Every nod is inside the audit's 0.15-0.35 degrees at a full-pace stride.
	for (float Sprint = 0.f; Sprint <= 1.f; Sprint += 0.1f)
	{
		const float Nod = -CameraMotion::StepKick(Sprint, 1.f, 1.f, T).Pitch;
		TestTrue(*FString::Printf(TEXT("sprint %.1f: a nod of %.3f is within 0.15-0.35"), Sprint, Nod), Nod >= 0.15f - 1.e-4f && Nod <= 0.35f + 1.e-4f);
	}

	// It is over before the next step at a walk (about 0.35 s apart), so footfalls never pile up.
	const FKickTrace Trace = TraceKick(Walk, 1.f / 120.f);
	TestTrue(TEXT("a walking nod is settled within 0.3 s"), Trace.SettledAt > 0.f && Trace.SettledAt <= 0.3f);
	TestTrue(TEXT("...and nods down"), Trace.Lowest < 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCameraMotionStridePhaseTest, "Looter.Camera.Motion.StridePhase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCameraMotionStridePhaseTest::RunTest(const FString& Parameters)
{
	const FTuning T;
	const float Peak = CameraMotion::PeakSeconds(T.StepHertz, T.StepDamping);

	// The nod's spring peaks about 35 ms after it is kicked, and PeakSeconds says exactly when (its swing there is 1, and it is the top).
	TestTrue(TEXT("the peak is a few frames after the kick"), Peak > 0.02f && Peak < 0.06f);
	TestTrue(TEXT("the swing is 1 at the peak"), FMath::IsNearlyEqual(FViewKickStack::Response(Peak, T.StepHertz, T.StepDamping), 1.f, 0.01f));
	TestTrue(TEXT("...and less just before"), FViewKickStack::Response(Peak * 0.8f, T.StepHertz, T.StepDamping) < 0.999f);
	TestTrue(TEXT("...and just after"), FViewKickStack::Response(Peak * 1.2f, T.StepHertz, T.StepDamping) < 0.999f);

	// The gun's bob (PlayerLocomotionViewModel.cpp): StepPhase += Dt x Speed / StepLength x pi, the gun at its lowest dip when
	// sin^2 of it is 1 (pi/2 into each step) and swaying right while sin of it is positive. At a walk and a sprint, from a clock
	// that starts anywhere, at 120 and 60 frames a second, over several wraps of the clock.
	struct FStride
	{
		const TCHAR* Name;
		float Speed;
		float StepLength;
		float Start;
		float Dt;
	};
	const FStride Strides[] = {
		{ TEXT("walk at 120 fps"), 459.f, 161.5f, 0.f, 1.f / 120.f },
		{ TEXT("walk at 60 fps, clock started late"), 459.f, 161.5f, 4.1f, 1.f / 60.f },
		{ TEXT("sprint at 120 fps"), 711.f, 212.5f, 2.2f, 1.f / 120.f },
		{ TEXT("sprint at 60 fps"), 711.f, 212.5f, 6.0f, 1.f / 60.f },
		{ TEXT("a slow creep at 144 fps"), 150.f, 120.f, 1.0f, 1.f / 144.f },
	};
	for (const FStride& Stride : Strides)
	{
		double Rate = 0.0;
		const TArray<FFootfall> Found = WalkTheStride(Stride.Speed, Stride.StepLength, 6.0, Stride.Start, Stride.Dt, Peak, Rate);
		const double StepsExpected = 6.0 * Rate / UE_DOUBLE_PI;
		TestTrue(*FString::Printf(TEXT("%s: one footfall a step (%d found, %.1f steps walked)"), Stride.Name, Found.Num(), StepsExpected),
			FMath::Abs(Found.Num() - StepsExpected) <= 1.5);

		const double FramePhase = Rate * Stride.Dt;
		bool bInPhase = true;
		bool bGunLow = true;
		bool bSideMatches = true;
		bool bAlternates = true;
		bool bEvenlySpaced = true;
		for (int32 Index = 0; Index < Found.Num(); ++Index)
		{
			const FFootfall& Each = Found[Index];
			// Where the clock is when the nod's spring peaks, against the nearest of the gun's lowest dips.
			const double AtPeak = Each.Phase + Rate * Peak;
			const double NearestDip = FMath::FloorToDouble((AtPeak - UE_DOUBLE_HALF_PI) / UE_DOUBLE_PI + 0.5) * UE_DOUBLE_PI + UE_DOUBLE_HALF_PI;
			const double Error = AtPeak - NearestDip;
			bInPhase &= Error >= -1.e-3 && Error <= FramePhase * 1.01 + 1.e-3;
			// The gun's own dip then (its StepDip is sin^2 of the clock): at its deepest.
			bGunLow &= FMath::Square(FMath::Sin(AtPeak)) >= 0.96;
			// And the side: the gun sways the way sin of the clock says; the camera's roll is given the same way.
			bSideMatches &= (FMath::Sin(AtPeak) >= 0.0 ? 1.f : -1.f) == Each.Side;
			if (Index > 0)
			{
				bAlternates &= Each.Side == -Found[Index - 1].Side;
				bEvenlySpaced &= FMath::Abs((Each.Time - Found[Index - 1].Time) - UE_DOUBLE_PI / Rate) <= Stride.Dt * 1.01;
			}
		}
		TestTrue(*FString::Printf(TEXT("%s: the nod peaks as the gun's dip does (within a frame of the clock)"), Stride.Name), bInPhase);
		TestTrue(*FString::Printf(TEXT("%s: ...the gun is at its lowest then"), Stride.Name), bGunLow);
		TestTrue(*FString::Printf(TEXT("%s: the roll's side is the side the gun sways to"), Stride.Name), bSideMatches);
		TestTrue(*FString::Printf(TEXT("%s: the feet alternate"), Stride.Name), bAlternates);
		TestTrue(*FString::Printf(TEXT("%s: one step's time apart"), Stride.Name), bEvenlySpaced);
	}

	// Standing still moves the clock by nothing: no footfall. The first reading only learns the clock.
	FCameraMotion Still;
	float Side = 0.f;
	bool bAny = Still.StepFootfall(1.4f, 1.f / 60.f, Peak, Side);
	for (int32 Frame = 0; Frame < 120; ++Frame)
	{
		bAny |= Still.StepFootfall(1.4f, 1.f / 60.f, Peak, Side);
	}
	TestFalse(TEXT("a clock that stands still gives no footfall"), bAny);

	// A clock jumped most of a stride in one frame (a teleport) is not walking either.
	FCameraMotion Jumped;
	Jumped.StepFootfall(0.2f, 1.f / 60.f, Peak, Side);
	TestFalse(TEXT("a jump of the clock gives none"), Jumped.StepFootfall(0.2f + UE_PI + 0.3f, 1.f / 60.f, Peak, Side));

	// Reset forgets the clock: the next reading is a first one again.
	FCameraMotion Walked;
	Walked.StepFootfall(1.0f, 1.f / 60.f, Peak, Side);
	Walked.Reset();
	TestFalse(TEXT("after Reset the next reading only learns the clock"), Walked.StepFootfall(1.5f, 1.f / 60.f, Peak, Side));
	return true;
}

// ---------------------------------------------------------------------------
// Jump, mantle, vault, strafe, the sights
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCameraMotionKicksTest, "Looter.Camera.Motion.Kicks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCameraMotionKicksTest::RunTest(const FString& Parameters)
{
	const FTuning T;

	const FViewKick Jump = CameraMotion::JumpKick(T);
	TestEqual(TEXT("a jump kicks the view up 0.3 degrees"), Jump.Pitch, 0.3f);
	TestEqual(TEXT("...with no roll"), Jump.Roll, 0.f);
	const FKickTrace JumpTrace = TraceKick(Jump, 1.f / 120.f);
	TestTrue(TEXT("...and it settles in under half a second"), JumpTrace.SettledAt > 0.f && JumpTrace.SettledAt <= 0.5f);

	const FViewKick Mantle = CameraMotion::TraversalKick(false, T);
	const FViewKick Vault = CameraMotion::TraversalKick(true, T);
	TestEqual(TEXT("a mantle's landing dips the view"), Mantle.Pitch, -0.5f);
	TestEqual(TEXT("a vault's landing dips it more"), Vault.Pitch, -0.8f);
	TestTrue(TEXT("a vault lands harder than a mantle"), Vault.Pitch < Mantle.Pitch);

	// Strafe: 0.6 degrees at the walk's speed, into the move (+ right), in proportion below it, held at it beyond.
	TestEqual(TEXT("full strafe right"), CameraMotion::StrafeTarget(459.f, 459.f, T), 0.6f);
	TestEqual(TEXT("full strafe left"), CameraMotion::StrafeTarget(-459.f, 459.f, T), -0.6f);
	TestEqual(TEXT("half speed, half the lean"), CameraMotion::StrafeTarget(229.5f, 459.f, T), 0.3f);
	TestEqual(TEXT("running straight ahead leans nothing"), CameraMotion::StrafeTarget(0.f, 459.f, T), 0.f);
	TestEqual(TEXT("past the walk's speed it holds"), CameraMotion::StrafeTarget(900.f, 459.f, T), 0.6f);
	TestEqual(TEXT("no walk speed: held, not divided by zero"), CameraMotion::StrafeTarget(10.f, 0.f, T), 0.6f);

	// Through the sights the lot keeps a quarter: the reticle stays on what it is on.
	TestEqual(TEXT("hip: all of it"), CameraMotion::AimedShare(0.f, T), 1.f);
	TestEqual(TEXT("sights up: a quarter"), CameraMotion::AimedShare(1.f, T), 0.25f);
	TestEqual(TEXT("half way up"), CameraMotion::AimedShare(0.5f, T), 0.625f);
	TestEqual(TEXT("an aim past 1 is held"), CameraMotion::AimedShare(3.f, T), 0.25f);
	TestEqual(TEXT("an aim under 0 is held"), CameraMotion::AimedShare(-1.f, T), 1.f);
	return true;
}

// ---------------------------------------------------------------------------
// The lean's ease
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCameraMotionLeanTest, "Looter.Camera.Motion.Lean",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCameraMotionLeanTest::RunTest(const FString& Parameters)
{
	const FTuning T;
	FCameraMotion Motion;
	// It comes in without a snap (the first frame under a fifth of the way) and is nearly there within half a second.
	const float First = Motion.EaseLean(0.6f, T.StrafeRate, 1.f / 60.f);
	TestTrue(TEXT("the first frame is a small step"), First > 0.f && First < 0.12f);
	float Lean = First;
	for (int32 Frame = 0; Frame < 30; ++Frame)
	{
		Lean = Motion.EaseLean(0.6f, T.StrafeRate, 1.f / 60.f);
	}
	TestTrue(TEXT("it is nearly there in half a second"), Lean > 0.57f && Lean <= 0.6f);
	TestEqual(TEXT("GetLean says the same"), Motion.GetLean(), Lean);

	// And goes out the same way, to the other side if the strafe turns.
	for (int32 Frame = 0; Frame < 60; ++Frame)
	{
		Lean = Motion.EaseLean(-0.6f, T.StrafeRate, 1.f / 60.f);
	}
	TestTrue(TEXT("a strafe turned the other way leans the other way"), Lean < -0.58f);

	// A long frame (a hitch) is eased as a tenth of a second, not leapt: 8 x 0.1 = 80% of the way, where an uncapped half second would
	// have snapped the whole way.
	FCameraMotion Hitch;
	TestTrue(TEXT("a half-second frame moves it 80% of the way, not all of it"), FMath::IsNearlyEqual(Hitch.EaseLean(0.6f, T.StrafeRate, 0.5f), 0.48f, 0.01f));
	return true;
}

// ---------------------------------------------------------------------------
// The master switch
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCameraMotionMasterTest, "Looter.Camera.Motion.Master",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCameraMotionMasterTest::RunTest(const FString& Parameters)
{
	// With the master at 0 every rule gives nothing: today's dead camera.
	auto CheckOff = [this](const TCHAR* Which, const FTuning& Off)
	{
		TestEqual(*FString::Printf(TEXT("%s: master reads 0"), Which), Off.Master, 0.f);
		TestFalse(*FString::Printf(TEXT("%s: a hard landing is not felt"), Which), CameraMotion::LandingFelt(2000.f, Off));
		TestEqual(*FString::Printf(TEXT("%s: no landing dip"), Which), CameraMotion::LandingPitch(2000.f, Off), 0.f);
		TestEqual(*FString::Printf(TEXT("%s: no eye drop"), Which), CameraMotion::LandingDrop(2000.f, Off), 0.f);
		TestTrue(*FString::Printf(TEXT("%s: no landing kick"), Which), CameraMotion::LandingKick(2000.f, Off).IsNone());
		TestTrue(*FString::Printf(TEXT("%s: no jump kick"), Which), CameraMotion::JumpKick(Off).IsNone());
		TestTrue(*FString::Printf(TEXT("%s: no mantle landing"), Which), CameraMotion::TraversalKick(false, Off).IsNone());
		TestTrue(*FString::Printf(TEXT("%s: no vault landing"), Which), CameraMotion::TraversalKick(true, Off).IsNone());
		TestTrue(*FString::Printf(TEXT("%s: no sprint step"), Which), CameraMotion::StepKick(1.f, 1.f, 1.f, Off).IsNone());
		TestTrue(*FString::Printf(TEXT("%s: no walking step"), Which), CameraMotion::StepKick(0.f, 1.f, 1.f, Off).IsNone());
		TestEqual(*FString::Printf(TEXT("%s: no strafe lean"), Which), CameraMotion::StrafeTarget(459.f, 459.f, Off), 0.f);
	};

	FTuning Off;
	Off.Master = 0.f;
	CheckOff(TEXT("a tuning with the master off"), Off);

	// The variable is what flips it, read live; back on it is all there again.
	{
		FScopedVariable Master(TEXT("Looter.Camera.Motion"), 0.f);
		CheckOff(TEXT("Looter.Camera.Motion 0"), CameraMotion::Live());
	}
	const FTuning On = CameraMotion::Live();
	TestEqual(TEXT("back on, the master reads 1"), On.Master, 1.f);
	TestTrue(TEXT("back on, a hard landing is felt"), CameraMotion::LandingFelt(2000.f, On));
	TestFalse(TEXT("back on, a sprint step nods"), CameraMotion::StepKick(1.f, 1.f, 1.f, On).IsNone());
	return true;
}

#endif
