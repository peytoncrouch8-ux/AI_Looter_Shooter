#include "Player/Animation/LooterStanceInput.h"
#include "Player/Animation/LooterStancePose.h"
#include "Player/Animation/LooterStancePoseDetail.h"
#include "AI_Looter_Shooter.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "HAL/IConsoleManager.h"
#include "Math/RotationMatrix.h"
#include "ReferenceSkeleton.h"

using namespace LooterStancePoseDetail;

// The left hand's grip on the gun in third person (2026-10-20: every character's left hand rested open, palm up, under the
// foregrip, the rifle animation's hand): the hand turned onto the gun by the hold kind, and its fingers and thumb closed
// round what it holds. The hidden mannequin's hand is posed; Ellis's and every hero's copy it, fingers included, through
// their retargeters. The hold tables are the tuning knobs (the gun's own space: X to the muzzle, Y right, Z up), started
// from the first-person hands' on the same guns (FirstPersonHandsGrips.cpp) and the gun models; the Looter.Grip.* console
// variables move them live in a development build (Looter.Grip.Dump prints what to bake).

namespace
{
	constexpr int32 FingerCount = 5;
	constexpr int32 JointCount = 3;
	constexpr int32 ThumbFinger = 0;
	/** The fingers' end joints rest bent this share of their middle joints (the skeleton has no fingertip bones to measure it by). */
	constexpr float EndJointRestShare = 0.6f;

	/**
	 * One hold kind's left hand on the gun. Its index-and-thumb way along what it holds (Handle) and the way its palm faces
	 * (Palm, made square to Handle) set the hand, its knuckles square to both; a last turn in its own frame (roll about the
	 * knuckles' line, pitch about the palm's facing, yaw about the handle); the palm's spot moved off the foregrip (cm of the
	 * modeled gun). The curls are degrees from straight, each joint from the knuckle out: every finger alike, each a little
	 * more bent than the one before it toward the pinky (Spread, at the first two joints, as a real fist); the thumb's first
	 * number swings it in across the palm, ThumbSpread swings it out from the fingers in the palm's plane.
	 */
	struct FStanceGrip
	{
		FVector Handle = FVector::ForwardVector;
		FVector Palm = FVector::UpVector;
		FRotator Turn = FRotator::ZeroRotator;
		FVector Shift = FVector::ZeroVector;
		float Fingers[JointCount] = { 0.f, 0.f, 0.f };
		float Spread = 0.f;
		float Thumb[JointCount] = { 0.f, 0.f, 0.f };
		float ThumbSpread = 0.f;
	};

	/**
	 * Under a long gun's handguard (the Bullpup; any gun not a shotgun or a six-gun): the palm up under it, the index and
	 * thumb along it slanting ahead and a little in (the first-person support hand's way), so the knuckles point across
	 * under it to its far (right) side and the fingers close up round that side; the thumb along its near side. The
	 * Bullpup's SOCKET_Foregrip is 1 cm under the shell's flat bottom (5 cm wide, 6.3 tall there): the fingers close well
	 * round to come up its side.
	 */
	FStanceGrip MakeHandguard()
	{
		FStanceGrip Grip;
		Grip.Handle = FVector(0.92f, -0.4f, 0.f);
		Grip.Palm = FVector(0.f, 0.1f, 1.f);
		// Tuned in play (2026-10-20, every hero): at 55 and 55 the fingers stood off the shell's side; these lay them on it.
		Grip.Fingers[0] = 67.f;
		Grip.Fingers[1] = 63.f;
		Grip.Fingers[2] = 30.f;
		Grip.Spread = 3.5f;
		Grip.Thumb[0] = 15.f;
		Grip.Thumb[1] = 10.f;
		Grip.Thumb[2] = 5.f;
		return Grip;
	}

	/**
	 * Round a shotgun's pump (the Ranchhand's: 4.4 cm wide, 3 tall, SOCKET_Foregrip at its middle): cupped from the lower
	 * left, the palm facing up and in, the fingers closed tight round its bottom and up its far side, the thumb laid along
	 * its near side (the first-person pump grip's rule: curled in, a thumb reads as a nub).
	 */
	FStanceGrip MakePump()
	{
		FStanceGrip Grip;
		Grip.Handle = FVector(0.94f, -0.34f, 0.f);
		Grip.Palm = FVector(0.f, 0.5f, 0.87f);
		Grip.Fingers[0] = 55.f;
		Grip.Fingers[1] = 70.f;
		Grip.Fingers[2] = 45.f;
		Grip.Spread = 3.5f;
		Grip.Thumb[0] = 15.f;
		Grip.Thumb[1] = 10.f;
		Grip.Thumb[2] = 0.f;
		return Grip;
	}

