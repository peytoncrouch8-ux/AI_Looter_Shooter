// WardenCrewPose: the story people's layer over the mannequin's idle (the crew at their stations, Delia with her shotgun).

#include "Story/WardenCrewPose.h"
#include "Animation/BoneReference.h"
#include "BoneContainer.h"
#include "Math/RotationMatrix.h"
#include "TwoBoneIK.h"

namespace
{
	// The mannequin faces its +Y; its right is its -X (the actor's +X is its front once the mesh is turned -90 degrees, as
	// every character in the game turns it).
	const FVector CrewForward(0.0, 1.0, 0.0);
	const FVector CrewRight(-1.0, 0.0, 0.0);
	const FVector CrewUp(0.0, 0.0, 1.0);

	/** The spine's five bones and each one's share of a lean, a sway and a twist (they add up to the whole). */
	const TCHAR* const SpineBones[] = { TEXT("spine_01"), TEXT("spine_02"), TEXT("spine_03"), TEXT("spine_04"), TEXT("spine_05") };
	constexpr float SpineShares[] = { 0.14f, 0.18f, 0.22f, 0.24f, 0.22f };
	/** A kick or a flinch rocks the upper chest only. */
	constexpr float ChestBackShares[] = { 0.f, 0.f, 0.2f, 0.4f, 0.4f };
	/** The neck and head and each one's share of a look. */
	const TCHAR* const LookBones[] = { TEXT("neck_01"), TEXT("neck_02"), TEXT("head") };
	constexpr float LookShares[] = { 0.3f, 0.25f, 0.45f };
	/** Of a turn on planted feet, the hips take this much and the spine the rest. */
	constexpr float HipShareOfTurn = 0.55f;
	/** Of a sway, the hips take this much. */
	constexpr float HipShareOfRoll = 0.4f;

	/** Where a shouldered gun's grip sits from the shoulder joint (cm): a hand ahead along the aim, in toward the chest, a little low. */
	constexpr float ShoulderedAhead = 17.f;
	constexpr float ShoulderedIn = 6.f;
	constexpr float ShoulderedDown = 5.f;
	/** A gun carried at rest: the grip at the right hip's front. */
	constexpr float CarriedAhead = 14.f;
	constexpr float CarriedDown = 44.f;
	constexpr float CarriedIn = 4.f;
	/** Broken open, the barrels drop this far (degrees) and the gun comes in to the belly. */
	constexpr float BreakDegrees = 50.f;
	/** A shot's kick at its height: the muzzle up (degrees) and the gun back (cm). */
	constexpr float KickDegrees = 12.f;
	constexpr float KickBack = 5.f;

	FCompactPoseBoneIndex CrewBone(const FBoneContainer& Bones, const TCHAR* Name)
	{
		FBoneReference Reference(Name);
		Reference.Initialize(Bones);
		return Reference.GetCompactPoseIndex(Bones);
	}

