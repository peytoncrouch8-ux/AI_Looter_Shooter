#include "Player/FirstPersonHandsRig.h"

// The hands touching what they hold (FPGRIP, 2026-10-22; the user: "the fingers and hand need to grip the gun better and
// like an actual military officer"): a handle's solid as a rounded box (FHandleShape), the palm seated on it, and each
// finger closed till it touches it, the way the approved look sheet posed them in Blender (Tools/Blender/fp_arms_sheet.py:
// close_fingers, close_thumb, reach_with) but on the handle's shape instead of the gun's mesh, so it runs in the game, on
// every gun and part, and follows any change to a hold. The trigger finger is searched onto the trigger's face. A hand
// held still on its gun reuses its last answer (ContactFingers' cache): the search runs when the hand moves on the gun.
// Also FHandPlacement's own math (made, blended, moved), which carries the contact.

namespace
{
	/** A finger counts as touching once its middle line comes this share of its half thickness from the surface (a glove gives a little). */
	constexpr float TouchSquash = 0.9f;
	/** The trigger finger may press this far into its glove against the grip on its way to the trigger. */
	constexpr float TriggerSquash = 0.6f;
	/** The march's step (degrees) and the halvings that then find the touch between two steps. */
	constexpr float MarchStep = 3.f;
	constexpr int32 MarchHalvings = 6;
	/** How far each joint may close (degrees from straight): knuckle, middle, end; the thumb's two outer joints. */
	constexpr float FingerLimit[FHandFingers::Joints] = { 90.f, 105.f, 80.f };
	constexpr float ThumbLimit[FHandFingers::Joints] = { 90.f, 85.f, 85.f };
	/** Where closing starts (degrees on every joint): nearly straight, so a finger finds the handle from outside it. */
	constexpr float OpenCurl = 5.f;
	/** Closing all three joints together, at these paces (a real finger rolls round a handle, knuckle first). */
	constexpr float TogetherPace[FHandFingers::Joints] = { 1.f, 1.15f, 0.8f };

	FVector SafeAcross(const FHandleShape& Shape, FVector& OutAxis, FVector& OutThrough)
	{
		OutAxis = Shape.Axis.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		FVector Across = Shape.Across - OutAxis * FVector::DotProduct(Shape.Across, OutAxis);
		if (Across.SizeSquared() < 1.e-6)
		{
			Across = FMath::Abs(OutAxis.Z) < 0.9 ? FVector::CrossProduct(OutAxis, FVector::UpVector) : FVector::CrossProduct(OutAxis, FVector::ForwardVector);
		}
		Across = Across.GetSafeNormal();
		OutThrough = FVector::CrossProduct(OutAxis, Across);
		return Across;
	}

	bool NearlySameShape(const FHandleShape& A, const FHandleShape& B)
	{
		return A.bValid == B.bValid && A.Center.Equals(B.Center, 0.02) && FVector::DotProduct(A.Axis.GetSafeNormal(), B.Axis.GetSafeNormal()) > 0.99995
			&& FVector::DotProduct(A.Across.GetSafeNormal(), B.Across.GetSafeNormal()) > 0.99995 && FMath::IsNearlyEqual(A.HalfAcross, B.HalfAcross, 0.01f)
			&& FMath::IsNearlyEqual(A.HalfThrough, B.HalfThrough, 0.01f) && FMath::IsNearlyEqual(A.Corner, B.Corner, 0.01f)
			&& FMath::IsNearlyEqual(A.HalfLength, B.HalfLength, 0.01f);
	}

	/** The curls the solve starts from or falls back on (the trigger finger's left out when the trigger search owns it). */
	bool NearlySameFingers(const FHandFingers& A, const FHandFingers& B, bool bSkipIndex)
	{
		for (int32 Finger = 0; Finger < FHandFingers::Count; ++Finger)
		{
			if (bSkipIndex && Finger == FHandFingers::IndexFinger)
			{
				continue;
			}
			if (!FMath::IsNearlyEqual(A.Spread[Finger], B.Spread[Finger], 0.05f))
			{
				return false;
			}
			for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
			{
				if (!FMath::IsNearlyEqual(A.Curl[Finger][Joint], B.Curl[Finger][Joint], 0.05f))
				{
					return false;
				}
			}
		}
		return true;
	}
}