	/**
	 * A six-gun's support hand (the Drover's: its palm out past the shooting hand's fingers on the grip's left and a little
	 * under, LooterStancePoseWeapon.cpp): cupping the shooting hand from under its left, the palm facing it and a little up,
	 * the thumb up and ahead beside the frame, the fingers ahead and closed round the front of the shooting hand's.
	 */
	FStanceGrip MakeSixGun()
	{
		FStanceGrip Grip;
		Grip.Handle = FVector(0.7f, 0.f, 0.7f);
		Grip.Palm = FVector(-0.1f, 0.8f, 0.6f);
		Grip.Fingers[0] = 45.f;
		Grip.Fingers[1] = 60.f;
		Grip.Fingers[2] = 40.f;
		Grip.Spread = 2.f;
		Grip.Thumb[0] = 18.f;
		Grip.Thumb[1] = 5.f;
		Grip.Thumb[2] = 5.f;
		return Grip;
	}

	const FStanceGrip& GripFor(ELooterGrip Kind)
	{
		static const FStanceGrip Handguard = MakeHandguard();
		static const FStanceGrip Pump = MakePump();
		static const FStanceGrip SixGun = MakeSixGun();
		switch (Kind)
		{
		case ELooterGrip::Pump:
			return Pump;
		case ELooterGrip::SixGun:
			return SixGun;
		default:
			return Handguard;
		}
	}

#if !UE_BUILD_SHIPPING
	// Live tuning in a development build (the visual pass: Tools\heroguns.ps1 between nudges); every number adds to the
	// table's. Read on the animation thread.
	TAutoConsoleVariable<int32> CVarGripOn(TEXT("Looter.Grip.On"), 1,
		TEXT("Third person: 1 the left hand grips the gun (turned onto it, fingers closed); 0 the animation's open hand, to compare."));
	TAutoConsoleVariable<int32> CVarGripKind(TEXT("Looter.Grip.Kind"), 0,
		TEXT("Which hold the Looter.Grip.* tuning moves: 0 every one, 1 a handguard, 2 a pump, 3 a six-gun."));
	TAutoConsoleVariable<float> CVarGripRoll(TEXT("Looter.Grip.PalmRoll"), 0.f, TEXT("Left hand on the gun: roll about its knuckles' line (degrees)."));
	TAutoConsoleVariable<float> CVarGripPitch(TEXT("Looter.Grip.PalmPitch"), 0.f, TEXT("Left hand on the gun: turn about its palm's facing (degrees)."));
	TAutoConsoleVariable<float> CVarGripYaw(TEXT("Looter.Grip.PalmYaw"), 0.f, TEXT("Left hand on the gun: roll about the handle (degrees)."));
	TAutoConsoleVariable<float> CVarGripAhead(TEXT("Looter.Grip.Ahead"), 0.f, TEXT("Left palm's spot ahead along the gun (cm; the tests expect 0)."));
	TAutoConsoleVariable<float> CVarGripRight(TEXT("Looter.Grip.Right"), 0.f, TEXT("Left palm's spot to the gun's right (cm; the tests expect 0)."));
	TAutoConsoleVariable<float> CVarGripUp(TEXT("Looter.Grip.Up"), 0.f, TEXT("Left palm's spot up the gun (cm; the tests expect 0)."));
	TAutoConsoleVariable<float> CVarGripKnuckle(TEXT("Looter.Grip.FingerKnuckle"), 0.f, TEXT("Left fingers' first joints closed this much more (degrees)."));
	TAutoConsoleVariable<float> CVarGripMiddle(TEXT("Looter.Grip.FingerMiddle"), 0.f, TEXT("Left fingers' middle joints closed this much more (degrees)."));
	TAutoConsoleVariable<float> CVarGripTip(TEXT("Looter.Grip.FingerTip"), 0.f, TEXT("Left fingers' end joints closed this much more (degrees)."));
	TAutoConsoleVariable<float> CVarGripThumbSwing(TEXT("Looter.Grip.ThumbSwing"), 0.f, TEXT("Left thumb swung in across the palm this much more (degrees)."));
	TAutoConsoleVariable<float> CVarGripThumbCurl(TEXT("Looter.Grip.ThumbCurl"), 0.f, TEXT("Left thumb's two outer joints closed this much more (degrees)."));
	TAutoConsoleVariable<float> CVarGripThumbSpread(TEXT("Looter.Grip.ThumbSpread"), 0.f, TEXT("Left thumb swung out from the fingers in the palm's plane (degrees)."));
#endif

