#include "Player/PlayerMeleeMotion.h"
#include "Player/PlayerMeleeRules.h"
#include "HAL/IConsoleManager.h"
#include "Math/RotationMatrix.h"

// MeleeMotion's tool arcs: each swing style's arc in the first-person camera's space and the tool's pose along it through
// the swing (the user's picks, 2026-10-27: the sheets in Art/Concepts/MeleeTools say how each tool swings).

namespace
{
	TAutoConsoleVariable<float> CVarArcScale(TEXT("Looter.Melee.ArcScale"), 1.f,
		TEXT("The melee tool's arm reach in first person (a share of each arc's; bigger: the tool swings farther out in the view)."));
	TAutoConsoleVariable<float> CVarWrist(TEXT("Looter.Melee.Wrist"), 1.f,
		TEXT("How far the wrist lays the tool's head back in the wind-up and snaps it through (a share of each arc's)."));
	TAutoConsoleVariable<float> CVarDrawDrop(TEXT("Looter.Melee.DrawDrop"), 32.f,
		TEXT("How far under the frame the tool comes up from as it's drawn, and goes back down to at the end (cm)."));
	TAutoConsoleVariable<float> CVarRecoil(TEXT("Looter.Melee.Recoil"), 0.15f,
		TEXT("A landed blow's bounce back off the body (a share of the wind-up's turn)."));
	TAutoConsoleVariable<float> CVarPistonPush(TEXT("Looter.Melee.PistonPush"), 7.f,
		TEXT("A forearm tool's piston kick past the fist at the blow (cm): Gauge's piston fist."));
	TAutoConsoleVariable<float> CVarAimTipFrom(TEXT("Looter.Melee.AimTipFrom"), 30.f,
		TEXT("A tool whose striking end is this far from the hand or more (cm) has its arc aimed so the end crosses the middle of "
			 "the view at the blow (the spade, the gaff)."));
	TAutoConsoleVariable<float> CVarAimTipDrop(TEXT("Looter.Melee.AimTipDrop"), 4.f,
		TEXT("How far under the crosshair a long tool's end crosses at the blow (cm, camera space)."));
	TAutoConsoleVariable<float> CVarFaceView(TEXT("Looter.Melee.FaceView"), 1.f,
		TEXT("Scales each swing's roll of its tool toward the eye (Ellis's spade shows its blade, not its edge; 0: none)."));

	/** The right shoulder the arm swings from (camera space): under the eye, a little back and out to the right. */
	const FVector RightShoulder(-6.f, 20.f, -30.f);

	/** Shoulder to hand: from RightShoulder to where the hand is at the blow. */
	FVector From(const FVector& HandAtBlow)
	{
		return HandAtBlow - RightShoulder;
	}

	/** An arc whose hand is at HandAtBlow at the blow. */
	FMeleeArc MakeArc(const FVector& HandAtBlow, const FVector& Axis, float Cock, float Through, float Wrist, const FVector& Handle,
		const FVector& Hint)
	{
		FMeleeArc Arc;
		Arc.Shoulder = RightShoulder;
		Arc.ContactDir = From(HandAtBlow).GetSafeNormal();
		Arc.Reach = static_cast<float>(From(HandAtBlow).Size());
		Arc.Axis = Axis.GetSafeNormal();
		Arc.CockDegrees = Cock;
		Arc.ThroughDegrees = Through;
		Arc.WristDegrees = Wrist;
		Arc.HandleAtContact = Handle.GetSafeNormal();
		Arc.StrikeHint = Hint.GetSafeNormal();
		return Arc;
	}

	/** The tool's turn at the blow: X the handle, Z the way the head is moving (or the hint). */
	FQuat ContactFrame(const FMeleeArc& Arc)
	{
		const FVector Handle = Arc.HandleAtContact.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
		// The arm turns from the cock's side through the blow: the head moves the other way round the axis.
		const float Drive = Arc.CockDegrees > 0.f ? -1.f : 1.f;
		FVector Strike = FVector::CrossProduct(Arc.Axis, Arc.ContactDir) * Drive;
		Strike -= Handle * FVector::DotProduct(Handle, Strike);
		if (Arc.bFaceHint || Strike.SizeSquared() < 0.04)
		{
			Strike = Arc.StrikeHint - Handle * FVector::DotProduct(Handle, Arc.StrikeHint);
		}
		return FRotationMatrix::MakeFromXZ(Handle, Strike.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector)).ToQuat();
	}
}

