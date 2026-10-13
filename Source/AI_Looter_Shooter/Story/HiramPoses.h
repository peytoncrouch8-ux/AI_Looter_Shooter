#pragma once

#include "CoreMinimal.h"
#include "Story/DeadFriendPoses.h"

/**
 * Hiram Coyle's poses (Docs/Areas/RansomsRest_Expansion.md, Dead Lift: "Hiram on his stool by the cold boiler"): the lift's
 * dead engineman on the Unpaid's rig, authored here in plain angles until his model and its table are made.
 *  - Sit: on his stool, his shroud falling over its edge like a man's knees and shins, forearms on his knees, his head a
 *    little bowed toward the boiler's gauge.
 *  - Tube: leaned to the speaking tube at his side (TubeSide: +1 his right, -1 his left), turned and tipped toward it, his
 *    near hand up to its mouthpiece, his far forearm still on his knee: listening for the lift head, or answering it.
 * Talking, his head turns to the listener and his jaw moves on top of either (FDeadFriendLayer).
 */
enum class EHiramPose : uint8
{
	Sit,
	Tube,
	Count
};

namespace HiramPoses
{
	/** The pose, on the side the tube is (+1 his right, -1 his left). */
	AI_LOOTER_SHOOTER_API FDeadFriendPose Make(EHiramPose Pose, float TubeSide = 1.f);

	/** "sit", "tube". */
	AI_LOOTER_SHOOTER_API const TCHAR* Name(EHiramPose Pose);

	/** Sitting, his pelvis is this far below its rest (cm): on a stool's height. */
	inline constexpr float SitDrop = 44.f;
}