	/** The hold for a kind as the hands use it: the table's, with (development builds) the Looter.Grip.* tuning on it. */
	FStanceGrip TunedGrip(ELooterGrip Kind)
	{
		FStanceGrip Grip = GripFor(Kind);
#if !UE_BUILD_SHIPPING
		const int32 Wanted = CVarGripKind.GetValueOnAnyThread();
		if (Wanted != 0 && Wanted != static_cast<int32>(Kind) + 1)
		{
			return Grip;
		}
		Grip.Turn += FRotator(CVarGripPitch.GetValueOnAnyThread(), CVarGripYaw.GetValueOnAnyThread(), CVarGripRoll.GetValueOnAnyThread());
		Grip.Shift += FVector(CVarGripAhead.GetValueOnAnyThread(), CVarGripRight.GetValueOnAnyThread(), CVarGripUp.GetValueOnAnyThread());
		Grip.Fingers[0] += CVarGripKnuckle.GetValueOnAnyThread();
		Grip.Fingers[1] += CVarGripMiddle.GetValueOnAnyThread();
		Grip.Fingers[2] += CVarGripTip.GetValueOnAnyThread();
		Grip.Thumb[0] += CVarGripThumbSwing.GetValueOnAnyThread();
		Grip.Thumb[1] += CVarGripThumbCurl.GetValueOnAnyThread();
		Grip.Thumb[2] += CVarGripThumbCurl.GetValueOnAnyThread();
		Grip.ThumbSpread += CVarGripThumbSpread.GetValueOnAnyThread();
#endif
		return Grip;
	}

	/** A joint's curl in a hold: the thumb's own, or the fingers' with the fist's spread toward the pinky. */
	float CurlOf(const FStanceGrip& Grip, int32 Finger, int32 Joint)
	{
		if (Finger == ThumbFinger)
		{
			return Grip.Thumb[Joint];
		}
		return Grip.Fingers[Joint] + (Joint < 2 ? Grip.Spread * static_cast<float>(Finger - 1) : 0.f);
	}

	/**
	 * The hand's frame on the gun (the gun's own space): X from the wrist to the knuckles, Y out of the palm. A left hand's
	 * knuckles are its palm's way cross its index-and-thumb way (a right hand's the other way round: they're mirror images).
	 */
	FQuat FrameOnGun(const FStanceGrip& Grip)
	{
		const FVector IndexWay = Grip.Handle.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		const FVector Palm = FVector::VectorPlaneProject(Grip.Palm, IndexWay).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		const FVector Knuckles = FVector::CrossProduct(Palm, IndexWay);
		return (FRotationMatrix::MakeFromXY(Knuckles, Palm).ToQuat() * Grip.Turn.Quaternion()).GetNormalized();
	}

	/** The left hand's bone names, made once. */
	struct FLeftHandNames
	{
		FName Hand = TEXT("hand_l");
		FName Joints[FingerCount][JointCount];

		FLeftHandNames()
		{
			const TCHAR* const Fingers[FingerCount] = { TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky") };
			for (int32 Finger = 0; Finger < FingerCount; ++Finger)
			{
				for (int32 Joint = 0; Joint < JointCount; ++Joint)
				{
					Joints[Finger][Joint] = FName(*FString::Printf(TEXT("%s_%02d_l"), Fingers[Finger], Joint + 1));
				}
			}
		}
	};

	const FLeftHandNames& LeftHandNames()
	{
		static const FLeftHandNames Names;
		return Names;
	}

	/**
	 * The left hand read off the reference skeleton (so the same at every LOD, and whatever the animation does with the
	 * fingers): the hand's frame in hand_l's own space (X from the wrist to the middle knuckle, Y out of the palm), and for
	 * each finger joint its curl axis in its own space, how bent it rests (degrees) and its reference turn.
	 */
	struct FLeftHandRig
	{
		bool bValid = false;
		FQuat Frame = FQuat::Identity;
		int32 Index[FingerCount][JointCount] = {};
		FQuat RefTurn[FingerCount][JointCount];
		FVector Axis[FingerCount][JointCount];
		float Rest[FingerCount][JointCount] = {};
		/** The thumb's swing out from the fingers, about the palm's facing (its first joint's own space). */
		FVector ThumbSpreadAxis = FVector::UpVector;
	};

