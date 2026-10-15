#include "Player/Animation/BodySwing.h"
#include "AnimationRuntime.h"
#include "ReferenceSkeleton.h"

namespace
{
	/** World gravity (cm/s^2); each kind takes a share (FBodySwingSettings::Gravity). */
	constexpr float GravityZ = -980.f;
	/** The anchor jumping this far in one frame (cm) is a teleport (a respawn, a lift's end): the chain starts over. */
	constexpr float TeleportDistance = 150.f;
	/** Every step's length (s), whatever the frame rate (FBodySwingPoints::Step). */
	constexpr float FixedStep = 1.f / 120.f;
	/** A frame longer than this (a hitch, a load) counts as this long. */
	constexpr float MaxFrame = 0.1f;
	/** A chain's last bone has no next bone to point at: its tip is as far on as the bone before it (or this, alone). */
	constexpr float LoneBoneLength = 18.f;

	/** Swing-bone chains by name: skirt_fl_01.., cape_bl_02, bandana_01, hair_02, lantern_01. */
	const TCHAR* const ChainPrefixes[] = { TEXT("skirt_"), TEXT("cape_"), TEXT("bandana_"), TEXT("hair_"), TEXT("lantern_") };

	/**
	 * The limbs points keep clear of, and the most they keep from each (cm): about the limb's radius with the clothes on
	 * it. A point keeps its rest distance from a limb, up to this (FBodySwing::Bind), so nothing needs hand-set radii.
	 */
	struct FLimbSpec
	{
		const TCHAR* From;
		const TCHAR* To;
		float MaxClearance;
	};
	const FLimbSpec LimbSpecs[] = {
		{ TEXT("thigh_l"), TEXT("calf_l"), 14.f }, { TEXT("calf_l"), TEXT("foot_l"), 10.f },
		{ TEXT("thigh_r"), TEXT("calf_r"), 14.f }, { TEXT("calf_r"), TEXT("foot_r"), 10.f },
		{ TEXT("pelvis"), TEXT("spine_01"), 18.f }, { TEXT("spine_01"), TEXT("spine_03"), 18.f },
		{ TEXT("spine_03"), TEXT("spine_05"), 18.f }, { TEXT("spine_05"), TEXT("neck_01"), 15.f },
		{ TEXT("neck_01"), TEXT("head"), 9.f },
		{ TEXT("upperarm_l"), TEXT("lowerarm_l"), 9.f }, { TEXT("upperarm_r"), TEXT("lowerarm_r"), 9.f },
		// The forearms and hands: at the game's idle they hang against the coat's sides (they sank 2.5 cm into it).
		{ TEXT("lowerarm_l"), TEXT("hand_l"), 7.f }, { TEXT("lowerarm_r"), TEXT("hand_r"), 7.f },
		{ TEXT("hand_l"), TEXT("middle_01_l"), 5.f }, { TEXT("hand_r"), TEXT("middle_01_r"), 5.f },
	};

	/** A rest point keeps this share of its distance from a limb (a little give, so it isn't shoved at rest). */
	constexpr float ClearanceShare = 0.95f;

	/**
	 * A bone's nearest place to a limb this close to its head (a share of its length) is left to the point above: turning
	 * the bone there would swing its tip by many times the overlap.
	 */
	constexpr double MinBoneAlong = 0.2;

	/** The swing points keep this far over the ground at his feet (cm): the ankle-length hem sank 11 cm into it crouching. */
	constexpr float FloorClearance = 2.f;

	/**
	 * How far out a chain's last point lies on the ground, a share of its bone's length (FBodySwingFloor::LastReach): laid
	 * out at full length the hem's flaps jutted out stiffly round a crouch; nearer, the bone shortens and the hem gathers.
	 */
	constexpr float GatherReach = 1.f / 3.f;

	/** A bone shortens on an axis of its own only when its tip lies this close along it (the normalised component). */
	constexpr double LengthAxisShare = 0.9;

	/**
	 * How much of the upper arm's movement the cape's corners take (FChain::FollowWeight): the front ones lie over the
	 * arms, the back ones over the shoulder blades.
	 */
	constexpr float CapeFollowFront = 0.6f;
	constexpr float CapeFollowBack = 0.4f;

