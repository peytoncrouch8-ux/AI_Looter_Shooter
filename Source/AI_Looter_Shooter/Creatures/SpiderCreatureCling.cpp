// ASpiderCreature's pose on its perch (an ambush's: SpiderCreatureLurk.cpp): its body flattened to a wall, a trunk or a
// ledge's top, head down a wall (the hunting spider's way) or out over a ledge's edge, its legs splayed and gripping the
// surface, breathing; its head turned along the surface to follow the player near it and lifted off it a little; in its
// coil before a leap pressed flatter, head up, abdomen raised, fangs wide; climbing down, its legs stepping down the wall in
// its gait's alternating groups. The same rig and solve as the gait (SpiderCreature.cpp), in the surface's frame instead of
// the ground's: the surface's normal stands for "up" (its knees arch away from the wall).

#include "Creatures/SpiderCreature.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"

namespace
{
	/** Its feet a little in from where they stand on the ground: gripping. */
	constexpr float GripShare = 0.92f;
	/** A foot strays this far from its spot on the surface (cm at size 1) before its group steps it back (a turn, a climb). */
	constexpr float ClingStray = 16.f;
	/** A step on the surface takes this long (s at size 1) and lifts this far off it (cm at size 1). */
	constexpr float ClingStepSeconds = 0.16f;
	constexpr float ClingStepLift = 12.f;
	/** In its coil it presses this much flatter (cm at size 1), its abdomen raised this far (degrees). */
	constexpr float CoilPress = 10.f;
	constexpr float CoilAbdomen = 10.f;
	/** Its breathing in and out of the surface (cm at size 1). */
	constexpr float ClingBreath = 1.2f;
	/** A foot finds the surface looking from this far out of it to this far in (cm at size 1). */
	constexpr float SurfaceLookOut = 60.f;
	constexpr float SurfaceLookIn = 90.f;
	/** Its fangs on its perch: barely parted, working a little as it watches, wide in its coil (degrees). */
	constexpr float PerchFang = 4.f;
	constexpr float WatchFang = 10.f;
	constexpr float CoilFang = 36.f;
	/** How quickly a knee's spread follows its foot (1/s), as walking. */
	constexpr float ClingKneeRate = 15.f;
}

FVector ASpiderCreature::SurfaceUnder(const FVector& Point) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return Point;
	}
	const float Scale = GetSizeScale();
	const FVector Normal = Lurk.Normal;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("SpiderGrip"), this, /*bTraceComplex*/ true);
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Point + Normal * (SurfaceLookOut * Scale), Point - Normal * (SurfaceLookIn * Scale),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params) && !Hit.bStartPenetrating)
	{
		return Hit.ImpactPoint;
	}
	return Point;
}

void ASpiderCreature::PlantOnSurface()
{
	// Every foot down on the surface round its body as it hangs now: no steps, the body settled over them at once.
	const float Scale = GetSizeScale();
	const float Ride = FSpiderLurkTuning().ClingRide;
	const FTransform Stance(SpiderAmbushRules::ClingRotation(Lurk.Normal, Lurk.Watch), Lurk.Point + Lurk.Normal * (Ride * Scale), FVector(Scale));
	for (FLeg& Leg : Legs)
	{
		Leg.Foot = SurfaceUnder(Stance.TransformPosition(FVector(Leg.Rest.X * GripShare, Leg.Rest.Y * GripShare, -Ride)));
		Leg.bStepping = false;
		Leg.bNeedsReset = false;
		Leg.StepAlpha = 1.f;
		Leg.HeldVelocity = FVector::ZeroVector;
		Leg.KneeSpread = 0.f;
		Leg.PosedFoot = Leg.Foot;
		Leg.bBiteHeld = false;
	}
	SteppingGroup = INDEX_NONE;
	HurtKick = HurtOffset = HurtVelocity = FVector::ZeroVector;
	HurtTiltKick = HurtTilt = HurtTiltVelocity = FVector::ZeroVector;
	HurtHold = 0.f;
}