// ---------------------------------------------------------------------------
// FHandPlacement
// ---------------------------------------------------------------------------

FHandPlacement FHandPlacement::Make(const FVector& InHold, const FVector& FingersDir, const FVector& PalmDir, const FHandFingers& InFingers,
	const FVector& InForearm)
{
	FHandPlacement Placement;
	Placement.Hold = InHold;
	Placement.Turn = FirstPersonHands::Frame(FingersDir, PalmDir);
	Placement.Fingers = InFingers;
	Placement.Forearm = InForearm.GetSafeNormal();
	return Placement;
}

FHandPlacement FHandPlacement::Blend(const FHandPlacement& From, const FHandPlacement& To, float Alpha)
{
	const float Share = FMath::Clamp(Alpha, 0.f, 1.f);
	FHandPlacement Placement;
	Placement.Hold = FMath::Lerp(From.Hold, To.Hold, Share);
	Placement.Turn = FQuat::Slerp(From.Turn, To.Turn, Share).GetNormalized();
	Placement.Fingers = FHandFingers::Blend(From.Fingers, To.Fingers, Share);
	Placement.Contact = FHandContact::Blend(From.Contact, To.Contact, Share);
	// A way only one end has is kept whole (the other's "the hand's usual way" is only known to the rig).
	if (From.Forearm.IsNearlyZero() || To.Forearm.IsNearlyZero())
	{
		Placement.Forearm = Share < 0.5f && !From.Forearm.IsNearlyZero() ? From.Forearm : To.Forearm;
		if (Placement.Forearm.IsNearlyZero())
		{
			Placement.Forearm = From.Forearm;
		}
	}
	else
	{
		Placement.Forearm = FMath::Lerp(From.Forearm, To.Forearm, Share).GetSafeNormal(UE_SMALL_NUMBER, To.Forearm);
	}
	return Placement;
}

FHandPlacement FHandPlacement::Moved(const FTransform& Space) const
{
	FHandPlacement Placement = *this;
	Placement.Hold = Space.TransformPosition(Hold);
	Placement.Turn = (Space.GetRotation() * Turn).GetNormalized();
	Placement.Forearm = Space.GetRotation().RotateVector(Forearm);
	Placement.Contact = Contact.Moved(Space);
	return Placement;
}

// ---------------------------------------------------------------------------
// FHandleShape, FHandContact
// ---------------------------------------------------------------------------

float FHandleShape::Distance(const FVector& Point) const
{
	if (!bValid)
	{
		return 1.e6f;
	}
	FVector AxisWay;
	FVector Through;
	const FVector AcrossWay = SafeAcross(*this, AxisWay, Through);
	const FVector Offset = Point - Center;
	const float Round = FMath::Clamp(Corner, 0.f, FMath::Min(HalfAcross, HalfThrough));
	// The cross-section's rounded rectangle, then (when it has ends) the box's ends.
	const float Qx = static_cast<float>(FMath::Abs(FVector::DotProduct(Offset, AcrossWay))) - (HalfAcross - Round);
	const float Qy = static_cast<float>(FMath::Abs(FVector::DotProduct(Offset, Through))) - (HalfThrough - Round);
	float Section = FVector2f(FMath::Max(Qx, 0.f), FMath::Max(Qy, 0.f)).Size() + FMath::Min(FMath::Max(Qx, Qy), 0.f) - Round;
	if (HalfLength > 0.f)
	{
		const float Qz = static_cast<float>(FMath::Abs(FVector::DotProduct(Offset, AxisWay))) - HalfLength;
		Section = FVector2f(FMath::Max(Section, 0.f), FMath::Max(Qz, 0.f)).Size() + FMath::Min(FMath::Max(Section, Qz), 0.f);
	}
	return Section;
}

FHandleShape FHandleShape::Moved(const FTransform& Space) const
{
	FHandleShape Shape = *this;
	const float Scale = static_cast<float>(Space.GetScale3D().GetAbsMax());
	Shape.Center = Space.TransformPosition(Center);
	Shape.Axis = Space.TransformVectorNoScale(Axis);
	Shape.Across = Space.TransformVectorNoScale(Across);
	Shape.HalfAcross *= Scale;
	Shape.HalfThrough *= Scale;
	Shape.Corner *= Scale;
	Shape.HalfLength *= Scale;
	return Shape;
}