	/** The name's chain ("skirt_fl" of "skirt_fl_02") and place in it (2), or false when it isn't a swing bone. */
	bool ParseSwingBone(const FString& Name, FString& OutChain, int32& OutIndex)
	{
		const FString Lower = Name.ToLower();
		bool bSwing = false;
		for (const TCHAR* Prefix : ChainPrefixes)
		{
			bSwing |= Lower.StartsWith(Prefix);
		}
		int32 Underscore = INDEX_NONE;
		if (!bSwing || !Lower.FindLastChar(TEXT('_'), Underscore) || Underscore + 1 >= Lower.Len())
		{
			return false;
		}
		const FString Number = Lower.Mid(Underscore + 1);
		if (!Number.IsNumeric())
		{
			return false;
		}
		OutChain = Lower.Left(Underscore);
		OutIndex = FCString::Atoi(*Number);
		return true;
	}
}

// ---------------------------------------------------------------------------
// FBodySwingPoints
// ---------------------------------------------------------------------------

void FBodySwingPoints::Reset(TConstArrayView<FVector> Targets)
{
	Points.Reset();
	Points.Append(Targets.GetData(), Targets.Num());
	Previous = Points;
	Shown = Points;
	Accumulator = 0.f;
	bStarted = true;
}

FVector FBodySwingPoints::PushOut(const FVector& P, const FBodySwingCapsule& Capsule)
{
	const FVector Closest = FMath::ClosestPointOnSegment(P, Capsule.A, Capsule.B);
	const FVector Away = P - Closest;
	const double Distance = Away.Size();
	if (Distance >= Capsule.Radius)
	{
		return P;
	}
	const FVector Out = Distance > UE_KINDA_SMALL_NUMBER ? Away / Distance : FVector::ForwardVector;
	return Closest + Out * Capsule.Radius;
}

FVector FBodySwingPoints::PushBoneOut(const FVector& Head, const FVector& Tip, const FBodySwingCapsule& Capsule,
	const FVector& Outward)
{
	if (Capsule.BoneRadius <= 0.f)
	{
		return Tip;
	}
	FVector OnBone;
	FVector OnLimb;
	FMath::SegmentDistToSegmentSafe(Head, Tip, Capsule.A, Capsule.B, OnBone, OnLimb);
	const FVector Away = OnBone - OnLimb;
	const double Distance = Away.Size();
	const double Length = FVector::Dist(Head, Tip);
	if (Distance >= Capsule.BoneRadius || Length <= UE_KINDA_SMALL_NUMBER)
	{
		return Tip;
	}
	const double Along = FVector::Dist(Head, OnBone) / Length;
	if (Along < MinBoneAlong)
	{
		return Tip;
	}
	// Out the body's side of the limb, square to the limb and the bone (turning the bone about its head): the shortest
	// way apart pointed sideways when the two lay in one front-to-back plane, and a stride's back shin swung the coat's
	// back panel out round its side.
	const FVector Axis = (Capsule.B - Capsule.A).GetSafeNormal();
	const FVector Side = FVector::VectorPlaneProject(FVector::VectorPlaneProject(Outward, Axis), (Tip - Head) / Length).GetSafeNormal();
	FVector Out;
	double Overlap = Capsule.BoneRadius - Distance;
	if (!Side.IsZero())
	{
		Out = Side;
		if (Distance > UE_KINDA_SMALL_NUMBER && FVector::DotProduct(Away / Distance, Side) < -0.5)
		{
			// The limb has come through to the cloth's outer side (clearly: near a limb's end the way apart can point
			// anywhere): back across it.
			Overlap = Capsule.BoneRadius + Distance;
		}
	}
	else
	{
		Out = Distance > UE_KINDA_SMALL_NUMBER ? Away / Distance :
			FVector::VectorPlaneProject(Tip - OnLimb, Axis).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	}
	// The tip moves by the overlap over how far down the bone it is, so the bone's nearest place clears by the overlap.
	return Tip + Out * (Overlap / Along);
}