	/** Bone's reference transform in Hand's space; false when Hand isn't above it. */
	bool InHand(const FReferenceSkeleton& Skeleton, int32 Bone, int32 Hand, FTransform& Out)
	{
		const TArray<FTransform>& RefPose = Skeleton.GetRefBonePose();
		Out = FTransform::Identity;
		for (int32 Index = Bone; Index != INDEX_NONE; Index = Skeleton.GetParentIndex(Index))
		{
			if (Index == Hand)
			{
				return true;
			}
			Out = Out * RefPose[Index];
		}
		return false;
	}

	/** The signed angle (degrees) from From to To about Axis. */
	float AngleAbout(const FVector& From, const FVector& To, const FVector& Axis)
	{
		return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(FVector::CrossProduct(From, To), Axis), FVector::DotProduct(From, To))));
	}

	FLeftHandRig ReadLeftHand(const FReferenceSkeleton& Skeleton, bool bFingers)
	{
		FLeftHandRig Rig;
		const FLeftHandNames& Names = LeftHandNames();
		const int32 Hand = Skeleton.FindBoneIndex(Names.Hand);
		if (Hand == INDEX_NONE)
		{
			return Rig;
		}
		FTransform Joint[FingerCount][JointCount];
		bool bHas[FingerCount][JointCount];
		for (int32 Finger = 0; Finger < FingerCount; ++Finger)
		{
			for (int32 Step = 0; Step < JointCount; ++Step)
			{
				Rig.Index[Finger][Step] = Skeleton.FindBoneIndex(Names.Joints[Finger][Step]);
				bHas[Finger][Step] = Rig.Index[Finger][Step] != INDEX_NONE && InHand(Skeleton, Rig.Index[Finger][Step], Hand, Joint[Finger][Step]);
				if (!bHas[Finger][Step])
				{
					Rig.Index[Finger][Step] = INDEX_NONE;
				}
			}
		}

		// The hand's frame: the wrist (the bone's origin) to the middle knuckle, and the palm's facing from the line across
		// the knuckles, pinky to index. A left hand's palm is that line cross the fingers' way (a right hand's the other way).
		constexpr int32 IndexFinger = 1;
		constexpr int32 MiddleFinger = 2;
		constexpr int32 PinkyFinger = 4;
		if (!bHas[MiddleFinger][0] || !bHas[IndexFinger][0] || !bHas[PinkyFinger][0])
		{
			return Rig;
		}
		const FVector Fingers = Joint[MiddleFinger][0].GetLocation().GetSafeNormal();
		const FVector Across = (Joint[IndexFinger][0].GetLocation() - Joint[PinkyFinger][0].GetLocation()).GetSafeNormal();
		const FVector Palm = FVector::VectorPlaneProject(FVector::CrossProduct(Across, Fingers), Fingers).GetSafeNormal();
		if (Fingers.IsNearlyZero() || Palm.IsNearlyZero())
		{
			return Rig;
		}
		Rig.Frame = FRotationMatrix::MakeFromXY(Fingers, Palm).ToQuat();
		Rig.bValid = true;
		if (!bFingers)
		{
			return Rig;
		}

		// Each joint curls about the line square to its own bone and to the palm's facing: turning about it carries the finger
		// toward the palm, whichever way the bone's own axes run. How bent it rests is measured the same way against the bone
		// before it (the finger's metacarpal, or the hand), so a curl asked for is from straight (as the first-person hands').
		const TArray<FTransform>& RefPose = Skeleton.GetRefBonePose();
		const FVector KnuckleLine = FVector::CrossProduct(Fingers, Palm);
		for (int32 Finger = 0; Finger < FingerCount; ++Finger)
		{
			for (int32 Step = 0; Step < JointCount; ++Step)
			{
				if (!bHas[Finger][Step])
				{
					continue;
				}
				const FVector Here = Joint[Finger][Step].GetLocation();
				FVector Along = Fingers;
				if (Step + 1 < JointCount && bHas[Finger][Step + 1])
				{
					Along = Joint[Finger][Step + 1].GetLocation() - Here;
				}
				else if (Step > 0 && bHas[Finger][Step - 1])
				{
					Along = Here - Joint[Finger][Step - 1].GetLocation();
				}
				Along = Along.GetSafeNormal(UE_SMALL_NUMBER, Fingers);
				const FVector Axis = FVector::CrossProduct(Along, Palm).GetSafeNormal(UE_SMALL_NUMBER, KnuckleLine);
				const FQuat Turn = Joint[Finger][Step].GetRotation();
				Rig.Axis[Finger][Step] = Turn.UnrotateVector(Axis).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
				Rig.RefTurn[Finger][Step] = RefPose[Rig.Index[Finger][Step]].GetRotation();

				float Rest = 0.f;
				if (Step == 0 && Finger != ThumbFinger)
				{
					const int32 Parent = Skeleton.GetParentIndex(Rig.Index[Finger][Step]);
					FTransform Before;
					const FVector Previous = Parent != Hand && Parent != INDEX_NONE && InHand(Skeleton, Parent, Hand, Before)
						? (Here - Before.GetLocation()).GetSafeNormal(UE_SMALL_NUMBER, Fingers) : Fingers;
					Rest = AngleAbout(Previous, Along, Axis);
				}
				else if (Step == 1 && bHas[Finger][0])
				{
					Rest = AngleAbout((Here - Joint[Finger][0].GetLocation()).GetSafeNormal(UE_SMALL_NUMBER, Along), Along, Axis);
				}
				Rig.Rest[Finger][Step] = FMath::Clamp(Rest, -30.f, 90.f);
				if (Step == 0 && Finger == ThumbFinger)
				{
					// About the palm's facing, turned the way that carries the knuckles' way toward the index side: out from the
					// fingers. (A left hand's index side is its knuckles cross its palm.)
					Rig.ThumbSpreadAxis = Turn.UnrotateVector(-Palm).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
				}
			}
			// The end joint has no fingertip bone to measure against: a share of the middle joint's bend.
			Rig.Rest[Finger][2] = Rig.Rest[Finger][1] * EndJointRestShare;
		}
		return Rig;
	}

