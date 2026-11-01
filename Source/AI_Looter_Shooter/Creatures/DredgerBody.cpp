#include "Creatures/DredgerBody.h"
#include "Animation/BoneReference.h"
#include "BoneContainer.h"
#include "BonePose.h"

// A Dredger's code layer over its animation (DredgerBody.h): what the mannequin's clips don't have. Every turn is about a
// bone's own pivot in component space, in the body's frame (its facing, its right, up), and its children follow.

namespace
{
	/** The way names of the mannequin's directional clips, way 0 (ahead) round clockwise. */
	const TCHAR* const WayNames[DredgerBody::Ways] = { TEXT("Fwd"), TEXT("Fwd_Right"), TEXT("Right"), TEXT("Bwd_Right"), TEXT("Bwd"),
		TEXT("Bwd_Left"), TEXT("Left"), TEXT("Fwd_Left") };

	/** Standing still below this (cm/s), fully moving above MovingFrom. */
	constexpr float StillBelow = 10.f;
	constexpr float MovingFrom = 90.f;

	/** The flinch spread down the spine (lower, middle, chest) and how much of a lean the neck gives back so the head stays up. */
	const float FlinchShare[] = { 0.25f, 0.35f, 0.4f };
	constexpr float NeckKeep = 0.6f;
	/** A stagger's pose at its fullest (degrees): rocked back, the head thrown down and to a side. */
	constexpr float StaggerBack = 16.f;
	constexpr float StaggerHead = 14.f;
	/** A wound stage's hunch (degrees forward a stage). */
	constexpr float WoundHunch = 5.f;
	/** A peek's lean (degrees at full), the brace's lean back (degrees) and the breath's rise (degrees on the chest). */
	constexpr float PeekLean = 14.f;
	constexpr float BraceBack = 12.f;
	constexpr float BreathDegrees = 1.1f;
	constexpr float BreathSeconds = 3.6f;
	/** The notice beat: straightened up (degrees back on the chest) with the head raised. */
	constexpr float NoticeStraighten = 4.f;
	/** The code fall's turn at its end (degrees about the feet) and how far it drops (cm). */
	constexpr float FallDegrees = 84.f;
	constexpr float FallDrop = 8.f;

	FCompactPoseBoneIndex FindDredgerBone(const FBoneContainer& Bones, const TCHAR* Name)
	{
		FBoneReference Reference(Name);
		Reference.Initialize(Bones);
		return Reference.GetCompactPoseIndex(Bones);
	}

	/** Turns a bone about its own pivot in component space (none for a bone this LOD lacks); its children follow. */
	void TurnBone(FCSPose<FCompactPose>& Pose, FCompactPoseBoneIndex Bone, const FQuat& Turn)
	{
		if (!Bone.IsValid())
		{
			return;
		}
		FTransform Transform = Pose.GetComponentSpaceTransform(Bone);
		Transform.SetRotation(Turn * Transform.GetRotation());
		const FBoneTransform Single[] = { FBoneTransform(Bone, Transform) };
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Single), 1.f);
	}

	/** A turn in the body's frame: Forward degrees tipping it ahead, Right leaning it to its right, Twist turning it right. */
	FQuat BodyTurn(const FVector& Forward, const FVector& Right, float ForwardDegrees, float RightDegrees, float TwistDegrees)
	{
		// About its right a positive turn tips it ahead; about its facing a positive one leans it to its left, so the right's
		// lean goes negative; about up a positive one turns it to its right (Docs: the stance layer's same frame).
		const FQuat Tip(Right, FMath::DegreesToRadians(ForwardDegrees));
		const FQuat Lean(Forward, FMath::DegreesToRadians(-RightDegrees));
		const FQuat Twist(FVector::UpVector, FMath::DegreesToRadians(TwistDegrees));
		return Twist * Lean * Tip;
	}
}