FMeleeArc MeleeMotion::ArcFor(EMeleeSwingStyle Style, EMeleeHeft Heft)
{
	// Every arc keeps the hand inside the view from the wind-up's top through the follow-through (within about 40 degrees
	// across and 24 up or down of the middle): with the gun dropped out of sight for the swing, the tool and the arm are
	// all there is to see (2026-10-27 play-check: the round 3-4 arcs swung the hand out of the frame for all but a tenth
	// of a second, so mid-swing showed nothing). The blow lands just under the crosshair.
	FMeleeArc Arc;
	switch (Style)
	{
	case EMeleeSwingStyle::Flick:
		// Odessa's razor fan: a backhand flick from low on the left up to high on the right, the fan snapping open mid-cut.
		// The swat's tilted axis the other way round: cocked low across the body, through high on the right.
		Arc = MakeArc(FVector(40.f, 2.f, -4.f), FVector(0.f, -0.35f, 0.94f), -28.f, 26.f, 45.f, FVector(0.45f, 0.35f, 0.82f),
			FVector(0.f, 1.f, 0.f));
		Arc.CockReach = 0.9f;
		break;
	case EMeleeSwingStyle::Chop:
		// Straight down from high on the right onto the crosshair: about the view's right-hand axis, tipped a little.
		Arc = MakeArc(FVector(42.f, 6.f, -6.f), FVector(0.f, 1.f, 0.15f), -22.f, 15.f, 40.f, FVector(0.65f, -0.1f, 0.75f),
			FVector(0.f, 0.f, -1.f));
		break;
	case EMeleeSwingStyle::Peck:
		// Pike's rock pick: a short overhand peck, pick first, down and to the left; small and quick, a field hammer's.
		Arc = MakeArc(FVector(40.f, 4.f, -6.f), FVector(0.f, 0.85f, -0.5f), -26.f, 14.f, 30.f, FVector(0.7f, 0.1f, 0.7f),
			FVector(0.f, 0.f, -1.f));
		break;
	case EMeleeSwingStyle::Gaff:
		// Crane's ferry gaff: a big overhand hooking chop, then the hook yanked back toward him (through goes back the cock's
		// way, the reach drawn in).
		Arc = MakeArc(FVector(44.f, 4.f, -6.f), FVector(0.f, 0.95f, -0.3f), -22.f, -12.f, 45.f, FVector(0.6f, -0.15f, 0.78f),
			FVector(0.f, 0.f, -1.f));
		Arc.ThroughReach = 0.75f;
		break;
	case EMeleeSwingStyle::Jab:
		// Gauge's piston fist: drawn back by the shoulder and driven straight into the crosshair, the back of the fist up.
		Arc = MakeArc(FVector(46.f, 4.f, -6.f), FVector::UpVector, 14.f, -3.f, 0.f, FVector(0.95f, -0.25f, 0.1f), FVector::UpVector);
		Arc.CockReach = 0.65f;
		Arc.ThroughReach = 1.06f;
		Arc.bFaceHint = true;
		break;
	default:
		// Ellis's grave spade: a flat backhand swat from high on the right down to low on the left, across the middle. The
		// axis is the view's up tipped toward the left, so the arm's turn falls as it crosses.
		Arc = MakeArc(FVector(42.f, 2.f, -6.f), FVector(0.f, -0.35f, 0.94f), 30.f, -27.f, 35.f, FVector(0.55f, -0.55f, 0.62f),
			FVector(0.f, -1.f, 0.f));
		Arc.CockReach = 0.95f;
		Arc.ThroughReach = 0.95f;
		// Swung flat, the spade's blade showed only its edge (a spear point, DEMO round 3): mostly turned to the eye.
		Arc.FaceView = 0.75f;
		break;
	}
	Arc.Reach *= FMath::Clamp(CVarArcScale.GetValueOnAnyThread(), 0.5f, 1.5f);
	// A heavy tool is wound up farther and its head laid back more; a quick one less. Never past a third of a turn.
	const float Weight = HeftScale(Heft);
	Arc.CockDegrees = FMath::Clamp(Arc.CockDegrees * Weight, -120.f, 120.f);
	Arc.WristDegrees *= Weight * FMath::Clamp(CVarWrist.GetValueOnAnyThread(), 0.f, 2.f);
	return Arc;
}