	void PlaceCrewBone(FCSPose<FCompactPose>& Pose, FCompactPoseBoneIndex Bone, const FTransform& Transform)
	{
		const FBoneTransform Single[] = { FBoneTransform(Bone, Transform) };
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Single), 1.f);
	}

	/** Turns a bone about its own head in component space; its children follow. */
	void TurnCrewBone(FCSPose<FCompactPose>& Pose, FCompactPoseBoneIndex Bone, const FQuat& Turn)
	{
		if (!Bone.IsValid())
		{
			return;
		}
		FTransform Transform = Pose.GetComponentSpaceTransform(Bone);
		Transform.SetRotation((Turn * Transform.GetRotation()).GetNormalized());
		PlaceCrewBone(Pose, Bone, Transform);
	}

	FQuat Degrees(const FVector& Axis, float Angle)
	{
		return FQuat(Axis, FMath::DegreesToRadians(Angle));
	}

	/** A body's turn about its up, lean forward about its right and sway to its right about its front. */
	FQuat BodyTurn(float Yaw, float Lean, float Roll)
	{
		return Degrees(CrewUp, Yaw) * Degrees(CrewRight, Lean) * Degrees(CrewForward, -Roll);
	}

	/** One limb's three bones (upper, lower, end). */
	struct FCrewLimb
	{
		FCompactPoseBoneIndex Upper, Lower, End;
		bool IsValid() const { return Upper.IsValid() && Lower.IsValid() && End.IsValid(); }
	};

	FCrewLimb FindLimb(const FBoneContainer& Bones, const TCHAR* Upper, const TCHAR* Lower, const TCHAR* End)
	{
		return FCrewLimb{ CrewBone(Bones, Upper), CrewBone(Bones, Lower), CrewBone(Bones, End) };
	}

	/**
	 * Two-bone IK on a limb, blended in by Weight: the end toward Target with its joint bent toward Pole (from where it is:
	 * a small weight leaves the limb nearly as the idle has it). EndTurn (component space) turns the end; without one it
	 * turns with the lower bone, so a wrist never wrenches.
	 */
	void ReachLimb(FCSPose<FCompactPose>& Pose, const FCrewLimb& Limb, const FVector& Target, const FVector& Pole, float Weight,
		const FQuat* EndTurn)
	{
		if (!Limb.IsValid() || Weight <= UE_KINDA_SMALL_NUMBER)
		{
			return;
		}
		const float Share = FMath::Min(Weight, 1.f);
		FTransform Upper = Pose.GetComponentSpaceTransform(Limb.Upper);
		FTransform Lower = Pose.GetComponentSpaceTransform(Limb.Lower);
		FTransform End = Pose.GetComponentSpaceTransform(Limb.End);
		const FQuat LowerBefore = Lower.GetRotation();
		const FQuat EndBefore = End.GetRotation();
		const FVector Effector = FMath::Lerp(End.GetLocation(), Target, Share);
		const FVector Bend = FMath::Lerp(Lower.GetLocation(), Pole, Share);
		AnimationCore::SolveTwoBoneIK(Upper, Lower, End, Bend, Effector, /*bAllowStretching*/ false, /*StartStretchRatio*/ 1.0,
			/*MaxStretchScale*/ 1.0);
		const FQuat Followed = (Lower.GetRotation() * LowerBefore.Inverse() * EndBefore).GetNormalized();
		End.SetRotation(EndTurn ? FQuat::Slerp(Followed, *EndTurn, Share).GetNormalized() : Followed);
		const FBoneTransform Chain[] = { FBoneTransform(Limb.Upper, Upper), FBoneTransform(Limb.Lower, Lower), FBoneTransform(Limb.End, End) };
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Chain), 1.f);
	}

	/** The way Facing points, as a yaw (toward the right) and a pitch (up), degrees, in the body's frame. */
	void YawPitchOf(const FVector& Facing, float& OutYaw, float& OutPitch)
	{
		const FVector Way = Facing.GetSafeNormal();
		OutYaw = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Way | CrewRight), static_cast<float>(Way | CrewForward)));
		OutPitch = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(static_cast<float>(Way | CrewUp), -1.f, 1.f)));
	}

	FVector WayOf(float Yaw, float Pitch)
	{
		const float Y = FMath::DegreesToRadians(Yaw);
		const float P = FMath::DegreesToRadians(Pitch);
		return CrewForward * (FMath::Cos(P) * FMath::Cos(Y)) + CrewRight * (FMath::Cos(P) * FMath::Sin(Y)) + CrewUp * FMath::Sin(P);
	}
}