FString DredgerBody::WalkClip(int32 Index)
{
	const TCHAR* Way = WayNames[((Index % Ways) + Ways) % Ways];
	return FString::Printf(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_%s.MF_Unarmed_Walk_%s"), Way, Way);
}

FString DredgerBody::JogClip(int32 Index)
{
	const TCHAR* Way = WayNames[((Index % Ways) + Ways) % Ways];
	return FString::Printf(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Jog/MF_Unarmed_Jog_%s.MF_Unarmed_Jog_%s"), Way, Way);
}

void DredgerBody::WayBlend(float Angle, int32& OutA, int32& OutB, float& OutBShare)
{
	const float Step = 360.f / static_cast<float>(Ways);
	float Around = FMath::Fmod(Angle, 360.f);
	if (Around < 0.f)
	{
		Around += 360.f;
	}
	const float Place = Around / Step;
	OutA = FMath::FloorToInt(Place) % Ways;
	OutB = (OutA + 1) % Ways;
	OutBShare = FMath::Clamp(Place - FMath::FloorToFloat(Place), 0.f, 1.f);
}

void DredgerBody::GaitShares(float Speed, float WalkClipSpeed, float JogClipSpeed, float& OutIdle, float& OutWalk, float& OutJog)
{
	const float Moving = FMath::SmoothStep(StillBelow, MovingFrom, Speed);
	const float Jog = FMath::Clamp((Speed - WalkClipSpeed) / FMath::Max(JogClipSpeed - WalkClipSpeed, 1.f), 0.f, 1.f);
	OutIdle = 1.f - Moving;
	OutWalk = Moving * (1.f - Jog);
	OutJog = Moving * Jog;
}

float DredgerBody::CycleRate(float Speed, float WalkLength, float JogLength, float JogShare, float WalkClipSpeed, float JogClipSpeed)
{
	// One cycle covers its clip's speed times its length: played at the speed moved, the feet keep their ground.
	const float WalkRate = Speed / FMath::Max(WalkClipSpeed * FMath::Max(WalkLength, 0.1f), 1.f);
	const float JogRate = Speed / FMath::Max(JogClipSpeed * FMath::Max(JogLength, 0.1f), 1.f);
	return FMath::Lerp(WalkRate, JogRate, FMath::Clamp(JogShare, 0.f, 1.f));
}

void DredgerBody::ApplyCodeLayer(FCompactPose& Output, const FDredgerBodyInput& In)
{
	const FBoneContainer& Bones = Output.GetBoneContainer();
	const FCompactPoseBoneIndex Root = FCompactPoseBoneIndex(0);
	const FCompactPoseBoneIndex Spine[] = { FindDredgerBone(Bones, TEXT("spine_01")), FindDredgerBone(Bones, TEXT("spine_03")),
		FindDredgerBone(Bones, TEXT("spine_05")) };
	const FCompactPoseBoneIndex Neck = FindDredgerBone(Bones, TEXT("neck_01"));
	const FCompactPoseBoneIndex Head = FindDredgerBone(Bones, TEXT("head"));
	if (!Spine[0].IsValid() || !Head.IsValid())
	{
		return;
	}

	FCSPose<FCompactPose> Pose;
	Pose.InitPose(Output);
	const FVector Forward = In.Stance.Forward.IsNearlyZero() ? FVector::YAxisVector : In.Stance.Forward;
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();

	// The body's lean as one number per axis: a flinch, a stagger rocked back, a wound's hunch, the brace leaning back on the
	// line with its tug, the notice's straightening, and the breath; a peek leans it out sideways.
	const float Breath = FMath::Sin(In.BreathTime * (2.f * PI / BreathSeconds)) * BreathDegrees;
	const float Ahead = In.Flinch.X - StaggerBack * In.Stagger + WoundHunch * In.Wound - BraceBack * In.Brace
		- NoticeStraighten * In.Notice - Breath + In.Tug * 0.6f;
	const float ToRight = In.Flinch.Y + PeekLean * In.Lean + In.Tug * 0.4f;
	const float Twist = In.Flinch.Z;
	if (!FMath::IsNearlyZero(Ahead) || !FMath::IsNearlyZero(ToRight) || !FMath::IsNearlyZero(Twist))
	{
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Spine); ++Index)
		{
			const float Share = FlinchShare[Index];
			TurnBone(Pose, Spine[Index], BodyTurn(Forward, Right, Ahead * Share, ToRight * Share, Twist * Share));
		}
		// The neck takes most of the lean back, so the eyes stay on the target (a flinch's whip is the head's own, below).
		TurnBone(Pose, Neck, BodyTurn(Forward, Right, -Ahead * NeckKeep, -ToRight * NeckKeep * 0.5f, 0.f));
	}

	// The head: a hit's whip, a stagger's throw down and aside, raised as it notices.
	const float HeadDown = In.HeadWhip.X + StaggerHead * In.Stagger - 6.f * In.Notice;
	const float HeadSide = In.HeadWhip.Y + StaggerHead * 0.4f * In.Stagger;
	if (!FMath::IsNearlyZero(HeadDown) || !FMath::IsNearlyZero(HeadSide))
	{
		TurnBone(Pose, Head, BodyTurn(Forward, Right, HeadDown, HeadSide, 0.f));
	}

	// The code fall (no ragdoll): the whole body tips over about its feet and settles on the ground.
	if (In.Fall > UE_KINDA_SMALL_NUMBER)
	{
		const float Eased = FMath::InterpEaseIn(0.f, 1.f, FMath::Clamp(In.Fall, 0.f, 1.f), 2.2f);
		FTransform RootTransform = Pose.GetComponentSpaceTransform(Root);
		const FQuat Tip(Right, FMath::DegreesToRadians(-FallDegrees * Eased * In.FallWay));
		RootTransform.SetRotation(Tip * RootTransform.GetRotation());
		RootTransform.SetLocation(Tip.RotateVector(RootTransform.GetLocation()) - FVector::UpVector * (FallDrop * Eased));
		const FBoneTransform Single[] = { FBoneTransform(Root, RootTransform) };
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Single), 1.f);
	}

	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(Pose), Output);
}