FHandleShape FHandleShape::Blend(const FHandleShape& From, const FHandleShape& To, float Alpha)
{
	if (!From.bValid || !To.bValid)
	{
		return To.bValid ? To : From;
	}
	const float Share = FMath::Clamp(Alpha, 0.f, 1.f);
	FHandleShape Shape;
	Shape.bValid = true;
	Shape.Center = FMath::Lerp(From.Center, To.Center, Share);
	Shape.Axis = FMath::Lerp(From.Axis, To.Axis, Share).GetSafeNormal(UE_SMALL_NUMBER, To.Axis);
	Shape.Across = FMath::Lerp(From.Across, To.Across, Share).GetSafeNormal(UE_SMALL_NUMBER, To.Across);
	Shape.HalfAcross = FMath::Lerp(From.HalfAcross, To.HalfAcross, Share);
	Shape.HalfThrough = FMath::Lerp(From.HalfThrough, To.HalfThrough, Share);
	Shape.Corner = FMath::Lerp(From.Corner, To.Corner, Share);
	Shape.HalfLength = FMath::Lerp(From.HalfLength, To.HalfLength, Share);
	return Shape;
}

FHandContact FHandContact::Moved(const FTransform& Space) const
{
	FHandContact Contact = *this;
	const float Scale = static_cast<float>(Space.GetScale3D().GetAbsMax());
	Contact.Handle = Handle.Moved(Space);
	Contact.TriggerPoint = Space.TransformPosition(TriggerPoint);
	Contact.SeatGap *= Scale;
	Contact.FingerRadius *= Scale;
	return Contact;
}

FHandContact FHandContact::Blend(const FHandContact& From, const FHandContact& To, float Alpha)
{
	// A side without a handle (a reload's spot, the bolt's knob) touches nothing: its shares are none.
	const float Share = FMath::Clamp(Alpha, 0.f, 1.f);
	FHandContact Contact;
	Contact.Handle = FHandleShape::Blend(From.Handle, To.Handle, Share);
	Contact.Share = FMath::Lerp(From.Handle.bValid ? From.Share : 0.f, To.Handle.bValid ? To.Share : 0.f, Share);
	Contact.Seat = FMath::Lerp(From.Handle.bValid ? From.Seat : 0.f, To.Handle.bValid ? To.Seat : 0.f, Share);
	Contact.SeatGap = FMath::Lerp(From.SeatGap, To.SeatGap, Share);
	Contact.FingerRadius = FMath::Lerp(From.FingerRadius, To.FingerRadius, Share);
	Contact.bTrigger = From.bTrigger || To.bTrigger;
	Contact.TriggerShare = FMath::Lerp(From.bTrigger ? From.TriggerShare : 0.f, To.bTrigger ? To.TriggerShare : 0.f, Share);
	Contact.TriggerPoint = From.bTrigger && To.bTrigger ? FMath::Lerp(From.TriggerPoint, To.TriggerPoint, Share) : (To.bTrigger ? To.TriggerPoint : From.TriggerPoint);
	return Contact;
}

// ---------------------------------------------------------------------------
// The rig's side
// ---------------------------------------------------------------------------

bool FFirstPersonHandsRig::FingerPoints(EFirstPersonHand Hand, const FTransform& HandSpace, int32 Finger, const float Curl[FHandFingers::Joints],
	float Spread, FVector Out[4]) const
{
	if (!bBound || Finger < 0 || Finger >= FHandFingers::Count)
	{
		return false;
	}
	const FArm& Arm = Arms[FirstPersonHands::Index(Hand)];
	for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
	{
		if (Arm.Fingers[Finger][Joint] == INDEX_NONE)
		{
			return false;
		}
	}
	FTransform Parent = Arm.FingerBase[Finger] * HandSpace;
	for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
	{
		Parent = JointLocal(Arm, Finger, Joint, Curl[Joint], Joint == 0 ? Spread : 0.f) * Parent;
		Out[Joint] = Parent.GetLocation();
	}
	Out[3] = Parent.TransformPosition(Arm.TipLocal[Finger]);
	return true;
}

