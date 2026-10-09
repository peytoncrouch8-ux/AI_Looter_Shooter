#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/Animation/LooterClimbPose.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/PlayerSize.h"
#include "Tests/PlayerAnimTestKit.h"

// The player's body animation without a world: the climbing pose worked out from a mantle's and a vault's plan, and the
// stance layer run on the mannequin's reference pose, bone by bone. Played on the real character, with the real Anim
// Blueprints: PlayerAnimPlayTests.cpp.

using namespace PlayerAnimTestKit;

namespace
{
	/** A pose's worst jump in one frame (cm) for the weights and for where the hands reach: these pop if they teleport. */
	constexpr float WeightStepLimit = 0.4f;
	constexpr float LedgeStepLimit = 25.f;
	/** The most a bone of the posed body may move in one frame (cm, component space): hands and feet follow targets that run at the climb's speed. */
	constexpr float BoneStepLimit = 30.f;

	struct FClimbStats
	{
		float PeakLegs = 0.f;
		float PeakHands = 0.f;
		float MaxWeightStep = 0.f;
		float MaxLedgeStep = 0.f;
		FLooterClimbInput AtEnd;
		FLooterClimbInput Last;
		FLooterClimbInput First;
		bool bSawEnd = false;
		bool bFinite = true;
	};

	FClimbStats Measure(const FPlannedMove& Planned)
	{
		FClimbStats Stats;
		FLooterClimbInput Previous;
		bool bHavePrevious = false;
		Follow(Planned, 0.6f, [&](const FLooterClimbInput& Input, float /*Alpha*/, bool bMoving)
		{
			const float Numbers[] = { Input.Legs, Input.Hands, Input.Lean, Input.LedgeAhead, Input.LedgeUp };
			for (const float Number : Numbers)
			{
				Stats.bFinite &= FMath::IsFinite(Number);
			}
			Stats.PeakLegs = FMath::Max(Stats.PeakLegs, Input.Legs);
			Stats.PeakHands = FMath::Max(Stats.PeakHands, Input.Hands);
			if (!bHavePrevious)
			{
				Stats.First = Input;
			}
			else
			{
				Stats.MaxWeightStep = FMath::Max3(Stats.MaxWeightStep, FMath::Abs(Input.Legs - Previous.Legs), FMath::Max(FMath::Abs(Input.Hands - Previous.Hands), FMath::Abs(Input.Lean - Previous.Lean)));
				// The ledge numbers are only meaningful while a hand is on them.
				if (Input.Hands > 0.05f && Previous.Hands > 0.05f)
				{
					Stats.MaxLedgeStep = FMath::Max3(Stats.MaxLedgeStep, FMath::Abs(Input.LedgeAhead - Previous.LedgeAhead), FMath::Abs(Input.LedgeUp - Previous.LedgeUp));
				}
			}
			if (!bMoving && !Stats.bSawEnd)
			{
				Stats.bSawEnd = true;
				Stats.AtEnd = Input;
			}
			Previous = Input;
			bHavePrevious = true;
			Stats.Last = Input;
		});
		return Stats;
	}

