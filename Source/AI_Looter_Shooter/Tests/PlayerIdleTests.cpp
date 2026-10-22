#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "PlayerAnimTestKit.h"
#include "Embers/HeroDefinition.h"
#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"

// The weary revenant's idle (LooterStancePose::DescribeIdle, LooterStancePoseIdle.cpp): its clock's numbers stay in
// bounds and repeat, and on the mannequin's reference pose the hips shift over the feet without moving them, the head
// comes down a little and the right thumb finds the belt. Since 2026-10-22 every hero's own stance (FHeroIdle,
// Looter.Pose.Idle.*): the hands curl at rest, each hero's posture and rests land where the user's stances put them, the
// card walks, the steam's breaths count, and Gauge's head lifts in every stance.

using namespace PlayerAnimTestKit;

namespace
{
	const FVector IdleAhead(0.0, 1.0, 0.0);
	const FVector IdleRight(-1.0, 0.0, 0.0);

	FTransform IdleBone(const FPoseRig& Rig, const FCompactPose& Pose, const TCHAR* Name)
	{
		FCSPose<FCompactPose> Space;
		Space.InitPose(Pose);
		FBoneReference Bone(Name);
		Bone.Initialize(Rig.Bones);
		const FCompactPoseBoneIndex Index = Bone.GetCompactPoseIndex(Rig.Bones);
		return Index.IsValid() ? Space.GetComponentSpaceTransform(Index) : FTransform::Identity;
	}

	/** The reference pose standing in Style's idle, Time seconds in, fully eased in. */
	FCompactPose IdlePose(const FPoseRig& Rig, const FHeroIdle& Style, float Time, float Alpha = 1.f)
	{
		FLooterStanceInput Input;
		Input.IdleAlpha = Alpha;
		Input.IdleTime = Time;
		Input.Idle = Style;
		FCompactPose Pose = Rig.RefPose();
		LooterStancePose::Apply(Pose, Input);
		return Pose;
	}

