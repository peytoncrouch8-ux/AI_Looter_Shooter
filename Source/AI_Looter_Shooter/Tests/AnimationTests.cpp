#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "BoneContainer.h"
#include "BonePose.h"
#include "Misc/MemStack.h"

namespace
{
	struct FClipStats
	{
		float Length = 0.f;
		/** Ground speed from root motion (cm/s). */
		float Speed = 0.f;
		/** Foot crossings per second: the gait's cadence. */
		float StepsPerSecond = 0.f;
		/** How far the feet jump when the clip wraps from its last frame to its first, vs a normal frame's motion. */
		float LoopPop = 0.f;
	};

	/** Component-space positions of the pelvis and feet at one time in a clip. */
	struct FLegPose
	{
		FVector Pelvis = FVector::ZeroVector;
		FVector FootL = FVector::ZeroVector;
		FVector FootR = FVector::ZeroVector;
	};

	FLegPose SampleLegs(const UAnimSequence& Anim, const FBoneContainer& Bones, double Time)
	{
		// Poses allocate from the animation memory stack, which needs a mark outside of normal anim evaluation.
		FMemMark Mark(FMemStack::Get());
		FCompactPose Pose;
		Pose.SetBoneContainer(&Bones);
		FBlendedCurve Curve;
		UE::Anim::FStackAttributeContainer Attributes;
		FAnimationPoseData Data(Pose, Curve, Attributes);
		Anim.GetAnimationPose(Data, FAnimExtractContext(Time));

		FCSPose<FCompactPose> ComponentPose;
		ComponentPose.InitPose(Pose);
		auto Where = [&](const TCHAR* Name)
		{
			FBoneReference Bone(Name);
			Bone.Initialize(Bones);
			const FCompactPoseBoneIndex Index = Bone.GetCompactPoseIndex(Bones);
			return Index.IsValid() ? ComponentPose.GetComponentSpaceTransform(Index).GetLocation() : FVector::ZeroVector;
		};
		FLegPose Legs;
		Legs.Pelvis = Where(TEXT("pelvis"));
		Legs.FootL = Where(TEXT("foot_l"));
		Legs.FootR = Where(TEXT("foot_r"));
		return Legs;
	}

