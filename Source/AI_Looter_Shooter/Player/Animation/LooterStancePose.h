#pragma once

#include "CoreMinimal.h"
#include "Player/Animation/HeroPoseTypes.h"

struct FCompactPose;
struct FLooterStanceInput;

/**
 * A grenade throw's body at one moment (LooterStancePose::DescribeThrow), as plain numbers: where the left arm throws
 * from and to, how the chest turns into it, how far the gun hand dips out of its way. Offsets are the full-size
 * mannequin's (cm) from the left shoulder, in the body's facing: X ahead, Y out to its left, Z up.
 */
struct FLooterThrowStance
{
	/** How strongly the left arm follows the throw (0: wherever the rest of the pose has it; 1: on the throw's path). */
	float Arm = 0.f;
	/** The left wrist, from the left shoulder. */
	FVector Hand = FVector::ZeroVector;
	/** Where the left elbow points, from the left shoulder. */
	FVector Elbow = FVector::ZeroVector;
	/** The chest's turn (degrees; + brings the left shoulder forward) and its lean (degrees; + forward). */
	float Twist = 0.f;
	float Lean = 0.f;
	/** How far the gun in the right hand has dipped out of the throw's way (0-1, ThrowMotion::GunDipShare). */
	float GunDip = 0.f;

	bool IsActive() const
	{
		return Arm > UE_KINDA_SMALL_NUMBER || GunDip > UE_KINDA_SMALL_NUMBER || !FMath::IsNearlyZero(Twist) || !FMath::IsNearlyZero(Lean);
	}
};

/**
 * A revolver's reload at one moment (LooterStancePose::DescribeCylinderReload): where the left hand works on the gun, or
 * how far it has gone to the belt for the speedloader, and the gun hand's flick as the cylinder shuts.
 */
struct FLooterCylinderReload
{
	/** The left palm's spot on the gun, from its grip in the gun's own space (cm: X along the barrel, Y right, Z up). */
	FVector OnGun = FVector::ZeroVector;
	/** How far the left hand has gone down to the belt for the speedloader (0: at OnGun, 1: at the belt). */
	float Pouch = 0.f;
	/** The gun hand's flick as the cylinder is shut (0-1, its peak as it latches). */
	float Flick = 0.f;
};

/**
 * Standing still unarmed, the weary revenant's idle at one moment (LooterStancePose::DescribeIdle; the user's picks,
 * 2026-10-15: "Weary revenant", with breathing, glances around and hand fidgets; the armed stance kept). Each a share or
 * an angle; the stance layer turns them into bones (LooterStancePoseIdle.cpp).
 */
struct FLooterIdleStance
{
	/** Which leg takes his weight, -1 the left to 1 the right: he leans on one a while, then the other. */
	float WeightShift = 0.f;
	/** The breath, 0 out to 1 in. */
	float Breath = 0.f;
	/** A look off to the side and back (degrees: + to his right, + up). */
	float GlanceYaw = 0.f;
	float GlancePitch = 0.f;
	/** Each hand's fingers flexing a little more (0-1). */
	float FlexLeft = 0.f;
	float FlexRight = 0.f;
	/** The right thumb hooked in the gun belt (0 the arm hanging, 1 the hand on the belt): a BeltThumb rest's spells. */
	float ThumbHook = 0.f;
	/** The piston sway's dip, 0 up to 1 down (FHeroIdle::SwayDepth; 0 without one). */
	float Sway = 0.f;
	/** Breaths begun since the idle's clock started (steam goes with some: FHeroIdle::SteamEvery). */
	int32 Breaths = 0;
	/**
	 * A card walking over a CardHand's knuckles: where it is, 0 on the index's knuckle to 3 on the pinky's (negative: no
	 * card hand), and how far it has turned over (radians: a half turn a knuckle).
	 */
	float Card = -1.f;
	float CardSpin = 0.f;
};