FVector FBodySwingPoints::KeepAbove(const FVector& Head, const FVector& Tip, double Length, double FloorZ, const FVector& Lean,
	double MaxReach)
{
	if (Tip.Z >= FloorZ)
	{
		return Tip;
	}
	FVector Flat(Tip.X - Head.X, Tip.Y - Head.Y, 0.0);
	if (!Flat.Normalize())
	{
		Flat = FVector(Lean.X, Lean.Y, 0.0);
		if (!Flat.Normalize())
		{
			Flat = FVector::ForwardVector;
		}
	}
	// As low as it can hang, on the floor, out the way it leans (it lies on the ground, not through it), no further out
	// than MaxReach.
	const double Drop = FMath::Clamp(Head.Z - FloorZ, 0.0, Length);
	const double Reach = FMath::Min(FMath::Sqrt(Length * Length - Drop * Drop), FMath::Max(MaxReach, 0.0));
	return Head + Flat * Reach - FVector(0.0, 0.0, Drop);
}

void FBodySwingPoints::Step(const FVector& Anchor, TConstArrayView<FVector> Targets, TConstArrayView<float> Lengths,
	TConstArrayView<FBodySwingCapsule> Capsules, const FBodySwingSettings& Settings, float DeltaSeconds,
	const FBodySwingFloor& Floor)
{
	// How far out each point may lie on the floor: its length, but the last its share (the hem gathers).
	auto ReachOf = [&](int32 Index) {
		return static_cast<double>(Lengths[Index]) * (Index == Targets.Num() - 1 ? Floor.LastReach : 1.f);
	};
	const int32 Count = Targets.Num();
	if (!bStarted || Points.Num() != Count || FVector::Dist(Anchor, LastAnchor) > TeleportDistance)
	{
		Reset(Targets);
		LastAnchor = Anchor;
		return;
	}
	// A fixed step, the leftover carried to the next frame: chains joined by length constraints swing differently at
	// different step sizes (a stretch of 1/60 s steps and one of 1/240 s ended 20 cm apart), so every frame rate takes the
	// same steps. A spring toward the animated pose (an acceleration: a nudge of the position each step was a stronger
	// spring the shorter the step), damped, under a share of gravity; Verlet.
	const float Frame = FMath::Clamp(DeltaSeconds, 0.f, MaxFrame);
	const float Carried = Accumulator;
	Accumulator += Frame;
	const float Keep = FMath::Exp(-Settings.Damping * FixedStep);
	const float Spring = Settings.Stiffness * Settings.Stiffness;
	const FVector Gravity(0.0, 0.0, GravityZ * Settings.Gravity);
	const int32 PerPoint = Capsules.Num() / FMath::Max(Count, 1);
	int32 Stepped = 0;
	while (Accumulator >= FixedStep - UE_KINDA_SMALL_NUMBER)
	{
		// Where the anchor (and the animated pose with it) was when this step ends, between last frame's and this one's.
		const float Along = Frame > 0.f ? FMath::Min(1.f, (FixedStep * static_cast<float>(Stepped + 1) - Carried) / Frame) : 1.f;
		const FVector Head0 = FMath::Lerp(LastAnchor, Anchor, static_cast<double>(Along));
		const FVector Shift = Head0 - Anchor;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Velocity = (Points[Index] - Previous[Index]) * Keep;
			const FVector Accel = (Targets[Index] + Shift - Points[Index]) * Spring + Gravity;
			Previous[Index] = Points[Index];
			Points[Index] += Velocity + Accel * (FixedStep * FixedStep);
		}
		// Each point its length from the one before, and out of the limbs, from the anchor down.
		FVector Head = Head0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Rest = (Targets[Index] + Shift - Head).GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector);
			FVector P = Head + (Points[Index] - Head).GetSafeNormal(UE_SMALL_NUMBER, Rest) * Lengths[Index];
			const FVector Free = P;
			for (int32 Limb = 0; Limb < PerPoint; ++Limb)
			{
				const FBodySwingCapsule& Capsule = Capsules[Index * PerPoint + Limb];
				P = PushOut(P, Capsule);
				P = PushBoneOut(Head, P, Capsule, Outward);
			}
			P = Head + (P - Head).GetSafeNormal(UE_SMALL_NUMBER, Rest) * Lengths[Index];
			const bool bGrounded = P.Z < Floor.Z;
			P = KeepAbove(Head, P, Lengths[Index], Floor.Z, Rest, ReachOf(Index));
			// A limb moves the cloth aside without throwing it: the push isn't carried on as speed (it flung a chain over
			// the thigh it was pushed off). On the ground it rests (its speed into the ground, kept, flipped a gathered
			// hem up over its head).
			Previous[Index] += P - Free;
			if (bGrounded)
			{
				Previous[Index] = P;
			}
			Points[Index] = P;
			Head = P;
		}
		Accumulator -= FixedStep;
		++Stepped;
	}
	LastAnchor = Anchor;

	// Shown: between the last two steps by the leftover (no stutter when a frame takes no step or two), hung from where
	// the anchor is now, each point its length from the one before.
	const float Blend = FMath::Clamp(Accumulator / FixedStep, 0.f, 1.f);
	Shown.SetNum(Count);
	FVector Head = Anchor;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Between = FMath::Lerp(Previous[Index], Points[Index], static_cast<double>(Blend));
		const FVector Rest = (Targets[Index] - Head).GetSafeNormal(UE_SMALL_NUMBER, FVector::DownVector);
		Shown[Index] = Head + (Between - Head).GetSafeNormal(UE_SMALL_NUMBER, Rest) * Lengths[Index];
		Shown[Index] = KeepAbove(Head, Shown[Index], Lengths[Index], Floor.Z, Rest, ReachOf(Index));
		Head = Shown[Index];
	}
}