FTransform MeleeMotion::ArcPose(const FMeleeArc& Arc, float Degrees, float ReachShare, float Wrist, const FVector& Drop)
{
	const FVector Axis = Arc.Axis.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const FQuat Arm(Axis, FMath::DegreesToRadians(Degrees));
	const FVector Hand = Arc.Shoulder + Arm.RotateVector(Arc.ContactDir.GetSafeNormal()) * (Arc.Reach * ReachShare) + Drop;
	// The wrist turns the tool about the same axis as the arm (through the hand): laid back it trails the hand.
	FQuat Turn = FQuat(Axis, FMath::DegreesToRadians(Degrees + Wrist)) * ContactFrame(Arc);
	const float Face = FMath::Clamp(Arc.FaceView * CVarFaceView.GetValueOnAnyThread(), 0.f, 1.f);
	if (Face > 0.f)
	{
		// Rolled about its handle so a face turns toward the eye at the camera's origin: the face nearer the eye at the blow,
		// kept through the whole arc (picking the nearer one each frame would flip it half a turn mid-swing).
		const FVector BlowHand = Arc.Shoulder + Arc.ContactDir.GetSafeNormal() * Arc.Reach;
		const FQuat Blow = ContactFrame(Arc);
		const float Which = FVector::DotProduct(Blow.GetAxisZ(), -BlowHand) >= 0.0 ? 1.f : -1.f;
		const FVector Handle = Turn.GetAxisX();
		FVector ToEye = (-Hand - Handle * FVector::DotProduct(-Hand, Handle)) * Which;
		if (ToEye.Normalize())
		{
			const FVector Side = Turn.GetAxisZ();
			const float Angle = static_cast<float>(FMath::Atan2(FVector::DotProduct(FVector::CrossProduct(Side, ToEye), Handle),
				FVector::DotProduct(Side, ToEye)));
			Turn = FQuat(Handle, Angle * Face) * Turn;
		}
	}
	return FTransform(Turn, Hand);
}

void MeleeMotion::AimAtTip(FMeleeArc& Arc, float TipLength)
{
	if (TipLength < FMath::Max(CVarAimTipFrom.GetValueOnAnyThread(), 1.f))
	{
		return;
	}
	// The hand at the blow, and the point on the view's middle line (a little under it) TipLength away from it.
	const FVector Hand = Arc.Shoulder + Arc.ContactDir.GetSafeNormal() * Arc.Reach;
	const float Below = -CVarAimTipDrop.GetValueOnAnyThread();
	const float Across = FMath::Square(static_cast<float>(Hand.Y)) + FMath::Square(Below - static_cast<float>(Hand.Z));
	const float Ahead = FMath::Sqrt(FMath::Max(FMath::Square(TipLength) - Across, 0.f));
	const FVector Target(Hand.X + Ahead, 0.f, Below);
	const FVector Handle = (Target - Hand).GetSafeNormal();
	if (!Handle.IsNearlyZero())
	{
		Arc.HandleAtContact = Handle;
	}
}

