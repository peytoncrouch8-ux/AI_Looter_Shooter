#include "Story/HiramPoses.h"

namespace
{
	/**
	 * Sat on his stool. The Unpaid have no legs: his pelvis stays upright and drops to the seat, and the shroud's first link
	 * is drawn forward and up to the stool's front edge at a knee's height, so the shroud's top runs forward like thighs
	 * and falls from there like shins (Amos's sit on his rail does the same with his tail's first link). He sits a little
	 * forward, his weight on his forearms on his knees.
	 */
	FDeadFriendPose Seated()
	{
		using namespace DeadFriendPoses;
		FDeadFriendPose Pose;
		Pose.Move(Pelvis, FVector(-4.f, 0.f, -HiramPoses::SitDrop));
		Pose.Bend(Spine, 6.f);
		Pose.Bend(Chest, 6.f);
		Pose.Bend(Neck, -4.f);
		Pose.Bend(Head, 6.f);
		Pose.Move(Tail(1), FVector(32.f, 0.f, 36.f));
		Pose.Bend(Tail(1), -18.f);
		Pose.Bend(Tail(2), 10.f);
		// Forearms on his knees, hands hanging loose over them.
		Pose.Arm(0, FVector(0.35f, -0.25f, -0.9f), FVector(0.9f, 0.1f, -0.42f), 18.f);
		Pose.Arm(1, FVector(0.35f, 0.25f, -0.9f), FVector(0.9f, -0.1f, -0.42f), 18.f);
		return Pose;
	}
}

namespace HiramPoses
{
	FDeadFriendPose Make(EHiramPose Pose, float TubeSide)
	{
		using namespace DeadFriendPoses;
		FDeadFriendPose Made = Seated();
		if (Pose != EHiramPose::Tube)
		{
			return Made;
		}
		// Leaned to the tube: turned and tipped toward it, his ear and mouth at it, his near hand up to its mouthpiece.
		const float Side = TubeSide >= 0.f ? 1.f : -1.f;
		const int32 Near = Side > 0.f ? 1 : 0;
		Made.Bend(Spine, 6.f, 22.f * Side, 10.f * Side);
		Made.Bend(Chest, 4.f, 12.f * Side, 6.f * Side);
		Made.Bend(Neck, 0.f, 12.f * Side, 4.f * Side);
		Made.Bend(Head, -4.f, 18.f * Side, 8.f * Side);
		Made.Arm(Near, FVector(0.4f, 0.55f * Side, 0.1f), FVector(-0.15f, -0.55f * Side, 0.82f), -10.f);
		return Made;
	}

	const TCHAR* Name(EHiramPose Pose)
	{
		static const TCHAR* const Names[] = { TEXT("sit"), TEXT("tube") };
		static_assert(UE_ARRAY_COUNT(Names) == static_cast<int32>(EHiramPose::Count), "A name per pose");
		const int32 Index = static_cast<int32>(Pose);
		return Index >= 0 && Index < static_cast<int32>(EHiramPose::Count) ? Names[Index] : TEXT("?");
	}
}