// ---------------------------------------------------------------------------
// FBodySwing
// ---------------------------------------------------------------------------

FBodySwingSettings FBodySwing::SettingsFor(const FString& ChainName)
{
	FBodySwingSettings Settings;
	// Gravity is a small share: the animated pose already hangs, and the spring would hold a full share's sag off it
	// (g x share / stiffness^2: 2 cm on the skirt at 0.08).
	if (ChainName.StartsWith(TEXT("skirt")))
	{
		// The duster's heavy oilskin: swings wide on a turn or a stop and takes a moment to settle.
		Settings.Stiffness = 7.f;
		Settings.Damping = 3.5f;
		Settings.Gravity = 0.08f;
	}
	else if (ChainName.StartsWith(TEXT("cape")))
	{
		// Short and held by the shoulders: a lift and a flutter.
		Settings.Stiffness = 12.f;
		Settings.Damping = 5.f;
		Settings.Gravity = 0.05f;
	}
	else if (ChainName.StartsWith(TEXT("bandana")) || ChainName.StartsWith(TEXT("hair")))
	{
		Settings.Stiffness = 9.f;
		Settings.Damping = 3.5f;
		Settings.Gravity = 0.05f;
	}
	else if (ChainName.StartsWith(TEXT("lantern")))
	{
		// A pendulum on its wire: gravity swings it (it hangs straight down at rest, so there's no sag), the spring
		// barely holds it, slow to settle.
		Settings.Stiffness = 2.f;
		Settings.Damping = 1.2f;
		Settings.Gravity = 1.f;
	}
	return Settings;
}

bool FBodySwing::IsBoundTo(const FReferenceSkeleton& Skeleton) const
{
	return BoundBoneCount == Skeleton.GetNum() && Skeleton.GetNum() > 0 && BoundFirstBone == Skeleton.GetBoneName(0);
}

