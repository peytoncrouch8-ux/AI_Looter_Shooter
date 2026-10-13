// ADeadMinerCreature's body: its arms laid over the Unpaid's pose (LayerPose, the hook a body on the Unpaid's rig uses, as
// the Bell-Ringer's bell arm does), the pick in its right fist and the pail in its left. Calm, its arms hang and drift as the
// Unpaid's do, the pick held as the model holds it; hunting, it's held low across the body in both hands; the swing heaves it
// overhead (the wind-up), brings it down in front (snapped) and leaves it in the floor (the recovery); at work it's raised
// and driven level into the wall; at the pail both hands are low before it, head bowed; a lamp shot in the wind-up flings
// both arms wide and snaps the head back.
//
// Each arm is laid by where its upper arm and forearm point (component space: X forward, Y its right, Z up), eased toward
// what it wants: a bone's turn is in the rest pose's frame under its parent's, so the upper arm's turn takes its rest
// direction to the wanted one and the forearm's undoes that first. The pick sits in the fist as modelled (on the models'
// pick bone, or held off the hand as the parts table puts it), so it never stands up before the face and the lamp; for the
// swing it's eased onto the right forearm's line out of the fist, its point turned along the blow, and back after.

#include "Creatures/DeadMinerCreature.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Math/RotationMatrix.h"

namespace
{
	FVector Dir(float X, float Y, float Z)
	{
		return FVector(X, Y, Z).GetSafeNormal();
	}

	/** Hunting: held low across the body in both hands. */
	const FVector HuntUpperR = Dir(0.35f, 0.25f, -0.9f);
	const FVector HuntForeR = Dir(0.85f, -0.25f, 0.45f);
	const FVector HuntUpperL = Dir(0.35f, -0.2f, -0.9f);
	const FVector HuntForeL = Dir(0.85f, 0.35f, 0.35f);
	/** The wind-up: both arms up, the pick swung back over the head. */
	const FVector RaisedUpperR = Dir(0.05f, 0.3f, 0.95f);
	const FVector RaisedForeR = Dir(-0.55f, -0.1f, 0.83f);
	const FVector RaisedUpperL = Dir(0.05f, -0.3f, 0.95f);
	const FVector RaisedForeL = Dir(-0.55f, 0.15f, 0.82f);
	/** The blow: both arms swung down before it, the pick into the floor. */
	const FVector BlowUpperR = Dir(0.92f, 0.12f, -0.38f);
	const FVector BlowForeR = Dir(0.75f, -0.05f, -0.66f);
	const FVector BlowUpperL = Dir(0.9f, -0.15f, -0.4f);
	const FVector BlowForeL = Dir(0.72f, 0.1f, -0.68f);
	/** At the wall: raised to the shoulder, then driven level into the rock. */
	const FVector WorkUpUpperR = Dir(0.3f, 0.3f, 0.9f);
	const FVector WorkUpForeR = Dir(-0.25f, -0.05f, 0.97f);
	const FVector WorkUpUpperL = Dir(0.3f, -0.25f, 0.92f);
	const FVector WorkUpForeL = Dir(-0.2f, 0.15f, 0.97f);
	const FVector WorkInUpperR = Dir(0.95f, 0.15f, 0.1f);
	const FVector WorkInForeR = Dir(0.97f, -0.1f, 0.05f);
	const FVector WorkInUpperL = Dir(0.93f, -0.2f, 0.1f);
	const FVector WorkInForeL = Dir(0.96f, 0.15f, 0.05f);
	/** Over its lunch pail: both hands low before the belly. */
	const FVector PailUpperR = Dir(0.25f, 0.2f, -0.95f);
	const FVector PailForeR = Dir(0.8f, -0.45f, 0.2f);
	const FVector PailUpperL = Dir(0.25f, -0.2f, -0.95f);
	const FVector PailForeL = Dir(0.8f, 0.45f, 0.2f);
	/** Its lamp shot: both arms flung out and back. */
	const FVector ReelUpperR = Dir(-0.3f, 0.85f, -0.42f);
	const FVector ReelForeR = Dir(-0.45f, 0.75f, -0.48f);
	const FVector ReelUpperL = Dir(-0.3f, -0.85f, -0.42f);
	const FVector ReelForeL = Dir(-0.45f, -0.75f, -0.48f);

