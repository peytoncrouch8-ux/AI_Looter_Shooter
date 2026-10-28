#include "Player/Animation/BodySwing.h"
#include "AnimationRuntime.h"
#include "ReferenceSkeleton.h"

namespace
{
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
		/** To when the skeleton has no To (nullptr: none). */
		const TCHAR* OrTo = nullptr;
		/** A hand's or forearm's own thickness (FLimb::Body; 0 for the rest). */
		float Body = 0.f;
		/** The boot: from the heel to the toe (FBodySwing::FootLine), not from joint to joint. */
		bool bFoot = false;
	};
	const FLimbSpec LimbSpecs[] = {
		// The shins: the boots round them are 8-10 cm (the heroes' hit capsules: Ellis 8, Crane 8.5, Pike 9.7, Odessa's
		// riding boots 10); at 10 a stride's back boot touched the coat's back and showed through between its points.
		{ TEXT("thigh_l"), TEXT("calf_l"), 14.f }, { TEXT("calf_l"), TEXT("foot_l"), 12.f },
		{ TEXT("thigh_r"), TEXT("calf_r"), 14.f }, { TEXT("calf_r"), TEXT("foot_r"), 12.f },
		{ TEXT("pelvis"), TEXT("spine_01"), 18.f }, { TEXT("spine_01"), TEXT("spine_03"), 18.f },
		{ TEXT("spine_03"), TEXT("spine_05"), 18.f }, { TEXT("spine_05"), TEXT("neck_01"), 15.f },
		{ TEXT("neck_01"), TEXT("head"), 9.f },
		{ TEXT("upperarm_l"), TEXT("lowerarm_l"), 9.f }, { TEXT("upperarm_r"), TEXT("lowerarm_r"), 9.f },
		// The forearms and hands: at the game's idle they hang against the coat's sides (they sank 2.5 cm into it). The
		// hands reach the fingers' last joints: ended at the knuckles, the fingers hanging down the coat's side sank into it.
		{ TEXT("lowerarm_l"), TEXT("hand_l"), 7.f, nullptr, 5.f }, { TEXT("lowerarm_r"), TEXT("hand_r"), 7.f, nullptr, 5.f },
		{ TEXT("hand_l"), TEXT("middle_03_l"), 5.f, TEXT("middle_01_l"), 4.f },
		{ TEXT("hand_r"), TEXT("middle_03_r"), 5.f, TEXT("middle_01_r"), 4.f },
		// The boots' feet, heel to toe (the shins alone let the ankle-length hem through a boot's toe in a stride; from the
		// ankle joint, a sprint's raised back heel came through the coat's back).
		{ TEXT("foot_l"), TEXT("ball_l"), 9.f, nullptr, 0.f, true }, { TEXT("foot_r"), TEXT("ball_r"), 9.f, nullptr, 0.f, true },
	};

	/**
	 * The boot's line (FBodySwing::FootLine, cm): its heel this far behind the ankle joint and this high over the ground
	 * (the hit capsules' heels: 4-5 cm behind, their backs 9-12 cm), its toe this far past the ball.
	 */
	constexpr double HeelBack = 5.0;
	constexpr double HeelUp = 4.0;
	constexpr double ToeOn = 4.0;

	/** The swing points keep this far over the ground at his feet (cm): the ankle-length hem sank 11 cm into it crouching. */
	constexpr float FloorClearance = 2.f;

	/**
	 * A forearm or hand moves the cloth aside by at most this (cm) past where the animation has it (FLimb::bHand): the
	 * coat's sides still stand off the hands at the idle (they sank 2.5 cm into Ellis's duster without), but a chain hung
	 * where the idle hand rests is no longer thrown out over the hip by a clearance measured in the bind pose's A.
	 */
	constexpr double HandGive = 3.0;

	/**
	 * How far out a chain's last point lies on the ground, a share of its bone's length (FBodySwingFloor::LastReach): laid
	 * out at full length the hem's flaps jutted out stiffly round a crouch; nearer, the bone shortens and the hem gathers.
	 */
	constexpr float GatherReach = 1.f / 3.f;

	/** A bone shortens on an axis of its own only when its tip lies this close along it (the normalised component). */
	constexpr double LengthAxisShare = 0.9;

	/**
	 * How much of the upper arm's movement a cape chain takes (FChain::FollowWeight), by its key in `cape_<key><side>`
	 * (Ellis.py, round 6): f the front, over the chest; a over the arm and s at the shoulder's side, which turn about the
	 * shoulder with it; b the back, over the shoulder blade. `cape_bm`, the middle of the back, follows nothing. Riding an
	 * arm half-and-half pulled the cloth into the sleeve as the arm rose; these shares with chains hung from the shoulder
	 * joint keep it on the arm.
	 */
	float CapeFollowShare(TCHAR Key)
	{
		switch (Key)
		{
		case TEXT('f'): return 0.2f;
		case TEXT('a'): return 1.f;
		case TEXT('s'): return 1.f;
		case TEXT('b'): return 0.5f;
		default: return 0.f;
		}
	}

	/**
	 * How slowly the cape's corners take up the arm (s, the time to most of the way): a sprint's arm swing (about 1.5 a
	 * second) comes through at about a third, a gun raised and held all of it.
	 */
	constexpr float FollowEaseSeconds = 0.35f;

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
// FBodySwing
// ---------------------------------------------------------------------------