	void CheckClimb(FAutomationTestBase& Test, const TCHAR* Name, const FPlannedMove& Planned, const FClimbStats& Stats)
	{
		Test.TestTrue(FString::Printf(TEXT("%s: every number finite"), Name), Stats.bFinite);
		Test.TestTrue(FString::Printf(TEXT("%s: the legs tuck (peak %.2f)"), Name, Stats.PeakLegs), Stats.PeakLegs >= 0.6f);
		Test.TestTrue(FString::Printf(TEXT("%s: the hands reach (peak %.2f)"), Name, Stats.PeakHands), Stats.PeakHands >= 0.6f);
		Test.TestTrue(FString::Printf(TEXT("%s: it comes in from nothing (legs %.2f, hands %.2f on the first frame)"), Name, Stats.First.Legs, Stats.First.Hands),
			Stats.First.Legs <= 0.2f && Stats.First.Hands <= 0.2f);
		Test.TestTrue(FString::Printf(TEXT("%s: the legs and hands are back to the graph's when the move ends (legs %.3f, hands %.3f)"), Name, Stats.AtEnd.Legs, Stats.AtEnd.Hands),
			Stats.bSawEnd && Stats.AtEnd.Legs <= 0.02f && Stats.AtEnd.Hands <= 0.02f);
		Test.TestTrue(FString::Printf(TEXT("%s: the lean is still in at the end (%.2f) and eases out after (%.3f)"), Name, Stats.AtEnd.Lean, Stats.Last.Lean),
			Stats.AtEnd.Lean >= 0.5f && Stats.Last.Lean <= 0.02f);
		Test.TestTrue(FString::Printf(TEXT("%s: no pop in the weights (%.2f a frame at most)"), Name, Stats.MaxWeightStep), Stats.MaxWeightStep <= WeightStepLimit);
		Test.TestTrue(FString::Printf(TEXT("%s: the hands' target moves smoothly (%.1f cm a frame at most)"), Name, Stats.MaxLedgeStep), Stats.MaxLedgeStep <= LedgeStepLimit);
		Test.TestTrue(FString::Printf(TEXT("%s: the pose knows what move it is"), Name), Stats.First.bVault == Planned.bVault);
	}

	/** A move's pose at the player's size, on a mesh turned as the player's is. */
	FLooterClimbInput DescribeIn(const FPlayerTraversal& Move, float Alpha, float Top)
	{
		return LooterClimbPose::Describe(Move, Alpha, Top, Radius, LooterPlayerSize::Scale, MeshToWorld());
	}