	/** The blow is snapped this long past the wind-up (s); then the pick stays down in the floor. */
	constexpr float BlowSeconds = 0.22f;

	/** How quickly the arms ease (1/s): calm, raised, snapped. */
	constexpr float EaseCalm = 5.f;
	constexpr float EaseRaise = 8.f;
	constexpr float EaseSnap = 26.f;

	/** The layer gives way to the Unpaid's slump this fast as it dies (1/s). */
	constexpr float DeathFade = 2.5f;

	FVector EaseDirection(const FVector& Now, const FVector& Want, float Rate, float DeltaSeconds)
	{
		const float Alpha = DeltaSeconds > 0.f ? 1.f - FMath::Exp(-Rate * DeltaSeconds) : 1.f;
		const FVector Eased = (Now + (Want - Now) * Alpha).GetSafeNormal();
		return Eased.IsNearlyZero() ? Want : Eased;
	}

	FQuat PitchBy(float Degrees)
	{
		return FQuat(FVector::RightVector, FMath::DegreesToRadians(Degrees));
	}

	void Set(ADeadMinerCreature::FMinerArms& Out, const FVector& UpperR, const FVector& ForeR, const FVector& UpperL, const FVector& ForeL,
		float Rate)
	{
		Out.RightUpper = UpperR;
		Out.RightFore = ForeR;
		Out.LeftUpper = UpperL;
		Out.LeftFore = ForeL;
		Out.RightWeight = 1.f;
		Out.LeftWeight = 1.f;
		Out.Rate = Rate;
	}

	/** The pick eases onto the forearm's line for the blow, and back into the fist as held after (1/s). */
	constexpr float PickLineEase = 12.f;
}

void ADeadMinerCreature::SetupArmLayer()
{
	bArmLayerReady = false;
	if (!bRigReady)
	{
		return;
	}
	RightUpper = FindPosedBone(Rig.RightArm.UpperArm);
	RightLower = FindPosedBone(Rig.RightArm.LowerArm);
	RightHand = FindPosedBone(Rig.RightArm.Hand);
	RightKnuckle = Rig.RightArm.Fingers.IsValidIndex(2) ? FindPosedBone(Rig.RightArm.Fingers[2]) : INDEX_NONE;
	LeftUpper = FindPosedBone(Rig.LeftArm.UpperArm);
	LeftLower = FindPosedBone(Rig.LeftArm.LowerArm);
	LeftHand = FindPosedBone(Rig.LeftArm.Hand);
	PelvisIndex = FindPosedBone(Rig.Pelvis);
	SpineIndex = FindPosedBone(Rig.Spine);
	HeadIndex = FindPosedBone(Rig.Head);
	for (const int32 Bone : { RightUpper, RightLower, RightHand, LeftUpper, LeftLower, LeftHand })
	{
		if (!Bones.IsValidIndex(Bone))
		{
			return;
		}
	}
	auto RestDirection = [this](int32 From, int32 To)
	{
		const FVector Way = (Bones[To].Rest.GetLocation() - Bones[From].Rest.GetLocation()).GetSafeNormal();
		return Way.IsNearlyZero() ? FVector::DownVector : Way;
	};
	RestRightUpper = RestDirection(RightUpper, RightLower);
	RestRightFore = RestDirection(RightLower, RightHand);
	RestLeftUpper = RestDirection(LeftUpper, LeftLower);
	RestLeftFore = RestDirection(LeftLower, LeftHand);
	bArmLayerReady = true;
	bArmsStarted = false;
}

