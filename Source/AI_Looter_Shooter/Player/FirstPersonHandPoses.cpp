#include "Player/FirstPersonHandPoses.h"
#include "Containers/Array.h"

// The pose library's data is generated (Art/Models/Characters/FPHandPoses.py -> FirstPersonHandPoseData.inl), as plain rows
// so it needs none of the game's types; they become FHandPoses once, the first time one is asked for.

namespace
{
#include "Player/FirstPersonHandPoseData.inl"

	TArray<FirstPersonHandPoses::FHandPose> BuildPoses()
	{
		TArray<FirstPersonHandPoses::FHandPose> Poses;
		Poses.Reserve(UE_ARRAY_COUNT(PoseRows));
		for (const FPoseRow& Row : PoseRows)
		{
			FirstPersonHandPoses::FHandPose& Pose = Poses.AddDefaulted_GetRef();
			Pose.Name = FName(Row.Name);
			for (int32 Finger = 0; Finger < FHandFingers::Count; ++Finger)
			{
				for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
				{
					Pose.Fingers.Curl[Finger][Joint] = Row.Curl[Finger][Joint];
				}
				Pose.Fingers.Spread[Finger] = Row.Spread[Finger];
			}
			Pose.ThumbSwing = Row.ThumbSwing;
			Pose.ThumbSpread = Row.ThumbSpread;
			Pose.TouchShare = Row.TouchShare;
			Pose.HandleHalfAcross = Row.HandleHalfAcross;
			Pose.HandleHalfThrough = Row.HandleHalfThrough;
			Pose.HandleCorner = Row.HandleCorner;
			Pose.bForearmBehindKnuckles = Row.bForearmBehindKnuckles;
		}
		return Poses;
	}

	const TArray<FirstPersonHandPoses::FHandPose>& Library()
	{
		// Built on first use (function-local, so thread-safe and after the engine's names exist).
		static const TArray<FirstPersonHandPoses::FHandPose> Poses = BuildPoses();
		return Poses;
	}
}

const FirstPersonHandPoses::FHandPose* FirstPersonHandPoses::Find(FName Pose)
{
	return Library().FindByPredicate([Pose](const FHandPose& Candidate) { return Candidate.Name == Pose; });
}

TConstArrayView<FirstPersonHandPoses::FHandPose> FirstPersonHandPoses::All()
{
	return Library();
}