/**
 * The procedural stance pose, layered on whatever the Anim Blueprint's graph produced (ULooterCharacterAnimInstance
 * calls it from the animation thread). A free function on a plain pose so the tests can run it on the mannequin's
 * reference pose, bone by bone, without a world:
 *  - crouch: torso leans forward, hips drop until the head reaches the crouched head height, and both legs are solved
 *    with two-bone IK back onto the feet the graph planted (so crouch-walking keeps the walk cycle's steps)
 *  - sprint: the torso leans into the run
 *  - slide: the torso leans back, the hips sit down near the ground, the right leg reaches out ahead and the left folds under
 *  - mantle and vault (Player/Animation/LooterClimbPose.h): the torso leans into the climb, the legs tuck up, the hands
 *    reach for the ledge (a vault plants the free hand)
 *  - armed: the torso pitches with the player's aim, and the left hand is solved onto the held gun's foregrip
 *    (the rifle animations were made for a different gun); a short gun (HoldReach) is first pushed out along its barrel
 *    by the right arm, and its foregrip is on its grip, so both hands hold it out like a pistol, the support hand
 *    wrapped round the shooting hand's fingers
 *  - carried (LooterStancePoseCarry.h): on the move at ease the gun drops to a low-ready, in a sprint it's carried across
 *    the chest (a six-gun low at the side in one hand); the hands turn with the gun, so it keeps its place in them
 *  - recoil: each shot rocks the chest back and lets the right arm give, so the gun (in that hand) jumps back and the
 *    left hand rides along on the foregrip
 *  - melee: the strike's jab twists the chest into the blow and drives the right hand (the gun's stock, or a fist) forward
 *  - grenade: an overarm toss with the left hand (cocked by the ear, slung out to where the jar leaves it), the chest
 *    winding up and turning into it, the gun hand dipping out of its way (LooterStancePoseThrow.cpp)
 *  - a revolver's reload: no animation; the gun comes in, the left hand rides the cylinder out, punches the ejector rod,
 *    fetches a speedloader from the belt and pushes it in, and bats the cylinder home as the gun hand flicks
 *    (LooterStancePoseReload.cpp), on the gun's own reload steps (Weapons/ReloadMotion)
 *  - standing still unarmed (Stance.IdleAlpha): the weary idle; shoulders a little forward, head a little low, his
 *    weight shifting from leg to leg, breathing, now and then a glance aside, fingers flexing, the right thumb hooked in
 *    the belt a while (LooterStancePoseIdle.cpp); each hero their own stance on that clock (Stance.Idle, FHeroIdle: the
 *    posture, the hands' rests by arm IK and their rest curl, LooterStancePoseIdleHands.cpp)
 *  - aiming down the sights (Stance.AimDownSights): the gun raised from the hip hold to the eye line, the head down onto
 *    it, the elbows in (Stance.Aim, FHeroAim; LooterStancePoseAim.cpp)
 */
namespace LooterStancePose
{
	/** Layers the stance on Pose in place. False, the pose untouched, for a skeleton without the mannequin's bones. */
	bool Apply(FCompactPose& Pose, const FLooterStanceInput& Stance);

	/**
	 * How far a melee strike's jab is out SwingTime seconds into the swing (UPlayerMeleeComponent::GetSwingTime), on
	 * FMeleeRules' clock: cocked back a little (-0.3) by MeleeMotion::CockSeconds, driven out to 1 at the blow
	 * (ContactSeconds), held a moment and eased back to 0 by the swing's end. 0 outside a swing.
	 */
	float MeleeJab(float SwingTime);

	/**
	 * A grenade throw ThrowTime seconds in, on FThrowRules' and ThrowMotion's clock: the arm comes up to the ear as the
	 * first-person jar comes up to the cheek (ThrowMotion::DrawSeconds), is slung out to the release (ReleaseSeconds),
	 * follows through across the body and goes back to what it was doing by the throw's end. Nothing at 0 or after.
	 */
	FLooterThrowStance DescribeThrow(float ThrowTime);

	/**
	 * A revolver's reload Progress (0 to 1) of the way through, on Weapons/ReloadMotion's steps for a cylinder: the left
	 * hand at the cylinder as the latch clicks, riding it out (LooterReload::CylinderSwing), the palm's punch on the
	 * ejector rod (Eject), down to the belt and back with a speedloader pushed in (RoundsIn), the cylinder batted home
	 * as the gun hand flicks (CylinderIn), then on it until the reload lets go.
	 */
	FLooterCylinderReload DescribeCylinderReload(float Progress);

	/**
	 * The middle of the Drover's cylinder swung SwingDegrees out on its crane, from the gun's grip in its own space (cm):
	 * Art/Models/Weapons/Drover.py's layout.
	 */
	FVector CylinderCenter(float SwingDegrees);

	/**
	 * The idle Time seconds in (its own clock, running on): the weight shift (about 9 s a side and back), the breath
	 * (about 4 s), a glance in most 10-second spells, a finger flex now and then, the thumb in the belt a few seconds in
	 * some 24-second spells. The same Time always gives the same idle (spells drawn from a hash of their number).
	 */
	FLooterIdleStance DescribeIdle(float Time);

	/**
	 * A hero's idle Time seconds in (FHeroIdle, the user's stances of 2026-10-22): the same clock with their breath's period,
	 * their share of the shift and the glances, their piston sway, the thumb's belt spells only for a BeltThumb rest, and a
	 * card walking over a CardHand's knuckles. DescribeIdle(Time) is this with the defaults (Ellis's).
	 */
	FLooterIdleStance DescribeIdle(float Time, const FHeroIdle& Style);

	/** Seconds a card takes to walk the four knuckles and be passed back under the hand to the index again. */
	inline constexpr float CardCycleSeconds = 3.2f;
}