FVector FFirstPersonHandsRig::FingerTipIn(EFirstPersonHand Hand, int32 Finger, const TArray<FTransform>& Space) const
{
	const FArm& Arm = Arms[FirstPersonHands::Index(Hand)];
	const int32 End = bBound && Finger >= 0 && Finger < FHandFingers::Count ? Arm.Fingers[Finger][2] : INDEX_NONE;
	return End != INDEX_NONE && Space.IsValidIndex(End) ? Space[End].TransformPosition(Arm.TipLocal[Finger]) : FVector::ZeroVector;
}

FHandPlacement FFirstPersonHandsRig::Seated(const FHandPlacement& Target) const
{
	const FHandContact& Contact = Target.Contact;
	if (Contact.Seat <= 0.f || !Contact.Handle.bValid)
	{
		return Target;
	}
	// The palm's skin under the grip point (the hand's own size: the rig's space is the arms mesh's): slid along the
	// palm's facing till it rests SeatGap off the handle, in or out. A hand far from its handle (on its way to a reload's
	// spot, the parked spot) is let go of: the pull fades out from MaxSeat to twice it. Three passes: on a rounded edge the
	// surface isn't square to the palm, so one slide lands a little off.
	const FVector Palm = Target.GetPalmDir();
	const FVector PalmPoint = Target.Hold - Palm * FirstPersonHands::GripOffPalmSkin;
	const float Share = FMath::Clamp(Contact.Seat, 0.f, 1.f);
	const float FirstGap = Contact.Handle.Distance(PalmPoint) - Contact.SeatGap;
	const float Reach = FMath::Clamp(2.f - FMath::Abs(FirstGap) / MaxSeat, 0.f, 1.f);
	float Slide = 0.f;
	for (int32 Pass = 0; Pass < 3; ++Pass)
	{
		const float Gap = Contact.Handle.Distance(PalmPoint + Palm * Slide) - Contact.SeatGap;
		Slide = FMath::Clamp(Slide + Gap, -MaxSeat, MaxSeat);
	}
	FHandPlacement Seat = Target;
	Seat.Hold += Palm * (Slide * Share * Reach);
	return Seat;
}

