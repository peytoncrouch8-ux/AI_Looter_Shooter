#include "Player/FirstPersonHandsMotion.h"
#include "Player/PlayerFlaskMotion.h"
#include "Player/PlayerFlaskRules.h"

// The support hand's drink from the soul flask: the first-person flask rides the camera on FlaskMotion's poses
// (UPlayerFlaskComponent), and the left hand is put round it on the same clock, as the throw's hand is round the jar
// (FirstPersonHandsThrow.cpp): off the gun and under the flask's belly as it comes up from under the view's lower left,
// cradling it as it's tipped to the lips and up, down out of sight with it, then back to the gun as it comes up out of its dip.

namespace
{
	/** The drinking arm's forearm, leaving the wrist down, back and out to the left (camera space): the elbow low and out. */
	const FVector FlaskForearm(-0.4f, -0.55f, -0.73f);
	/**
	 * Which way the palm faces, before it's squared to the flask (camera space): up and to the right, so it cradles the belly
	 * from below and its left side.
	 */
	const FVector PalmFrom(0.f, 0.5f, 1.f);
	/** Off the gun and round the flask in this long (seconds), while it's still rising out of sight under the view. */
	constexpr float GrabSeconds = 0.06f;
	/** Back to the gun from here (the flask out of sight, stowed) to the drink's end, as the gun comes up out of its dip. */
	constexpr float BackFrom = FlaskMotion::LowerSeconds;
}

FHandPlacement FirstPersonHandsMotion::OnFlask(float DrinkTime)
{
	bool bShown = false;
	const FThrowPose Pose = FlaskMotion::FlaskPose(DrinkTime, bShown);
	const FVector FlaskUp = Pose.Rotation.Quaternion().GetUpVector();
	const FVector Middle = FlaskMotion::MiddleOf(Pose);
	// Holding a bottle: the palm under the belly and round its left side (it faces up and to the right, square to the
	// flask), so the flask's near side and foot show above the fingers. The knuckles run on round its far right side, the
	// wrist back toward the lower left where the arm comes in from: a left hand's knuckles are the flask's up cross the
	// palm. (Round the flask's own side, as the jar is held, the tipped flask put the hand over its top: the 2026-10-30
	// play-check.)
	const FVector Palm = FVector::VectorPlaneProject(PalmFrom, FlaskUp).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
	const FVector Fingers = FVector::CrossProduct(FlaskUp, Palm);
	// The palm's skin on the belly's glass (the grip point sits GripOffPalmSkin off it).
	const FVector Hold = Middle - Palm * (FlaskMotion::HeldRadius - FirstPersonHands::GripOffPalmSkin);
	return FHandPlacement::Make(Hold, Fingers, Palm, FHandFingers::Of(EHandGrip::Wrap), FlaskForearm);
}

FHandPlacement FirstPersonHandsMotion::SupportInDrink(const FHandPlacement& OnGun, float DrinkTime)
{
	const float End = FFlaskRules::DrinkSeconds;
	if (DrinkTime <= 0.f || DrinkTime >= End)
	{
		return OnGun;
	}
	if (DrinkTime < GrabSeconds)
	{
		return FHandPlacement::Blend(OnGun, OnFlask(DrinkTime), FMath::SmoothStep(0.f, GrabSeconds, DrinkTime));
	}
	if (DrinkTime < BackFrom)
	{
		return OnFlask(DrinkTime);
	}
	// The flask is put away under the view where the lowering left it; the hand comes back up to the gun.
	return FHandPlacement::Blend(OnFlask(BackFrom), OnGun, FMath::SmoothStep(BackFrom, End, DrinkTime));
}