	FClipStats MeasureClip(const UAnimSequence& Anim)
	{
		FClipStats Stats;
		const USkeleton* Skeleton = Anim.GetSkeleton();
		if (!Skeleton)
		{
			return Stats;
		}
		const FReferenceSkeleton& Reference = Skeleton->GetReferenceSkeleton();
		TArray<FBoneIndexType> Required;
		for (int32 Index = 0; Index < Reference.GetNum(); ++Index)
		{
			Required.Add(static_cast<FBoneIndexType>(Index));
		}
		FBoneContainer Bones(Required, UE::Anim::FCurveFilterSettings(), *const_cast<USkeleton*>(Skeleton));

		Stats.Length = static_cast<float>(Anim.GetPlayLength());
		const FTransform Root = Anim.ExtractRootMotionFromRange(0.0, Stats.Length, FAnimExtractContext());
		Stats.Speed = Stats.Length > 0.f ? Root.GetTranslation().Size() / Stats.Length : 0.f;
		const FVector MoveDir = Root.GetTranslation().GetSafeNormal2D();

		// Steps: the feet swap places along the direction of travel once per step.
		constexpr int32 Samples = 120;
		int32 Crossings = 0;
		float LastGap = 0.f;
		float FrameMotion = 0.f;
		FLegPose Previous = SampleLegs(Anim, Bones, 0.0);
		const FLegPose First = Previous;
		for (int32 Step = 1; Step <= Samples; ++Step)
		{
			const FLegPose Legs = SampleLegs(Anim, Bones, Stats.Length * Step / Samples);
			const float Gap = FVector::DotProduct(Legs.FootL - Legs.FootR, MoveDir.IsNearlyZero() ? FVector::ForwardVector : MoveDir);
			if (Step > 1 && FMath::Sign(Gap) != FMath::Sign(LastGap) && FMath::Abs(Gap - LastGap) > 0.5f)
			{
				++Crossings;
			}
			LastGap = Gap;
			FrameMotion = FMath::Max(FrameMotion, FVector::Dist(Legs.FootL - Legs.Pelvis, Previous.FootL - Previous.Pelvis));
			Previous = Legs;
		}
		Stats.StepsPerSecond = Stats.Length > 0.f ? Crossings / Stats.Length : 0.f;

		// Wrapping from the end back to the start should look like any other frame.
		const float Wrap = FMath::Max(FVector::Dist(Previous.FootL - Previous.Pelvis, First.FootL - First.Pelvis),
			FVector::Dist(Previous.FootR - Previous.Pelvis, First.FootR - First.Pelvis));
		Stats.LoopPop = FrameMotion > UE_KINDA_SMALL_NUMBER ? Wrap / FrameMotion : 0.f;
		return Stats;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocomotionClipsTest, "Looter.Animation.LocomotionClips",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLocomotionClipsTest::RunTest(const FString& Parameters)
{
	// A blend space syncs its clips by normalized time, so every moving clip in it has to loop cleanly and share a
	// gait: ground speed matching its row, and a similar step rate across the row. The rifle set broke both (a 2-step
	// strafe next to 5-step diagonals, a clip authored at double speed, popping loops), and the legs jittered.
	const TCHAR* Spaces[] = {
		TEXT("/Game/Characters/Mannequins/Anims/Rifle/BS_Rifle_Locomotion.BS_Rifle_Locomotion"),
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/BS_Idle_Walk_Run.BS_Idle_Walk_Run"),
	};
	constexpr float MaxLoopPop = 2.f;
	constexpr float MaxSpeedError = 0.15f;
	constexpr float MaxCadenceRatio = 1.3f;
	for (const TCHAR* Path : Spaces)
	{
		const UBlendSpace* Space = LoadObject<UBlendSpace>(nullptr, Path);
		if (!TestNotNull(FString::Printf(TEXT("%s loads"), Path), Space))
		{
			continue;
		}
		TMap<int32, TPair<float, float>> CadenceByRow;   // row speed -> (lowest, highest) steps per second
		TSet<const UAnimSequence*> Seen;
		for (const FBlendSample& Sample : Space->GetBlendSamples())
		{
			const UAnimSequence* Anim = Cast<UAnimSequence>(Sample.Animation);
			if (!Anim || Sample.SampleValue.Y <= 0.f || Seen.Contains(Anim))
			{
				continue;
			}
			Seen.Add(Anim);
			const FClipStats Stats = MeasureClip(*Anim);
			const float RowSpeed = Sample.SampleValue.Y;
			AddInfo(FString::Printf(TEXT("%s %-26s at (%4.0f, %3.0f): length %.3f s, speed %5.0f cm/s, %.2f steps/s, loop pop %.1fx"),
				*Space->GetName(), *Anim->GetName(), Sample.SampleValue.X, RowSpeed, Stats.Length, Stats.Speed, Stats.StepsPerSecond, Stats.LoopPop));

			TestTrue(FString::Printf(TEXT("%s loops cleanly (pop %.1fx)"), *Anim->GetName(), Stats.LoopPop), Stats.LoopPop <= MaxLoopPop);
			TestTrue(FString::Printf(TEXT("%s moves at its row's speed (%.0f vs %.0f cm/s)"), *Anim->GetName(), Stats.Speed, RowSpeed),
				FMath::Abs(Stats.Speed - RowSpeed) <= RowSpeed * MaxSpeedError);
			TPair<float, float>& Range = CadenceByRow.FindOrAdd(FMath::RoundToInt(RowSpeed), TPair<float, float>(TNumericLimits<float>::Max(), 0.f));
			Range.Key = FMath::Min(Range.Key, Stats.StepsPerSecond);
			Range.Value = FMath::Max(Range.Value, Stats.StepsPerSecond);
		}
		for (const TPair<int32, TPair<float, float>>& Row : CadenceByRow)
		{
			const float Ratio = Row.Value.Value / FMath::Max(Row.Value.Key, UE_KINDA_SMALL_NUMBER);
			TestTrue(FString::Printf(TEXT("%s speed %d: clips share a cadence (%.2f-%.2f steps/s)"), *Space->GetName(), Row.Key, Row.Value.Key, Row.Value.Value),
				Ratio <= MaxCadenceRatio);
		}
	}
	return true;
}

#endif