FHandFingers FFirstPersonHandsRig::SolveContact(EFirstPersonHand Hand, const FTransform& HandSpace, const FHandPlacement& Target,
	uint8& OutFound) const
{
	OutFound = 0;
	FHandFingers Solved = Target.Fingers;
	const FHandContact& Contact = Target.Contact;
	const FHandleShape& Handle = Contact.Handle;
	const float Touch = Contact.FingerRadius * TouchSquash;

	// Whether a finger posed so touches the handle anywhere from joint FromJoint out (the joint being turned can't be
	// what stops it: its own segment is sampled from a third of the way).
	auto TouchesAt = [&](const FVector (&Points)[4], int32 FromJoint, float Within = 0.f)
	{
		const float Limit = Within > 0.f ? Within : Touch;
		for (int32 Segment = FromJoint; Segment < 3; ++Segment)
		{
			for (const float T : { 0.34f, 0.67f, 1.f })
			{
				if (Handle.Distance(FMath::Lerp(Points[Segment], Points[Segment + 1], T)) < Limit)
				{
					return true;
				}
			}
		}
		return false;
	};
	auto Touches = [&](int32 Finger, const float Curl[FHandFingers::Joints], float Spread, int32 FromJoint)
	{
		FVector Points[4];
		return FingerPoints(Hand, HandSpace, Finger, Curl, Spread, Points) && TouchesAt(Points, FromJoint);
	};

	// Closes the joints by Pace (degrees per step for each) till the finger touches or a joint reaches its limit, then
	// halves the last step to land on the touch. True when it touched.
	auto March = [&](int32 Finger, float Curl[FHandFingers::Joints], float Spread, const float Pace[FHandFingers::Joints], int32 FromJoint,
		const float Limit[FHandFingers::Joints])
	{
		for (int32 Step = 0; Step < 80; ++Step)
		{
			float Trial[FHandFingers::Joints];
			bool bCapped = false;
			for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
			{
				Trial[Joint] = Curl[Joint] + Pace[Joint] * MarchStep;
				bCapped = bCapped || (Pace[Joint] > 0.f && Trial[Joint] > Limit[Joint]);
			}
			if (bCapped)
			{
				return false;
			}
			if (Touches(Finger, Trial, Spread, FromJoint))
			{
				float Free = 0.f;
				float Blocked = 1.f;
				for (int32 Halving = 0; Halving < MarchHalvings; ++Halving)
				{
					const float Middle = 0.5f * (Free + Blocked);
					float Probe[FHandFingers::Joints];
					for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
					{
						Probe[Joint] = Curl[Joint] + Pace[Joint] * MarchStep * Middle;
					}
					(Touches(Finger, Probe, Spread, FromJoint) ? Blocked : Free) = Middle;
				}
				for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
				{
					Curl[Joint] += Pace[Joint] * MarchStep * Free;
				}
				return true;
			}
			FMemory::Memcpy(Curl, Trial, sizeof(Trial));
		}
		return false;
	};

	// The trigger finger is searched onto the trigger only while it's wanted there (an indexed finger keeps its curls).
	const bool bTriggerFinger = Contact.bTrigger;
	const bool bSearchTrigger = bTriggerFinger && Contact.TriggerShare > 0.01f;
	for (int32 Finger = 0; Finger < FHandFingers::Count; ++Finger)
	{
		if (Finger == FHandFingers::IndexFinger && bTriggerFinger)
		{
			continue;
		}
		const float Spread = Target.Fingers.Spread[Finger];
		bool bTouched = false;
		float Curl[FHandFingers::Joints];
		if (Finger == FHandFingers::Thumb)
		{
			// The thumb's base swing is the hold's (round the grip's far side, or laid along a handguard); backed off where
			// that puts its free part in the handle (its first bone, the metacarpal, lies in the palm's pad against the grip
			// anyway), then its two outer joints close till they touch.
			Curl[0] = Target.Fingers.Curl[Finger][0];
			Curl[1] = 0.f;
			Curl[2] = 0.f;
			for (int32 Back = 0; Back < 20 && Touches(Finger, Curl, Spread, 1); ++Back)
			{
				Curl[0] -= 3.f;
			}
			for (int32 Joint = 1; Joint < FHandFingers::Joints; ++Joint)
			{
				float Pace[FHandFingers::Joints] = { 0.f, 0.f, 0.f };
				Pace[Joint] = 1.f;
				bTouched = March(Finger, Curl, Spread, Pace, Joint, ThumbLimit) || bTouched;
			}
		}
		else
		{
			// Nearly straight; lifted where even that is in the handle (the palm sits deep on a thick grip); then all three
			// joints together, then each on its own from the knuckle out, every one till it touches.
			Curl[0] = Curl[1] = Curl[2] = OpenCurl;
			for (int32 Lift = 1; Lift <= 4 && Touches(Finger, Curl, Spread, 0); ++Lift)
			{
				Curl[0] = OpenCurl - 6.f * Lift;
				Curl[1] = OpenCurl - 8.f * Lift;
				Curl[2] = OpenCurl - 5.f * Lift;
			}
			bTouched = March(Finger, Curl, Spread, TogetherPace, 0, FingerLimit);
			for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
			{
				float Pace[FHandFingers::Joints] = { 0.f, 0.f, 0.f };
				Pace[Joint] = 1.f;
				bTouched = March(Finger, Curl, Spread, Pace, Joint, FingerLimit) || bTouched;
			}
		}
		if (bTouched)
		{
			OutFound |= 1 << Finger;
			for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
			{
				Solved.Curl[Finger][Joint] = Curl[Joint];
			}
		}
	}

	if (bSearchTrigger)
	{
		// The trigger finger's pad (past the middle of its end bone) put on the trigger's face: of the poses that stay out
		// of the grip, the nearest, searched coarse then fine (fp_arms_sheet.py's reach_with).
		const int32 Index = FHandFingers::IndexFinger;
		const float BaseSpread = Target.Fingers.Spread[Index];
		float BestDistance = TNumericLimits<float>::Max();
		float Best[4] = { Target.Fingers.Curl[Index][0], Target.Fingers.Curl[Index][1], Target.Fingers.Curl[Index][2], BaseSpread };
		auto Try = [&](float Knuckle, float Middle, float Spread)
		{
			const float Curl[FHandFingers::Joints] = { Knuckle, Middle, Middle * 0.6f };
			FVector Points[4];
			// It may lie against the grip on its way round to the trigger (as a real one does), only not sink into it.
			if (!FingerPoints(Hand, HandSpace, Index, Curl, Spread, Points) || TouchesAt(Points, 0, Contact.FingerRadius * TriggerSquash))
			{
				return;
			}
			const float Distance = static_cast<float>(FVector::Dist(FMath::Lerp(Points[2], Points[3], 0.55f), Contact.TriggerPoint));
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best[0] = Knuckle;
				Best[1] = Middle;
				Best[2] = Middle * 0.6f;
				Best[3] = Spread;
			}
		};
		for (float Spread = -6.f; Spread <= 6.f; Spread += 6.f)
		{
			for (float Knuckle = -10.f; Knuckle <= 80.f; Knuckle += 6.f)
			{
				for (float Middle = 0.f; Middle <= 100.f; Middle += 6.f)
				{
					Try(Knuckle, Middle, BaseSpread + Spread);
				}
			}
		}
		const float Coarse[3] = { Best[0], Best[1], Best[3] };
		for (float Spread = -2.f; Spread <= 2.f; Spread += 2.f)
		{
			for (float Knuckle = -6.f; Knuckle <= 6.f; Knuckle += 2.f)
			{
				for (float Middle = -6.f; Middle <= 6.f; Middle += 2.f)
				{
					Try(Coarse[0] + Knuckle, FMath::Max(Coarse[1] + Middle, 0.f), Coarse[2] + Spread);
				}
			}
		}
		if (BestDistance < TNumericLimits<float>::Max())
		{
			OutFound |= 1 << Index;
			Solved.Curl[Index][0] = Best[0];
			Solved.Curl[Index][1] = Best[1];
			Solved.Curl[Index][2] = Best[2];
			Solved.Spread[Index] = Best[3];
		}
	}
	return Solved;
}

