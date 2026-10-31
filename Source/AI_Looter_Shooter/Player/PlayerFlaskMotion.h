#pragma once

#include "CoreMinimal.h"
#include "Player/PlayerThrowMotion.h"
#include "Player/ViewKick.h"

/**
 * How a drink from the soul flask moves, done in code like the grenade throw (no animation assets), on FFlaskRules' clock:
 * the flask in the off hand, the gun dipping out of its way exactly as the throw dips it (ThrowMotion::GunDipPose, the
 * gun's melee swing state), and the view's small lift and tip back, through the shared camera modifier.
 *
 * Over the drink's 1.1 s: the flask comes up from under the view's lower left (0.25 s) and is tipped to the lips by the
 * sip (FFlaskRules::SipAt, 0.35 s): low and left of the middle, its neck running down and back out of the bottom of the
 * view toward the mouth under the camera, its foot tilted up and away, so its side and foot show in the bottom fifth of the
 * view, cut off by its edge, never over the crosshair or the view's middle third. It's tipped on up as the soul pours,
 * with two small gulps, until 0.85 s, then lowered away out of sight by 1.0 s, the hand back on the gun by the end. The
 * gun is all the way down by 0.15 s and back up by the end. The third-person body follows the same clock
 * (LooterStancePose::DescribeDrink), and the first-person hand cradles the belly from below on it
 * (FirstPersonHandsMotion::OnFlask).
 *
 * Poses are the drawn flask's foot (its pivot) off the first-person camera (X ahead, Y right, Z up; cm), worked out from
 * where its mouth is and the way its neck points, turned so its open front (between the straps) faces the eye. The view's
 * edges: at 90 degrees across and 16:9, a point at (X, Y, Z) shows where |Y| < X and |Z| < 0.5625 X.
 */
namespace FlaskMotion
{
	/**
	 * The model's measures (Art/Models/Items/SoulFlask.py, 16 cm tall with the cork, its pivot the middle of its foot): its
	 * mouth (SOCKET_Mouth, the lip) above the foot, the belly's widest where the hand closes, its radius there (10.4 cm
	 * across), and the foot's radius.
	 */
	inline constexpr float MouthUp = 14.2f;
	inline constexpr float MiddleUp = 4.5f;
	inline constexpr float Radius = 5.2f;
	inline constexpr float FootRadius = 4.f;

	/**
	 * The first-person flask is drawn at this share of its size (as the grenade's JarModelScale can), and held further out
	 * to match: a full-size flask at the lips filled 40% of the view and sat over the crosshair (the 2026-10-30
	 * play-check). Small and out at arm's length it reads the same size a held flask feels, a fifth of the view at most.
	 * The poses below are the drawn (scaled) flask's.
	 */
	inline constexpr float ViewScale = 0.6f;

	/** The drawn flask's mouth above its foot, its belly's middle and radius, and its foot's radius (camera cm). */
	inline constexpr float HeldMouthUp = MouthUp * ViewScale;
	inline constexpr float HeldMiddleUp = MiddleUp * ViewScale;
	inline constexpr float HeldRadius = Radius * ViewScale;
	inline constexpr float HeldFootRadius = FootRadius * ViewScale;

	/** At the lips by here; tipped on up from the sip to here; out of sight under the view by here (seconds into the drink). */
	inline constexpr float RaiseSeconds = 0.25f;
	inline constexpr float TipEndSeconds = 0.85f;
	inline constexpr float LowerSeconds = 1.f;

	/** The gun all the way down by here, held, and back up from here to the drink's end. */
	inline constexpr float DipInSeconds = 0.15f;
	inline constexpr float DipOutFrom = 0.85f;

	/** The flask's key poses off the first-person camera. */
	AI_LOOTER_SHOOTER_API FThrowPose StartPose();
	AI_LOOTER_SHOOTER_API FThrowPose RaisedPose();
	AI_LOOTER_SHOOTER_API FThrowPose AtLipsPose();
	AI_LOOTER_SHOOTER_API FThrowPose TippedPose();

	/** The flask is up out of the belt and in the off hand DrinkTime seconds in (shown then, first and third person alike). */
	AI_LOOTER_SHOOTER_API bool IsInHand(float DrinkTime);

	/** The flask off the first-person camera DrinkTime seconds in, and whether it shows. */
	AI_LOOTER_SHOOTER_API FThrowPose FlaskPose(float DrinkTime, bool& bOutShown);

	/** The flask's mouth in the pose's space (camera space for FlaskPose's). */
	AI_LOOTER_SHOOTER_API FVector MouthOf(const FThrowPose& Pose);

	/** The middle of its body, where the hand closes round it. */
	AI_LOOTER_SHOOTER_API FVector MiddleOf(const FThrowPose& Pose);

	/** How far into its dip the gun is DrinkTime seconds in (0 in its hold, 1 all the way down). */
	AI_LOOTER_SHOOTER_API float GunDipShare(float DrinkTime);

	/** The gun DrinkTime seconds into a drink: ThrowMotion::GunDipPose by GunDipShare; rest (zero) before and after. */
	AI_LOOTER_SHOOTER_API FThrowPose GunPose(float DrinkTime);

	/** The view as the flask comes up and the cork is pulled: a small lift toward the off hand. */
	AI_LOOTER_SHOOTER_API FViewKick Lift();

	/** The view at the sip: the head tipping back a touch with the flask, slow, a breath in. */
	AI_LOOTER_SHOOTER_API FViewKick Sip();
}