	/** How far a finger bends over its first two joints (degrees): the knuckle's way in against the end bone's. */
	float Bend(const FPoseRig& Rig, const FCompactPose& Pose, const TCHAR* Side, const TCHAR* Finger)
	{
		const FVector Hand = Rig.Where(Pose, *(FString(TEXT("hand")) + Side));
		const FVector First = Rig.Where(Pose, *FString::Printf(TEXT("%s_01%s"), Finger, Side));
		const FVector Second = Rig.Where(Pose, *FString::Printf(TEXT("%s_02%s"), Finger, Side));
		const FVector Third = Rig.Where(Pose, *FString::Printf(TEXT("%s_03%s"), Finger, Side));
		const FVector In = (First - Hand).GetSafeNormal();
		const FVector Out = (Third - Second).GetSafeNormal();
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(In, Out), -1.0, 1.0)));
	}

	/** The way a bone's front turned from another pose's (the head's look), in the body's frame. */
	FVector TurnedAhead(const FPoseRig& Rig, const FCompactPose& Pose, const FCompactPose& From, const TCHAR* Name)
	{
		return (IdleBone(Rig, Pose, Name).GetRotation() * IdleBone(Rig, From, Name).GetRotation().Inverse()).RotateVector(IdleAhead);
	}
}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoseIdleClockTest, "Looter.Pose.Idle.Clock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPoseIdleClockTest::RunTest(const FString& Parameters)
{
	// The defaults are Ellis's weary idle: the hero's form of the clock gives the same numbers.
	const FLooterIdleStance Old = LooterStancePose::DescribeIdle(53.7f);
	const FLooterIdleStance Ellis = LooterStancePose::DescribeIdle(53.7f, FHeroIdle());
	TestTrue(TEXT("Ellis's stance is the weary idle"), Old.WeightShift == Ellis.WeightShift && Old.Breath == Ellis.Breath
		&& Old.GlanceYaw == Ellis.GlanceYaw && Old.ThumbHook == Ellis.ThumbHook && Ellis.Card < 0.f && Ellis.Sway == 0.f);

	// A card hand: the card walks index to pinky, turning over a half turn a knuckle, and goes back under to the index.
	FHeroIdle Cardsharp;
	Cardsharp.LeftRest = EHeroIdleRest::CardHand;
	Cardsharp.RightRest = EHeroIdleRest::HipHand;
	float Furthest = 0.f;
	bool bInBounds = true;
	bool bNoBelt = true;
	for (float Time = 0.f; Time < LooterStancePose::CardCycleSeconds * 3.f; Time += 0.05f)
	{
		const FLooterIdleStance Idle = LooterStancePose::DescribeIdle(Time, Cardsharp);
		bInBounds &= Idle.Card >= 0.f && Idle.Card <= 3.f;
		Furthest = FMath::Max(Furthest, Idle.Card);
		bNoBelt &= Idle.ThumbHook == 0.f;
	}
	TestTrue(TEXT("The card stays on the knuckles"), bInBounds);
	TestTrue(FString::Printf(TEXT("It reaches the pinky (%.2f)"), Furthest), Furthest > 2.99f);
	TestNearlyEqual(TEXT("A knuckle in: a half turn over"), LooterStancePose::DescribeIdle(0.8f, Cardsharp).CardSpin, UE_PI, 0.01f);
	TestTrue(TEXT("Back at the index as the cycle starts again"),
		LooterStancePose::DescribeIdle(LooterStancePose::CardCycleSeconds * 2.f + 0.01f, Cardsharp).Card < 0.05f);
	TestTrue(TEXT("No belt spells without a belt thumb"), bNoBelt);

	// Gauge's clock: a slower breath (the steam goes with every few), the piston sway between up and down.
	FHeroIdle Engine;
	Engine.BreathPeriod = 5.5f;
	Engine.SwayDepth = 1.5f;
	Engine.SwayPeriod = 5.5f;
	TestEqual(TEXT("Breaths counted on his own breath"), LooterStancePose::DescribeIdle(5.5f * 7.f + 0.1f, Engine).Breaths, 7);
	TestNearlyEqual(TEXT("The sway at its deepest half way through"), LooterStancePose::DescribeIdle(2.75f, Engine).Sway, 1.f, 1e-3f);
	TestNearlyEqual(TEXT("...and up again"), LooterStancePose::DescribeIdle(5.5f, Engine).Sway, 0.f, 1e-3f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoseIdleCurlTest, "Looter.Pose.Idle.Curl",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPoseIdleCurlTest::RunTest(const FString& Parameters)
{
	USkeleton* Skeleton = MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const FPoseRig Rig(*Skeleton);
	const FCompactPose Rest = Rig.RefPose();

	// Every idle hand rests curled (CharArt item 2: no flat, splayed fingers), the thumb in; with no curl, as it was.
	FHeroIdle Open;
	Open.RestCurl = 0.f;
	const FCompactPose Curled = IdlePose(Rig, FHeroIdle(), 0.5f);
	const FCompactPose Opened = IdlePose(Rig, Open, 0.5f);
	TestTrue(TEXT("Curled: sound"), FPoseRig::IsSound(Curled));
	for (const TCHAR* Side : { TEXT("_l"), TEXT("_r") })
	{
		for (const TCHAR* Finger : { TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") })
		{
			const float Now = Bend(Rig, Curled, Side, Finger);
			const float Was = Bend(Rig, Opened, Side, Finger);
			TestTrue(FString::Printf(TEXT("%s%s curled at rest (%.0f degrees, open %.0f)"), Finger, Side, Now, Was), Now >= 50.f && Now > Was + 20.f);
		}
		const FString Thumb = FString(TEXT("thumb_03")) + Side;
		const FString Index = FString(TEXT("ring_01")) + Side;
		const double ThumbNow = FVector::Dist(Rig.Where(Curled, *Thumb), Rig.Where(Curled, *Index));
		const double ThumbWas = FVector::Dist(Rig.Where(Rest, *Thumb), Rig.Where(Rest, *Index));
		TestTrue(FString::Printf(TEXT("The thumb%s in across the palm (%.1f cm from the ring knuckle, %.1f at rest)"), Side, ThumbNow, ThumbWas), ThumbNow < ThumbWas);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoseIdleHeroesTest, "Looter.Pose.Idle.Heroes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPoseIdleHeroesTest::RunTest(const FString& Parameters)
{
	USkeleton* Skeleton = MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const FPoseRig Rig(*Skeleton);
	const FCompactPose Rest = Rig.RefPose();
	const UHeroDefinition* Heroes[] = { UHeroDefinition::Load(TEXT("Ellis")), UHeroDefinition::Load(TEXT("Crane")), UHeroDefinition::Load(TEXT("Odessa")),
		UHeroDefinition::Load(TEXT("Gauge")), UHeroDefinition::Load(TEXT("Pike")) };
	for (const UHeroDefinition* Hero : Heroes)
	{
		if (!TestNotNull(TEXT("A hero's definition (run create_ember_assets.py)"), Hero))
		{
			return false;
		}
	}
	const UHeroDefinition& Ellis = *Heroes[0];
	const UHeroDefinition& Crane = *Heroes[1];
	const UHeroDefinition& Odessa = *Heroes[2];
	const UHeroDefinition& Gauge = *Heroes[3];
	const UHeroDefinition& Pike = *Heroes[4];

	// The data: the user's stances (2026-10-22). Ellis keeps the weary revenant.
	TestTrue(TEXT("Ellis: the weary idle (thumb in the belt, the left hanging, no lean)"), Ellis.Idle.RightRest == EHeroIdleRest::BeltThumb
		&& Ellis.Idle.LeftRest == EHeroIdleRest::Hang && FMath::IsNearlyZero(Ellis.Idle.Lean) && FMath::IsNearlyZero(Ellis.Idle.HipCock));
	TestTrue(TEXT("Crane: stooped, a hand on the toll chain"), Crane.Idle.Lean > 0.f && Crane.Idle.LeftRest == EHeroIdleRest::ChainHand);
	TestTrue(TEXT("Odessa: hip cocked, hand on hip, the card hand"), !FMath::IsNearlyZero(Odessa.Idle.HipCock)
		&& Odessa.Idle.RightRest == EHeroIdleRest::HipHand && Odessa.Idle.LeftRest == EHeroIdleRest::CardHand);
	TestTrue(TEXT("Gauge: wide, swaying, steaming, head up"), Gauge.Idle.StanceWidth > 0.f && Gauge.Idle.SwayDepth > 0.f
		&& Gauge.Idle.SteamEvery > 0 && !Gauge.Idle.SteamVent.IsNearlyZero() && Gauge.Idle.HeadLift > 0.f);
	TestTrue(TEXT("Pike: weight back, thumbs in the straps, looking off"), Pike.Idle.HipsBack > 0.f && Pike.Idle.LeftRest == EHeroIdleRest::StrapThumb
		&& Pike.Idle.RightRest == EHeroIdleRest::StrapThumb && FMath::Abs(Pike.Idle.HeadYaw) > 10.f);

	// On the mannequin, half a second in (no glance, no belt spell yet).
	const float Time = 0.5f;
	const FCompactPose EllisPose = IdlePose(Rig, Ellis.Idle, Time);
	const FVector Chest = Rig.Where(Rest, TEXT("spine_05"));
	const FVector Hips = Rig.Where(Rest, TEXT("pelvis"));
	for (const UHeroDefinition* Hero : Heroes)
	{
		const FCompactPose Pose = IdlePose(Rig, Hero->Idle, Time);
		const FString Who = Hero->Id.ToString();
		TestTrue(FString::Printf(TEXT("%s: sound"), *Who), FPoseRig::IsSound(Pose));
		for (const TCHAR* Foot : { TEXT("foot_l"), TEXT("foot_r") })
		{
			const FVector Planted = Rig.Where(Rest, Foot) + IdleRight * ((FString(Foot).EndsWith(TEXT("_r")) ? 1.0 : -1.0) * Hero->Idle.StanceWidth);
			TestTrue(FString::Printf(TEXT("%s: %s planted (%.1f cm off)"), *Who, Foot, FVector::Dist(Rig.Where(Pose, Foot), Planted)),
				FVector::Dist(Rig.Where(Pose, Foot), Planted) <= 1.5);
		}
	}

	// Crane: the head lower and further forward than Ellis's; the left hand at the middle of his chest.
	const FCompactPose CranePose = IdlePose(Rig, Crane.Idle, Time);
	const FVector CraneHead = Rig.Where(CranePose, TEXT("head"));
	const FVector EllisHead = Rig.Where(EllisPose, TEXT("head"));
	TestTrue(FString::Printf(TEXT("Crane's head low and forward (%.1f down, %.1f ahead of Ellis's)"), EllisHead.Z - CraneHead.Z,
		FVector::DotProduct(CraneHead - EllisHead, IdleAhead)), CraneHead.Z < EllisHead.Z - 2.0 && FVector::DotProduct(CraneHead - EllisHead, IdleAhead) > 2.0);
	const FVector ChainHand = Rig.Where(CranePose, TEXT("hand_l"));
	TestTrue(FString::Printf(TEXT("Crane's left hand on his chest (%.1f ahead, %.1f across, %.1f up from it)"), FVector::DotProduct(ChainHand - Chest, IdleAhead),
		FVector::DotProduct(ChainHand - Chest, IdleRight), ChainHand.Z - Chest.Z),
		FVector::DotProduct(ChainHand - Chest, IdleAhead) > 5.0 && FMath::Abs(FVector::DotProduct(ChainHand - Chest, IdleRight)) < 14.0
		&& FMath::Abs(ChainHand.Z - Chest.Z) < 25.0);

	// Odessa: the hips over her right leg; the right hand on the hip; the left raised, the hand over the elbow.
	const FCompactPose OdessaPose = IdlePose(Rig, Odessa.Idle, Time);
	const double Cock = FVector::DotProduct(Rig.Where(OdessaPose, TEXT("pelvis")) - Hips, IdleRight) * FMath::Sign(Odessa.Idle.HipCock);
	TestTrue(FString::Printf(TEXT("Odessa's hip cocked (%.1f cm)"), Cock), Cock > 2.5);
	const FVector HipHand = Rig.Where(OdessaPose, TEXT("hand_r")) - Rig.Where(OdessaPose, TEXT("pelvis"));
	TestTrue(FString::Printf(TEXT("Odessa's right hand on the hip (%.1f out, %.1f up)"), FVector::DotProduct(HipHand, IdleRight), HipHand.Z),
		FVector::DotProduct(HipHand, IdleRight) > 10.0 && HipHand.Z > -5.0 && HipHand.Z < 25.0);
	TestTrue(TEXT("Odessa's left hand raised over its elbow"), Rig.Where(OdessaPose, TEXT("hand_l")).Z > Rig.Where(OdessaPose, TEXT("lowerarm_l")).Z + 10.0);

	// Gauge: the head up against Ellis's; both hands out clear of the body.
	const FCompactPose GaugePose = IdlePose(Rig, Gauge.Idle, Time);
	const FVector GaugeLook = TurnedAhead(Rig, GaugePose, EllisPose, TEXT("head"));
	TestTrue(FString::Printf(TEXT("Gauge's head lifted (%.1f degrees over Ellis's)"), FMath::RadiansToDegrees(FMath::Asin(GaugeLook.Z))), GaugeLook.Z > 0.08);
	for (const TCHAR* Hand : { TEXT("hand_l"), TEXT("hand_r") })
	{
		const double Out = FMath::Abs(FVector::DotProduct(Rig.Where(GaugePose, Hand) - Hips, IdleRight));
		const double Was = FMath::Abs(FVector::DotProduct(Rig.Where(Rest, Hand) - Hips, IdleRight));
		TestTrue(FString::Printf(TEXT("Gauge's %s clear of the boiler (%.1f cm out, %.1f at rest)"), Hand, Out, Was), Out > Was + 4.0);
	}

	// Pike: the head turned off to the horizon; both hands up at the straps.
	const FCompactPose PikePose = IdlePose(Rig, Pike.Idle, Time);
	const FVector PikeLook = TurnedAhead(Rig, PikePose, EllisPose, TEXT("head"));
	const float PikeYaw = FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(PikeLook, IdleRight), FVector::DotProduct(PikeLook, IdleAhead)));
	TestTrue(FString::Printf(TEXT("Pike looks off (%.0f degrees)"), PikeYaw), FMath::Abs(PikeYaw) > 15.f);
	for (const TCHAR* Hand : { TEXT("hand_l"), TEXT("hand_r") })
	{
		const FVector Strap = Rig.Where(PikePose, Hand) - Chest;
		TestTrue(FString::Printf(TEXT("Pike's %s at a strap (%.1f ahead, %.1f up of the chest)"), Hand, FVector::DotProduct(Strap, IdleAhead), Strap.Z),
			FVector::DotProduct(Strap, IdleAhead) > 4.0 && FMath::Abs(Strap.Z) < 30.0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoseIdleHeadLiftTest, "Looter.Pose.Idle.HeadLift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FPoseIdleHeadLiftTest::RunTest(const FString& Parameters)
{
	USkeleton* Skeleton = MannequinSkeleton();
	if (!TestNotNull(TEXT("The mannequin's skeleton"), Skeleton))
	{
		return false;
	}
	FMemMark Mark(FMemStack::Get());
	const FPoseRig Rig(*Skeleton);
	const FCompactPose Rest = Rig.RefPose();

	// On the move (no idle), the head lifted by the stance's HeadLift alone: Gauge's, the user's "looking down all the time".
	FLooterStanceInput Moving;
	Moving.Idle.HeadLift = 6.f;
	TestTrue(TEXT("A head lift needs the layer"), Moving.NeedsLayer());
	FCompactPose Lifted = Rest;
	LooterStancePose::Apply(Lifted, Moving);
	const FVector Look = TurnedAhead(Rig, Lifted, Rest, TEXT("head"));
	TestNearlyEqual(TEXT("The head 6 degrees up"), FMath::RadiansToDegrees(FMath::Asin(static_cast<float>(Look.Z))), 6.f, 0.5f);
	TestTrue(TEXT("Nothing else turns"), Rig.Where(Lifted, TEXT("hand_r")).Equals(Rig.Where(Rest, TEXT("hand_r")), 0.01));
	return true;
}

#endif