#if !UE_BUILD_SHIPPING
	const TCHAR* GripName(ELooterGrip Kind)
	{
		switch (Kind)
		{
		case ELooterGrip::Pump:
			return TEXT("Pump (MakePump)");
		case ELooterGrip::SixGun:
			return TEXT("Six-gun (MakeSixGun)");
		default:
			return TEXT("Handguard (MakeHandguard)");
		}
	}

	/** Looter.Grip.Dump: every hold with the tuning on it, as lines to paste into the tables. */
	void DumpGrips()
	{
		for (const ELooterGrip Kind : { ELooterGrip::Handguard, ELooterGrip::Pump, ELooterGrip::SixGun })
		{
			const FStanceGrip Grip = TunedGrip(Kind);
			UE_LOG(LogLooter, Display, TEXT("Grip, %s: Handle (%.2f, %.2f, %.2f), Palm (%.2f, %.2f, %.2f), Turn = FRotator(%.1ff, %.1ff, %.1ff); Shift = FVector(%.2ff, %.2ff, %.2ff); Fingers %.1f, %.1f, %.1f (spread %.1f); Thumb %.1f, %.1f, %.1f (spread %.1f)"),
				GripName(Kind), Grip.Handle.X, Grip.Handle.Y, Grip.Handle.Z, Grip.Palm.X, Grip.Palm.Y, Grip.Palm.Z, Grip.Turn.Pitch, Grip.Turn.Yaw, Grip.Turn.Roll,
				Grip.Shift.X, Grip.Shift.Y, Grip.Shift.Z, Grip.Fingers[0], Grip.Fingers[1], Grip.Fingers[2], Grip.Spread, Grip.Thumb[0], Grip.Thumb[1], Grip.Thumb[2],
				Grip.ThumbSpread);
		}
	}

	FAutoConsoleCommand CmdGripDump(TEXT("Looter.Grip.Dump"),
		TEXT("Prints the third-person left hand's holds with the Looter.Grip.* tuning on them, ready to bake into LooterStancePoseGrip.cpp."),
		FConsoleCommandDelegate::CreateStatic(&DumpGrips));
#endif
}

