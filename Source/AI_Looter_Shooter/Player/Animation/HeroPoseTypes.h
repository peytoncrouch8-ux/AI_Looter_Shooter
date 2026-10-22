#pragma once

#include "CoreMinimal.h"
#include "HeroPoseTypes.generated.h"

/**
 * Where an idle hand rests (FHeroIdle::LeftRest / RightRest; the user's stances of 2026-10-22, CharArt sheet 2): the
 * stance layer's arm IK takes the hidden mannequin's wrist there and turns the hand, and every hero's hands, pinned to its
 * hands, follow (LooterStancePoseIdleHands.cpp).
 */
UENUM(BlueprintType)
enum class EHeroIdleRest : uint8
{
	/** Hanging as the graph's idle has it (moved by the rest offset when one is set: Gauge's, clear of the boiler). */
	Hang,
	/** Thumb hooked in the gun belt at the hip's front a few seconds in some spells (Ellis's). */
	BeltThumb,
	/** Hand on the hip, fingers forward, elbow out (Odessa's right). */
	HipHand,
	/** Hand closed on the toll chain across the chest (Crane's left). */
	ChainHand,
	/** Thumb hooked in the pack's shoulder strap at the upper chest, fingers hanging over it (Pike's, both). */
	StrapThumb,
	/** Raised before the shoulder, back of the hand out, a card walking over the knuckles (Odessa's left). */
	CardHand
};

/**
 * A hero's stance standing still unarmed (the user's picks, 2026-10-22: Ellis keeps the weary revenant; Crane stooped,
 * head low and forward, a hand on the toll chain; Odessa hip cocked, hand on hip, a card walking over her knuckles, chin
 * up; Gauge wide and planted, arms clear of the boiler, a slow piston sway, steam every few breaths; Pike upright, weight
 * back, thumbs in his pack straps, looking off). Angles in degrees, distances in cm on the full-size body. The defaults
 * are Ellis's weary idle as it was approved (2026-10-15), plus the hands' rest curl. Filled per hero by
 * Tools/Unreal/create_ember_assets.py (HERO_IDLE); read by the stance layer through UBodyFigureComponent::GetIdle.
 */
USTRUCT(BlueprintType)
struct FHeroIdle
{
	GENERATED_BODY()

	/** The chest forward, the shoulders rounded, the neck down and the head back up a little (the posture). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float ChestSlump = 4.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float ShoulderRound = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float NeckDown = 7.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float HeadUp = 2.f;

	/** The whole back leaning forward (+, a stoop) or back (-), spread down the spine, the neck not taking it back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float Lean = 0.f;

	/** The hips back over the heels (+, weight back) or forward (-), cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float HipsBack = 0.f;

	/**
	 * The hips held over one leg, in shares of the weight shift (+ the right; 1 = a full shift's 2.5 cm, 3 degrees of hip
	 * drop): a cocked hip. The shift from leg to leg rides on it by WeightShift.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float HipCock = 0.f;

	/** How much of the slow leg-to-leg weight shift shows (1 Ellis's; a planted body less). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float WeightShift = 1.f;

	/** The feet each this far further out than the graph planted them (cm): a wide stance, the hips settling to reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float StanceWidth = 0.f;

	/** A held turn of the head (+ to his right) and tilt (+ up), on top of the posture: a look off to the horizon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float HeadYaw = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float HeadPitch = 0.f;

	/**
	 * The head lifted this far (+ up) in every stance, not only the idle: a body whose head the retarget leaves looking
	 * down (Gauge, the user 2026-10-22: "looking down all the time").
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float HeadLift = 0.f;

	/** How much of the glances aside show (1 Ellis's). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float Glances = 1.f;

	/** A slow piston sway: the hips dipping this far (cm) and up again over SwayPeriod seconds, the knees taking it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float SwayDepth = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle", meta = (ClampMin = "0.5"))
	float SwayPeriod = 5.f;

	/** One breath (s) and how deep (1 Ellis's). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle", meta = (ClampMin = "0.5"))
	float BreathPeriod = 4.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float BreathDepth = 1.f;

	/** Steam vented every this many breaths, as one breathes out (0 none; Gauge's stacks, UBodyFigureComponent). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle", meta = (ClampMin = "0"))
	int32 SteamEvery = 0;

	/**
	 * Where the steam leaves, one each side (cm on the full-size body from between the feet: ahead, out to each side, up):
	 * Gauge's stack mouths (Tools/Blender/gauge_parts.py: 14 back, 19.7 out, 191 up).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	FVector SteamVent = FVector::ZeroVector;

	/** Where each hand rests. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	EHeroIdleRest LeftRest = EHeroIdleRest::Hang;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	EHeroIdleRest RightRest = EHeroIdleRest::BeltThumb;

	/**
	 * Each rest's wrist moved by this for this body (cm: ahead, out from the body's middle on that side, up). A hero's hands
	 * are pinned to the mannequin's, so a rest set on the mannequin's chest lands on a taller hero's belly without it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	FVector LeftRestOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	FVector RightRestOffset = FVector::ZeroVector;

	/** How far the idle hands curl at rest (1: knuckles 35, middle 45, end 30 degrees, the thumb in; 0 the animation's open hand). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idle")
	float RestCurl = 1.f;
};

/**
 * A hero's aim down the sights in third person (the user, 2026-10-22: "when they aim down sights ... no aim-in animation in
 * 3rd person"): the gun raised from the hip hold to the eye line, the stock to the shoulder, the head down onto it, the
 * elbows in, blended in over the gun's own aim time (UPlayerViewComponent::GetAimAlpha) and out on release
 * (LooterStancePoseAim.cpp). The eye is the mannequin's (Looter.Pose.Aim.Eye*), moved by the hero's ThirdPersonHold (their
 * head against its) and EyeOffset here. Filled per hero by create_ember_assets.py (HERO_AIM).
 */
USTRUCT(BlueprintType)
struct FHeroAim
{
	GENERATED_BODY()

	/** How far up to the eye line the sights come (1 all the way; 0 the hip hold, as before 2026-10-22). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim", meta = (ClampMin = "0", ClampMax = "1"))
	float Raise = 1.f;

	/** The eye the sight lines up with, moved for this body (cm in the aim's frame: ahead, right, up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	FVector EyeOffset = FVector::ZeroVector;

	/** The head down onto the gun (degrees, neck and head), and canted toward the gun's shoulder (degrees, a long gun's cheek). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float HeadDown = 7.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float HeadCant = 6.f;

	/** How far the elbows come in under the gun (1 tucked; 0 out at the sides as the hip hold has them). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim", meta = (ClampMin = "0", ClampMax = "1.5"))
	float ElbowIn = 1.f;

	/** The gun's grip this much further ahead of the eye than its kind puts it (cm; Looter.Pose.Aim.Grip.<Kind>). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim")
	float GripAhead = 0.f;
};