FTransform WardenCrewPose::GunFrame(const FWardenGunHold& Gun, const FVector& Shoulder, const FVector& AimAtLocal, const FVector& Forward,
	const FVector& Right, const FVector& Up)
{
	// Shouldered: the butt in the shoulder's pocket, the grip a hand ahead of it, the barrels on the aim.
	FVector AimWay = (AimAtLocal - Shoulder).GetSafeNormal();
	if (AimWay.IsNearlyZero())
	{
		AimWay = Forward;
	}
	const FQuat Aimed = FRotationMatrix::MakeFromXZ(AimWay, Up).ToQuat();
	const FVector AimedGrip = Shoulder + AimWay * ShoulderedAhead - Right * ShoulderedIn - Up * ShoulderedDown;
	// At rest: low across the body, the grip at the right hip, the muzzle down and away to the left (never at the player).
	const FVector CarryWay = (Forward * 0.8 - Right * 0.45 - Up * 0.4).GetSafeNormal();
	const FQuat Carried = FRotationMatrix::MakeFromXZ(CarryWay, Up).ToQuat();
	const FVector CarriedGrip = Shoulder + Forward * CarriedAhead - Up * CarriedDown - Right * CarriedIn;

	const float Raise = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Gun.Shoulder, 0.f, 1.f));
	FQuat Turn = FQuat::Slerp(Carried, Aimed, Raise).GetNormalized();
	FVector Grip = FMath::Lerp(CarriedGrip, AimedGrip, Raise);

	// Broken open: the barrels drop (about the gun's own right) and it comes in to the belly for the shells.
	const float Broken = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Gun.Break, 0.f, 1.f));
	if (Broken > 0.f)
	{
		Turn = (Degrees(Turn.GetAxisY(), BreakDegrees * Broken) * Turn).GetNormalized();
		Grip += Forward * (4.f * Broken) - Up * (8.f * Broken);
	}
	// A shot's kick: the muzzle jumps and the gun drives back into the shoulder.
	const float Kick = FMath::Clamp(Gun.Kick, 0.f, 1.f);
	if (Kick > 0.f)
	{
		Turn = (Degrees(Turn.GetAxisY(), -KickDegrees * Kick) * Turn).GetNormalized();
		Grip -= Turn.GetAxisX() * (KickBack * Kick);
	}
	return FTransform(Turn, Grip);
}

