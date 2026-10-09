#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimSequence.h"
#include "Animation/BoneReference.h"
#include "Animation/Skeleton.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Misc/MemStack.h"
#include "Player/PlayerSize.h"
#include "Player/PlayerTraversal.h"
#include "Player/TraversalRules.h"
#include "Player/ViewEase.h"
#include "Player/Animation/LooterClimbPose.h"

// What the player's animation tests share (PlayerAnimTests.cpp, PlayerAnimPlayTests.cpp): the mannequin's bones as a pose
// to run the stance layer on, and a mantle's and a vault's plans followed frame by frame the way the locomotion component
// follows them.

namespace PlayerAnimTestKit
{
	inline constexpr float FrameTime = 1.f / 60.f;

	/** The mannequin's skeleton, the one every player animation shares. */
	inline USkeleton* MannequinSkeleton()
	{
		const UAnimSequence* Clip = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd"));
		return Clip ? const_cast<USkeleton*>(Clip->GetSkeleton()) : nullptr;
	}

	/**
	 * The mannequin's bones as a pose: the reference pose to start from, and where its bones stand once posed. Poses are
	 * allocated from the animation memory stack, which needs an FMemMark in the test around everything that uses them.
	 */
	struct FPoseRig
	{
		explicit FPoseRig(USkeleton& Skeleton)
			: Required(AllBones(Skeleton))
			, Bones(Required, UE::Anim::FCurveFilterSettings(), Skeleton)
		{
		}

		FCompactPose RefPose() const
		{
			FCompactPose Pose;
			Pose.SetBoneContainer(&Bones);
			Pose.ResetToRefPose();
			return Pose;
		}

		/** A bone's place in component space once Space has been made from a pose; the origin for a bone the skeleton lacks. */
		FVector Where(FCSPose<FCompactPose>& Space, const TCHAR* Name) const
		{
			FBoneReference Bone(Name);
			Bone.Initialize(Bones);
			const FCompactPoseBoneIndex Index = Bone.GetCompactPoseIndex(Bones);
			return Index.IsValid() ? Space.GetComponentSpaceTransform(Index).GetLocation() : FVector::ZeroVector;
		}

		FVector Where(const FCompactPose& Pose, const TCHAR* Name) const
		{
			FCSPose<FCompactPose> Space;
			Space.InitPose(Pose);
			return Where(Space, Name);
		}

		/** No bone has a NaN in it, and every rotation is a unit quaternion. */
		static bool IsSound(const FCompactPose& Pose)
		{
			for (const FCompactPoseBoneIndex Index : Pose.ForEachBoneIndex())
			{
				const FTransform& Transform = Pose[Index];
				if (Transform.ContainsNaN() || !Transform.IsRotationNormalized())
				{
					return false;
				}
			}
			return true;
		}

		/** Every bone of the skeleton, and the container the poses are made against (the order matters: Bones is made from Required). */
		TArray<FBoneIndexType> Required;
		FBoneContainer Bones;

	private:
		static TArray<FBoneIndexType> AllBones(const USkeleton& Skeleton)
		{
			const FReferenceSkeleton& Reference = Skeleton.GetReferenceSkeleton();
			TArray<FBoneIndexType> Indices;
			for (int32 Index = 0; Index < Reference.GetNum(); ++Index)
			{
				Indices.Add(static_cast<FBoneIndexType>(Index));
			}
			return Indices;
		}
	};

	/** The mannequin's mesh turned as the player's is: its +Y (its front) faces the world's +X, where the planned moves go. */
	inline FQuat MeshToWorld()
	{
		return FQuat(FVector::UpVector, FMath::DegreesToRadians(-90.f));
	}

	/** The player's capsule at its size: the full-size 42 x 96 scaled by 0.85, as the traversal tests have it. */
	inline constexpr float Radius = 42.f * LooterPlayerSize::Scale;
	inline constexpr float HalfHeight = 96.f * LooterPlayerSize::Scale;
	inline constexpr float StandingEye = 140.f;

	/** A move planned at the player's size, standing at the origin and facing +X, and the obstacle's top over its feet. */
	struct FPlannedMove
	{
		FPlayerTraversal Move;
		float Top = 0.f;
		bool bVault = false;
		bool bPlanned = false;
	};

