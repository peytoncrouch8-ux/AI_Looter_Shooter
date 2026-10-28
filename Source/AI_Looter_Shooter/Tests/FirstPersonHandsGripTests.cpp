#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Player/FirstPersonHandsFraming.h"
#include "Player/FirstPersonHandsGrips.h"
#include "Player/FirstPersonHandsRig.h"
#include "Weapons/WeaponModelComponent.h"
#include "Engine/SkeletalMesh.h"

// The first-person hands touching their guns (FPGRIP, 2026-10-22): the handle's solid as plain math, then Ellis's arms
// (SK_FP_Arms, made with the eye at the origin, so the camera's space is the mesh's) on every gun kind's hold: the palms
// seated, the fingers closed on the handles (touching, never in them), the trigger finger on the trigger when it's wanted
// there and straight along the frame when it isn't.

namespace
{
	/** Every gun kind, at a hip-like hold, with its default parts' sockets (Art/Models/Weapons/<Gun>.py). */
	FHandsGun GripTestGun(EWeaponKind Kind, float TriggerReady)
	{
		FHandsGun Gun;
		Gun.ToCamera = FTransform(FRotator(4.f, -3.f, 6.f), FVector(30.f, 18.f, -22.f));
		Gun.Kind = Kind;
		Gun.TriggerReady = TriggerReady;
		switch (Kind)
		{
		case EWeaponKind::Shotgun:
			Gun.Grip = FVector(-4.5f, 0.f, -6.f);
			Gun.Foregrip = FVector(37.f, 0.f, -3.f);
			break;
		case EWeaponKind::Revolver:
			Gun.Grip = FVector(-1.97f, 0.f, -7.4f);
			Gun.Foregrip = FVector(-1.29f, -3.2f, -8.2f);
			break;
		case EWeaponKind::SMG:
			Gun.Grip = FVector(5.4f, 0.f, -8.6f);
			Gun.Foregrip = FVector(25.f, -1.2f, -8.45f);
			break;
		case EWeaponKind::Sniper:
			Gun.Grip = FVector(19.8f, 0.f, -9.f);
			Gun.Foregrip = FVector(52.f, 0.f, -5.2f);
			break;
		case EWeaponKind::LMG:
			Gun.Grip = FVector(5.8f, 0.f, -9.6f);
			Gun.Foregrip = FVector(44.f, 0.f, -4.8f);
			break;
		case EWeaponKind::Launcher:
			Gun.Grip = FVector(48.6f, 0.f, -10.f);
			Gun.Foregrip = FVector(70.f, 0.f, -9.5f);
			break;
		default:
			Gun.Grip = FVector(22.5f, 0.f, -10.f);
			Gun.Foregrip = FVector(44.f, 0.f, -4.5f);
			break;
		}
		Gun.Scale = UWeaponModelComponent::ScaleFor(Kind);
		Gun.Grip *= Gun.Scale;
		Gun.Foregrip *= Gun.Scale;
		return Gun;
	}

	/** Points down a posed finger (its joints, the segments between, the tip past the end joint). */
	void FingerSamples(const FFirstPersonHandsRig& Rig, EFirstPersonHand Hand, int32 Finger, const TArray<FTransform>& Space, TArray<FVector>& Out)
	{
		Out.Reset();
		FVector Joints[3];
		for (int32 Joint = 0; Joint < 3; ++Joint)
		{
			const int32 Bone = Rig.GetFingerBone(Hand, Finger, Joint);
			if (Bone == INDEX_NONE)
			{
				return;
			}
			Joints[Joint] = Space[Bone].GetLocation();
		}
		const FVector Chain[4] = { Joints[0], Joints[1], Joints[2], Rig.FingerTipIn(Hand, Finger, Space) };
		for (int32 Segment = 0; Segment < 3; ++Segment)
		{
			for (const float T : { 0.34f, 0.67f, 1.f })
			{
				Out.Add(FMath::Lerp(Chain[Segment], Chain[Segment + 1], T));
			}
		}
	}

	/** How deep a finger's middle line goes into the handle past its glove (negative: in it), and how near it comes. */
	void FingerGap(const FFirstPersonHandsRig& Rig, EFirstPersonHand Hand, int32 Finger, const TArray<FTransform>& Space, const FHandContact& Contact,
		float& OutDeepest, float& OutNearest)
	{
		TArray<FVector> Samples;
		FingerSamples(Rig, Hand, Finger, Space, Samples);
		OutDeepest = TNumericLimits<float>::Max();
		OutNearest = TNumericLimits<float>::Max();
		for (const FVector& Sample : Samples)
		{
			const float Gap = Contact.Handle.Distance(Sample) - Contact.FingerRadius;
			OutDeepest = FMath::Min(OutDeepest, Gap);
			OutNearest = FMath::Min(OutNearest, FMath::Abs(Gap));
		}
	}