void WardenCrewPose::Apply(FCompactPose& LocalPose, const FWardenPoseInput& Input, const FTransform& ComponentToWorld)
{
	const FBoneContainer& Bones = LocalPose.GetBoneContainer();
	const FCompactPoseBoneIndex Pelvis = CrewBone(Bones, TEXT("pelvis"));
	if (!Pelvis.IsValid())
	{
		return;
	}
	auto ToLocal = [&ComponentToWorld](const FVector& World) { return ComponentToWorld.InverseTransformPosition(World); };

	FCSPose<FCompactPose> Pose;
	Pose.InitPose(LocalPose);

	// The legs as the idle has them: the feet stay planted wherever the hips go.
	const FCrewLimb Legs[] = { FindLimb(Bones, TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l")),
		FindLimb(Bones, TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r")) };
	FTransform Feet[2];
	for (int32 Side = 0; Side < 2; ++Side)
	{
		Feet[Side] = Legs[Side].IsValid() ? Pose.GetComponentSpaceTransform(Legs[Side].End) : FTransform::Identity;
	}

	// The hips: lowered for a brace, turned for a turn and swayed with the deck, the spine taking the rest.
	const float Yaw = FMath::Clamp(Input.BodyYaw, -MaxBodyYaw, MaxBodyYaw);
	const float HipYaw = Yaw * HipShareOfTurn;
	{
		FTransform Hips = Pose.GetComponentSpaceTransform(Pelvis);
		Hips.SetRotation((BodyTurn(HipYaw, 0.f, Input.Roll * HipShareOfRoll) * Hips.GetRotation()).GetNormalized());
		Hips.AddToTranslation(-CrewUp * FMath::Max(Input.Crouch, 0.f));
		PlaceCrewBone(Pose, Pelvis, Hips);
	}
	const float SpineYaw = Yaw - HipYaw + Input.ChestYaw;
	const float SpineRoll = Input.Roll * (1.f - HipShareOfRoll);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(SpineBones); ++Index)
	{
		const float Share = SpineShares[Index];
		const float Back = Input.ChestBack * ChestBackShares[Index];
		TurnCrewBone(Pose, CrewBone(Bones, SpineBones[Index]), BodyTurn(SpineYaw * Share, Input.Lean * Share - Back, SpineRoll * Share));
	}

	// The legs bent to keep the feet where they were, the knees over the toes the hips now face.
	const FVector HipForward = Degrees(CrewUp, HipYaw).RotateVector(CrewForward);
	for (int32 Side = 0; Side < 2; ++Side)
	{
		if (Legs[Side].IsValid())
		{
			const FVector Knee = Pose.GetComponentSpaceTransform(Legs[Side].Lower).GetLocation() + HipForward * 40.f;
			const FQuat FootTurn = Feet[Side].GetRotation();
			ReachLimb(Pose, Legs[Side], Feet[Side].GetLocation(), Knee, 1.f, &FootTurn);
		}
	}

	// The head to what it looks at, from where the chest now faces, within a neck's reach.
	const FCompactPoseBoneIndex Head = CrewBone(Bones, TEXT("head"));
	if (Input.LookWeight > UE_KINDA_SMALL_NUMBER && Head.IsValid())
	{
		const FVector HeadAt = Pose.GetComponentSpaceTransform(Head).GetLocation();
		const FVector ChestFacing = BodyTurn(Yaw + Input.ChestYaw, Input.Lean, 0.f).RotateVector(CrewForward);
		float FacingYaw = 0.f;
		float FacingPitch = 0.f;
		YawPitchOf(ChestFacing, FacingYaw, FacingPitch);
		float WantYaw = 0.f;
		float WantPitch = 0.f;
		YawPitchOf(ToLocal(Input.LookAt) - HeadAt, WantYaw, WantPitch);
		const float Weight = FMath::Min(Input.LookWeight, 1.f);
		const float TurnYaw = FMath::Clamp(FRotator::NormalizeAxis(WantYaw - FacingYaw), -MaxLookYaw, MaxLookYaw) * Weight;
		const float TurnPitch = FMath::Clamp(WantPitch - FacingPitch, -MaxLookDown, MaxLookUp) * Weight;
		const FQuat Look = FQuat::FindBetweenNormals(WayOf(FacingYaw, FacingPitch), WayOf(FacingYaw + TurnYaw, FacingPitch + TurnPitch));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(LookBones); ++Index)
		{
			TurnCrewBone(Pose, CrewBone(Bones, LookBones[Index]), FQuat::Slerp(FQuat::Identity, Look, LookShares[Index]).GetNormalized());
		}
	}

	// The hands: on a gun, or each on its own place in the world.
	const FCrewLimb Arms[] = { FindLimb(Bones, TEXT("upperarm_l"), TEXT("lowerarm_l"), TEXT("hand_l")),
		FindLimb(Bones, TEXT("upperarm_r"), TEXT("lowerarm_r"), TEXT("hand_r")) };
	const float GunWeight = FMath::Clamp(Input.Gun.Weight, 0.f, 1.f);
	if (GunWeight > UE_KINDA_SMALL_NUMBER && Arms[0].IsValid() && Arms[1].IsValid())
	{
		const FVector Shoulder = Pose.GetComponentSpaceTransform(Arms[1].Upper).GetLocation();
		const FVector LeftShoulder = Pose.GetComponentSpaceTransform(Arms[0].Upper).GetLocation();
		const FTransform Grip = GunFrame(Input.Gun, Shoulder, ToLocal(Input.Gun.AimAt), CrewForward, CrewRight, CrewUp);
		// The right hand where the gun's grip in its socket lands on the frame (the socket = the hand's frame times the socket's own).
		const FTransform Hand = Input.Gun.SocketInHand.Inverse() * Grip;
		const FQuat HandTurn = Hand.GetRotation();
		ReachLimb(Pose, Arms[1], Hand.GetLocation(), Shoulder + CrewRight * 30.f - CrewUp * 25.f, GunWeight, &HandTurn);
		// The left hand under the forend, or off to the apron pocket for shells.
		const FVector Forend = Grip.TransformPosition(Input.Gun.Forend) - Grip.GetRotation().GetAxisX() * 7.f - Grip.GetRotation().GetAxisZ() * 3.f;
		const FVector Pocket = Pose.GetComponentSpaceTransform(Pelvis).GetLocation() + CrewForward * 10.f - CrewRight * 20.f + CrewUp * 2.f;
		const FVector LeftWrist = FMath::Lerp(Forend, Pocket, FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Input.Gun.LeftToPocket, 0.f, 1.f)));
		ReachLimb(Pose, Arms[0], LeftWrist, LeftShoulder - CrewUp * 30.f - CrewRight * 20.f, GunWeight, nullptr);
	}
	else
	{
		for (int32 Side = 0; Side < 2; ++Side)
		{
			const FWardenHand& Hand = Input.Hands[Side];
			ReachLimb(Pose, Arms[Side], ToLocal(Hand.Wrist), ToLocal(Hand.Elbow), Hand.Weight, nullptr);
		}
	}

	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Pose), LocalPose);
}
