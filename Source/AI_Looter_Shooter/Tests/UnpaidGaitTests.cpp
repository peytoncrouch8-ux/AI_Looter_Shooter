#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Creatures/UnpaidCreature.h"
#include "Creatures/UnpaidGaitRules.h"
#include "Tests/EncounterTestWorld.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Tests/AutomationCommon.h"

// The Unpaid's body language (2026-10-31; Creatures/UnpaidGaitRules, UnpaidCreatureGait.cpp): its lurch (a quick heave and a
// slow drag each beat, thrown side to side, one side weaker, its head held level, its arms swung with it, no jump between
// frames, no creep), its breath, its head's tilt at rest and hunting (a loll, now and then a crack; level as it attacks), the
// coil and shiver of a wind-up, a rake's twist, a hit's loll and stumble, how a plain kill falls, and in a test level its head
// moving at rest and its body never jumping through idle, a hunt, a hit and a fall.

namespace
{
	constexpr float Frame = 1.f / 60.f;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidGaitRulesTest, "Looter.Creatures.Unpaid.Gait.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidGaitRulesTest::RunTest(const FString& Parameters)
{
	using namespace UnpaidGait;
	// Its beat: quicker the faster it drifts.
	TestTrue(TEXT("It lurches quicker chasing than drifting"), LurchHz(1.f) > LurchHz(0.3f) && LurchHz(0.3f) > LurchHz(0.f));
	TestNearlyEqual(TEXT("Past its chase it lurches no quicker"), LurchHz(1.3f), LurchHz(1.f), 0.0001f);

	// A beat's shape: up quickly from nothing to its peak, a slow drag back down to nothing.
	TestNearlyEqual(TEXT("A beat starts at nothing"), BeatShape(0.f), 0.f, 0.001f);
	TestNearlyEqual(TEXT("...and drags back to nothing"), BeatShape(1.f), 0.f, 0.001f);
	float Peak = 0.f;
	float PeakAt = 0.f;
	for (float Into = 0.f; Into <= 1.f; Into += 0.01f)
	{
		if (BeatShape(Into) > Peak)
		{
			Peak = BeatShape(Into);
			PeakAt = Into;
		}
	}
	TestTrue(FString::Printf(TEXT("...its heave peaks early (%.2f in), its drag is long"), PeakAt), Peak > 0.99f && PeakAt < 0.4f);

	// Still: no lurch at all.
	const FLurch Still = Lurch(0.1f, 0.f, 0.5f, 1.f);
	TestTrue(TEXT("Hanging still, no lurch"), Still.Surge == 0.f && Still.Roll == 0.f && Still.Twist == 0.f && Still.ArmSwing[0] == 0.f);

	// Over a cycle at full drift: no creep forward, thrown one way then the other, the weak beat smaller, the head held level,
	// the heaving side's arm thrown forward, and no jump between two frames at 120 Hz.
	double SurgeSum = 0.0;
	int32 Samples = 0;
	float StrongRoll = 0.f;
	float WeakRoll = 0.f;
	float BiggestStep = 0.f;
	bool bHeadCounters = true;
	bool bArmsWithBeat = true;
	FLurch Last = Lurch(0.f, 1.f, 1.f, 1.f);
	for (float Phase = 0.f; Phase < 1.f; Phase += 1.f / 240.f)
	{
		const FLurch Now = Lurch(Phase, 1.f, 1.f, 1.f);
		SurgeSum += Now.Surge;
		++Samples;
		(Phase < 0.5f ? StrongRoll : WeakRoll) = FMath::Max(Phase < 0.5f ? StrongRoll : WeakRoll, FMath::Abs(Now.Roll));
		bHeadCounters &= Now.Roll == 0.f || FMath::Sign(Now.HeadRoll) == -FMath::Sign(Now.Roll);
		// Its strong side is its right (Side +1): heaving right on the first beat, its right arm thrown forward, its left trailing.
		if (Phase < 0.5f && Now.Roll != 0.f)
		{
			bArmsWithBeat &= Now.Roll < 0.f && Now.ArmSwing[1] <= 0.f && Now.ArmSwing[0] >= 0.f;
		}
		BiggestStep = FMath::Max(BiggestStep, FMath::Abs(Now.Surge - Last.Surge) + FMath::Abs(Now.Heave - Last.Heave));
		Last = Now;
	}
	TestTrue(FString::Printf(TEXT("No creep: its surge swings about nothing (mean %.2f cm)"), SurgeSum / Samples), FMath::Abs(SurgeSum / Samples) < 1.0);
	TestTrue(FString::Printf(TEXT("Its limp: the weak beat half the strong one (%.1f against %.1f)"), WeakRoll, StrongRoll),
		WeakRoll > 0.3f * StrongRoll && WeakRoll < 0.7f * StrongRoll);
	TestTrue(TEXT("Its head held level against the trunk's roll"), bHeadCounters);
	TestTrue(TEXT("Thrown onto its right on its strong beat, its right arm forward, its left trailing"), bArmsWithBeat);
	TestTrue(FString::Printf(TEXT("No jump between frames (at most %.2f cm at 120 Hz)"), BiggestStep), BiggestStep < 1.5f);
	const FLurch Left = Lurch(0.1f, 1.f, 0.f, -1.f);
	const FLurch Right = Lurch(0.1f, 1.f, 0.f, 1.f);
	TestTrue(TEXT("Its strong side its own: the other way round for a left-sided one"), FMath::Sign(Left.Roll) == -FMath::Sign(Right.Roll));

	// Its breath: a slow, small draw, nothing while it moves.
	float Deepest = 0.f;
	for (float Time = 0.f; Time < 8.f; Time += 0.05f)
	{
		Deepest = FMath::Max(Deepest, FMath::Abs(Breath(Time, 1.f).X));
	}
	TestTrue(FString::Printf(TEXT("Its breath shows (%.1f degrees) and stays small"), Deepest), Deepest > 1.f && Deepest < 3.f);
	TestEqual(TEXT("...none while it lurches"), Breath(1.3f, 0.f).X, 0.f);

	// The coil: drawn in over the wind-up's first third, held, tightened at the end; the shiver only at its very end.
	TestNearlyEqual(TEXT("No coil as a wind-up begins"), Coil(0.f), 0.f, 0.001f);
	TestNearlyEqual(TEXT("...full by a third in"), Coil(0.4f), 1.f, 0.001f);
	TestTrue(TEXT("...tightened at the end"), Coil(1.f) > 1.f && Coil(1.f) <= 1.25f + 0.001f);
	TestEqual(TEXT("No shiver mid-wind-up"), ReleaseShiver(0.5f), 0.f);
	TestNearlyEqual(TEXT("...a full shiver as it's about to go"), ReleaseShiver(1.f), 1.f, 0.001f);

	// A rake's twist: wound away from the drawn arm, thrown on through the swipe, never past its limit.
	const float LeftDrawn[2] = { -1.f, 0.f };
	const float LeftSwept[2] = { 1.f, 0.f };
	const float RightSwept[2] = { 0.f, 1.f };
	const float Both[2] = { 1.f, -1.f };
	TestTrue(TEXT("Left arm drawn back: the trunk wound to the left"), SwipeTwist(LeftDrawn) < 0.f);
	TestTrue(TEXT("Left arm swept across: the trunk thrown to the right"), SwipeTwist(LeftSwept) > 0.f);
	TestTrue(TEXT("Right arm swept across: thrown to the left"), SwipeTwist(RightSwept) < 0.f);
	TestTrue(TEXT("Never past its limit"), FMath::Abs(SwipeTwist(Both)) <= 30.f + 0.01f);

	// A hit: the head knocked over the way it was pushed, more for a harder hit, never past its limit; the stumble.
	TestTrue(TEXT("Pushed to its right, the head tips right"), HitHeadRoll(FVector(0.0, 1.0, 0.0), 1.f) < 0.f);
	TestTrue(TEXT("Pushed to its left, left"), HitHeadRoll(FVector(0.0, -1.0, 0.0), 1.f) > 0.f);
	TestTrue(TEXT("Harder, further"), FMath::Abs(HitHeadRoll(FVector(0.0, 1.0, 0.0), 1.5f)) > FMath::Abs(HitHeadRoll(FVector(0.0, 1.0, 0.0), 0.5f)));
	TestTrue(TEXT("Never past its limit"), FMath::Abs(HitHeadRoll(FVector(0.0, 1.0, 0.0), 10.f)) <= 24.f);
	TestEqual(TEXT("A graze doesn't stumble it"), HitStumble(0.1f), 0.f);
	TestTrue(TEXT("A real hit does, briefly"), HitStumble(1.f) > 0.1f && HitStumble(5.f) <= 0.4f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidGaitTiltTest, "Looter.Creatures.Unpaid.Gait.Tilt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidGaitTiltTest::RunTest(const FString& Parameters)
{
	using namespace UnpaidGait;
	// Calm: it lolls over to its goal and hangs there, slowly, without overshooting its reach.
	FTilt Calm;
	StepTilt(Calm, 0.f, ETiltMood::Calm, 0.9f, 1.f, 0.5f);
	TestTrue(TEXT("A calm loll, not a crack"), !Calm.bCrack && Calm.Goal != 0.f);
	const float Goal = Calm.Goal;
	float Widest = 0.f;
	for (float Time = 0.f; Time < 2.f; Time += Frame)
	{
		StepTilt(Calm, Frame, ETiltMood::Calm, 0.5f, 0.5f, 0.5f);
		Widest = FMath::Max(Widest, FMath::Abs(Calm.Roll));
	}
	TestTrue(FString::Printf(TEXT("...it hangs over at its goal (%.1f of %.1f degrees)"), Calm.Roll, Goal), FMath::IsNearlyEqual(Calm.Roll, Goal, 1.5f));
	TestTrue(TEXT("...never past its reach"), Widest <= CalmRoll[1] + 1.f);

	// A crack: snapped over, quickly, with a jerk of the head round; reported once.
	FTilt Crack;
	const bool bCracked = StepTilt(Crack, 0.f, ETiltMood::Calm, 0.2f, 0.5f, 0.f);
	TestTrue(TEXT("A crack, reported"), bCracked && Crack.bCrack && Crack.CrackYaw != 0.f);
	float CrackTime = -1.f;
	for (float Time = 0.f; Time < 1.f && CrackTime < 0.f; Time += Frame)
	{
		TestFalse(TEXT("...once"), StepTilt(Crack, Frame, ETiltMood::Calm, 0.5f, 0.5f, 0.f));
		if (FMath::Abs(Crack.Roll) >= 0.8f * FMath::Abs(Crack.Goal))
		{
			CrackTime = Time;
		}
	}
	TestTrue(FString::Printf(TEXT("...snapped over within a fifth of a second (%.2f s)"), CrackTime), CrackTime >= 0.f && CrackTime <= 0.2f);

	// Hunting: a predator's set tilt, held; attacking: level again.
	FTilt Hunt;
	StepTilt(Hunt, 0.f, ETiltMood::Hunting, 0.7f, 0.5f, 0.f);
	TestTrue(TEXT("Hunting, a set tilt in its range, never a crack"), !Hunt.bCrack && FMath::Abs(Hunt.Goal) >= HuntRoll[0] && FMath::Abs(Hunt.Goal) <= HuntRoll[1]);
	for (float Time = 0.f; Time < 1.f; Time += Frame)
	{
		StepTilt(Hunt, Frame, ETiltMood::Hunting, 0.7f, 0.5f, 0.f);
	}
	for (float Time = 0.f; Time < 1.5f; Time += Frame)
	{
		StepTilt(Hunt, Frame, ETiltMood::Level, 0.7f, 0.5f, 0.f);
	}
	TestTrue(FString::Printf(TEXT("Attacking, its head comes level (%.1f degrees)"), Hunt.Roll), FMath::Abs(Hunt.Roll) < 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidGaitFallsTest, "Looter.Creatures.Unpaid.Gait.Falls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidGaitFallsTest::RunTest(const FString& Parameters)
{
	using namespace UnpaidGait;
	// From behind it falls away: never a reach back at one it can't see, nor a twist round to them.
	bool FromBehind[4] = { false, false, false, false };
	bool FromFront[4] = { false, false, false, false };
	for (float Roll = 0.f; Roll < 1.f; Roll += 0.01f)
	{
		FromBehind[static_cast<int32>(ChooseFall(-1.f, Roll))] = true;
		FromFront[static_cast<int32>(ChooseFall(1.f, Roll))] = true;
	}
	TestFalse(TEXT("Shot from behind: no reach"), FromBehind[static_cast<int32>(EFall::Reach)]);
	TestFalse(TEXT("...no twist"), FromBehind[static_cast<int32>(EFall::Twist)]);
	TestTrue(TEXT("Shot from the front: any of the four"), FromFront[0] && FromFront[1] && FromFront[2] && FromFront[3]);

	// The slump is today's crumple, nothing added.
	const FFallPose Slump = FallPose(EFall::Slump, 0.5f, 1.f);
	TestTrue(TEXT("The slump adds nothing"), Slump.Yaw == 0.f && Slump.Sink == 0.f && Slump.Arm == -1);

	// The reach: the arm on its killer's side, up within a third of a second, dropping as it breaks up.
	const FFallPose ReachUp = FallPose(EFall::Reach, 0.35f, 1.f);
	const FFallPose ReachLate = FallPose(EFall::Reach, 1.2f, 1.f);
	TestEqual(TEXT("The reach is its right arm when its killer stands on its right"), ReachUp.Arm, 1);
	TestEqual(TEXT("...its left for one on its left"), FallPose(EFall::Reach, 0.35f, -1.f).Arm, 0);
	TestTrue(FString::Printf(TEXT("...up within a third of a second (%.2f)"), ReachUp.Reach), ReachUp.Reach > 0.9f);
	TestTrue(TEXT("...and dropping as it breaks up"), ReachLate.Reach < 0.5f);
	TestNearlyEqual(TEXT("...from nothing"), FallPose(EFall::Reach, 0.f, 1.f).Reach, 0.f, 0.001f);

	// The twist: round toward its killer's side and over; the buckle: dropped, folding, its head fallen over.
	TestTrue(TEXT("Twisted round to its killer's side"), FallPose(EFall::Twist, 0.6f, 1.f).Yaw > 40.f && FallPose(EFall::Twist, 0.6f, -1.f).Yaw < -40.f);
	const FFallPose Buckled = FallPose(EFall::Buckle, 0.6f, 1.f);
	TestTrue(TEXT("Buckled: dropped, folding, its head fallen over"), Buckled.Sink < -20.f && Buckled.Pitch > 5.f && FMath::Abs(Buckled.HeadRoll) > 15.f);
	TestTrue(TEXT("...dropping faster as it goes (its strings cut)"),
		FallPose(EFall::Buckle, 0.2f, 1.f).Sink - FallPose(EFall::Buckle, 0.1f, 1.f).Sink < FallPose(EFall::Buckle, 0.1f, 1.f).Sink - FallPose(EFall::Buckle, 0.f, 1.f).Sink);
	for (const EFall Fall : { EFall::Reach, EFall::Twist, EFall::Buckle })
	{
		const FFallPose Start = FallPose(Fall, 0.f, 1.f);
		TestTrue(TEXT("Every fall starts from the pose it died in"), FMath::Abs(Start.Yaw) < 0.01f && FMath::Abs(Start.Sink) < 0.01f
			&& FMath::Abs(Start.HeadRoll) < 0.01f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnpaidGaitPoseTest, "Looter.Creatures.Unpaid.Gait.Pose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnpaidGaitPoseTest::RunTest(const FString& Parameters)
{
	// In a level: hanging about, its head moves on its own (a loll, a tilt); hunting, hit and killed, no bone jumps.
	FTestWorldWrapper Wrapper;
	if (!TestTrue(TEXT("Test world created"), Wrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = Wrapper.GetTestWorld();
	AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(EncounterTestWorld::SpawnCreature(World, AUnpaidCreature::StaticClass(), FVector::ZeroVector));
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(1500.0, 0.0, 90.0));
	if (!TestNotNull(TEXT("Unpaid spawned"), Unpaid) || !TestNotNull(TEXT("Player stand-in"), Player))
	{
		return false;
	}
	if (!Unpaid->GetMesh()->GetSkeletalMeshAsset() || Unpaid->GetBonePose().Num() < 20)
	{
		AddWarning(TEXT("SK_Unpaid isn't imported: its pose isn't checked."));
		return true;
	}
	const FName Head = Unpaid->Rig.Head;
	auto Pose = [Unpaid]()
	{
		TMap<FName, FTransform> Bones;
		for (const FCreatureBonePose& Bone : Unpaid->GetBonePose())
		{
			Bones.Add(Bone.Bone, Bone.Transform);
		}
		return Bones;
	};
	TMap<FName, FTransform> Last = Pose();
	float Jump = 0.f;
	FString JumpWhere;
	auto Run = [&](float Seconds, const TCHAR* When, TArray<FQuat>* HeadTurns)
	{
		for (float Time = 0.f; Time < Seconds; Time += Frame)
		{
			Unpaid->Tick(Frame);
			const TMap<FName, FTransform> Now = Pose();
			for (const TPair<FName, FTransform>& Bone : Now)
			{
				const FTransform* Before = Last.Find(Bone.Key);
				const float Step = Before ? static_cast<float>(FVector::Dist(Before->GetLocation(), Bone.Value.GetLocation())) : 0.f;
				if (Step > Jump)
				{
					Jump = Step;
					JumpWhere = FString::Printf(TEXT("%s, %s"), *Bone.Key.ToString(), When);
				}
			}
			if (HeadTurns && Now.Contains(Head))
			{
				HeadTurns->Add(Now[Head].GetRotation());
			}
			Last = Now;
		}
	};
	TArray<FQuat> Turns;
	Run(8.f, TEXT("hanging about"), &Turns);
	float Widest = 0.f;
	for (const FQuat& Each : Turns)
	{
		Widest = FMath::Max(Widest, FMath::RadiansToDegrees(static_cast<float>(Turns[0].AngularDistance(Each))));
	}
	TestTrue(FString::Printf(TEXT("Hanging about, its head moves on its own: lolls, tilts, looks round (%.1f degrees)"), Widest), Widest > 4.f);
	Unpaid->AlertTo(Player);
	Run(2.f, TEXT("hunting"), nullptr);
	BossTestWorld::Hurt(Unpaid, 1.f);
	Run(0.6f, TEXT("hit"), nullptr);
	BossTestWorld::Kill(Unpaid);
	Run(1.5f, TEXT("falling"), nullptr);
	TestTrue(FString::Printf(TEXT("No bone jumps (at most %.1f cm a frame: %s)"), Jump, *JumpWhere), Jump <= 45.f);
	return true;
}

#endif