void ASpiderCreature::AnimateCling(float DeltaSeconds)
{
	ComponentToWorld = GetMesh()->GetComponentTransform();
	const FSpiderLurkTuning Tuning = FSpiderLurkTuning::Live();
	const float Scale = GetSizeScale();
	const FVector Normal = Lurk.Normal;
	const bool bCoiling = Lurk.Phase == ESpiderLurkPhase::Coiling;
	const bool bClimbing = Lurk.Phase == ESpiderLurkPhase::Climbing;
	const bool bWatching = !Lurk.LastPreySpot.IsZero() && FVector::Dist2D(Lurk.Foot, Lurk.LastPreySpot) <= Tuning.WatchRadius * Scale;
	const float Coil = bCoiling ? FMath::SmoothStep(0.f, FMath::Max(Tuning.CoilSeconds * 0.6f, 0.05f), Lurk.Clock) : 0.f;

	// The body: flattened to the surface, breathing in and out of it, head along Watch, lifted off it by Raise. A hit jolts
	// and tilts it as on the ground (its springs: SpiderCreatureGait.cpp).
	StepBodySprings(DeltaSeconds);
	AbdomenKick = FMath::FInterpTo(AbdomenKick, 0.f, DeltaSeconds, 8.f);
	const FHitFeelPose Feel = HitFeelPose();
	const float Ride = (Tuning.ClingRide - CoilPress * Coil + FMath::Sin(AnimTime * 2.2f) * ClingBreath) * Scale;
	const FQuat Stance = SpiderAmbushRules::ClingRotation(Normal, Lurk.Watch);
	const FQuat Turned = Stance * FRotator(Lurk.Raise + HurtTilt.X + Feel.Pitch, Feel.Yaw, HurtTilt.Y + Feel.Roll).Quaternion();
	BodyFrame = FTransform(Turned, Lurk.Point + Normal * Ride + HurtOffset, FVector(Scale));
	SetBone(SlotBody, BodyInFrame, BodyFrame);
	const float WantedFang = bCoiling ? CoilFang : (bWatching ? WatchFang + 3.f * FMath::Sin(AnimTime * 7.f) : PerchFang);
	FangOpen = DeltaSeconds > 0.f ? FMath::FInterpTo(FangOpen, WantedFang, DeltaSeconds, 10.f) : WantedFang;
	PoseHeadAndAbdomen(1.f, CoilAbdomen * Coil, Feel);

	// The legs: each foot gripping its spot on the surface round the body as it hangs; a turn of its head or a climb takes
	// the spots from under the feet, and its groups step them back, lifting off the surface (none in its coil: it grips).
	const FTransform StanceFrame(Stance, Lurk.Point + Normal * (Tuning.ClingRide * Scale), FVector(Scale));
	const FVector Climb = bClimbing ? Lurk.Forward * (Tuning.ClimbSpeed * Scale) : FVector::ZeroVector;
	const float StepTime = FMath::Max(ClingStepSeconds * Scale, DeltaSeconds);
	for (FLeg& Leg : Legs)
	{
		if (Leg.bStepping)
		{
			Leg.StepAlpha = FMath::Min(1.f, Leg.StepAlpha + DeltaSeconds / StepTime);
			Leg.Foot = FMath::Lerp(Leg.StepFrom, Leg.StepTo, static_cast<double>(SpiderGait::StepAlong(Leg.StepAlpha)))
				+ Normal * (SpiderGait::StepLift(Leg.StepAlpha) * ClingStepLift * Scale);
			if (Leg.StepAlpha >= 1.f)
			{
				Leg.Foot = Leg.StepTo;
				Leg.bStepping = false;
				Leg.LastStepTime = AnimTime;
			}
		}
	}
	TArray<FVector, TInlineAllocator<8>> RestSpots;
	bool bGroupWants[2] = { false, false };
	for (FLeg& Leg : Legs)
	{
		const FVector& Rest = RestSpots.Add_GetRef(StanceFrame.TransformPosition(FVector(Leg.Rest.X * GripShare, Leg.Rest.Y * GripShare,
			-Tuning.ClingRide)) + Climb * (StepTime * 0.9f));
		if (!Leg.bStepping && !bCoiling && FVector::Dist(Leg.Foot, Rest) > ClingStray * Scale)
		{
			bGroupWants[Leg.Group] = true;
		}
	}
	if (SteppingGroup != INDEX_NONE && !Legs.ContainsByPredicate([](const FLeg& Leg) { return Leg.bStepping; }))
	{
		LastGroup = SteppingGroup;
		SteppingGroup = INDEX_NONE;
	}
	if (SteppingGroup == INDEX_NONE && (bGroupWants[0] || bGroupWants[1]))
	{
		SteppingGroup = bGroupWants[0] && bGroupWants[1] ? 1 - LastGroup : (bGroupWants[0] ? 0 : 1);
		for (int32 Index = 0; Index < Legs.Num(); ++Index)
		{
			FLeg& Leg = Legs[Index];
			if (Leg.Group == SteppingGroup && !Leg.bStepping)
			{
				Leg.StepFrom = Leg.Foot;
				Leg.StepTo = SurfaceUnder(RestSpots[Index]);
				Leg.StepAlpha = 0.f;
				Leg.bStepping = true;
			}
		}
	}

	// Each leg solved as walking, with the surface's normal for "up": its knee arches away from the surface.
	const FVector Center = BodyFrame.GetLocation();
	for (int32 Index = 0; Index < Legs.Num(); ++Index)
	{
		FLeg& Leg = Legs[Index];
		const FVector Hip = BodyFrame.TransformPosition(Leg.Hip);
		FVector Outward = Hip - Center;
		Outward -= Normal * FVector::DotProduct(Outward, Normal);
		Outward = Outward.GetSafeNormal(UE_SMALL_NUMBER, StanceFrame.GetRotation().GetRightVector() * Leg.Side);
		const float WantedSpread = WantedKneeSpread(Leg, static_cast<float>(FVector::Dist(Hip, Leg.Foot)) / Scale);
		Leg.KneeSpread = DeltaSeconds > 0.f ? FMath::FInterpTo(Leg.KneeSpread, WantedSpread, DeltaSeconds, ClingKneeRate) : WantedSpread;
		FVector Pole = KneePole(Normal, Outward) + Outward * Leg.KneeSpread;
		FVector Knee;
		FVector Foot;
		PoseGaitLeg(Leg, Hip, Leg.Foot, Outward, /*bHeld*/ false, Pole, Knee, Foot);
		Leg.PosedFoot = Foot;
		Leg.HeldVelocity = FVector::ZeroVector;
		SetBone(SlotFirstLeg + Index * 2, Leg.FemurInSegment, SegmentFrame(Hip, Knee, Pole, Scale));
		SetBone(SlotFirstLeg + Index * 2 + 1, Leg.TibiaInSegment, SegmentFrame(Knee, Foot, Pole, Scale));
	}
}