void FBodySwing::Bind(const FReferenceSkeleton& Skeleton)
{
	Chains.Reset();
	Limbs.Reset();
	bReferencesValid = false;
	BoundBoneCount = Skeleton.GetNum();
	BoundFirstBone = BoundBoneCount > 0 ? Skeleton.GetBoneName(0) : NAME_None;

	TArray<FTransform> Rest;
	FAnimationRuntime::FillUpComponentSpaceTransforms(Skeleton, Skeleton.GetRefBonePose(), Rest);

	// The chains: bones grouped by name, in their numbered order.
	TMap<FString, TArray<TPair<int32, int32>>> Found;
	for (int32 Bone = 0; Bone < Skeleton.GetNum(); ++Bone)
	{
		FString Chain;
		int32 Place = 0;
		if (ParseSwingBone(Skeleton.GetBoneName(Bone).ToString(), Chain, Place))
		{
			Found.FindOrAdd(Chain).Add(TPair<int32, int32>(Place, Bone));
		}
	}

	struct FLimbRest
	{
		FVector A;
		FVector B;
		float MaxClearance;
	};
	TArray<FLimbRest> LimbRests;
	for (const FLimbSpec& Spec : LimbSpecs)
	{
		const int32 From = Skeleton.FindBoneIndex(Spec.From);
		const int32 To = Skeleton.FindBoneIndex(Spec.To);
		if (From == INDEX_NONE || To == INDEX_NONE)
		{
			continue;
		}
		FLimb& Limb = Limbs.AddDefaulted_GetRef();
		Limb.From = FBoneReference(Skeleton.GetBoneName(From));
		Limb.To = FBoneReference(Skeleton.GetBoneName(To));
		Limb.MaxClearance = Spec.MaxClearance;
		LimbRests.Add({ Rest[From].GetLocation(), Rest[To].GetLocation(), Spec.MaxClearance });
	}
	const int32 Pelvis = Skeleton.FindBoneIndex(TEXT("pelvis"));

	for (TPair<FString, TArray<TPair<int32, int32>>>& Entry : Found)
	{
		TArray<TPair<int32, int32>>& Ordered = Entry.Value;
		Ordered.Sort([](const TPair<int32, int32>& A, const TPair<int32, int32>& B) { return A.Key < B.Key; });
		FChain& Chain = Chains.AddDefaulted_GetRef();
		Chain.Name = Entry.Key;
		Chain.Settings = SettingsFor(Entry.Key);
		// The cape's corners ride on the upper arms (cape_fl: front, the figure's left), the front ones more.
		int32 FollowIndex = INDEX_NONE;
		if (Chain.Name.StartsWith(TEXT("cape_")) && Chain.Name.Len() >= 7)
		{
			const TCHAR Side = Chain.Name[6];
			FollowIndex = Skeleton.FindBoneIndex(Side == TEXT('l') ? TEXT("upperarm_l") : TEXT("upperarm_r"));
			Chain.FollowWeight = Chain.Name[5] == TEXT('f') ? CapeFollowFront : CapeFollowBack;
		}
		if (FollowIndex != INDEX_NONE)
		{
			Chain.Follow = FBoneReference(Skeleton.GetBoneName(FollowIndex));
		}
		else
		{
			Chain.FollowWeight = 0.f;
		}
		// Where the chain's cloth ends (Ellis.py's unweighted `<chain>_end`): an FBX bone has no tail, and the guess past
		// the last bone fell 11 cm short of the ankle-length hem, which then went through the ground in a crouch.
		const int32 End = Skeleton.FindBoneIndex(FName(*(Entry.Key + TEXT("_end"))));
		for (int32 Index = 0; Index < Ordered.Num(); ++Index)
		{
			const int32 Bone = Ordered[Index].Value;
			const FVector Head = Rest[Bone].GetLocation();
			FVector Tip;
			if (Ordered.IsValidIndex(Index + 1))
			{
				Tip = Rest[Ordered[Index + 1].Value].GetLocation();
			}
			else if (End != INDEX_NONE)
			{
				Tip = Rest[End].GetLocation();
			}
			else if (Index > 0)
			{
				const FVector Before = Rest[Ordered[Index - 1].Value].GetLocation();
				Tip = Head + (Head - Before);
			}
			else
			{
				Tip = Head + FVector(0.0, 0.0, -LoneBoneLength);
			}
			Chain.Bones.Add(FBoneReference(Skeleton.GetBoneName(Bone)));
			const FVector TipInBone = Rest[Bone].InverseTransformPosition(Tip);
			Chain.TipInBone.Add(TipInBone);
			const FVector Along = TipInBone.GetSafeNormal();
			const int32 Axis = FMath::Abs(Along.X) >= LengthAxisShare ? 0 : FMath::Abs(Along.Y) >= LengthAxisShare ? 1 :
				FMath::Abs(Along.Z) >= LengthAxisShare ? 2 : INDEX_NONE;
			Chain.LengthAxes.Add(Axis);
			Chain.TipInFollow.Add(FollowIndex != INDEX_NONE ? Rest[FollowIndex].InverseTransformPosition(Tip) : FVector::ZeroVector);
			Chain.Lengths.Add(static_cast<float>(FVector::Dist(Head, Tip)));
			for (const FLimbRest& Limb : LimbRests)
			{
				const double Distance = FVector::Dist(Tip, FMath::ClosestPointOnSegment(Tip, Limb.A, Limb.B));
				Chain.Clearances.Add(FMath::Min(static_cast<float>(Distance) * ClearanceShare, Limb.MaxClearance));
				FVector OnBone;
				FVector OnLimb;
				FMath::SegmentDistToSegmentSafe(Head, Tip, Limb.A, Limb.B, OnBone, OnLimb);
				const double BoneDistance = FVector::Dist(OnBone, OnLimb);
				Chain.BoneClearances.Add(FMath::Min(static_cast<float>(BoneDistance) * ClearanceShare, Limb.MaxClearance));
			}
		}
		// Its way out of the body: level, from the body's middle (the pelvis) to where it ends.
		const FVector LastTip = Rest[Ordered.Last().Value].TransformPosition(Chain.TipInBone.Last());
		const FVector Middle = Pelvis != INDEX_NONE ? Rest[Pelvis].GetLocation() : FVector::ZeroVector;
		const FVector Outward = FVector(LastTip.X - Middle.X, LastTip.Y - Middle.Y, 0.0).GetSafeNormal();
		Chain.OutwardInBone = Rest[Ordered[0].Value].InverseTransformVectorNoScale(Outward);
	}
}

