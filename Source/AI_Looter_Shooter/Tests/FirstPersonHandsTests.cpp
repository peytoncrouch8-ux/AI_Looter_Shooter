#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/FirstPersonHandsGrips.h"
#include "Player/FirstPersonHandsMotion.h"
#include "Player/FirstPersonHandsRig.h"
#include "Player/PlayerThrowMotion.h"
#include "Player/PlayerThrowRules.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponModelComponent.h"
#include "AnimationRuntime.h"
#include "Engine/SkeletalMesh.h"
#include "ReferenceSkeleton.h"

// The first-person hands as plain math: the rig on the mannequin's rest pose (hands reach their spots, far ones too, the
// fingers curl into the palm, the body squeezed away), each reload's support-hand path (on the part it works at the
// gun's own steps, home at both ends, never a jump) and the throw's (round the jar while it's held).

namespace
{
	const TCHAR* HandsTestMannequin = TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple");
	/** How near a hand must come to its spot (cm), and the most a path may move between two samples 1/500 of a reload apart. */
	constexpr float HandsTestNear = 0.5f;
	constexpr float HandsTestMaxStep = 6.f;

	/** A gun held as a view model holds one: half a metre out on the right, low, turned a little. */
	FHandsGun HandsTestGun(EWeaponKind Kind, EWeaponReloadPart Part)
	{
		FHandsGun Gun;
		Gun.ToCamera = FTransform(FRotator(4.f, -3.f, 10.f), FVector(28.f, 18.f, -20.f));
		Gun.Kind = Kind;
		Gun.ReloadPart = Part;
		switch (Kind)
		{
		case EWeaponKind::Shotgun:
			Gun.Grip = FVector(-4.5f, 0.f, -6.f);
			Gun.Foregrip = FVector(37.f, 0.f, -3.f);
			break;
		case EWeaponKind::Revolver:
			Gun.Grip = FVector(-1.97f, 0.f, -7.4f);
			Gun.Foregrip = FVector(-1.29f, -3.2f, -8.2f);
			Gun.CylinderRear = FVector(2.1f, -3.f, -3.f);
			Gun.CylinderAxis = FVector::ForwardVector;
			Gun.bHasCylinder = true;
			break;
		default:
			Gun.Grip = FVector(22.5f, 0.f, -10.f);
			Gun.Foregrip = FVector(44.f, 0.f, -4.5f);
			Gun.MagazineSeat = FVector(14.f, 0.f, -5.6f);
			Gun.bHasMagazineSeat = true;
			break;
		}
		// Drawn bigger than modeled, as the game draws it: its sockets with it.
		Gun.Scale = UWeaponModelComponent::ScaleFor(Kind);
		Gun.Grip *= Gun.Scale;
		Gun.Foregrip *= Gun.Scale;
		Gun.MagazineSeat *= Gun.Scale;
		Gun.CylinderRear *= Gun.Scale;
		return Gun;
	}

	FHandPlacement HandsTestAt(FHandsGun Gun, float Progress)
	{
		Gun.ReloadProgress = Progress;
		return FirstPersonHandsMotion::SupportInReload(Gun);
	}

	/** The biggest move of the support hand between neighbouring samples of a whole reload. */
	float HandsTestBiggestStep(const FHandsGun& Gun)
	{
		float Biggest = 0.f;
		FVector Last = HandsTestAt(Gun, 0.f).Hold;
		for (int32 Step = 1; Step <= 500; ++Step)
		{
			const FVector Now = HandsTestAt(Gun, Step / 500.f).Hold;
			Biggest = FMath::Max(Biggest, static_cast<float>(FVector::Dist(Now, Last)));
			Last = Now;
		}
		return Biggest;
	}

