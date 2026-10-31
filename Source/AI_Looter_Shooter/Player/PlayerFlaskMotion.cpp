#include "Player/PlayerFlaskMotion.h"
#include "Player/PlayerFlaskRules.h"
#include "Math/RotationMatrix.h"

namespace
{
	/**
	 * A pose whose mouth sits at Mouth (camera space) with its neck pointing along Neck (foot to mouth, any length): the foot
	 * worked back from it, the flask turned about its own axis so its open front (the model's -Y, between the straps,
	 * where the soul-light shows) faces the eye.
	 */
	FThrowPose AtMouth(const FVector& Mouth, const FVector& Neck)
	{
		const FVector Up = Neck.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		const FVector Middle = Mouth - Up * (FlaskMotion::HeldMouthUp - FlaskMotion::HeldMiddleUp);
		// Its +Y away from the eye, so its -Y (the front) toward it.
		const FVector Away = Middle.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		const FQuat Turn = FRotationMatrix::MakeFromZY(Up, Away).ToQuat();
		FThrowPose Pose;
		Pose.Rotation = Turn.Rotator();
		Pose.Offset = Mouth - Turn.RotateVector(FVector(0.f, 0.f, FlaskMotion::HeldMouthUp));
		return Pose;
	}

	FThrowPose Blend(const FThrowPose& From, const FThrowPose& To, float Alpha)
	{
		FThrowPose Pose;
		Pose.Offset = FMath::Lerp(From.Offset, To.Offset, Alpha);
		// On the shortest arc: from the belt to the lips the flask turns over by well over a quarter turn.
		Pose.Rotation = FQuat::Slerp(From.Rotation.Quaternion(), To.Rotation.Quaternion(), Alpha).Rotator();
		return Pose;
	}

	/** The two gulps through the pour: the foot nods up a few degrees about the mouth, twice, easing in and out. */
	constexpr float GulpDegrees = 2.5f;
	constexpr int32 Gulps = 2;

	/** Pose nodded about its mouth by Degrees, the foot rising for positive ones. */
	FThrowPose Nodded(const FThrowPose& Pose, float Degrees)
	{
		const FQuat Turn = Pose.Rotation.Quaternion();
		const FVector Up = Turn.GetUpVector();
		const FVector Axis = FVector::CrossProduct(Up, FVector::UpVector).GetSafeNormal();
		if (Axis.IsNearlyZero())
		{
			return Pose;
		}
		// About (neck x up), a positive turn lifts the neck: the foot comes down. The other way round raises the foot.
		const FQuat Nod(Axis, FMath::DegreesToRadians(-Degrees));
		const FQuat Turned = Nod * Turn;
		FThrowPose Out;
		Out.Rotation = Turned.Rotator();
		Out.Offset = FlaskMotion::MouthOf(Pose) - Turned.RotateVector(FVector(0.f, 0.f, FlaskMotion::HeldMouthUp));
		return Out;
	}
}

// The poses, with the view's edges in mind (|Z| < 0.5625 X shows; the bottom fifth of the view is Z under -0.34 X). The
// drawn flask is HeldMouthUp (8.5 cm) to the lip, its belly 6.2 cm across: at 30 cm out its highest point (the foot's rim)
// sits about 0.35 X under the eye, so the flask fills the bottom fifth at most, left of the middle (Y about -0.35 X), its
// neck out of the bottom of the view toward the mouth under the camera.

FThrowPose FlaskMotion::StartPose()
{
	// At the belt, out of sight under the view's lower left (its foot about 1.2 X under the eye), neck down.
	return AtMouth(FVector(22.f, -18.f, -36.f), FVector(-0.2f, 0.2f, -0.96f));
}

FThrowPose FlaskMotion::RaisedPose()
{
	// Up from the lower left to under the lips, already tipped neck-down toward the mouth: only its foot and the bottom of its
	// belly show, low in the corner.
	return AtMouth(FVector(26.f, -13.5f, -21.f), FVector(-0.5f, 0.2f, -0.84f));
}

FThrowPose FlaskMotion::AtLipsPose()
{
	// At the lips for the first swallow: its foot tilted up and away, the side and foot in the bottom fifth of the view.
	return AtMouth(FVector(27.f, -13.f, -19.5f), FVector(-0.62f, 0.22f, -0.75f));
}

