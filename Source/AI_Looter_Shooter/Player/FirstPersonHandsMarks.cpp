#include "Player/FirstPersonHandsMarks.h"
#include "Player/FirstPersonHandPoses.h"
#include "Player/FirstPersonHandsGrips.h"
#include "HAL/IConsoleManager.h"

// The hands on the grip spots the gun's parts mark (2026-10-29, grip spots; the marker grammar in Weapons/WeaponHandMarks.h).
// A marker says where on the part the palm lies and which way the knuckles point; the pose library says how the fingers
// close and on what. Everything else the hands do (the framing of the forearms, the reloads' paths, the bolt, the parking
// of a pistol's support hand) stays as it was: this only replaces the kinds' hold tables where a gun is marked.

namespace
{
	TAutoConsoleVariable<int32> CVarMarks(TEXT("Looter.Hands.Marks"), 1,
		TEXT("1: the first-person hands hold the grip spots a gun's parts mark (where marked). 0: every gun by its kind's tables, as before."));

	const FName PistolPose(TEXT("Pistol"));
	const FName CupPose(TEXT("Cup"));
	const FName PumpPose(TEXT("Pump"));
	const FName ForendPose(TEXT("Forend"));
	const FName WrapPose(TEXT("Wrap"));
	const FName RevolverPose(TEXT("Revolver"));

	/** What a hand takes from its pose: the library's entry, or today's numbers for a pose it doesn't have. */
	struct FMarkPose
	{
		FHandFingers Fingers;
		/** The pose sets the thumb's base swing and spread (else its curls' own stand). */
		bool bThumb = false;
		float ThumbSwing = 0.f;
		float ThumbSpread = 0.f;
		/** The fingers close by touch on a handle at the marker (else the curls as they are, no contact: today's reload grabs). */
		bool bHandle = false;
		float TouchShare = 1.f;
		/** The handle (cm of the modeled gun, as FHandleSpec: a grip's front to back or a fore-end's height, its width), its rounding. */
		float HalfAcross = 2.f;
		float HalfThrough = 2.f;
		float Corner = 1.f;
		bool bBehindKnuckles = false;
	};

	/** Today's grip a pose stands for (its fallback curls); a name it doesn't know: the trigger hand's grip, or the fore-end's. */
	EHandGrip GripNamed(FName Pose, EFirstPersonHand Hand)
	{
		if (Pose == PistolPose)
		{
			return EHandGrip::Trigger;
		}
		if (Pose == CupPose || Pose == ForendPose)
		{
			return EHandGrip::Forend;
		}
		if (Pose == PumpPose)
		{
			return EHandGrip::Pump;
		}
		if (Pose == WrapPose)
		{
			return EHandGrip::Wrap;
		}
		if (Pose == RevolverPose)
		{
			return EHandGrip::Support;
		}
		return Hand == EFirstPersonHand::Right ? EHandGrip::Trigger : EHandGrip::Forend;
	}

	void SetHandle(FMarkPose& Pose, const float HalfAcross, const float HalfThrough, const float Corner)
	{
		Pose.bHandle = true;
		Pose.HalfAcross = HalfAcross;
		Pose.HalfThrough = HalfThrough;
		Pose.Corner = Corner;
	}

	/**
	 * The handle under the palm at the marker (the gun's space), its sizes read as the tables' FHandleSpec has them (the pose
	 * library's too): HalfThrough the handle's width across the gun, HalfAcross its other short way. A palm on a handle's
	 * side (a pistol grip, a magazine, a six-gun's shooting hand: the palm faces across the gun) sits on its width: the
	 * handle runs across the palm (the marker's Y, so a raked grip leans with its marker), front to back along the knuckles.
	 * A palm under or over it (a fore-end, a pump, a cup: the palm faces up or down the gun) sits on its height: the handle
	 * runs along the gun (whatever way the knuckles point: a cup's go forward and to the right, up its far side), its width
	 * square to that. The face under the palm is at the marker: the middle is that face's half size deep into the part.
	 */
	FHandleSpec HandleAt(const FVector& Fingers, const FVector& Palm, const FVector& AcrossPalm, const FMarkPose& Pose)
	{
		FHandleSpec Spec;
		Spec.HalfAcross = FMath::Max(Pose.HalfAcross, 0.1f);
		Spec.HalfThrough = FMath::Max(Pose.HalfThrough, 0.1f);
		Spec.Corner = FMath::Max(Pose.Corner, 0.f);
		Spec.bValid = true;
		const bool bOnSide = FMath::Abs(Palm.Y) > FMath::Abs(Palm.Z);
		if (bOnSide)
		{
			Spec.Axis = AcrossPalm;
			Spec.Across = Fingers;
			Spec.Offset = Palm * Spec.HalfThrough;
			return Spec;
		}
		const FVector Along = FVector::ForwardVector - Palm * FVector::DotProduct(FVector::ForwardVector, Palm);
		Spec.Axis = Along.GetSafeNormal(UE_SMALL_NUMBER, AcrossPalm);
		Spec.Across = Palm;
		Spec.Offset = Palm * Spec.HalfAcross;
		return Spec;
	}