FHandFingers FFirstPersonHandsRig::ContactFingers(EFirstPersonHand Hand, const FTransform& HandSpace, const FHandPlacement& Target) const
{
	const int32 Side = FirstPersonHands::Index(Hand);
	const FHandContact& Contact = Target.Contact;
	if (!bBound || Contact.Share <= 0.f || !Contact.Handle.bValid)
	{
		return Target.Fingers;
	}
	// The answer depends only on where the handle (and the trigger) is from the hand: the same on a gun held still,
	// whatever the gun does on the screen (recoil, sway, a strike).
	const FTransform ToHand = HandSpace.Inverse();
	const FHandleShape HandleInHand = Contact.Handle.Moved(ToHand);
	const FVector TriggerInHand = ToHand.TransformPosition(Contact.TriggerPoint);
	FContactCache& Cache = ContactCache[Side];
	const bool bSearch = Contact.bTrigger && Contact.TriggerShare > 0.01f;
	const bool bSame = Cache.bValid && Cache.bTrigger == bSearch && NearlySameShape(Cache.Handle, HandleInHand)
		&& (!bSearch || Cache.TriggerPoint.Equals(TriggerInHand, 0.02)) && NearlySameFingers(Cache.From, Target.Fingers, Contact.bTrigger);
	if (!bSame)
	{
		uint8 Found = 0;
		Cache.Solved = SolveContact(Hand, HandSpace, Target, Found);
		Cache.Found = Found;
		Cache.Handle = HandleInHand;
		Cache.TriggerPoint = TriggerInHand;
		Cache.From = Target.Fingers;
		Cache.bTrigger = bSearch;
		Cache.bValid = true;
	}

	// The touch-closed fingers by the contact's share (the trigger finger by its own), over the placement's curls; a finger
	// that touched nothing keeps the placement's.
	FHandFingers Out = Target.Fingers;
	for (int32 Finger = 0; Finger < FHandFingers::Count; ++Finger)
	{
		if (!(Cache.Found & (1 << Finger)))
		{
			continue;
		}
		const bool bTrigger = Finger == FHandFingers::IndexFinger && Contact.bTrigger;
		const float Share = FMath::Clamp(bTrigger ? Contact.TriggerShare : Contact.Share, 0.f, 1.f);
		for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
		{
			Out.Curl[Finger][Joint] = FMath::Lerp(Target.Fingers.Curl[Finger][Joint], Cache.Solved.Curl[Finger][Joint], Share);
		}
		Out.Spread[Finger] = FMath::Lerp(Target.Fingers.Spread[Finger], Cache.Solved.Spread[Finger], Share);
	}
	return Out;
}
