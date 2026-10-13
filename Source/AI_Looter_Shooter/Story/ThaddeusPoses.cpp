#include "Story/ThaddeusPoses.h"

namespace
{
	/** Stood at the bench bent over his work, his head bowed to the board. */
	FDeadFriendPose AtTheBench()
	{
		using namespace DeadFriendPoses;
		FDeadFriendPose Pose;
		Pose.Bend(Spine, 14.f);
		Pose.Bend(Chest, 10.f);
		Pose.Bend(Neck, 6.f);
		Pose.Bend(Head, 16.f);
		return Pose;
	}

	/** Both hands on the board before him: together at no stretch (the forearms turned in), a coffin's width apart at full. */
	void HandsOnBoard(FDeadFriendPose& Pose, float Stretch)
	{
		const float Out = FMath::Lerp(-0.45f, 0.3f, FMath::Clamp(Stretch, 0.f, 1.f));
		Pose.Arm(0, FVector(0.5f, -0.28f, -0.82f), FVector(0.86f, -Out, -0.42f), -12.f);
		Pose.Arm(1, FVector(0.5f, 0.28f, -0.82f), FVector(0.86f, Out, -0.42f), -12.f);
	}
}

namespace ThaddeusPoses
{
	FDeadFriendPose Make(EThaddeusPose Pose, float Amount)
	{
		using namespace DeadFriendPoses;
		switch (Pose)
		{
		case EThaddeusPose::Mark:
		{
			// The tape held at its stretch; the right hand lifted off the board, the wrist flicking a pencil's tick.
			FDeadFriendPose Made = AtTheBench();
			HandsOnBoard(Made, 1.f);
			Made.Arm(1, FVector(0.55f, 0.3f, -0.78f), FVector(0.88f, 0.3f, -0.18f), 28.f);
			return Made;
		}
		case EThaddeusPose::Knock:
		{
			// The right fist raised over the board and brought down on it; the left hand still on the tape.
			FDeadFriendPose Made = AtTheBench();
			HandsOnBoard(Made, 0.5f);
			const float Rap = FMath::Clamp(Amount, 0.f, 1.f);
			const FVector Up(0.35f, 0.3f, 0.95f);
			const FVector Down(0.85f, 0.05f, -0.52f);
			Made.Arm(1, FVector(0.55f, 0.32f, -0.6f), FMath::Lerp(Up, Down, static_cast<double>(Rap)), 10.f * Rap);
			return Made;
		}
		case EThaddeusPose::Look:
		{
			// Straightened to the listener, the tape slack before him in both hands.
			FDeadFriendPose Made;
			Made.Bend(Spine, 4.f);
			Made.Bend(Chest, 2.f);
			Made.Bend(Head, -2.f);
			Made.Arm(0, FVector(0.25f, -0.2f, -0.95f), FVector(0.8f, 0.3f, 0.12f), -6.f);
			Made.Arm(1, FVector(0.25f, 0.2f, -0.95f), FVector(0.8f, -0.3f, 0.12f), -6.f);
			return Made;
		}
		default:
		{
			FDeadFriendPose Made = AtTheBench();
			HandsOnBoard(Made, Amount);
			return Made;
		}
		}
	}

	const TCHAR* Name(EThaddeusPose Pose)
	{
		static const TCHAR* const Names[] = { TEXT("measure"), TEXT("mark"), TEXT("knock"), TEXT("look") };
		static_assert(UE_ARRAY_COUNT(Names) == static_cast<int32>(EThaddeusPose::Count), "A name per pose");
		const int32 Index = static_cast<int32>(Pose);
		return Index >= 0 && Index < static_cast<int32>(EThaddeusPose::Count) ? Names[Index] : TEXT("?");
	}
}