	inline FTraversalSetup PlayerSetup(ETraversalKind Kind)
	{
		FTraversalSetup Setup;
		Setup.Kind = Kind;
		Setup.Origin = FVector(0.0, 0.0, HalfHeight);
		Setup.Direction = FVector::ForwardVector;
		Setup.Radius = Radius;
		Setup.HalfHeight = HalfHeight;
		Setup.EyeZ = StandingEye;
		Setup.EyeForward = 15.f;
		return Setup;
	}

	/** A mantle onto a 1.2 m ledge 10 cm away from a stand, stepping on at a walk. */
	inline FPlannedMove MakeMantle()
	{
		FTraversalSetup Setup = PlayerSetup(ETraversalKind::Mantle);
		Setup.NearFace = Radius + 10.f;
		Setup.TopZ = 120.f;
		Setup.EndAlong = Setup.NearFace + Radius + 8.f;
		Setup.EndFeetZ = 122.15f;
		Setup.ExitSpeed = 230.f;
		Setup.Duration = LooterTraversal::MantleSeconds(Setup.EndFeetZ);
		Setup.EyeEndHeight = StandingEye - LooterTraversal::MantleEyeDip;
		FPlannedMove Planned;
		Planned.Top = Setup.TopZ;
		Planned.bPlanned = Planned.Move.Plan(Setup);
		return Planned;
	}

	/** A vault at a sprint over a 1 m rail 10 cm thick, 60 cm off. */
	inline FPlannedMove MakeVault()
	{
		const float Sprint = LooterPlayerSize::FullSizeWalkSpeed * LooterPlayerSize::SpeedScale * 1.55f;
		FTraversalSetup Setup = PlayerSetup(ETraversalKind::Vault);
		Setup.Velocity = FVector(Sprint, 0.0, 0.0);
		Setup.NearFace = Radius + 60.f;
		Setup.FarFace = Setup.NearFace + 10.f;
		Setup.TopZ = 100.f;
		Setup.Tuck = LooterTraversal::VaultTuck;
		Setup.Duration = LooterTraversal::VaultSeconds(Setup.TopZ - Setup.Tuck + 3.f);
		Setup.EndAlong = FMath::Max(0.9f * Sprint * Setup.Duration, Setup.FarFace + Radius + 10.f);
		Setup.EndFeetZ = 2.15f;
		Setup.ExitSpeed = Sprint;
		Setup.EyeEndHeight = StandingEye - LooterTraversal::VaultEyeDip;
		FPlannedMove Planned;
		Planned.Top = Setup.TopZ;
		Planned.bVault = true;
		Planned.bPlanned = Planned.Move.Plan(Setup);
		return Planned;
	}

	/**
	 * Follows a planned move frame by frame, then Extra seconds after it, easing the pose alpha the way the locomotion
	 * component does (UpdateAlphas: a minimum-jerk ease, 0.12 s in and 0.25 s out, times 1.25 for the curve, a vault to
	 * 0.6), and hands Visit each frame's description of the pose and whether the move is still under way.
	 */
	inline void Follow(FPlannedMove Planned, float Extra, TFunctionRef<void(const FLooterClimbInput& Input, float Alpha, bool bMoving)> Visit)
	{
		FViewEase Ease;
		Ease.Reset(0.f);
		const float Peak = Planned.bVault ? LooterClimbPose::VaultAlphaShare : 1.f;
		const float Length = Planned.Move.GetDuration() + Extra;
		for (float Time = 0.f; Time <= Length; Time += FrameTime)
		{
			Planned.Move.Advance(FrameTime);
			const bool bMoving = Planned.Move.IsActive();
			const float Target = bMoving ? Peak : 0.f;
			const float Share = FMath::Clamp(FMath::Abs(Target - Ease.GetValue()), 0.35f, 1.f);
			Ease.SetTarget(Target, (bMoving ? 0.12f : 0.25f) * 1.25f * Share);
			Ease.Advance(FrameTime);
			const float Alpha = FMath::Clamp(Ease.GetValue(), 0.f, 1.f);
			Visit(LooterClimbPose::Describe(Planned.Move, Alpha, Planned.Top, Radius, LooterPlayerSize::Scale, MeshToWorld()), Alpha, bMoving);
		}
	}
}

#endif