FMeleeToolPose MeleeMotion::ToolPoseOnArc(const FMeleeArc& Arc, float SwingTime, bool bLanded)
{
	FMeleeToolPose Pose;
	const float End = FMeleeRules::SwingSeconds;
	const float Contact = FMeleeRules::ContactSeconds;
	if (SwingTime <= 0.f || SwingTime >= End)
	{
		Pose.Grip = ArcPose(Arc, Arc.CockDegrees, Arc.CockReach, 0.f, FVector(-4.f, 6.f, -CVarDrawDrop.GetValueOnAnyThread()));
		return Pose;
	}
	// The wrist laid back the cock's way (the head trails the hand into the blow).
	const float Side = Arc.CockDegrees < 0.f ? -1.f : 1.f;
	const float Wrist = Side * Arc.WristDegrees;
	const FVector Under(-4.f, 6.f, -FMath::Max(CVarDrawDrop.GetValueOnAnyThread(), 0.f));

	float Degrees = 0.f;
	float Reach = 1.f;
	float Lay = 0.f;
	FVector Drop = FVector::ZeroVector;
	if (SwingTime < CockSeconds)
	{
		// The draw: up from under the frame already turned past the cock, settling into it.
		const float Up = FMath::SmoothStep(0.f, CockSeconds, SwingTime);
		Degrees = Arc.CockDegrees * (1.f + 0.25f * (1.f - Up));
		Reach = FMath::Lerp(Arc.CockReach * 0.85f, Arc.CockReach, Up);
		Lay = Wrist;
		Drop = Under * (1.f - Up);
	}
	else if (SwingTime < Contact)
	{
		// The drive: accelerating into the blow (fastest as it lands: that speed is what reads as weight), the wrist holding
		// the head back till late and snapping it through.
		const float Drive = (SwingTime - CockSeconds) / (Contact - CockSeconds);
		const float Speed = Drive * Drive;
		Degrees = Arc.CockDegrees * (1.f - Speed);
		Reach = FMath::Lerp(Arc.CockReach, 1.f, Speed);
		Lay = Wrist * (1.f - Drive * Drive * Drive);
	}
	else
	{
		const float Since = SwingTime - Contact;
		if (bLanded)
		{
			// Stopped dead on the body (the hit-stop held the clock there), then a little bounce back off it.
			const float Back = FMath::SmoothStep(0.f, 1.f, FMath::Min(Since / RecoilSeconds, 1.f));
			const float Recoil = FMath::Clamp(CVarRecoil.GetValueOnAnyThread(), 0.f, 0.6f);
			Degrees = Arc.CockDegrees * Recoil * Back;
			Lay = Wrist * Recoil * Back;
		}
		else
		{
			// Through: past the blow (or a gaff's yank back), easing out, the head rolling over.
			const float Through = FMath::Sin(UE_HALF_PI * FMath::Min(Since / FollowThroughSeconds, 1.f));
			Degrees = Arc.ThroughDegrees * Through;
			Reach = FMath::Lerp(1.f, Arc.ThroughReach, Through);
			Lay = -0.5f * Wrist * Through;
		}
		// A forearm tool's piston kicks out past the fist at the blow and draws back.
		Pose.Push = FMath::Max(CVarPistonPush.GetValueOnAnyThread(), 0.f) * FMath::Sin(UE_PI * FMath::Min(Since / 0.08f, 1.f));
		// The return: back down under the frame, the reach drawn in.
		const float Away = FMath::SmoothStep(ReturnFrom, End, SwingTime);
		Drop = Under * (1.2f * Away);
		Reach *= FMath::Lerp(1.f, 0.8f, Away);
	}
	Pose.Grip = ArcPose(Arc, Degrees, Reach, Lay, Drop);
	// The hand off the gun's grip onto the tool as it's drawn, and back as it's gone under the frame.
	Pose.HandShare = FMath::SmoothStep(0.f, HandSeconds, SwingTime) * (1.f - FMath::SmoothStep(End - HandSeconds, End, SwingTime));
	Pose.bShown = true;
	return Pose;
}

FMeleeToolPose MeleeMotion::ToolPose(EMeleeSwingStyle Style, EMeleeHeft Heft, float SwingTime, bool bLanded, float TipLength)
{
	FMeleeArc Arc = ArcFor(Style, Heft);
	AimAtTip(Arc, TipLength);
	return ToolPoseOnArc(Arc, SwingTime, bLanded);
}
