#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "PlayerAnimTestKit.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"

// The weary revenant's idle (LooterStancePose::DescribeIdle, LooterStancePoseIdle.cpp): its clock's numbers stay in
// bounds and repeat, and on the mannequin's reference pose the hips shift over the feet without moving them, the head
// comes down a little and the right thumb finds the belt.

using namespace PlayerAnimTestKit;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerIdleDescribeTest, "Looter.PlayerAnim.Idle.Describe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerIdleDescribeTest::RunTest(const FString& Parameters)
{
	float MinShift = 0.f;
	float MaxShift = 0.f;
	float MaxGlance = 0.f;
	float MaxHook = 0.f;
	int32 Glances = 0;
	bool bInBounds = true;
	bool bFlexOffBelt = true;
	bool bWasGlancing = false;
	for (float Time = 0.f; Time < 240.f; Time += 0.05f)
	{
		const FLooterIdleStance Idle = LooterStancePose::DescribeIdle(Time);
		MinShift = FMath::Min(MinShift, Idle.WeightShift);
		MaxShift = FMath::Max(MaxShift, Idle.WeightShift);
		MaxGlance = FMath::Max(MaxGlance, FMath::Abs(Idle.GlanceYaw));
		MaxHook = FMath::Max(MaxHook, Idle.ThumbHook);
		const bool bGlancing = FMath::Abs(Idle.GlanceYaw) > 1.f;
		Glances += bGlancing && !bWasGlancing ? 1 : 0;
		bWasGlancing = bGlancing;
		bInBounds &= FMath::Abs(Idle.WeightShift) <= 1.f && Idle.Breath >= 0.f && Idle.Breath <= 1.f && FMath::Abs(Idle.GlanceYaw) <= 35.01f
			&& FMath::Abs(Idle.GlancePitch) <= 3.01f && Idle.FlexLeft >= 0.f && Idle.FlexLeft <= 1.f && Idle.ThumbHook >= 0.f && Idle.ThumbHook <= 1.f;
		bFlexOffBelt &= Idle.ThumbHook < 0.999f || Idle.FlexRight < 0.001f;
	}
	TestTrue(TEXT("Every number in bounds"), bInBounds);
	TestTrue(FString::Printf(TEXT("The weight goes onto each leg in turn (%.2f to %.2f)"), MinShift, MaxShift), MinShift < -0.9f && MaxShift > 0.9f);
	TestTrue(FString::Printf(TEXT("He glances aside now and then (%d in 4 minutes, up to %.0f degrees)"), Glances, MaxGlance),
		Glances >= 10 && Glances <= 24 && MaxGlance > 18.f);
	TestTrue(FString::Printf(TEXT("The thumb goes to the belt within 4 minutes (%.2f)"), MaxHook), MaxHook > 0.99f);
	TestTrue(TEXT("The right hand's fingers keep still while it's on the belt"), bFlexOffBelt);

	const FLooterIdleStance A = LooterStancePose::DescribeIdle(37.3f);
	const FLooterIdleStance B = LooterStancePose::DescribeIdle(37.3f);
	TestTrue(TEXT("The same moment is the same idle"), A.GlanceYaw == B.GlanceYaw && A.ThumbHook == B.ThumbHook && A.FlexRight == B.FlexRight);
	TestNearlyEqual(TEXT("A breath every 4.2 s"), LooterStancePose::DescribeIdle(10.f).Breath, LooterStancePose::DescribeIdle(14.2f).Breath, 1e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerIdlePoseTest, "Looter.PlayerAnim.Idle.Pose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPlayerIdlePoseTest::RunTest(const FString& Parameters)
{
	USkeleton* Skeleton = MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const FPoseRig Rig(*Skeleton);
	const FCompactPose Rest = Rig.RefPose();
	const FVector Forward = FVector::YAxisVector;
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);

	// His weight full on the right leg (4.5 s in: the shift's peak).
	FLooterStanceInput Input;
	Input.IdleAlpha = 1.f;
	Input.IdleTime = 4.5f;
	FCompactPose Pose = Rest;
	TestTrue(TEXT("Applied"), LooterStancePose::Apply(Pose, Input));
	TestTrue(TEXT("Sound"), FPoseRig::IsSound(Pose));
	const double Shift = FVector::DotProduct(Rig.Where(Pose, TEXT("pelvis")) - Rig.Where(Rest, TEXT("pelvis")), Right);
	TestTrue(FString::Printf(TEXT("The hips over the right leg (%.1f cm)"), Shift), Shift > 1.5);
	TestTrue(TEXT("The feet stay planted"), Rig.Where(Pose, TEXT("foot_l")).Equals(Rig.Where(Rest, TEXT("foot_l")), 1.0)
		&& Rig.Where(Pose, TEXT("foot_r")).Equals(Rig.Where(Rest, TEXT("foot_r")), 1.0));
	const FVector Head = Rig.Where(Pose, TEXT("head"));
	const FVector RestHead = Rig.Where(Rest, TEXT("head"));
	TestTrue(FString::Printf(TEXT("The head a little low and forward (%.1f cm down, %.1f ahead)"), RestHead.Z - Head.Z,
		FVector::DotProduct(Head - RestHead, Forward)), Head.Z < RestHead.Z && FVector::DotProduct(Head - RestHead, Forward) > 0.0);

	// The thumb in the belt: the first moment it's fully there.
	float HookTime = -1.f;
	for (float Time = 0.f; Time < 240.f && HookTime < 0.f; Time += 0.1f)
	{
		HookTime = LooterStancePose::DescribeIdle(Time).ThumbHook > 0.999f ? Time : -1.f;
	}
	if (TestTrue(TEXT("A moment with the thumb in the belt"), HookTime >= 0.f))
	{
		Input.IdleTime = HookTime;
		FCompactPose Hooked = Rest;
		LooterStancePose::Apply(Hooked, Input);
		TestTrue(TEXT("Hooked: sound"), FPoseRig::IsSound(Hooked));
		const FVector Hips = Rig.Where(Hooked, TEXT("pelvis"));
		const FVector Hand = Rig.Where(Hooked, TEXT("hand_r"));
		const FVector Belt = Hips + Right * 15.0 + Forward * 12.0 + FVector::UpVector * 8.0;
		TestTrue(FString::Printf(TEXT("The right wrist at the belt's front by the hip (%.1f cm off)"), FVector::Dist(Hand, Belt)), FVector::Dist(Hand, Belt) < 6.0);
	}
	return true;
}

#endif