	FMarkPose PoseFor(FName Name, EFirstPersonHand Hand, const FGunHands& Table)
	{
		FMarkPose Out;
		if (const FirstPersonHandPoses::FHandPose* Pose = FirstPersonHandPoses::Find(Name))
		{
			Out.Fingers = Pose->Fingers;
			Out.bThumb = true;
			Out.ThumbSwing = Pose->ThumbSwing;
			Out.ThumbSpread = Pose->ThumbSpread;
			Out.TouchShare = Pose->TouchShare;
			SetHandle(Out, Pose->HandleHalfAcross, Pose->HandleHalfThrough, Pose->HandleCorner);
			Out.bBehindKnuckles = Pose->bForearmBehindKnuckles;
			return Out;
		}
		// Not in the library (yet): the kind's own curls, thumb and handle for the grip it names.
		const EHandGrip Grip = GripNamed(Name, Hand);
		Out.Fingers = FHandFingers::Of(Grip);
		if (Name == CupPose)
		{
			Out.bThumb = true;
			Out.ThumbSwing = Table.CupThumbSwing;
			Out.ThumbSpread = Table.CupThumbSpread;
			Out.bBehindKnuckles = true;
			if (Table.CupShape.bValid)
			{
				SetHandle(Out, Table.CupShape.HalfAcross, Table.CupShape.HalfThrough, Table.CupShape.Corner);
			}
			return Out;
		}
		switch (Grip)
		{
		case EHandGrip::Trigger:
			for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
			{
				Out.Fingers.Curl[FHandFingers::IndexFinger][Joint] = Table.TriggerCurl[Joint];
			}
			if (Table.GripShape.bValid)
			{
				Out.bThumb = true;
				Out.ThumbSwing = Table.GripThumbSwing;
				Out.ThumbSpread = Table.GripThumbSpread;
				SetHandle(Out, Table.GripShape.HalfAcross, Table.GripShape.HalfThrough, Table.GripShape.Corner);
			}
			break;
		case EHandGrip::Forend:
		case EHandGrip::Pump:
		case EHandGrip::Support:
			if (Table.ForeShape.bValid && Table.ForeContact > 0.f)
			{
				Out.bThumb = true;
				Out.ThumbSwing = Table.ForeThumbSwing;
				Out.ThumbSpread = Table.ForeThumbSpread;
				Out.TouchShare = Table.ForeContact;
				SetHandle(Out, Table.ForeShape.HalfAcross, Table.ForeShape.HalfThrough, Table.ForeShape.Corner);
			}
			break;
		default:
			// A wrap or a pinch (a reload's grab) keeps its curls, as today's reload spots do.
			break;
		}
		return Out;
	}

	/** The cup's wrist bend (Looter.Hands.CupWristBend, FirstPersonHandsCup.cpp's), radians. */
	float CupWristBend()
	{
		static IConsoleVariable* const Bend = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Hands.CupWristBend"));
		return FMath::DegreesToRadians(FMath::Clamp(Bend ? Bend->GetFloat() : 50.f, 0.f, 75.f));
	}
}

bool FirstPersonHandsMarks::UseMarks()
{
	return CVarMarks.GetValueOnAnyThread() != 0;
}

const FHandMark* FirstPersonHandsMarks::Find(const FHandsGun& Gun, EHandMarkRole Role, EFirstPersonHand Hand)
{
	if (!UseMarks())
	{
		return nullptr;
	}
	const FHandMark& Mark = Gun.Marks.Get(Role, FirstPersonHands::Index(Hand));
	return Mark.bValid ? &Mark : nullptr;
}