float FBodySwing::MaxClearanceOf(const FString& FromBone)
{
	for (const FLimbSpec& Spec : LimbSpecs)
	{
		if (FromBone.Equals(Spec.From, ESearchCase::IgnoreCase))
		{
			return Spec.MaxClearance;
		}
	}
	return 0.f;
}

void FBodySwing::FootLine(const FVector& Ankle, const FVector& Ball, FVector& OutHeel, FVector& OutToe)
{
	// Forward along the ground (component space, Z up, the ground at the root): from the ankle toward the ball.
	FVector Forward(Ball.X - Ankle.X, Ball.Y - Ankle.Y, 0.0);
	if (!Forward.Normalize())
	{
		OutHeel = Ankle;
		OutToe = Ball;
		return;
	}
	OutHeel = Ankle - Forward * HeelBack;
	OutHeel.Z = FMath::Min(Ankle.Z, HeelUp);
	OutToe = Ball + Forward * ToeOn;
}

float FBodySwing::HandRadius(float RestClearance, double AnimatedAway, float Body, double Scale)
{
	const double Give = AnimatedAway + HandGive * Scale;
	return static_cast<float>(FMath::Min(static_cast<double>(RestClearance), FMath::Max(Give, static_cast<double>(Body) * Scale)));
}

bool FBodySwing::HasLimb(FName From, FName To) const
{
	return Limbs.ContainsByPredicate([From, To](const FLimb& Limb) { return Limb.From.BoneName == From && Limb.To.BoneName == To; });
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
		/** A leg's From bone's turn at rest (its cloth side is carried in it); identity for the rest. */
		FQuat Frame;
		bool bLeg;
	};
	TArray<FLimbRest> LimbRests;
	for (const FLimbSpec& Spec : LimbSpecs)
	{
		const int32 From = Skeleton.FindBoneIndex(Spec.From);
		int32 To = Skeleton.FindBoneIndex(Spec.To);
		if (To == INDEX_NONE && Spec.OrTo != nullptr)
		{
			To = Skeleton.FindBoneIndex(Spec.OrTo);
		}
		if (From == INDEX_NONE || To == INDEX_NONE)
		{
			continue;
		}
		FLimb& Limb = Limbs.AddDefaulted_GetRef();
		Limb.From = FBoneReference(Skeleton.GetBoneName(From));
		Limb.To = FBoneReference(Skeleton.GetBoneName(To));
		Limb.MaxClearance = Spec.MaxClearance;
		Limb.Body = Spec.Body;
		const FString FromName = Skeleton.GetBoneName(From).ToString();
		Limb.bHand = FromName.StartsWith(TEXT("lowerarm")) || FromName.StartsWith(TEXT("hand"));
		Limb.bLeg = FromName.StartsWith(TEXT("thigh")) || FromName.StartsWith(TEXT("calf")) || FromName.StartsWith(TEXT("foot"));
		FVector RestA = Rest[From].GetLocation();
		FVector RestB = Rest[To].GetLocation();
		if (Spec.bFoot)
		{
			// The boot from its heel to its toe, each end carried by its own bone.
			FootLine(Rest[From].GetLocation(), Rest[To].GetLocation(), RestA, RestB);
			Limb.FromOffset = Rest[From].InverseTransformPosition(RestA);
			Limb.ToOffset = Rest[To].InverseTransformPosition(RestB);
		}
		// The upper arm's rest is the bind pose's, raised 31 degrees off the idle: the cape's clearances are measured
		// against the arm as it hangs at the game's idle (Ellis.py's unweighted `cape_arm_<side>`) when the mesh has it.
		if (FromName.StartsWith(TEXT("upperarm")))
		{
			const int32 IdleArm = Skeleton.FindBoneIndex(FName(*(TEXT("cape_arm") + FromName.RightChop(8))));
			if (IdleArm != INDEX_NONE)
			{
				RestB = Rest[IdleArm].TransformPosition(Rest[From].InverseTransformPosition(Rest[To].GetLocation()));
			}
		}
		LimbRests.Add({ RestA, RestB, Spec.MaxClearance, Limb.bLeg ? Rest[From].GetRotation() : FQuat::Identity, Limb.bLeg });
	}
	const int32 Pelvis = Skeleton.FindBoneIndex(TEXT("pelvis"));
	// A coat whose skirt has chains but none at its front is a tailcoat: its chains are tails (Odessa's).
	bool bAnySkirt = false;
	bool bFrontSkirt = false;
	for (const TPair<FString, TArray<TPair<int32, int32>>>& Entry : Found)
	{
		bAnySkirt |= Entry.Key.StartsWith(TEXT("skirt_"));
		bFrontSkirt |= Entry.Key.StartsWith(TEXT("skirt_f"));
	}
	const bool bTailcoat = bAnySkirt && !bFrontSkirt;

	for (TPair<FString, TArray<TPair<int32, int32>>>& Entry : Found)
	{
		TArray<TPair<int32, int32>>& Ordered = Entry.Value;
		Ordered.Sort([](const TPair<int32, int32>& A, const TPair<int32, int32>& B) { return A.Key < B.Key; });
		FChain& Chain = Chains.AddDefaulted_GetRef();
		Chain.Name = Entry.Key;
		// Its kind by its name and what it hangs from (a lantern on the chest is a hook), a skirt by the coat it's on.
		const int32 Parent = Skeleton.GetParentIndex(Ordered[0].Value);
		Chain.Settings = SettingsFor(KindOf(Entry.Key, Parent != INDEX_NONE ? Skeleton.GetBoneName(Parent).ToString() : FString(), bTailcoat));
		// The cape's chains ride on the upper arms (`cape_<key><side>`: cape_al, the arm chain on the figure's left), each
		// by its key's share; its rest on the arm is measured against the arm as it hangs at the idle (`cape_arm_<side>`)
		// when the mesh has it, else the bind pose's.
		int32 FollowIndex = INDEX_NONE;
		int32 FollowRest = INDEX_NONE;
		if (Chain.Name.StartsWith(TEXT("cape_")) && Chain.Name.Len() == 7)
		{
			const TCHAR Side = Chain.Name[6];
			if (Side == TEXT('l') || Side == TEXT('r'))
			{
				const FString Suffix = Side == TEXT('l') ? TEXT("_l") : TEXT("_r");
				FollowIndex = Skeleton.FindBoneIndex(FName(*(TEXT("upperarm") + Suffix)));
				FollowRest = Skeleton.FindBoneIndex(FName(*(TEXT("cape_arm") + Suffix)));
				Chain.FollowWeight = CapeFollowShare(Chain.Name[5]);
			}
		}
		if (FollowIndex != INDEX_NONE && Chain.FollowWeight > 0.f)
		{
			Chain.Follow = FBoneReference(Skeleton.GetBoneName(FollowIndex));
			if (FollowRest == INDEX_NONE)
			{
				FollowRest = FollowIndex;
			}
			Chain.Settings.bRisesToTarget = true;
		}
		else
		{
			FollowIndex = INDEX_NONE;
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
			Chain.TipInFollow.Add(FollowIndex != INDEX_NONE ? Rest[FollowRest].InverseTransformPosition(Tip) : FVector::ZeroVector);
			Chain.Lengths.Add(static_cast<float>(FVector::Dist(Head, Tip)));
			for (const FLimbRest& Limb : LimbRests)
			{
				const double Distance = FVector::Dist(Tip, FMath::ClosestPointOnSegment(Tip, Limb.A, Limb.B));
				// A thick thing (the lantern) may keep more than cloth does, its body's thickness more.
				const float Cap = Limb.MaxClearance + Chain.Settings.Bulk;
				Chain.Clearances.Add(FMath::Min(static_cast<float>(Distance) * ClearanceShare, Cap));
				FVector OnBone;
				FVector OnLimb;
				FMath::SegmentDistToSegmentSafe(Head, Tip, Limb.A, Limb.B, OnBone, OnLimb);
				const double BoneDistance = FVector::Dist(OnBone, OnLimb);
				Chain.BoneClearances.Add(FMath::Min(static_cast<float>(BoneDistance) * ClearanceShare, Cap));
			}
		}
		// Its way out of the body: level, from the body's middle (the pelvis) to where it ends.
		const FVector LastTip = Rest[Ordered.Last().Value].TransformPosition(Chain.TipInBone.Last());
		const FVector Middle = Pelvis != INDEX_NONE ? Rest[Pelvis].GetLocation() : FVector::ZeroVector;
		const FVector Outward = FVector(LastTip.X - Middle.X, LastTip.Y - Middle.Y, 0.0).GetSafeNormal();
		Chain.OutwardInBone = Rest[Ordered[0].Value].InverseTransformVectorNoScale(Outward);
		// And in each leg's own space, so the cloth's side of a leg turns with it (FBodySwingCapsule::Side).
		for (const FLimbRest& Limb : LimbRests)
		{
			Chain.OutwardInLimbs.Add(Limb.bLeg ? Limb.Frame.UnrotateVector(Outward) : FVector::ZeroVector);
		}
	}

	TArray<FVector> LimbA;
	TArray<FVector> LimbB;
	TArray<float> LimbMax;
	for (const FLimbRest& Limb : LimbRests)
	{
		LimbA.Add(Limb.A);
		LimbB.Add(Limb.B);
		LimbMax.Add(Limb.MaxClearance);
	}
	BindRing(Skeleton, Rest, LimbA, LimbB, LimbMax);
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
	LimbTurns.Reset();
	for (const FLimb& Limb : Limbs)
	{
		const FCompactPoseBoneIndex From = Limb.From.GetCompactPoseIndex(Bones);
		const FCompactPoseBoneIndex To = Limb.To.GetCompactPoseIndex(Bones);
		FBodySwingCapsule& Capsule = LimbsNow.AddDefaulted_GetRef();
		Capsule.bShortestWay = Limb.bHand;
		Capsule.bLifts = Limb.bLeg;
		FQuat& Turn = LimbTurns.Add_GetRef(ComponentToWorld.GetRotation());
		if (From.IsValid() && To.IsValid())
		{
			const FTransform& FromNow = Pose.GetComponentSpaceTransform(From);
			Capsule.A = ComponentToWorld.TransformPosition(FromNow.TransformPosition(Limb.FromOffset));
			Capsule.B = ComponentToWorld.TransformPosition(Pose.GetComponentSpaceTransform(To).TransformPosition(Limb.ToOffset));
			Turn = (ComponentToWorld.GetRotation() * FromNow.GetRotation()).GetNormalized();
		}
	}

	for (FChain& Chain : Chains)
	{
		const int32 Count = Chain.Bones.Num();
		TArray<FCompactPoseBoneIndex>& Indices = Chain.Indices;
		Indices.Reset();
		bool bWhole = Count > 0;
		for (const FBoneReference& Bone : Chain.Bones)
		{
			const FCompactPoseBoneIndex Index = Bone.GetCompactPoseIndex(Bones);
			bWhole &= Index.IsValid();
			Indices.Add(Index);
		}
		Chain.bLive = bWhole;
		if (!bWhole)
		{
			// A LOD without the chain: it starts over when it's back.
			Chain.Sim.bStarted = false;
			continue;
		}

		// Where the animation has the chain this frame.
		TArray<FTransform>& Animated = Chain.Animated;
		Animated.Reset();
		TArray<FVector, TInlineAllocator<4>> Targets;
		TArray<float, TInlineAllocator<4>> Lengths;
		const FCompactPoseBoneIndex FollowIndex = Chain.FollowWeight > 0.f ? Chain.Follow.GetCompactPoseIndex(Bones) : FCompactPoseBoneIndex(INDEX_NONE);
		FTransform Follow = FTransform::Identity;
		if (FollowIndex.IsValid())
		{
			// The arm as the chain's own bone carries it, eased: a gun coming up lifts the cape, a sprint's quick arm
			// swing mostly doesn't (riding the arm itself, the corners flapped up at the shoulders as he ran).
			const FTransform& Carrier = Pose.GetComponentSpaceTransform(Indices[0]);
			const FTransform ArmOnCarrier = Pose.GetComponentSpaceTransform(FollowIndex).GetRelativeTransform(Carrier);
			if (!Chain.bFollowStarted)
			{
				Chain.FollowEased = ArmOnCarrier;
				Chain.bFollowStarted = true;
			}
			else
			{
				const float Ease = 1.f - FMath::Exp(-FMath::Max(DeltaSeconds, 0.f) / FollowEaseSeconds);
				Chain.FollowEased.SetRotation(FQuat::Slerp(Chain.FollowEased.GetRotation(), ArmOnCarrier.GetRotation(), Ease).GetNormalized());
				Chain.FollowEased.SetLocation(FMath::Lerp(Chain.FollowEased.GetLocation(), ArmOnCarrier.GetLocation(), static_cast<double>(Ease)));
			}
			Follow = Chain.FollowEased * Carrier;
		}
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FTransform& InComponent = Pose.GetComponentSpaceTransform(Indices[Index]);
			Animated.Add(InComponent);
			FVector Target = InComponent.TransformPosition(Chain.TipInBone[Index]);
			if (FollowIndex.IsValid())
			{
				const FVector Riding = Follow.TransformPosition(Chain.TipInFollow[Index]);
				Target = FMath::Lerp(Target, Riding, static_cast<double>(Chain.FollowWeight));
			}
			Targets.Add(ComponentToWorld.TransformPosition(Target));
			Lengths.Add(Chain.Lengths[Index] * static_cast<float>(Scale));
		}
		Chain.Anchor = ComponentToWorld.TransformPosition(Animated[0].GetLocation());
		TArray<FBodySwingCapsule, TInlineAllocator<64>> Capsules;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			for (int32 Limb = 0; Limb < LimbsNow.Num(); ++Limb)
			{
				FBodySwingCapsule Capsule = LimbsNow[Limb];
				Capsule.Radius = Chain.Clearances[Index * LimbsNow.Num() + Limb] * static_cast<float>(Scale);
				Capsule.BoneRadius = Chain.BoneClearances[Index * LimbsNow.Num() + Limb] * static_cast<float>(Scale);
				if (Chain.OutwardInLimbs.IsValidIndex(Limb) && !Chain.OutwardInLimbs[Limb].IsZero() && LimbTurns.IsValidIndex(Limb))
				{
					// A leg: the cloth's side of it as it lay at rest, turned with the leg (over a shin kicked back past level).
					Capsule.Side = LimbTurns[Limb].RotateVector(Chain.OutwardInLimbs[Limb]);
				}
				if (Limbs.IsValidIndex(Limb) && Limbs[Limb].bHand)
				{
					// A hand or forearm moves the cloth aside by at most HandGive past where the animation has it, but keeps
					// it off its own thickness.
					const FVector& Target = Targets[Index];
					const FVector& Before = Index > 0 ? Targets[Index - 1] : Chain.Anchor;
					const double PointAway = FVector::Dist(Target, FMath::ClosestPointOnSegment(Target, Capsule.A, Capsule.B));
					FVector OnBone;
					FVector OnLimb;
					FMath::SegmentDistToSegmentSafe(Before, Target, Capsule.A, Capsule.B, OnBone, OnLimb);
					const double BoneAway = FVector::Dist(OnBone, OnLimb);
					Capsule.Radius = HandRadius(Capsule.Radius, PointAway, Limbs[Limb].Body, Scale);
					Capsule.BoneRadius = HandRadius(Capsule.BoneRadius, BoneAway, Limbs[Limb].Body, Scale);
				}
				Capsules.Add(Capsule);
			}
		}
		FBodySwingFloor Floor;
		Floor.Z = FloorZ;
		// The hem gathers on the ground only when its bone can shorten (along an axis of its own).
		Floor.LastReach = Chain.LengthAxes.Last() != INDEX_NONE ? GatherReach : 1.f;
		// Its way out of the body as the body is turned and leans now.
		Chain.Sim.Outward = ComponentToWorld.TransformVectorNoScale(Animated[0].TransformVectorNoScale(Chain.OutwardInBone));
		// The body's turn, about its root (the actor turned by the mouse, the hero stand spun): the cloth turns with it.
		Chain.Sim.BodyRotation = ComponentToWorld.GetRotation();
		Chain.Sim.BodyPivot = ComponentToWorld.GetLocation();
		Chain.Sim.Step(Chain.Anchor, Targets, Lengths, Capsules, Chain.Settings, DeltaSeconds, Floor);
	}

	// The cloth between neighbouring chains, off the limbs too (what's shown only: each chain swings on its own).
	ApplyRing(LimbsNow, Scale);

	TArray<FBoneTransform, TInlineAllocator<48>> Moved;
	for (FChain& Chain : Chains)
	{
		if (!Chain.bLive)
		{
			continue;
		}
		const int32 Count = Chain.Bones.Num();
		const TArray<FTransform>& Animated = Chain.Animated;
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
			Moved.Add(FBoneTransform(Chain.Indices[Index], Bone));
			Head = Tip;
		}
	}
	if (Moved.Num() > 0)
	{
		Moved.Sort(FCompareBoneTransformIndex());
		Pose.LocalBlendCSBoneTransforms(MakeArrayView(Moved), 1.f);
	}
}