	FVector TriggerPad(const FFirstPersonHandsRig& Rig, const TArray<FTransform>& Space)
	{
		const int32 End = Rig.GetFingerBone(EFirstPersonHand::Right, FHandFingers::IndexFinger, 2);
		return FMath::Lerp(Space[End].GetLocation(), Rig.FingerTipIn(EFirstPersonHand::Right, FHandFingers::IndexFinger, Space), 0.55f);
	}

	/** The game's arms are drawn at 0.88 (UFirstPersonHandsComponent::ArmsScale): the camera's space is 1/0.88 of the mesh's. */
	const FTransform GripTestToMesh(FQuat::Identity, FVector::ZeroVector, FVector(1.0 / 0.88));

	const EWeaponKind GripTestKinds[] = { EWeaponKind::Rifle, EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::SMG, EWeaponKind::Sniper,
		EWeaponKind::LMG, EWeaponKind::Launcher };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsGripShapeTest, "Looter.Player.Hands.Grip.Shape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsGripShapeTest::RunTest(const FString& Parameters)
{
	// A 4 x 3 cm grip, edges rounded 1, upright: its faces, a rounded edge, inside, and the same moved and scaled.
	FHandleShape Grip;
	Grip.bValid = true;
	Grip.Center = FVector(10.0, 0.0, 0.0);
	Grip.Axis = FVector::UpVector;
	Grip.Across = FVector::ForwardVector;
	Grip.HalfAcross = 2.f;
	Grip.HalfThrough = 1.5f;
	Grip.Corner = 1.f;
	TestTrue(TEXT("1 cm ahead of its front face"), FMath::IsNearlyEqual(Grip.Distance(FVector(13.0, 0.0, 5.0)), 1.f, 0.01f));
	TestTrue(TEXT("1 cm off its side"), FMath::IsNearlyEqual(Grip.Distance(FVector(10.0, 2.5, -3.0)), 1.f, 0.01f));
	TestTrue(TEXT("inside: as deep as its nearest face"), FMath::IsNearlyEqual(Grip.Distance(Grip.Center), -1.5f, 0.01f));
	// Off the rounded edge along the diagonal: the corner's circle (middle at 11, 0.5) is 1 cm across.
	const FVector Edge = FVector(11.0, 0.5, 0.0) + FVector(1.0, 1.0, 0.0).GetSafeNormal() * 2.0;
	TestTrue(TEXT("off a rounded edge, from its rounding"), FMath::IsNearlyEqual(Grip.Distance(Edge), 1.f, 0.01f));
	const FHandleShape Moved = Grip.Moved(FTransform(FRotator(0.f, 90.f, 0.f), FVector(0.0, 0.0, 5.0), FVector(2.0)));
	TestTrue(TEXT("moved and scaled, the same point is twice as far"),
		FMath::IsNearlyEqual(Moved.Distance(FTransform(FRotator(0.f, 90.f, 0.f), FVector(0.0, 0.0, 5.0), FVector(2.0)).TransformPosition(FVector(13.0, 0.0, 5.0))), 2.f, 0.02f));
	FHandleShape Nothing;
	TestTrue(TEXT("no shape: far from everything"), Nothing.Distance(FVector::ZeroVector) > 1000.f);
	TestTrue(TEXT("a blend with no shape keeps the shape"), FHandleShape::Blend(Nothing, Grip, 0.2f).Center.Equals(Grip.Center));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsGripContactTest, "Looter.Player.Hands.Grip.Contact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsGripContactTest::RunTest(const FString& Parameters)
{
	const USkeletalMesh* Arms = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Art/Characters/SK_FP_Arms.SK_FP_Arms"));
	FFirstPersonHandsRig Rig;
	if (!TestNotNull(TEXT("SK_FP_Arms"), Arms) || !TestTrue(TEXT("The rig binds to it"), Rig.Bind(Arms, false)))
	{
		return false;
	}
	// How deep a glove may sink (cm; it gives a little), and how near a finger must come to count as touching.
	constexpr float MostIn = 0.35f;
	constexpr float Touching = 0.4f;
	for (const EWeaponKind Kind : GripTestKinds)
	{
		const FString Name = UEnum::GetDisplayValueAsText(Kind).ToString();
		const FHandsGun Gun = GripTestGun(Kind, 1.f);
		FHandsRigTargets Targets;
		const int32 Left = FirstPersonHands::Index(EFirstPersonHand::Left);
		const int32 Right = FirstPersonHands::Index(EFirstPersonHand::Right);
		Targets.Hands[Right] = FirstPersonHandsGrips::OnGrip(Gun).Moved(GripTestToMesh);
		Targets.Hands[Left] = FirstPersonHandsGrips::OnForegrip(Gun).Moved(GripTestToMesh);
		TArray<FTransform> Local;
		TArray<FTransform> Space;
		Rig.Pose(Targets, Local, Space);

		for (const EFirstPersonHand Hand : { EFirstPersonHand::Right, EFirstPersonHand::Left })
		{
			const FHandPlacement& Target = Targets.Hands[FirstPersonHands::Index(Hand)];
			const TCHAR* Which = Hand == EFirstPersonHand::Right ? TEXT("trigger hand") : TEXT("support hand");
			if (!TestTrue(FString::Printf(TEXT("%s, %s: has a handle to close on"), *Name, Which), Target.Contact.Handle.bValid))
			{
				continue;
			}
			// The palm seated: its middle rests on the handle (unless the seat was capped: then the table is wrong).
			const FHandPlacement Seated = Rig.Seated(Target);
			const float PalmGap = Target.Contact.Handle.Distance(Seated.Hold - Seated.GetPalmDir() * FirstPersonHands::GripOffPalmSkin);
			TestTrue(FString::Printf(TEXT("%s, %s: the palm rests on the handle (%.2f cm off)"), *Name, Which, PalmGap),
				FMath::Abs(PalmGap - Target.Contact.SeatGap) < 0.3f);

			int32 Touches = 0;
			int32 Closed = 0;
			for (int32 Finger = 2; Finger < FHandFingers::Count; ++Finger)
			{
				float Deepest = 0.f;
				float Nearest = 0.f;
				FingerGap(Rig, Hand, Finger, Space, Target.Contact, Deepest, Nearest);
				// The six-gun's support hand closes on the shooting hand, its handle only roughly that hand: not checked for depth.
				if (Target.Contact.Share >= 0.99f)
				{
					TestTrue(FString::Printf(TEXT("%s, %s, finger %d: not in the handle (%.2f cm)"), *Name, Which, Finger, Deepest), Deepest > -MostIn);
				}
				const FHandFingers& Solved = Rig.GetLastSolved(Hand);
				AddInfo(FString::Printf(TEXT("%s, %s, finger %d: touched %d, curls %.0f/%.0f/%.0f, deepest %.2f, nearest %.2f"), *Name, Which, Finger,
					(Rig.GetLastTouched(Hand) >> Finger) & 1, Solved.Curl[Finger][0], Solved.Curl[Finger][1], Solved.Curl[Finger][2], Deepest, Nearest));
				Touches += Nearest < Touching ? 1 : 0;
				++Closed;
			}
			// The six-gun's support hand closes partly on the shooting hand (its handle is that hand, roughly).
			const int32 Wanted = Target.Contact.Share >= 0.99f ? Closed : 1;
			TestTrue(FString::Printf(TEXT("%s, %s: middle, ring and pinky touch the handle (%d of %d)"), *Name, Which, Touches, Closed), Touches >= Wanted);
		}

		// The trigger finger: on the trigger when it's wanted there, along the frame (straighter, off it) when not.
		const FHandPlacement& Grip = Targets.Hands[Right];
		if (TestTrue(FString::Printf(TEXT("%s: has a trigger spot"), *Name), Grip.Contact.bTrigger))
		{
			// The Ranchhand's trigger is 13 cm ahead of its stock's wrist: the finger reaches toward it but can't get there
			// (its trigger hand is under the frame's bottom edge at the hip).
			const float OnIt = static_cast<float>(FVector::Dist(TriggerPad(Rig, Space), Grip.Contact.TriggerPoint));
			const float Reach = Kind == EWeaponKind::Shotgun ? 7.f : 3.f;
			TestTrue(FString::Printf(TEXT("%s: the trigger finger's pad on the trigger (%.2f cm off)"), *Name, OnIt), OnIt < Reach);
			FHandsRigTargets Indexed = Targets;
			Indexed.Hands[Right] = FirstPersonHandsGrips::OnGrip(GripTestGun(Kind, 0.f)).Moved(GripTestToMesh);
			TArray<FTransform> IndexedSpace;
			Rig.Pose(Indexed, Local, IndexedSpace);
			const float OffIt = static_cast<float>(FVector::Dist(TriggerPad(Rig, IndexedSpace), Grip.Contact.TriggerPoint));
			TestTrue(FString::Printf(TEXT("%s: indexed, the finger leaves the trigger (%.2f cm off)"), *Name, OffIt),
				Kind == EWeaponKind::Shotgun || OffIt > OnIt + 1.f);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonHandsGripCupTest, "Looter.Player.Hands.Grip.Cup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FFirstPersonHandsGripCupTest::RunTest(const FString& Parameters)
{
	// The user (2026-10-27): the support hand cups beneath the gun's middle (the AR's trigger housing, the LMG's box), not
	// out toward the front; the revolver stays one-handed; the pump gun's hand goes to the pump only to rack and reload.
	TestTrue(TEXT("Looter.Hands.Cup on"), FirstPersonHandsGrips::UseCup());
	const EWeaponKind Kinds[] = { EWeaponKind::Rifle, EWeaponKind::Shotgun, EWeaponKind::SMG, EWeaponKind::Sniper, EWeaponKind::LMG,
		EWeaponKind::Launcher };
	for (const EWeaponKind Kind : Kinds)
	{
		const FString Name = UEnum::GetValueAsString(Kind);
		const FGunHands& Hands = FirstPersonHandsGrips::For(Kind);
		TestTrue(Name + TEXT(": cups"), Hands.bCup && Hands.CupShape.bValid);
		const FHandsGun Gun = GripTestGun(Kind, 0.f);
		const FHandPlacement Rest = FirstPersonHandsGrips::OnForegrip(Gun);
		const FVector Grip = Gun.ToCameraPoint(Gun.Grip);
		const FVector Fore = Gun.ToCameraPoint(Gun.Foregrip);
		const FVector Expected = Gun.ToCameraPoint(Gun.Grip + (Hands.CupHold + FPHandsFraming::SupportNudge(Kind)) * Gun.Scale);
		TestTrue(FString::Printf(TEXT("%s: at rest on its cup (%.2f cm off)"), *Name, FVector::Dist(Rest.Hold, Expected)), Rest.Hold.Equals(Expected, 0.01));
		TestTrue(Name + TEXT(": nearer the grip than the fore-end was (not out toward the front)"),
			FVector::Dist(Rest.Hold, Grip) < FVector::Dist(Fore, Grip));
		TestTrue(Name + TEXT(": palm up under the gun"), FVector::DotProduct(Rest.GetPalmDir(), Gun.ToCameraDir(FVector::UpVector)) > 0.7f);
		TestTrue(Name + TEXT(": its forearm up from below"), FVector::DotProduct(Rest.Forearm, Gun.ToCameraDir(FVector::UpVector)) < -0.6f);
		// A real cup (the user, 2026-10-27: "straighten it out"): the knuckles forward along the gun, the forearm straight
		// behind them, the wrist bent back only (no sideways bend: the forearm in the knuckles' and palm's plane).
		TestTrue(Name + TEXT(": knuckles forward along the gun"), FVector::DotProduct(Rest.GetFingersDir(), Gun.ToCameraDir(FVector::ForwardVector)) > 0.7f);
		const FVector Side = FVector::CrossProduct(Rest.GetFingersDir(), Rest.GetPalmDir()).GetSafeNormal();
		const float Sideways = FMath::RadiansToDegrees(FMath::Asin(FMath::Abs(static_cast<float>(FVector::DotProduct(Rest.Forearm, Side)))));
		TestTrue(FString::Printf(TEXT("%s: no sideways bend at the wrist (%.1f degrees)"), *Name, Sideways), Sideways <= 12.f);
		TestTrue(Name + TEXT(": the forearm behind the knuckles"), FVector::DotProduct(Rest.Forearm, Rest.GetFingersDir()) < -0.3f);
		TestTrue(Name + TEXT(": the fingers close on what it cups"), Rest.Contact.Handle.bValid && Rest.Contact.Share > 0.5f);
		// The cup's palm sits under what it holds: the handle's underside is above the grip point's palm side.
		const float Under = Rest.Contact.Handle.Distance(Rest.Hold - Rest.GetPalmDir() * FirstPersonHands::GripOffPalmSkin);
		TestTrue(FString::Printf(TEXT("%s: the palm against the underside (%.2f cm off it)"), *Name, Under), FMath::Abs(Under) < 1.f);
	}
	TestFalse(TEXT("The revolver stays one-handed (no cup)"), FirstPersonHandsGrips::For(EWeaponKind::Revolver).bCup);

	// The pump gun: on the pump as far as its share, and the reloads and racks on the pump hold.
	FHandsGun Pump = GripTestGun(EWeaponKind::Shotgun, 0.f);
	Pump.PumpShare = 1.f;
	TestTrue(TEXT("Shotgun: on the pump when racking"), FirstPersonHandsGrips::OnForegrip(Pump, 4.f).Hold.Equals(FirstPersonHandsGrips::OnForeEnd(Pump, 4.f).Hold, 0.01));
	Pump.PumpCycle = 0.2f;
	TestEqual(TEXT("...going there as a rack starts"), FirstPersonHandsGrips::NextPumpShare(Pump, 0.f, 0.04f), 0.5f, 0.01f);
	Pump.PumpCycle = -1.f;
	Pump.ReloadProgress = -1.f;
	TestEqual(TEXT("...back to the cup after"), FirstPersonHandsGrips::NextPumpShare(Pump, 1.f, 1.f), 0.f);
	return true;
}

#endif