void FBodySwing::InitBoneReferences(const FBoneContainer& Bones)
{
	for (FLimb& Limb : Limbs)
	{
		Limb.From.Initialize(Bones);
		Limb.To.Initialize(Bones);
	}
	for (FChain& Chain : Chains)
	{
		for (FBoneReference& Bone : Chain.Bones)
		{
			Bone.Initialize(Bones);
		}
		if (Chain.FollowWeight > 0.f)
		{
			Chain.Follow.Initialize(Bones);
		}
	}
	BoundSerial = Bones.GetSerialNumber();
	bReferencesValid = true;
}

void FBodySwing::Apply(FCSPose<FCompactPose>& Pose, const FTransform& ComponentToWorld, float DeltaSeconds)
{
	const FBoneContainer& Bones = Pose.GetPose().GetBoneContainer();
	if (!bReferencesValid || BoundSerial != Bones.GetSerialNumber())
	{
		InitBoneReferences(Bones);
	}
	const double Scale = ComponentToWorld.GetMaximumAxisScale();
	// The ground at his feet: the component's origin (the mannequin's root stays on the floor, crouched too).
	const double FloorZ = ComponentToWorld.GetLocation().Z + FloorClearance * Scale;

	// The limbs as they are this frame (world space).
	TArray<FBodySwingCapsule, TInlineAllocator<16>> LimbsNow;
	for (const FLimb& Limb : Limbs)
	{
		const FCompactPoseBoneIndex From = Limb.From.GetCompactPoseIndex(Bones);
		const FCompactPoseBoneIndex To = Limb.To.GetCompactPoseIndex(Bones);
		FBodySwingCapsule& Capsule = LimbsNow.AddDefaulted_GetRef();
		if (From.IsValid() && To.IsValid())
		{
			Capsule.A = ComponentToWorld.TransformPosition(Pose.GetComponentSpaceTransform(From).GetLocation());
			Capsule.B = ComponentToWorld.TransformPosition(Pose.GetComponentSpaceTransform(To).GetLocation());
		}
	}

	TArray<FBoneTransform, TInlineAllocator<48>> Moved;
	for (FChain& Chain : Chains)
	{
		const int32 Count = Chain.Bones.Num();
		TArray<FCompactPoseBoneIndex, TInlineAllocator<4>> Indices;
		bool bWhole = Count > 0;
		for (const FBoneReference& Bone : Chain.Bones)
		{
			const FCompactPoseBoneIndex Index = Bone.GetCompactPoseIndex(Bones);
			bWhole &= Index.IsValid();
			Indices.Add(Index);
		}
		if (!bWhole)
		{
			// A LOD without the chain: it starts over when it's back.
			Chain.Sim.bStarted = false;
			continue;
		}

		// Where the animation has the chain this frame.
		TArray<FTransform, TInlineAllocator<4>> Animated;
		TArray<FVector, TInlineAllocator<4>> Targets;
		TArray<float, TInlineAllocator<4>> Lengths;
		const FCompactPoseBoneIndex FollowIndex = Chain.FollowWeight > 0.f ? Chain.Follow.GetCompactPoseIndex(Bones) : FCompactPoseBoneIndex(INDEX_NONE);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FTransform& InComponent = Pose.GetComponentSpaceTransform(Indices[Index]);
			Animated.Add(InComponent);
			FVector Target = InComponent.TransformPosition(Chain.TipInBone[Index]);
			if (FollowIndex.IsValid())
			{
				const FVector Riding = Pose.GetComponentSpaceTransform(FollowIndex).TransformPosition(Chain.TipInFollow[Index]);
				Target = FMath::Lerp(Target, Riding, static_cast<double>(Chain.FollowWeight));
			}
			Targets.Add(ComponentToWorld.TransformPosition(Target));
			Lengths.Add(Chain.Lengths[Index] * static_cast<float>(Scale));
		}
		TArray<FBodySwingCapsule, TInlineAllocator<64>> Capsules;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			for (int32 Limb = 0; Limb < LimbsNow.Num(); ++Limb)
			{
				FBodySwingCapsule Capsule = LimbsNow[Limb];
				Capsule.Radius = Chain.Clearances[Index * LimbsNow.Num() + Limb] * static_cast<float>(Scale);
				Capsule.BoneRadius = Chain.BoneClearances[Index * LimbsNow.Num() + Limb] * static_cast<float>(Scale);
				Capsules.Add(Capsule);
			}
		}
		const FVector Anchor = ComponentToWorld.TransformPosition(Animated[0].GetLocation());
		FBodySwingFloor Floor;
		Floor.Z = FloorZ;
		// The hem gathers on the ground only when its bone can shorten (along an axis of its own).
		Floor.LastReach = Chain.LengthAxes.Last() != INDEX_NONE ? GatherReach : 1.f;
		// Its way out of the body as the body is turned and leans now.
		Chain.Sim.Outward = ComponentToWorld.TransformVectorNoScale(Animated[0].TransformVectorNoScale(Chain.OutwardInBone));
		Chain.Sim.Step(Anchor, Targets, Lengths, Capsules, Chain.Settings, DeltaSeconds, Floor);

		// Each bone turned from its animated way to the simulated one, its head on the tip before it; one the floor
		// gathered (its tip nearer than its length) shortened along its length.
		FVector Head = Animated[0].GetLocation();
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Tip = ComponentToWorld.InverseTransformPosition(Chain.Sim.Shown[Index]);
			const FVector From = (Animated[Index].TransformPosition(Chain.TipInBone[Index]) - Animated[Index].GetLocation()).GetSafeNormal();
			const FVector To = (Tip - Head).GetSafeNormal();
			FTransform Bone = Animated[Index];
			if (!From.IsNearlyZero() && !To.IsNearlyZero())
			{
				Bone.SetRotation((FQuat::FindBetweenNormals(From, To) * Bone.GetRotation()).GetNormalized());
			}
			const int32 Axis = Chain.LengthAxes[Index];
			const float Share = Chain.Lengths[Index] > UE_KINDA_SMALL_NUMBER ?
				static_cast<float>(FVector::Dist(Head, Tip)) / Chain.Lengths[Index] : 1.f;
			if (Axis != INDEX_NONE && Share < 0.995f)
			{
				FVector Scale3D = Bone.GetScale3D();
				Scale3D[Axis] *= FMath::Max(Share, 0.05f);
				Bone.SetScale3D(Scale3D);
			}
			Bone.SetLocation(Head);
			Moved.Add(FBoneTransform(Indices[Index], Bone));
			Head = Tip;
		}
	}
	if (Moved.Num() > 0)
	{
		Moved.Sort(FCompareBoneTransformIndex());
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Moved), 1.f);
	}
}