	/** Every bone of two poses the same, within a millimetre. */
	bool SamePose(const FCompactPose& A, const FCompactPose& B)
	{
		for (const FCompactPoseBoneIndex Index : A.ForEachBoneIndex())
		{
			if (!A[Index].Equals(B[Index], 0.01))
			{
				return false;
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerAnimClimbPlanTest, "Looter.PlayerAnim.ClimbPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerAnimClimbPlanTest::RunTest(const FString& Parameters)
{
	// The climbing pose follows the plan: the legs tuck and the hands reach for a point fixed on the obstacle, then let go as
	// the body comes over it; nothing pops; a vault's lower pose alpha (0.6) counts as full.
	const FPlannedMove Mantle = MakeMantle();
	const FPlannedMove Vault = MakeVault();
	if (!TestTrue(TEXT("A mantle and a vault planned"), Mantle.bPlanned && Vault.bPlanned))
	{
		return false;
	}

	FPlayerTraversal Nothing;
	TestFalse(TEXT("No move: no pose"), DescribeIn(Nothing, 1.f, 100.f).IsActive());
	TestFalse(TEXT("A move with no pose alpha: no pose"), DescribeIn(Mantle.Move, 0.f, Mantle.Top).IsActive());

	const FClimbStats MantleStats = Measure(Mantle);
	CheckClimb(*this, TEXT("Mantle"), Mantle, MantleStats);
	const FClimbStats VaultStats = Measure(Vault);
	CheckClimb(*this, TEXT("Vault"), Vault, VaultStats);

	// A mantle's hands start up on the ledge (1.2 m over the feet, 141 cm at the mannequin's size) and come down to its top, by
	// the feet, as the body rises to it.
	{
		FPlayerTraversal Move = Mantle.Move;
		Move.Advance(FrameTime);
		const FLooterClimbInput Start = DescribeIn(Move, 1.f, Mantle.Top);
		TestTrue(FString::Printf(TEXT("Mantle: the hands start up at the ledge (%.1f cm)"), Start.LedgeUp),
			FMath::IsNearlyEqual(Start.LedgeUp, Mantle.Top / LooterPlayerSize::Scale + 2.f, 4.f));
		// The move goes along the world's +X, which is the mesh's +Y (its front): that is the heading the hands reach along.
		TestTrue(FString::Printf(TEXT("Mantle: ...along the move's heading in the mesh's space (%s)"), *Start.Heading.ToCompactString()),
			Start.Heading.Equals(FVector(0.0, 1.0, 0.0), 0.05));
		Move.Finish();
		const FLooterClimbInput End = DescribeIn(Move, 1.f, Mantle.Top);
		TestTrue(FString::Printf(TEXT("Mantle: ...and end on its top, level with the feet (%.1f cm)"), End.LedgeUp), FMath::Abs(End.LedgeUp) <= 5.f);

		// Heard starting or worked out from the plan alone, the top is nearly the same.
		FPlayerTraversal Half = Mantle.Move;
		Half.Advance(Half.GetDuration() * 0.3f);
		const FLooterClimbInput Heard = DescribeIn(Half, 1.f, Mantle.Top);
		const FLooterClimbInput Guessed = DescribeIn(Half, 1.f, LooterClimbPose::UnknownTop);
		TestTrue(FString::Printf(TEXT("Mantle: a top worked out from the plan is close (%.1f against %.1f cm)"), Guessed.LedgeUp, Heard.LedgeUp),
			FMath::Abs(Heard.LedgeUp - Guessed.LedgeUp) <= 5.f);
	}

	// A vault's pose alpha tops out at 0.6 (the locomotion's VaultPoseShare); the body counts that as a full pose. Looked at
	// on the way up, just before the hop's top (the hand lets go from 5% of the move before it): a fixed 40% of the move fell
	// after the top of the vault as planned now (a third of the way in), with the hand already lifting.
	{
		FPlayerTraversal Move = Vault.Move;
		const float Apex = FMath::Clamp(LooterClimbPose::ApexTime(Move) / Move.GetDuration(), 0.3f, 0.65f);
		Move.Advance(Move.GetDuration() * (Apex - 0.07f));
		const FLooterClimbInput Full = DescribeIn(Move, LooterClimbPose::VaultAlphaShare, Vault.Top);
		TestTrue(FString::Printf(TEXT("Vault: the locomotion's top alpha is a full lean (%.2f)"), Full.Lean), FMath::IsNearlyEqual(Full.Lean, 1.f, 0.01f));
		TestTrue(FString::Printf(TEXT("Vault: the free hand is on the rail (%.2f), the legs tucked (%.2f)"), Full.Hands, Full.Legs), Full.Hands >= 0.9f && Full.Legs >= 0.9f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerAnimStancePoseTest, "Looter.PlayerAnim.StancePose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerAnimStancePoseTest::RunTest(const FString& Parameters)
{
	// The stance layer on the mannequin's reference pose: it does nothing when nothing is asked, a crouch drops the head and
	// keeps the feet, a mantle and a vault tuck the legs and reach the hands, and every pose is sound.
	USkeleton* Skeleton = MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const FPoseRig Rig(*Skeleton);
	const FCompactPose Rest = Rig.RefPose();
	TestTrue(TEXT("The reference pose is sound"), FPoseRig::IsSound(Rest));
	const FVector RestHead = Rig.Where(Rest, TEXT("head"));
	const FVector RestFootL = Rig.Where(Rest, TEXT("foot_l"));
	const FVector RestFootR = Rig.Where(Rest, TEXT("foot_r"));
	const FVector RestHandL = Rig.Where(Rest, TEXT("hand_l"));
	const FVector RestHandR = Rig.Where(Rest, TEXT("hand_r"));
	if (!TestTrue(FString::Printf(TEXT("The mannequin's bones are found (head %s)"), *RestHead.ToCompactString()), RestHead.Z > 100.0 && RestFootL.Z < 30.0))
	{
		return false;
	}

	// Nothing asked: the pose comes back as it went in.
	FLooterStanceInput Input;
	FCompactPose Posed = Rest;
	TestTrue(TEXT("Applied"), LooterStancePose::Apply(Posed, Input));
	TestTrue(TEXT("Nothing asked: the pose is untouched"), SamePose(Rest, Posed));

	// A full crouch: the head comes down, the feet stay where the graph put them.
	{
		FLooterStanceInput Crouch;
		Crouch.CrouchAlpha = 1.f;
		Crouch.CrouchTorsoLean = 18.f;
		Crouch.CrouchHipsBack = 7.f;
		Crouch.MaxHipDrop = 50.f;
		Crouch.KneeSplay = 12.f;
		Crouch.CrouchedHeadHeight = 120.f;
		FCompactPose Pose = Rest;
		LooterStancePose::Apply(Pose, Crouch);
		TestTrue(TEXT("Crouch: sound"), FPoseRig::IsSound(Pose));
		const FVector Head = Rig.Where(Pose, TEXT("head"));
		TestTrue(FString::Printf(TEXT("Crouch: the head comes down (%.1f of %.1f cm)"), Head.Z, RestHead.Z), Head.Z <= RestHead.Z - 20.0);
		TestTrue(TEXT("Crouch: the feet stay planted"), Rig.Where(Pose, TEXT("foot_l")).Equals(RestFootL, 3.0) && Rig.Where(Pose, TEXT("foot_r")).Equals(RestFootR, 3.0));
	}

	// A mantle at full pose, reaching for a ledge 40 cm ahead and 100 cm up: the left leg leads up to it, the right hangs bent,
	// both hands reach.
	{
		FLooterStanceInput Mantle;
		Mantle.Climb.Legs = 1.f;
		Mantle.Climb.Hands = 1.f;
		Mantle.Climb.Lean = 1.f;
		Mantle.Climb.TorsoLean = 18.f;
		Mantle.Climb.LedgeAhead = 40.f;
		Mantle.Climb.LedgeUp = 100.f;
		FCompactPose Pose = Rest;
		LooterStancePose::Apply(Pose, Mantle);
		TestTrue(TEXT("Mantle: sound"), FPoseRig::IsSound(Pose));
		FCSPose<FCompactPose> Space;
		Space.InitPose(Pose);
		const FVector FootL = Rig.Where(Space, TEXT("foot_l"));
		const FVector FootR = Rig.Where(Space, TEXT("foot_r"));
		TestTrue(FString::Printf(TEXT("Mantle: the leading foot is up (%.1f of %.1f cm)"), FootL.Z, RestFootL.Z), FootL.Z >= RestFootL.Z + 25.0);
		TestTrue(FString::Printf(TEXT("Mantle: ...the trailing one lifted too (%.1f of %.1f cm)"), FootR.Z, RestFootR.Z), FootR.Z >= RestFootR.Z + 15.0);
		// The mannequin faces +Y in its component space, so its left is +X: the hands take the ledge a shoulder's width apart.
		const FVector LeftLedge(13.0, 40.0, 100.0);
		const FVector RightLedge(-13.0, 40.0, 100.0);
		const FVector HandL = Rig.Where(Space, TEXT("hand_l"));
		const FVector HandR = Rig.Where(Space, TEXT("hand_r"));
		TestTrue(FString::Printf(TEXT("Mantle: the left hand reaches (%.1f cm from the ledge, was %.1f)"), FVector::Dist(HandL, LeftLedge), FVector::Dist(RestHandL, LeftLedge)),
			FVector::Dist(HandL, LeftLedge) < 0.7 * FVector::Dist(RestHandL, LeftLedge));
		TestTrue(FString::Printf(TEXT("Mantle: ...and the right (%.1f cm from the ledge, was %.1f)"), FVector::Dist(HandR, RightLedge), FVector::Dist(RestHandR, RightLedge)),
			FVector::Dist(HandR, RightLedge) < 0.7 * FVector::Dist(RestHandR, RightLedge));

		// Half way in is half way there: the pose blends, it doesn't switch.
		FLooterStanceInput Half = Mantle;
		Half.Climb.Legs = 0.5f;
		FCompactPose HalfPose = Rest;
		LooterStancePose::Apply(HalfPose, Half);
		const double HalfFoot = Rig.Where(HalfPose, TEXT("foot_l")).Z;
		TestTrue(FString::Printf(TEXT("Mantle: half the weight, half the tuck (%.1f between %.1f and %.1f)"), HalfFoot, RestFootL.Z, FootL.Z),
			HalfFoot > RestFootL.Z + 3.0 && HalfFoot < FootL.Z - 3.0);

		// A gun in the right hand keeps that hand off the ledge.
		FLooterStanceInput Armed = Mantle;
		Armed.bAimWithTorso = true;
		FCompactPose ArmedPose = Rest;
		LooterStancePose::Apply(ArmedPose, Armed);
		TestTrue(TEXT("Mantle, armed: sound"), FPoseRig::IsSound(ArmedPose));
		const FVector ArmedHandR = Rig.Where(ArmedPose, TEXT("hand_r"));
		TestTrue(FString::Printf(TEXT("Mantle, armed: the right hand keeps the gun (%.1f cm from the ledge, unarmed %.1f)"), FVector::Dist(ArmedHandR, RightLedge), FVector::Dist(HandR, RightLedge)),
			FVector::Dist(ArmedHandR, RightLedge) > FVector::Dist(HandR, RightLedge) + 10.0);
	}

	// A vault: both legs tucked, one hand planted. The right hand is left alone.
	{
		FLooterStanceInput Vault;
		Vault.Climb.bVault = true;
		Vault.Climb.Legs = 1.f;
		Vault.Climb.Hands = 1.f;
		Vault.Climb.Lean = 1.f;
		Vault.Climb.TorsoLean = 24.f;
		Vault.Climb.LedgeAhead = 30.f;
		Vault.Climb.LedgeUp = 90.f;
		FCompactPose Pose = Rest;
		LooterStancePose::Apply(Pose, Vault);
		TestTrue(TEXT("Vault: sound"), FPoseRig::IsSound(Pose));
		TestTrue(FString::Printf(TEXT("Vault: both feet are tucked up (%.1f and %.1f cm)"), Rig.Where(Pose, TEXT("foot_l")).Z, Rig.Where(Pose, TEXT("foot_r")).Z),
			Rig.Where(Pose, TEXT("foot_l")).Z >= RestFootL.Z + 15.0 && Rig.Where(Pose, TEXT("foot_r")).Z >= RestFootR.Z + 15.0);
		const FVector Rail(13.0, 30.0, 90.0);
		TestTrue(FString::Printf(TEXT("Vault: the left hand is planted (%.1f cm from the rail, was %.1f)"), FVector::Dist(Rig.Where(Pose, TEXT("hand_l")), Rail), FVector::Dist(RestHandL, Rail)),
			FVector::Dist(Rig.Where(Pose, TEXT("hand_l")), Rail) < 0.7 * FVector::Dist(RestHandL, Rail));
		FLooterStanceInput NoHands = Vault;
		NoHands.Climb.Hands = 0.f;
		FCompactPose NoHandsPose = Rest;
		LooterStancePose::Apply(NoHandsPose, NoHands);
		TestTrue(TEXT("Vault: the right hand isn't driven to the rail"), Rig.Where(Pose, TEXT("hand_r")).Equals(Rig.Where(NoHandsPose, TEXT("hand_r")), 0.5));
	}

	// Followed through whole moves on the reference pose: every pose sound, no bone jumping from one frame to the next.
	const TCHAR* Watched[] = { TEXT("pelvis"), TEXT("spine_05"), TEXT("head"), TEXT("hand_l"), TEXT("hand_r"), TEXT("calf_l"), TEXT("calf_r"), TEXT("foot_l"), TEXT("foot_r") };
	for (const FPlannedMove& Planned : { MakeMantle(), MakeVault() })
	{
		const TCHAR* Name = Planned.bVault ? TEXT("Vault") : TEXT("Mantle");
		TArray<FVector> Last;
		bool bSound = true;
		float MaxStep = 0.f;
		const TCHAR* WorstBone = TEXT("");
		Follow(Planned, 0.6f, [&](const FLooterClimbInput& Climb, float /*Alpha*/, bool /*bMoving*/)
		{
			FLooterStanceInput Frame;
			Frame.Climb = Climb;
			Frame.Climb.TorsoLean = Planned.bVault ? 24.f : 18.f;
			FCompactPose Pose = Rest;
			LooterStancePose::Apply(Pose, Frame);
			bSound &= FPoseRig::IsSound(Pose);
			FCSPose<FCompactPose> Space;
			Space.InitPose(Pose);
			TArray<FVector> Now;
			for (const TCHAR* Bone : Watched)
			{
				Now.Add(Rig.Where(Space, Bone));
			}
			if (Last.Num() == Now.Num())
			{
				for (int32 Index = 0; Index < Now.Num(); ++Index)
				{
					const float Step = static_cast<float>(FVector::Dist(Now[Index], Last[Index]));
					if (Step > MaxStep)
					{
						MaxStep = Step;
						WorstBone = Watched[Index];
					}
				}
			}
			Last = MoveTemp(Now);
		});
		TestTrue(FString::Printf(TEXT("%s: every posed frame is sound"), Name), bSound);
		TestTrue(FString::Printf(TEXT("%s: no bone jumps (%.1f cm in a frame at most, the %s)"), Name, MaxStep, WorstBone), MaxStep <= BoneStepLimit);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerAnimMeleeJabTest, "Looter.PlayerAnim.MeleeJab",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerAnimMeleeJabTest::RunTest(const FString& Parameters)
{
	// Third person had no melee strike: the body stood still while the first-person stock swung. The stance layer jabs it
	// now, on the strike's clock: cocked back a little, out at the blow, back by the swing's end, never in a jump.
	TestEqual(TEXT("At rest, no jab"), LooterStancePose::MeleeJab(0.f), 0.f);
	TestTrue(TEXT("Cocked back first"), LooterStancePose::MeleeJab(0.05f) < -0.2f);
	TestNearlyEqual(TEXT("Out at the blow"), LooterStancePose::MeleeJab(0.12f), 1.f, 0.01f);
	TestEqual(TEXT("Back by the swing's end"), LooterStancePose::MeleeJab(0.45f), 0.f);
	float BiggestStep = 0.f;
	for (float Time = 0.f; Time < 0.5f; Time += 1.f / 120.f)
	{
		BiggestStep = FMath::Max(BiggestStep, FMath::Abs(LooterStancePose::MeleeJab(Time + 1.f / 120.f) - LooterStancePose::MeleeJab(Time)));
	}
	TestTrue(FString::Printf(TEXT("The jab never jumps (%.2f of its reach in a 120th of a second at most)"), BiggestStep), BiggestStep < 0.35f);

	USkeleton* Skeleton = MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const FPoseRig Rig(*Skeleton);
	const FCompactPose Rest = Rig.RefPose();
	FLooterStanceInput Jab;
	Jab.MeleeJab = 1.f;
	FCompactPose Pose = Rest;
	LooterStancePose::Apply(Pose, Jab);
	TestTrue(TEXT("Jab: sound"), FPoseRig::IsSound(Pose));
	// The mannequin faces +Y in its component space.
	const FVector HandAt = Rig.Where(Pose, TEXT("hand_r"));
	const FVector HandWas = Rig.Where(Rest, TEXT("hand_r"));
	TestTrue(FString::Printf(TEXT("Jab: the right hand drives forward (%.1f cm)"), HandAt.Y - HandWas.Y), HandAt.Y - HandWas.Y > 12.0);
	TestTrue(TEXT("Jab: the feet stay put"), Rig.Where(Pose, TEXT("foot_l")).Equals(Rig.Where(Rest, TEXT("foot_l")), 0.5)
		&& Rig.Where(Pose, TEXT("foot_r")).Equals(Rig.Where(Rest, TEXT("foot_r")), 0.5));
	return true;
}

#endif