void ADeadMinerCreature::WantedArms(FMinerArms& Out) const
{
	const float Drift = 0.04f * FMath::Sin(ArmClock * 0.9f);
	if (Act == EAct::Reel || (GetCreatureState() == ECreatureState::Attack && bSwingBroken))
	{
		Set(Out, ReelUpperR, ReelForeR, ReelUpperL, ReelForeL, EaseSnap);
		Out.HeadPitch = -28.f;
		return;
	}
	if (GetCreatureState() == ECreatureState::Attack)
	{
		const float In = GetStateTime();
		if (In < AttackWindup)
		{
			// Heaved overhead through the wind-up, leaning back into it.
			Set(Out, RaisedUpperR, RaisedForeR, RaisedUpperL, RaisedForeL, EaseRaise);
			Out.Bow = -6.f;
			Out.HeadPitch = -8.f;
		}
		else
		{
			// Brought down in front, snapped, and left in the floor.
			Set(Out, BlowUpperR, BlowForeR, BlowUpperL, BlowForeL, In < AttackWindup + BlowSeconds ? EaseSnap : EaseCalm);
			Out.Bow = 16.f;
			Out.HeadPitch = 8.f;
		}
		return;
	}
	switch (Act)
	{
	case EAct::Work:
	{
		// Raised slowly, driven in at the blow, drawn back out of the rock.
		const float Phase = DeadMinerRules::WorkPhase(WorkTime, Tuning.WorkSwingSeconds);
		const float Blow = DeadMinerRules::WorkBlowShare;
		if (Phase < Blow - 0.12f)
		{
			Set(Out, WorkUpUpperR, WorkUpForeR, WorkUpUpperL, WorkUpForeL, Phase < 0.2f ? EaseCalm : EaseRaise);
			Out.Bow = -3.f;
		}
		else if (Phase < Blow + 0.1f)
		{
			Set(Out, WorkInUpperR, WorkInForeR, WorkInUpperL, WorkInForeL, EaseSnap);
			Out.Bow = 10.f;
		}
		else
		{
			Set(Out, HuntUpperR, HuntForeR, HuntUpperL, HuntForeL, EaseCalm);
			Out.Bow = 4.f;
		}
		return;
	}
	case EAct::Pail:
		Set(Out, PailUpperR, PailForeR, PailUpperL, PailForeL, EaseCalm);
		Out.RightFore = (PailForeR + FVector(0.f, 0.f, Drift)).GetSafeNormal();
		Out.Bow = 18.f;
		Out.HeadPitch = 22.f;
		return;
	default:
		break;
	}
	if (GetTarget())
	{
		Set(Out, HuntUpperR, HuntForeR, HuntUpperL, HuntForeL, EaseRaise);
		Out.Bow = 6.f;
		return;
	}
	// Calm: both arms the Unpaid's own, hanging and drifting, the pick held low in its fist as the model holds it.
	Out.RightWeight = 0.f;
	Out.LeftWeight = 0.f;
	Out.Rate = EaseCalm;
}

void ADeadMinerCreature::LayArm(int32 UpperBone, int32 LowerBone, int32 HandBone, const FVector& RestUpper, const FVector& RestFore,
	const FVector& Upper, const FVector& Fore, float Wrist, float Weight)
{
	// The upper arm turned from its rest to Upper; the forearm's own turn undoes the upper arm's first, so it points along
	// Fore whatever the upper arm did; the hand follows the forearm, the wrist flexed by Wrist.
	const FQuat UpperTurn = FQuat::FindBetweenNormals(RestUpper, Upper);
	const FQuat ForeTurn = FQuat::FindBetweenNormals(RestFore, Fore);
	const FQuat LowerOwn = UpperTurn.Inverse() * ForeTurn;
	const FQuat HandOwn(FVector::RightVector, FMath::DegreesToRadians(Wrist));
	Bones[UpperBone].Own = FQuat::Slerp(Bones[UpperBone].Own, UpperTurn, Weight).GetNormalized();
	Bones[LowerBone].Own = FQuat::Slerp(Bones[LowerBone].Own, LowerOwn, Weight).GetNormalized();
	Bones[HandBone].Own = FQuat::Slerp(Bones[HandBone].Own, HandOwn, Weight).GetNormalized();
}

