// AAbelKeeper's pose: his pose table (AbelPoses, from Abel.py) laid over the Unpaid's body as the Unpaid's code works it out
// each frame. The table has his arms, his props, his skirts and his shroud whenever it lays one; the Unpaid's channels keep
// his trunk alive (the hover, the lean into his drift, the look at his target, a hit's flinch) until he kneels or sits,
// when the table has all of him. Blends run from wherever the last one had got to, so a moment cut short never pops.

#include "Bosses/AbelKeeper.h"
#include "Creatures/CreatureBase.h"

namespace
{
	/** How much of the Unpaid's trunk motion he keeps: he leans and bobs, but less than a hungry Unpaid. */
	constexpr float TrunkShare = 0.7f;

	/** His jaw while he speaks (degrees past closed: 6 to 10), and how quickly it follows. */
	constexpr float SpeakJaw = 8.f;
	constexpr float SpeakJawSwing = 2.f;
	constexpr float JawFollow = 12.f;

	/** The bones whose motion the Unpaid's channels keep, under the table's: his trunk, neck and head. */
	bool IsTrunk(FName Bone)
	{
		static const FName Trunk[] = { TEXT("pelvis"), TEXT("spine_01"), TEXT("spine_02"), TEXT("neck"), TEXT("head") };
		for (const FName& Each : Trunk)
		{
			if (Bone == Each)
			{
				return true;
			}
		}
		return false;
	}

	float Smooth(float Alpha)
	{
		return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f));
	}
}

EAbelPose AAbelKeeper::WantedPose(float& OutBlendSeconds) const
{
	switch (Move)
	{
	case EAbelMove::Grieve:
		OutBlendSeconds = 0.6f;
		return EAbelPose::Sunset;
	case EAbelMove::Pulled:
		OutBlendSeconds = 0.25f;
		return EAbelPose::Sunset;
	case EAbelMove::Flare:
		OutBlendSeconds = 0.3f;
		return EAbelPose::Flare;
	case EAbelMove::Fire:
		OutBlendSeconds = 0.08f;
		return EAbelPose::Fire;
	case EAbelMove::DriftOut:
	case EAbelMove::WalkOff:
		OutBlendSeconds = 0.8f;
		return EAbelPose::Sunset;
	case EAbelMove::InFog:
	case EAbelMove::DragBack:
		OutBlendSeconds = 0.6f;
		return EAbelPose::Idle;
	case EAbelMove::Kneel:
		OutBlendSeconds = 1.2f;
		return EAbelPose::Kneel;
	case EAbelMove::Scene:
		OutBlendSeconds = PoseBlendSeconds;
		return TargetPose;
	default:
		break;
	}
	if (IsDead())
	{
		OutBlendSeconds = 1.2f;
		return EAbelPose::Kneel;
	}
	// The Unpaid's attack is his stock lunge: wound up into it, held through the flight, eased out in the recovery.
	if (IsLunging() || (GetCreatureState() == ECreatureState::Attack && GetStateTime() < AttackWindup + AttackRecovery * 0.4f))
	{
		OutBlendSeconds = 0.22f;
		return EAbelPose::Lunge;
	}
	OutBlendSeconds = 0.45f;
	return EAbelPose::Idle;
}

void AAbelKeeper::BuildTableMap()
{
	const int32 Count = AbelPoses::NumBones();
	TableToRig.SetNum(Count);
	FromOwn.SetNum(Count);
	FromShift.SetNum(Count);
	NowOwn.SetNum(Count);
	NowShift.SetNum(Count);
	for (int32 Bone = 0; Bone < Count; ++Bone)
	{
		TableToRig[Bone] = FindPosedBone(AbelPoses::BoneName(Bone));
		NowOwn[Bone] = FromOwn[Bone] = AbelPoses::Own(TargetPose, Bone);
		NowShift[Bone] = FromShift[Bone] = AbelPoses::Shift(TargetPose, Bone);
	}
	NowLying = FromLying = AbelPoses::LaysShroud(TargetPose) ? 1.f : 0.f;
}

void AAbelKeeper::SetPose(EAbelPose NewPose, float BlendSeconds)
{
	if (TableToRig.IsEmpty() && bRigReady)
	{
		BuildTableMap();
	}
	// From wherever the blend under way has got to.
	FromOwn = NowOwn;
	FromShift = NowShift;
	FromLying = NowLying;
	TargetPose = NewPose;
	PoseBlendTime = 0.f;
	PoseBlendSeconds = FMath::Max(BlendSeconds, 0.f);
}

float AAbelKeeper::GetPoseBlend() const
{
	return PoseBlendSeconds > 0.f ? FMath::Clamp(PoseBlendTime / PoseBlendSeconds, 0.f, 1.f) : 1.f;
}

void AAbelKeeper::LayerPose(float DeltaSeconds)
{
	if (TableToRig.IsEmpty())
	{
		BuildTableMap();
	}
	PoseBlendTime += DeltaSeconds;
	const float Alpha = Smooth(GetPoseBlend());
	// Knelt or sat, the table has all of him: the channels' motion and the shroud's swing blend out.
	NowLying = FMath::Lerp(FromLying, AbelPoses::LaysShroud(TargetPose) ? 1.f : 0.f, Alpha);
	const float Alive = 1.f - NowLying;
	ChainSwing = Alive;

	SpeakClock += DeltaSeconds;
	const float WantedJaw = bSpeaking ? SpeakJaw + SpeakJawSwing * FMath::Sin(SpeakClock * 11.f) : 0.f;
	JawNow = FMath::FInterpTo(JawNow, WantedJaw, DeltaSeconds, JawFollow);

	static const FName Jaw(TEXT("jaw"));
	static const FName Gun(TEXT("gun"));
	for (int32 Bone = 0; Bone < TableToRig.Num(); ++Bone)
	{
		const int32 Rigged = TableToRig[Bone];
		if (!Bones.IsValidIndex(Rigged))
		{
			continue;
		}
		const FName Name = AbelPoses::BoneName(Bone);
		// The stock lunge turns the pump butt-first on its bone; without the flip it keeps its idle turn.
		const EAbelPose Own = TargetPose == EAbelPose::Lunge && Name == Gun && !bLungeFlipsPump ? EAbelPose::Idle : TargetPose;
		NowOwn[Bone] = FQuat::Slerp(FromOwn[Bone], AbelPoses::Own(Own, Bone), Alpha).GetNormalized();
		NowShift[Bone] = FMath::Lerp(FromShift[Bone], AbelPoses::Shift(TargetPose, Bone), Alpha);

		FPosedBone& Posed = Bones[Rigged];
		if (IsTrunk(Name))
		{
			// The table's turn, the Unpaid's motion on top (fading as he kneels or sits).
			Posed.Own = NowOwn[Bone] * FQuat::Slerp(FQuat::Identity, Posed.Own, Alive * TrunkShare);
			Posed.Shift = NowShift[Bone] + Posed.Shift * Alive;
		}
		else if (Name == Jaw)
		{
			// His jaw is closed at rest; it moves while he speaks.
			Posed.Own = NowOwn[Bone] * FQuat(FVector::RightVector, FMath::DegreesToRadians(JawNow));
			Posed.Shift = NowShift[Bone];
		}
		else
		{
			// His arms, fists, props, skirts and the shroud's links (their swing is the chains' while he stands).
			Posed.Own = NowOwn[Bone];
			Posed.Shift = NowShift[Bone];
		}
	}
}
