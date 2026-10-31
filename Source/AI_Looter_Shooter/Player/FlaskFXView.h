#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraTypes.h"

class APawn;
class UStaticMeshComponent;

/**
 * Where the soul flask's light is drawn (FlaskFX.cpp moves and draws it): the eye that sees the flask, and the flask as it
 * shows to that eye. A first-person model is drawn at the view model's own field of view, so its points are moved to
 * where they show (FMinimalViewInfo::TransformWorldToFirstPerson, as AWeaponBase::GetVisibleLocation does for a muzzle).
 */
namespace FlaskFXView
{
	/** The eye looking out of the drinker (the first-person camera, or the pawn's view), and the camera the picture is
	 *  taken from (behind the shoulder in third person), which things are sized against. */
	struct FEye
	{
		FVector Camera = FVector::ZeroVector;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector Forward = FVector::ForwardVector;
		FVector Right = FVector::RightVector;
		FVector Up = FVector::UpVector;
		bool bFirstPerson = false;
		FMinimalViewInfo View;
	};

	/** The flask as it shows now: its foot's middle, its mouth, the neck's way out (unit), and whether a model was found. */
	struct FPlace
	{
		FVector Foot = FVector::ZeroVector;
		FVector Mouth = FVector::ZeroVector;
		FVector Out = FVector::UpVector;
		bool bModel = false;

		/** A point up its axis, Share of the way from the foot to the mouth (0.35: the belly, where the light is). */
		FVector Along(float Share) const { return FMath::Lerp(Foot, Mouth, Share); }
		float Length() const { return static_cast<float>(FVector::Dist(Foot, Mouth)); }
	};

	FEye EyeOf(const APawn& Drinker);

	/** Where a point of a model shows: moved for the view model's field of view when the model is drawn first person. */
	FVector ShownAt(const FEye& Eye, const FVector& Point, bool bFirstPersonModel);

	/** The flask the eye sees on the drinker (a visible SM_SoulFlask*, or a component named *Flask*: the stand-in before the
	 *  import): only a first-person model in first person, only a world model (the body's HandGrip_L) in third, never one
	 *  hidden from its owner; null with none shown. */
	const UStaticMeshComponent* FindFlask(const APawn& Drinker, bool bFirstPerson);

	/**
	 * Where the flask shows now: from its model; else, in third person, at the head (tipped up at the lips); else, in first
	 * person, from MouthInEye (a mouth held in the eye's frame, the flask taken to lie straight away from the eye). False only
	 * in first person with neither.
	 */
	bool PlaceOf(const APawn& Drinker, const FEye& Eye, const FVector* MouthInEye, FPlace& Out);

	/** The flask stowed at the hip (a charge filled out of sight): upright there. */
	FPlace HipPlaceOf(const APawn& Drinker);

	/** The body's middle (between hips and chest): third-person motes stay near it. */
	FVector BodyMiddleOf(const APawn& Drinker);

	/** Where the soul goes in: the lips just under the eye in first person, the chest in third. */
	FVector SinkOf(const APawn& Drinker, const FEye& Eye);

	/** A size Share of At's distance from the camera, within Min..Max (cm): the same on screen near or far. */
	float SizeAt(const FEye& Eye, const FVector& At, float Share, float Min, float Max);

	/** A world point into the eye's own frame, and back: a mouth held still in the view as the drinker turns. */
	FVector ToEye(const FEye& Eye, const FVector& World);
	FVector FromEye(const FEye& Eye, const FVector& Local);
}