void ADeadMinerCreature::LayerPose(float DeltaSeconds)
{
	ArmClock += DeltaSeconds;
	if (!bArmLayerReady)
	{
		return;
	}
	FMinerArms Want;
	WantedArms(Want);
	if (!bArmsStarted || DeltaSeconds <= 0.f)
	{
		NowRightUpper = Want.RightUpper;
		NowRightFore = Want.RightFore;
		NowLeftUpper = Want.LeftUpper;
		NowLeftFore = Want.LeftFore;
		NowRightWeight = Want.RightWeight;
		NowLeftWeight = Want.LeftWeight;
		NowBow = Want.Bow;
		NowKneel = Want.Kneel;
		NowHeadPitch = Want.HeadPitch;
		bArmsStarted = true;
	}
	else
	{
		NowRightUpper = EaseDirection(NowRightUpper, Want.RightUpper, Want.Rate, DeltaSeconds);
		NowRightFore = EaseDirection(NowRightFore, Want.RightFore, Want.Rate, DeltaSeconds);
		NowLeftUpper = EaseDirection(NowLeftUpper, Want.LeftUpper, Want.Rate, DeltaSeconds);
		NowLeftFore = EaseDirection(NowLeftFore, Want.LeftFore, Want.Rate, DeltaSeconds);
		NowRightWeight = FMath::FInterpTo(NowRightWeight, Want.RightWeight, DeltaSeconds, 6.f);
		NowLeftWeight = FMath::FInterpTo(NowLeftWeight, Want.LeftWeight, DeltaSeconds, 6.f);
		const float TrunkRate = FMath::Min(Want.Rate, 12.f);
		NowBow = FMath::FInterpTo(NowBow, Want.Bow, DeltaSeconds, TrunkRate);
		NowKneel = FMath::FInterpTo(NowKneel, Want.Kneel, DeltaSeconds, 3.f);
		NowHeadPitch = FMath::FInterpTo(NowHeadPitch, Want.HeadPitch, DeltaSeconds, TrunkRate);
	}
	// Dying, a miner lets go: the Unpaid's slump takes it back (the Foreman holds his kneel).
	const bool bLetGo = IsDead() && !HoldsPoseInDeath();
	LayerWeight = DeltaSeconds > 0.f ? FMath::FInterpConstantTo(LayerWeight, bLetGo ? 0.f : 1.f, DeltaSeconds, DeathFade) : (bLetGo ? 0.f : 1.f);
	LayArm(RightUpper, RightLower, RightHand, RestRightUpper, RestRightFore, NowRightUpper, NowRightFore, Want.Wrist,
		LayerWeight * NowRightWeight);
	LayArm(LeftUpper, LeftLower, LeftHand, RestLeftUpper, RestLeftFore, NowLeftUpper, NowLeftFore, 0.f, LayerWeight * NowLeftWeight);

	// The trunk: bent at the waist, lowered (a kneel), the head nodded, on top of the Unpaid's own motion.
	if (Bones.IsValidIndex(SpineIndex) && !FMath::IsNearlyZero(NowBow, 0.01f))
	{
		Bones[SpineIndex].Own = (Bones[SpineIndex].Own * PitchBy(NowBow * LayerWeight)).GetNormalized();
	}
	if (Bones.IsValidIndex(PelvisIndex) && !FMath::IsNearlyZero(NowKneel, 0.01f))
	{
		Bones[PelvisIndex].Shift.Z -= NowKneel * LayerWeight;
	}
	if (Bones.IsValidIndex(HeadIndex) && !FMath::IsNearlyZero(NowHeadPitch, 0.01f))
	{
		Bones[HeadIndex].Own = (Bones[HeadIndex].Own * PitchBy(NowHeadPitch * LayerWeight)).GetNormalized();
	}
}

// ---------------------------------------------------------------------------
// The pick and the pail in its fists
// ---------------------------------------------------------------------------

void ADeadMinerCreature::GetExtraRigBones(TArray<FName>& OutBones) const
{
	// Posed with the rest (at rest under the hand), so the pick can be laid against it as this frame poses it.
	OutBones.Add(PickBone);
}

bool ADeadMinerCreature::HoldsPickOnLine() const
{
	// The swing: heaved, driven down and left in the floor along the forearm (a broken one is flung wide as held).
	return GetCreatureState() == ECreatureState::Attack && !bSwingBroken && Act != EAct::Reel;
}