	/** When the Nth of a step comes in a reload. */
	float HandsTestStep(EWeaponReloadPart Part, EReloadStep Step, int32 Nth = 0)
	{
		int32 Seen = 0;
		for (const FReloadStepAt& At : LooterReload::Steps(Part))
		{
			if (At.Step == Step && Seen++ == Nth)
			{
				return At.Progress;
			}
		}
		return -1.f;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsRigTest, "Looter.Player.Hands.Rig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsRigTest::RunTest(const FString& Parameters)
{
	const USkeletalMesh* Mannequin = LoadObject<USkeletalMesh>(nullptr, HandsTestMannequin);
	FFirstPersonHandsRig Rig;
	if (!TestNotNull(TEXT("The mannequin (the arms' stand-in)"), Mannequin) || !TestTrue(TEXT("The rig binds to its arms"), Rig.Bind(Mannequin, true)))
	{
		return false;
	}
	TestTrue(TEXT("The mannequin faces its +Y"), Rig.GetForward().Equals(FVector(0.0, 1.0, 0.0), 0.05));

	// Spots in front of the chest: one each hand reaches with its arm alone, and one for each far out where a pump would be.
	const FVector Ahead = Rig.GetForward();
	// Up cross ahead is the body's right (the mannequin's left is its +X).
	const FVector Rightward = FVector::CrossProduct(Rig.GetUp(), Ahead);
	const FVector Chest = Rig.GetShoulderMiddle();
	struct FHandsTestCase
	{
		const TCHAR* Name;
		double Out;
	};
	const FHandsTestCase Cases[] = { { TEXT("within reach"), 40.0 }, { TEXT("far out"), 95.0 } };
	for (const FHandsTestCase& Case : Cases)
	{
		FHandsRigTargets Targets;
		const int32 Left = FirstPersonHands::Index(EFirstPersonHand::Left);
		const int32 Right = FirstPersonHands::Index(EFirstPersonHand::Right);
		Targets.Hands[Right] = FHandPlacement::Make(Chest + Ahead * Case.Out + Rightward * 12.0 - Rig.GetUp() * 18.0, Ahead, -Rightward,
			FHandFingers::Of(EHandGrip::Trigger));
		Targets.Hands[Left] = FHandPlacement::Make(Chest + Ahead * (Case.Out + 15.0) - Rig.GetUp() * 12.0, Rightward, Rig.GetUp(),
			FHandFingers::Of(EHandGrip::Forend));
		// The forearms leave the wrists back, out and down (in the mannequin's own frame).
		Targets.DefaultForearm[Left] = (-Ahead * 0.5 - Rightward * 0.5 - Rig.GetUp() * 0.7).GetSafeNormal();
		Targets.DefaultForearm[Right] = (-Ahead * 0.7 + Rightward * 0.45 - Rig.GetUp() * 0.55).GetSafeNormal();
		TArray<FTransform> Local;
		TArray<FTransform> Space;
		Rig.Pose(Targets, Local, Space);
		for (const EFirstPersonHand Hand : { EFirstPersonHand::Left, EFirstPersonHand::Right })
		{
			const FHandPlacement& Wanted = Targets.Hands[FirstPersonHands::Index(Hand)];
			const TCHAR* Which = Hand == EFirstPersonHand::Left ? TEXT("left") : TEXT("right");
			const float Off = static_cast<float>(FVector::Dist(Rig.HoldPointIn(Hand, Space), Wanted.Hold));
			TestTrue(FString::Printf(TEXT("%s, %s hand: its grip point on its spot (%.2f cm off)"), Case.Name, Which, Off), Off <= HandsTestNear);
			TestTrue(FString::Printf(TEXT("%s, %s hand: turned as told (knuckles and palm)"), Case.Name, Which),
				Rig.FingersDirIn(Hand, Space).Equals(Wanted.GetFingersDir(), 0.02) && Rig.PalmDirIn(Hand, Space).Equals(Wanted.GetPalmDir(), 0.02));
		}
		// The local pose rebuilds the component pose (what the poseable mesh does with it).
		bool bRebuilt = true;
		TArray<FTransform> Rebuilt = Local;
		for (int32 Bone = 0; Bone < Local.Num(); ++Bone)
		{
			const int32 Parent = Mannequin->GetRefSkeleton().GetParentIndex(Bone);
			Rebuilt[Bone] = Parent != INDEX_NONE ? Local[Bone] * Rebuilt[Parent] : Local[Bone];
			bRebuilt = bRebuilt && Rebuilt[Bone].GetLocation().Equals(Space[Bone].GetLocation(), 0.01);
		}
		TestTrue(FString::Printf(TEXT("%s: the bones against their parents make the same pose"), Case.Name), bRebuilt);
	}

	// The fingers: a grip's curl brings a fingertip joint toward the palm's side, an open hand leaves it out.
	FHandsRigTargets Open;
	FHandsRigTargets Gripping;
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FVector Spot = Chest + Ahead * 40.0 + Rightward * (Side == 0 ? -15.0 : 15.0) - Rig.GetUp() * 15.0;
		Open.Hands[Side] = FHandPlacement::Make(Spot, Ahead, -Rig.GetUp(), FHandFingers::Of(EHandGrip::Open));
		Gripping.Hands[Side] = FHandPlacement::Make(Spot, Ahead, -Rig.GetUp(), FHandFingers::Of(EHandGrip::Wrap));
	}
	TArray<FTransform> Local;
	TArray<FTransform> OpenSpace;
	TArray<FTransform> GripSpace;
	Rig.Pose(Open, Local, OpenSpace);
	Rig.Pose(Gripping, Local, GripSpace);
	const int32 MiddleTip = Rig.GetFingerBone(EFirstPersonHand::Right, 2, 2);
	if (TestTrue(TEXT("The right hand's middle finger"), MiddleTip != INDEX_NONE))
	{
		const FVector Palm = Rig.PalmDirIn(EFirstPersonHand::Right, GripSpace);
		const FVector Hold = Rig.HoldPointIn(EFirstPersonHand::Right, GripSpace);
		const double OpenDepth = FVector::DotProduct(OpenSpace[MiddleTip].GetLocation() - Hold, Palm);
		const double GripDepth = FVector::DotProduct(GripSpace[MiddleTip].GetLocation() - Hold, Palm);
		TestTrue(FString::Printf(TEXT("A wrapping hand's fingertips curl round toward its palm side (%.1f cm, open %.1f)"), GripDepth, OpenDepth),
			GripDepth > OpenDepth + 1.0);
	}

	// The stand-in shows only its arms: its head is squeezed away, its hands aren't.
	const int32 Head = Mannequin->GetRefSkeleton().FindBoneIndex(TEXT("head"));
	TestTrue(TEXT("The stand-in's head is squeezed away"), Head != INDEX_NONE && Rig.IsCollapsed(Head)
		&& GripSpace[Head].GetScale3D().X < 0.01);
	TestFalse(TEXT("...its hands aren't"), Rig.IsCollapsed(Rig.GetHandBone(EFirstPersonHand::Left)) || Rig.IsCollapsed(Rig.GetHandBone(EFirstPersonHand::Right)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsArmsTest, "Looter.Player.Hands.Arms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsArmsTest::RunTest(const FString& Parameters)
{
	// SK_FP_Arms read as its maker measured it (Intermediate/FPArms/FPArms_rig.json, Unreal component space; the import
	// turns the bones' axes, so the rig reads places only), then posed on a hip hold's spots: both hands on them, each
	// forearm leaving its wrist the way it's asked (the wrist straight), the arms never stretched.
	const USkeletalMesh* Arms = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Art/Characters/SK_FP_Arms.SK_FP_Arms"));
	FFirstPersonHandsRig Rig;
	if (!TestNotNull(TEXT("SK_FP_Arms"), Arms) || !TestTrue(TEXT("The rig binds to it"), Rig.Bind(Arms, false)))
	{
		return false;
	}
	TestTrue(TEXT("It faces ahead (+X)"), Rig.GetForward().Equals(FVector::ForwardVector, 0.02));
	TestTrue(FString::Printf(TEXT("The right palm faces in, as made (%s)"), *Rig.GetRestPalm(EFirstPersonHand::Right).ToCompactString()),
		Rig.GetRestPalm(EFirstPersonHand::Right).Equals(FVector(-0.214, -0.958, -0.189), 0.05));
	TestTrue(TEXT("...the left its mirror"), Rig.GetRestPalm(EFirstPersonHand::Left).Equals(FVector(-0.214, 0.958, -0.189), 0.05));
	const float GripOff = static_cast<float>(FVector::Dist(Rig.GetRestHoldPoint(EFirstPersonHand::Right), FVector(35.07, 11.335, -32.539)));
	TestTrue(FString::Printf(TEXT("The right grip point where its maker put it (%.2f cm off)"), GripOff), GripOff <= 0.6f);
	const float Knuckle = Rig.GetRestCurl(EFirstPersonHand::Right, 2, 0);
	const float Middle = Rig.GetRestCurl(EFirstPersonHand::Right, 2, 1);
	TestTrue(FString::Printf(TEXT("Its fingers rest a little curled (middle finger %.0f, %.0f degrees)"), Knuckle, Middle),
		Knuckle > 8.f && Knuckle < 25.f && Middle > 12.f && Middle < 28.f);

	// The Bullpup's hip hold's spots (camera space, the eye at the origin, as SK_FP_Arms is made).
	const FReferenceSkeleton& Skeleton = Arms->GetRefSkeleton();
	FHandsRigTargets Targets;
	const int32 Left = FirstPersonHands::Index(EFirstPersonHand::Left);
	const int32 Right = FirstPersonHands::Index(EFirstPersonHand::Right);
	Targets.Hands[Right] = FHandPlacement::Make(FVector(44.0, 16.5, -16.5), FVector(1.0, 0.0, -0.2), FVector(0.1, -1.0, 0.0),
		FHandFingers::Of(EHandGrip::Trigger), FVector(-0.8, 0.35, -0.45));
	Targets.Hands[Left] = FHandPlacement::Make(FVector(68.0, 14.0, -10.0), FVector(0.0, 1.0, -0.2), FVector(0.0, 0.3, 1.0),
		FHandFingers::Of(EHandGrip::Forend), FVector(-0.45, -0.5, -0.75));
	TArray<FTransform> Local;
	TArray<FTransform> Space;
	Rig.Pose(Targets, Local, Space);
	const TCHAR* const Sides[2] = { TEXT("l"), TEXT("r") };
	for (const EFirstPersonHand Hand : { EFirstPersonHand::Left, EFirstPersonHand::Right })
	{
		const int32 Side = FirstPersonHands::Index(Hand);
		const FHandPlacement& Wanted = Targets.Hands[Side];
		const float Off = static_cast<float>(FVector::Dist(Rig.HoldPointIn(Hand, Space), Wanted.Hold));
		TestTrue(FString::Printf(TEXT("Hand %s on its spot (%.2f cm off)"), Sides[Side], Off), Off <= HandsTestNear);
		const int32 Elbow = Skeleton.FindBoneIndex(FName(*FString::Printf(TEXT("lowerarm_%s"), Sides[Side])));
		const int32 Upper = Skeleton.FindBoneIndex(FName(*FString::Printf(TEXT("upperarm_%s"), Sides[Side])));
		const int32 Wrist = Rig.GetHandBone(Hand);
		if (!TestTrue(TEXT("Its arm's bones"), Elbow != INDEX_NONE && Upper != INDEX_NONE && Wrist != INDEX_NONE))
		{
			continue;
		}
		const FVector Forearm = (Space[Elbow].GetLocation() - Space[Wrist].GetLocation()).GetSafeNormal();
		TestTrue(FString::Printf(TEXT("Hand %s: its forearm leaves the wrist as asked (%.2f)"), Sides[Side], FVector::DotProduct(Forearm, Wanted.Forearm)),
			FVector::DotProduct(Forearm, Wanted.Forearm) > 0.9);
		const double UpperLength = FVector::Dist(Space[Upper].GetLocation(), Space[Elbow].GetLocation());
		const double RestUpper = FVector::Dist(FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Upper).GetLocation(),
			FAnimationRuntime::GetComponentSpaceTransformRefPose(Skeleton, Elbow).GetLocation());
		TestTrue(FString::Printf(TEXT("Hand %s: the arm isn't stretched"), Sides[Side]), FMath::IsNearlyEqual(UpperLength, RestUpper, 0.05));
		// The shoulder (where the sleeve ends) is out of the view (90 degrees, 16:9: under the bottom edge, 0.5625 cm down per
		// cm ahead, or behind the eye), the elbow down at the bottom edge or under it.
		const FVector ShoulderAt = Space[Upper].GetLocation();
		const FVector ElbowAt = Space[Elbow].GetLocation();
		TestTrue(FString::Printf(TEXT("Hand %s: the shoulder is out of the view (%s)"), Sides[Side], *ShoulderAt.ToCompactString()),
			ShoulderAt.X <= 0.0 || ShoulderAt.Z / ShoulderAt.X < -0.5625);
		TestTrue(FString::Printf(TEXT("Hand %s: the elbow is low (%s)"), Sides[Side], *ElbowAt.ToCompactString()),
			ElbowAt.X <= 0.0 || ElbowAt.Z / ElbowAt.X < -0.5);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsReloadsTest, "Looter.Player.Hands.Reloads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsReloadsTest::RunTest(const FString& Parameters)
{
	using namespace FirstPersonHandsMotion;
	struct FHandsTestGunCase
	{
		const TCHAR* Name;
		EWeaponKind Kind;
		EWeaponReloadPart Part;
	};
	const FHandsTestGunCase Guns[] = {
		{ TEXT("Rifle (magazine)"), EWeaponKind::Rifle, EWeaponReloadPart::Magazine },
		{ TEXT("Shotgun (shells and pump)"), EWeaponKind::Shotgun, EWeaponReloadPart::Pump },
		{ TEXT("Revolver (cylinder)"), EWeaponKind::Revolver, EWeaponReloadPart::Cylinder },
	};
	for (const FHandsTestGunCase& Case : Guns)
	{
		const FHandsGun Gun = HandsTestGun(Case.Kind, Case.Part);
		const FVector Home = FirstPersonHandsGrips::OnForegrip(Gun).Hold;
		TestTrue(FString::Printf(TEXT("%s: on the foregrip as the reload starts"), Case.Name), HandsTestAt(Gun, 0.f).Hold.Equals(Home, 0.01));
		TestTrue(FString::Printf(TEXT("%s: ...and back on it as it ends"), Case.Name), HandsTestAt(Gun, 1.f).Hold.Equals(Home, 0.01));
		const float Biggest = HandsTestBiggestStep(Gun);
		TestTrue(FString::Printf(TEXT("%s: the hand never jumps (%.2f cm at most between samples)"), Case.Name, Biggest), Biggest <= HandsTestMaxStep);
	}

	// The magazine: on it as it's pulled out and as the fresh one rises, on the handle for the yank.
	{
		const FHandsGun Gun = HandsTestGun(EWeaponKind::Rifle, EWeaponReloadPart::Magazine);
		for (const float Progress : { 0.2f, 0.27f, 0.55f, 0.6f })
		{
			bool bShown = true;
			const float Travel = LooterReload::MagazineTravel(Progress, bShown);
			const float Off = static_cast<float>(FVector::Dist(HandsTestAt(Gun, Progress).Hold, OnMagazine(Gun, Travel).Hold));
			TestTrue(FString::Printf(TEXT("Rifle at %.2f: the hand on the magazine where it is (%.2f cm off)"), Progress, Off), bShown && Off <= HandsTestNear);
		}
		const float Bolt = HandsTestStep(EWeaponReloadPart::Magazine, EReloadStep::Bolt);
		const float OnHandleOff = static_cast<float>(FVector::Dist(HandsTestAt(Gun, Bolt).Hold, OnHandle(Gun, 0.f).Hold));
		TestTrue(FString::Printf(TEXT("Rifle: on the charging handle at its step (%.2f cm off its rest)"), OnHandleOff), OnHandleOff <= 4.5f);
	}

	// The shotgun: a shell thumbed into the port at every shell step, on the pump as it's racked.
	{
		const FHandsGun Gun = HandsTestGun(EWeaponKind::Shotgun, EWeaponReloadPart::Pump);
		for (int32 Shell = 0; Shell < 4; ++Shell)
		{
			const float At = HandsTestStep(EWeaponReloadPart::Pump, EReloadStep::ShellIn, Shell);
			TestTrue(FString::Printf(TEXT("Shotgun: shell %d pushed home at its step"), Shell + 1),
				At >= 0.f && HandsTestAt(Gun, At).Hold.Equals(AtLoadingPort(Gun, 1.f).Hold, HandsTestNear));
		}
		const float PumpBack = 0.86f;
		const FVector Racked = FirstPersonHandsGrips::OnForegrip(Gun, LooterReload::PumpTravel(PumpBack)).Hold;
		TestTrue(TEXT("Shotgun: round the pump as it's racked back"), HandsTestAt(Gun, PumpBack).Hold.Equals(Racked, HandsTestNear));
		TestTrue(TEXT("...which is well back"), LooterReload::PumpTravel(PumpBack) > 5.f);
	}

	// The revolver: the palm on the rod as it punches, the speedloader in as the rounds drop.
	{
		const FHandsGun Gun = HandsTestGun(EWeaponKind::Revolver, EWeaponReloadPart::Cylinder);
		const float Eject = HandsTestStep(EWeaponReloadPart::Cylinder, EReloadStep::Eject);
		const float RoundsIn = HandsTestStep(EWeaponReloadPart::Cylinder, EReloadStep::RoundsIn);
		const float Out = HandsTestStep(EWeaponReloadPart::Cylinder, EReloadStep::CylinderOut);
		TestTrue(TEXT("Revolver: cradling the cylinder as it swings out"), HandsTestAt(Gun, Out).Hold.Equals(OnCylinder(Gun).Hold, HandsTestNear));
		TestTrue(TEXT("Revolver: the palm through the rod's stroke as the empties go"), HandsTestAt(Gun, Eject).Hold.Equals(OnEjectorRod(Gun, 1.f).Hold, HandsTestNear));
		TestTrue(TEXT("Revolver: the speedloader pushed in as the rounds drop"), HandsTestAt(Gun, RoundsIn).Hold.Equals(WithSpeedloader(Gun, 1.f).Hold, HandsTestNear));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsThrowTest, "Looter.Player.Hands.Throw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsThrowTest::RunTest(const FString& Parameters)
{
	using namespace FirstPersonHandsMotion;
	const FHandsGun Gun = HandsTestGun(EWeaponKind::Rifle, EWeaponReloadPart::Magazine);
	const FHandPlacement OnGun = FirstPersonHandsGrips::OnForegrip(Gun);
	TestTrue(TEXT("On the gun as the throw starts"), SupportInThrow(OnGun, 0.f).Hold.Equals(OnGun.Hold, 0.01));
	TestTrue(TEXT("...and as it ends"), SupportInThrow(OnGun, FThrowRules::ThrowSeconds).Hold.Equals(OnGun.Hold, 0.01));
	for (const float Time : { ThrowMotion::DrawSeconds, (ThrowMotion::DrawSeconds + FThrowRules::ReleaseSeconds) * 0.5f })
	{
		TestTrue(FString::Printf(TEXT("Round the jar %.2f s in, while it's in the hand"), Time), ThrowMotion::IsJarInHand(Time)
			&& SupportInThrow(OnGun, Time).Hold.Equals(OnJar(Time).Hold, 0.01));
		// The jar's middle is inside the fist: within the tin's radius of the hand's grip point.
		bool bShown = false;
		const FThrowPose Jar = ThrowMotion::JarPose(Time, bShown);
		const FVector Middle = Jar.Offset + Jar.Rotation.Quaternion().GetUpVector() * 5.f;
		TestTrue(TEXT("...the jar in the fist"), FVector::Dist(OnJar(Time).Hold, Middle) < 3.75);
	}
	float Biggest = 0.f;
	FVector Last = SupportInThrow(OnGun, 0.f).Hold;
	for (int32 Step = 1; Step <= 250; ++Step)
	{
		const FVector Now = SupportInThrow(OnGun, FThrowRules::ThrowSeconds * Step / 250.f).Hold;
		Biggest = FMath::Max(Biggest, static_cast<float>(FVector::Dist(Now, Last)));
		Last = Now;
	}
	TestTrue(FString::Printf(TEXT("The hand never jumps through the throw (%.2f cm at most per 2 ms)"), Biggest), Biggest <= HandsTestMaxStep);
	return true;
}

#endif