FThrowPose FlaskMotion::TippedPose()
{
	// Tipped on up as it empties: the neck flatter, the foot further out and a touch higher, still under the bottom fifth's line.
	return AtMouth(FVector(27.f, -13.f, -19.f), FVector(-0.8f, 0.25f, -0.55f));
}

bool FlaskMotion::IsInHand(float DrinkTime)
{
	return DrinkTime > 0.f && DrinkTime < LowerSeconds;
}

FThrowPose FlaskMotion::FlaskPose(float DrinkTime, bool& bOutShown)
{
	bOutShown = IsInHand(DrinkTime);
	if (DrinkTime <= 0.f || DrinkTime >= LowerSeconds)
	{
		return StartPose();
	}
	if (DrinkTime < RaiseSeconds)
	{
		// Up from the belt, slowing as it reaches the mouth.
		return Blend(StartPose(), RaisedPose(), FMath::InterpEaseOut(0.f, 1.f, DrinkTime / RaiseSeconds, 2.f));
	}
	const float Sip = FFlaskRules::SipAt;
	if (DrinkTime < Sip)
	{
		return Blend(RaisedPose(), AtLipsPose(), FMath::SmoothStep(RaiseSeconds, Sip, DrinkTime));
	}
	if (DrinkTime < TipEndSeconds)
	{
		const FThrowPose Pose = Blend(AtLipsPose(), TippedPose(), FMath::SmoothStep(Sip, TipEndSeconds, DrinkTime));
		// Each gulp a soft half-wave: the foot nods up a few degrees about the lips, then settles.
		const float Through = (DrinkTime - Sip) / (TipEndSeconds - Sip);
		const float Gulp = FMath::Square(FMath::Sin(UE_PI * Gulps * Through));
		return Nodded(Pose, GulpDegrees * Gulp);
	}
	// Lowered away: accelerating down out of sight, so it's gone before the hand goes back to the gun.
	const float Down = (DrinkTime - TipEndSeconds) / (LowerSeconds - TipEndSeconds);
	return Blend(TippedPose(), StartPose(), Down * Down);
}

FVector FlaskMotion::MouthOf(const FThrowPose& Pose)
{
	return Pose.Offset + Pose.Rotation.RotateVector(FVector(0.f, 0.f, HeldMouthUp));
}

FVector FlaskMotion::MiddleOf(const FThrowPose& Pose)
{
	return Pose.Offset + Pose.Rotation.RotateVector(FVector(0.f, 0.f, HeldMiddleUp));
}

float FlaskMotion::GunDipShare(float DrinkTime)
{
	const float End = FFlaskRules::DrinkSeconds;
	if (DrinkTime <= 0.f || DrinkTime >= End)
	{
		return 0.f;
	}
	if (DrinkTime < DipInSeconds)
	{
		return FMath::SmoothStep(0.f, 1.f, DrinkTime / DipInSeconds);
	}
	if (DrinkTime > DipOutFrom)
	{
		return 1.f - FMath::SmoothStep(0.f, 1.f, (DrinkTime - DipOutFrom) / (End - DipOutFrom));
	}
	return 1.f;
}

FThrowPose FlaskMotion::GunPose(float DrinkTime)
{
	// The throw's own dip, held longer: the user asked for the gun out of the way exactly as the grenade does it.
	const float Share = GunDipShare(DrinkTime);
	if (Share <= 0.f)
	{
		return FThrowPose();
	}
	const FThrowPose Deepest = ThrowMotion::GunDipPose();
	FThrowPose Pose;
	Pose.Offset = Deepest.Offset * Share;
	Pose.Rotation = Deepest.Rotation * Share;
	return Pose;
}

FViewKick FlaskMotion::Lift()
{
	FViewKick Kick;
	Kick.Pitch = 0.2f;
	Kick.Yaw = -0.25f;
	Kick.Roll = -0.35f;
	Kick.Frequency = 4.f;
	Kick.Damping = 0.75f;
	return Kick;
}

FViewKick FlaskMotion::Sip()
{
	FViewKick Kick;
	Kick.Pitch = 0.9f;
	Kick.Roll = 0.25f;
	Kick.FieldOfView = -0.008f;
	// Slow and soft: a head tipping back to swallow, not a jolt.
	Kick.Frequency = 2.6f;
	Kick.Damping = 0.8f;
	return Kick;
}