ELooterGrip LooterStancePoseDetail::GripOf(const FLooterStanceInput& Stance)
{
	// A six-gun is the gun held out (its HoldReach), as the support spot on it is (ApplyLeftHandOnForegrip).
	if (Stance.HoldReach > 0.f || Stance.WeaponKind == EWeaponKind::Revolver)
	{
		return ELooterGrip::SixGun;
	}
	return Stance.WeaponKind == EWeaponKind::Shotgun ? ELooterGrip::Pump : ELooterGrip::Handguard;
}

float LooterStancePoseDetail::GripShare(const FLooterStanceInput& Stance)
{
	if (!Stance.bHandOnForegrip)
	{
		return 0.f;
	}
#if !UE_BUILD_SHIPPING
	if (CVarGripOn.GetValueOnAnyThread() == 0)
	{
		return 0.f;
	}
#endif
	// A throw's arm and a climb's hands set out from the hand as the pose has it (the throw turns it with the forearm): the
	// grip eases off as they come in, so they look as they always have.
	const float Throw = FMath::Clamp(LooterStancePose::DescribeThrow(Stance.ThrowTime).Arm, 0.f, 1.f);
	const float Climb = FMath::Clamp(Stance.Climb.Hands, 0.f, 1.f);
	return (1.f - Throw) * (1.f - Climb);
}

FQuat LooterStancePoseDetail::LeftHandOnGun(const FBoneContainer& Bones, const FLooterStanceInput& Stance, const FQuat& Gun, const FQuat& AnimHand)
{
	const float Share = GripShare(Stance);
	if (Share <= UE_KINDA_SMALL_NUMBER)
	{
		return AnimHand;
	}
	const FLeftHandRig Rig = ReadLeftHand(Bones.GetReferenceSkeleton(), /*bFingers*/ false);
	if (!Rig.bValid)
	{
		return AnimHand;
	}
	// The hand's frame where the hold puts it on the gun; the bone turned so its own frame lands there.
	const FQuat Gripping = (Gun * FrameOnGun(TunedGrip(GripOf(Stance))) * Rig.Frame.Inverse()).GetNormalized();
	return FQuat::Slerp(AnimHand, Gripping, FMath::Min(Share, 1.f)).GetNormalized();
}

FVector LooterStancePoseDetail::LeftGripShift(const FLooterStanceInput& Stance)
{
	// The hold's numbers are the modeled gun's; it's drawn bigger than modeled.
	return TunedGrip(GripOf(Stance)).Shift * (Stance.WeaponScale * GripShare(Stance));
}

void LooterStancePoseDetail::ApplyLeftGripFingers(FCompactPose& Pose, const FLooterStanceInput& Stance, float Weight)
{
	const float Share = FMath::Clamp(Weight, 0.f, 1.f);
	if (Share <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}
	const FBoneContainer& Bones = Pose.GetBoneContainer();
	const FLeftHandRig Rig = ReadLeftHand(Bones.GetReferenceSkeleton(), /*bFingers*/ true);
	if (!Rig.bValid)
	{
		return;
	}
	const FStanceGrip Grip = TunedGrip(GripOf(Stance));
	for (int32 Finger = 0; Finger < FingerCount; ++Finger)
	{
		for (int32 Step = 0; Step < JointCount; ++Step)
		{
			if (Rig.Index[Finger][Step] == INDEX_NONE)
			{
				continue;
			}
			// Absent at this LOD: nothing to pose.
			const FCompactPoseBoneIndex Bone = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(Rig.Index[Finger][Step]));
			if (!Bone.IsValid())
			{
				continue;
			}
			// From the reference turn: swung out (the thumb), then curled toward the palm from its rest bend to the hold's, each
			// about the joint's own axis so the bones beyond it follow it round.
			FQuat Held = Rig.RefTurn[Finger][Step];
			if (Finger == ThumbFinger && Step == 0 && !FMath::IsNearlyZero(Grip.ThumbSpread))
			{
				Held = Held * FQuat(Rig.ThumbSpreadAxis, FMath::DegreesToRadians(Grip.ThumbSpread));
			}
			Held = (Held * FQuat(Rig.Axis[Finger][Step], FMath::DegreesToRadians(CurlOf(Grip, Finger, Step) - Rig.Rest[Finger][Step]))).GetNormalized();
			FTransform& Local = Pose[Bone];
			Local.SetRotation(FQuat::Slerp(Local.GetRotation(), Held, Share).GetNormalized());
		}
	}
}