FHandPlacement FirstPersonHandsMarks::Placement(const FHandsGun& Gun, const FHandMark& Mark, EFirstPersonHand Hand)
{
	const FGunHands Table = FirstPersonHandsGrips::Tuned(Gun.Kind);
	const FMarkPose Pose = PoseFor(Mark.Pose, Hand, Table);
	// The marker's frame (the gun's space): +X the knuckles' way, +Z out of the part toward the palm, so the palm faces -Z;
	// +Y runs across the palm, the way a handle lies in a closed fist.
	const FQuat Turn = Mark.Local.GetRotation().GetNormalized();
	const FVector Fingers = Turn.GetAxisX();
	const FVector Palm = -Turn.GetAxisZ();
	const FVector AcrossPalm = Turn.GetAxisY();
	const FVector Point = Mark.Local.GetLocation();
	// The marker is the palm's skin; the grip point is inside the hand from it (the hand's size, the arms' scale).
	const FVector Hold = Point + Palm * (FirstPersonHands::GripOffPalmSkin * Gun.HandScale);

	FHandFingers Curls = Pose.Fingers;
	// The trigger hand on a pistol hold: the trigger finger on the trigger as far as it's wanted there, else laid along the
	// frame (OnGrip's discipline); the pose's curls are its finger on the trigger.
	const bool bTrigger = Hand == EFirstPersonHand::Right && GripNamed(Mark.Pose, Hand) == EHandGrip::Trigger;
	const float Ready = FMath::Clamp(Gun.TriggerReady, 0.f, 1.f);
	if (bTrigger)
	{
		for (int32 Joint = 0; Joint < FHandFingers::Joints; ++Joint)
		{
			Curls.Curl[FHandFingers::IndexFinger][Joint] = FMath::Lerp(Table.IndexedCurl[Joint], Pose.Fingers.Curl[FHandFingers::IndexFinger][Joint], Ready);
		}
		Curls.Spread[FHandFingers::IndexFinger] = FMath::Lerp(Table.IndexedSpread, Pose.Fingers.Spread[FHandFingers::IndexFinger], Ready);
	}
	if (Pose.bThumb)
	{
		// As the tables' holds set it: the thumb's base swing (its first joint) and spread; touch closes its outer joints.
		Curls.Curl[FHandFingers::Thumb][0] = Pose.ThumbSwing;
		Curls.Spread[FHandFingers::Thumb] = Pose.ThumbSpread;
	}

	// The framing turns a forearm left zero (FPHandsFraming::FrameForearm); a cup's is fitted to the hand instead, straight
	// back behind the knuckles with the wrist bent back only (OnCup's rule, the user's "straighten it out", 2026-10-27).
	FVector Forearm = FVector::ZeroVector;
	if (Pose.bBehindKnuckles)
	{
		const float Bend = CupWristBend();
		Forearm = -Fingers * FMath::Cos(Bend) - Palm * FMath::Sin(Bend);
	}
	FHandPlacement Out = FirstPersonHandsGrips::InGun(Gun, Hold, Fingers, Palm, Curls, Forearm);
	if (Pose.bHandle)
	{
		const FHandleSpec Spec = HandleAt(Fingers, Palm, AcrossPalm, Pose);
		const FVector TriggerSpot = bTrigger ? TriggerAt(Gun, Table.TriggerAt) : FVector::ZeroVector;
		Out.Contact = FirstPersonHandsGrips::ContactOnGun(Gun, Point, Spec, FMath::Clamp(Pose.TouchShare, 0.f, 1.f), TriggerSpot, Ready);
	}
	return Out;
}

bool FirstPersonHandsMarks::Place(const FHandsGun& Gun, EHandMarkRole Role, EFirstPersonHand Hand, FHandPlacement& Out, const FVector& Slide)
{
	const FHandMark* Mark = Find(Gun, Role, Hand);
	if (!Mark)
	{
		return false;
	}
	FHandMark Moved = *Mark;
	Moved.Local.AddToTranslation(Slide * Gun.Scale);
	Out = Placement(Gun, Moved, Hand);
	return true;
}

bool FirstPersonHandsMarks::PoseCups(FName Pose)
{
	if (const FirstPersonHandPoses::FHandPose* Found = FirstPersonHandPoses::Find(Pose))
	{
		return Found->bForearmBehindKnuckles;
	}
	return Pose == CupPose;
}

bool FirstPersonHandsMarks::SupportRest(const FHandsGun& Gun, FHandPlacement& Out)
{
	const FHandMark* Mark = Find(Gun, EHandMarkRole::Hold, EFirstPersonHand::Left);
	// Looter.Hands.Cup 0 still means the fore-end hold of before 2026-10-27: a marked cup gives way to the table's.
	if (!Mark || (PoseCups(Mark->Pose) && !FirstPersonHandsGrips::UseCup()))
	{
		return false;
	}
	Out = Placement(Gun, *Mark, EFirstPersonHand::Left);
	return true;
}

bool FirstPersonHandsMarks::SupportCups(const FHandsGun& Gun)
{
	if (const FHandMark* Mark = Find(Gun, EHandMarkRole::Hold, EFirstPersonHand::Left))
	{
		// A marked cup cups while Looter.Hands.Cup is on (off, SupportRest gives the table's fore-end: no cup); any other
		// marked hold doesn't.
		return PoseCups(Mark->Pose) && FirstPersonHandsGrips::UseCup();
	}
	return FirstPersonHandsGrips::For(Gun.Kind).bCup && FirstPersonHandsGrips::UseCup();
}

FVector FirstPersonHandsMarks::TriggerAt(const FHandsGun& Gun, const FVector& TableAt)
{
	// The marks are drawn to scale (FHandsGun::Marks); the trigger spot is given in the modeled gun's cm (ContactOnGun scales it).
	return UseMarks() && Gun.Marks.bTrigger ? Gun.Marks.Trigger / FMath::Max(Gun.Scale, UE_KINDA_SMALL_NUMBER) : TableAt;
}