void ADeadMinerCreature::PlaceHeldParts()
{
	// As the model holds them (the parts table): the pick on the pick bone (or off the right hand), the pail's handle in the
	// left fist hanging plumb.
	const USkeletalMesh* Model = GetMesh()->GetSkeletalMeshAsset();
	HoldPick(Model, PickAttachBone, PickHeld);
	Pick->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, PickAttachBone);
	Pick->SetRelativeTransform(PickHeld);
	PickAttachIndex = bRigReady ? FindPosedBone(PickAttachBone) : INDEX_NONE;
	PickLineShare = 0.f;
	bPickAsHeld = true;
	PailHeld = PartOnBone(Model, Rig.LeftArm.Hand, PailFromHand, FQuat::Identity);
	Pail->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, Rig.LeftArm.Hand);
	Pail->SetRelativeTransform(PailHeld);
}

void ADeadMinerCreature::UpdatePick(float DeltaSeconds)
{
	if (!Pick)
	{
		return;
	}
	// Onto the forearm's line for the blow, back into the fist as held after; eased, so it never jumps.
	const float Wanted = HoldsPickOnLine() ? 1.f : 0.f;
	PickLineShare = DeltaSeconds > 0.f ? FMath::FInterpTo(PickLineShare, Wanted, DeltaSeconds, PickLineEase) : Wanted;
	if (PickLineShare < 0.002f)
	{
		PickLineShare = 0.f;
		if (!bPickAsHeld)
		{
			Pick->SetRelativeTransform(PickHeld);
			bPickAsHeld = true;
		}
		return;
	}
	if (!bArmLayerReady || !Bones.IsValidIndex(RightHand) || !Bones.IsValidIndex(RightLower) || !Bones.IsValidIndex(PickAttachIndex))
	{
		return;
	}
	bPickAsHeld = false;
	// This frame's pose, solved already: the mesh takes the same pose this frame, so the pick sits where it's set here.
	const FTransform& Holder = Bones[PickAttachIndex].Posed;
	const FTransform AsHeld = PickHeld * Holder;
	const FTransform& HandPose = Bones[RightHand].Posed;
	const FVector Wrist = HandPose.GetLocation();
	const FVector Knuckle = Bones.IsValidIndex(RightKnuckle) ? Bones[RightKnuckle].Posed.GetLocation() : Wrist;
	const FVector Grip = FMath::Lerp(Wrist, Knuckle, 0.8);
	// On the line: the haft runs on along the forearm out of the fist; the point is square to it, leading the swing
	// (forward and up with the pick raised, down into the floor with it driven in).
	FVector Haft = (Wrist - Bones[RightLower].Posed.GetLocation()).GetSafeNormal();
	if (Haft.IsNearlyZero())
	{
		Haft = FVector::ForwardVector;
	}
	FVector Point = FVector::CrossProduct(FVector::RightVector, Haft).GetSafeNormal();
	if (Point.IsNearlyZero())
	{
		Point = FVector::ForwardVector;
	}
	const FQuat OnLine = FRotationMatrix::MakeFromZX(Haft, Point).ToQuat();
	const FTransform Swung(FQuat::Slerp(AsHeld.GetRotation(), OnLine, PickLineShare).GetNormalized(),
		FMath::Lerp(AsHeld.GetLocation(), Grip, static_cast<double>(PickLineShare)), AsHeld.GetScale3D());
	Pick->SetRelativeTransform(Swung.GetRelativeTransform(Holder));
}

void ADeadMinerCreature::UpdatePail()
{
	if (!Pail || !Pail->IsVisible() || !bArmLayerReady || !Bones.IsValidIndex(LeftHand))
	{
		return;
	}
	// Hanging plumb from its handle in the left fist, square to the body, however the hand turns.
	const FTransform& HandPose = Bones[LeftHand].Posed;
	const FTransform Plumb(FQuat::Identity, HandPose.TransformPosition(PailHeld.GetLocation()));
	Pail->SetRelativeTransform(Plumb.GetRelativeTransform(HandPose));
}
