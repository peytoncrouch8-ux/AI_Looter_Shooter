#pragma once

#include "CoreMinimal.h"
#include "Story/DeadFriendPoses.h"

/**
 * Thaddeus Bright's poses (Docs/Areas/RansomsRest_Expansion.md, Knocking Below: "at his bench, a man in an undertaker's apron
 * is measuring a coffin that isn't there"): Tilly's dead father on the Unpaid's rig, authored here in plain angles until his
 * model and its table are made. He stands at his bench bent over his work:
 *  - Measure: both hands on the bench, his tape stretched between them, Stretch (0-1) from hands together to a coffin's
 *    width apart; his head bowed over it.
 *  - Mark: the tape held, his right hand lifted a little, wrist flicked: a pencil's tick at the measure.
 *  - Knock: his right fist raised, Rap (0-1) from up to down on the board: the knocking Tilly hears at night.
 *  - Look: straightened from the bench to whoever he talks to, the tape slack in his hands.
 * Talking, his head turns to the listener and his jaw moves on top of any of them (FDeadFriendLayer).
 */
enum class EThaddeusPose : uint8
{
	Measure,
	Mark,
	Knock,
	Look,
	Count
};

namespace ThaddeusPoses
{
	/** The pose: Amount is Measure's stretch or Knock's rap (0-1); the others take none. */
	AI_LOOTER_SHOOTER_API FDeadFriendPose Make(EThaddeusPose Pose, float Amount = 0.f);

	/** "measure", "mark", "knock", "look". */
	AI_LOOTER_SHOOTER_API const TCHAR* Name(EThaddeusPose Pose);

	/** His work's cycle (s): stretch the tape, hold it, mark it, let it go; a knock every so often between. */
	inline constexpr float StretchSeconds = 1.6f;
	inline constexpr float HoldSeconds = 0.8f;
	inline constexpr float MarkSeconds = 0.6f;
	inline constexpr float ReleaseSeconds = 1.2f;
	inline constexpr float CycleSeconds = StretchSeconds + HoldSeconds + MarkSeconds + ReleaseSeconds;
	/** A knock: two raps on the board over this long (s). */
	inline constexpr float KnockSeconds = 1.1f;
}
