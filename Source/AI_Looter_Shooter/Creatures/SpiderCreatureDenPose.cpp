// ASpiderCreature's pose coming out of its den (its beats: SpiderCreatureDen.cpp): its body along the throat and over the lip
// worked out from its clock (DenBodyNow), its feet groping out of the mouth, gripping the lip, braced on the throat's sides
// and stepping down onto the face (DenFootTarget), each leg solved as walking with its body's back for "up"; and the grit
// spilling off the lip. The den is the contract's fixed burrow (no collision): its throat and lip are worked out in the den's
// frame, not traced; only the face spots are, once as it tips (PlanDenFaceSpots).

#include "Creatures/SpiderCreature.h"
#include "Creatures/GravemotherFX.h"
#include "Creatures/StoneSentinelFX.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	/** In the throat its thorax rides this high over the throat's floor (cm at size 1: squeezed down from its 60). */
	constexpr float ThroatRide = 40.f;
	/** Its thorax ends the crawl this far out past the wall plane (cm at size 1): its head out over the lip, its abdomen inside. */
	constexpr float CrawlOut = 28.f;
	/** Its nose dips this far over the lip as the crawl ends, and it wriggles this much as it squeezes (degrees). */
	constexpr float CrawlNoseDown = 14.f;
	constexpr float WriggleRoll = 5.f;
	constexpr float WriggleYaw = 3.f;
	/** Over the lip, its body swings this far out from the wall at the tip's middle (cm at size 1). */
	constexpr float TipSwingOut = 30.f;
	/** The throat's half width at the mouth and at its back (cm), this deep: the feet in it brace on its sides, this high. */
	constexpr float ThroatHalfWidth[2] = { 78.f, 52.f };
	constexpr float ThroatDepth = 220.f;
	constexpr float BraceHeight = 18.f;
	/** A foot kept in the throat is at least this far behind the mouth (cm). */
	constexpr float InThroat = 15.f;
	/** A foot on the lip stays outside the mouth's edge (the opening's ellipse, times this). */
	constexpr float LipOutside = 1.12f;
	/** Its feet grip a little in from where they'd stand (as on a perch: SpiderCreatureCling.cpp). */
	constexpr float GripShare = 0.92f;
	/** A step: its time (s at size 1) and lift (cm at size 1); a reach out of the mouth longer and higher. */
	constexpr float StepSeconds = 0.15f;
	constexpr float ReachSeconds = 0.21f;
	constexpr float StepLift = 12.f;
	constexpr float ReachLift = 22.f;
	/** A foot following the body steps once it strays this far (cm at size 1); one given a new grip goes once it's this far off. */
	constexpr float FollowStray = 20.f;
	constexpr float GripMoved = 4.f;
	/** In the tip each pair steps onto the face this far along it (front first); a left foot a little after its right. */
	constexpr float FaceStepAt[4] = { 0.02f, 0.25f, 0.5f, 0.72f };
	constexpr float LeftLater = 0.06f;
	/** Its knees' spread follows its feet (1/s), as walking. */
	constexpr float KneeRate = 15.f;
	/** Its fangs working as it feels its way out, wide as it tips over (degrees). */
	constexpr float CrawlFang = 12.f;
	constexpr float TipFang = 22.f;

	/** A front foot's grip through the crawl, until Until of it (+ LeftLater on the left): in the throat, or at (Y, Z) on the lip. */
	struct FDenGrip
	{
		float Until;
		bool bInside;
		/** The den's frame (cm): Y out to the leg's side, Z up from the mouth's middle; X is the lip's. */
		float Y;
		float Z;
	};
	// The front pair: still in, then out to touch the upper rim, re-set a hand further out, then the lip's side, gripping.
	constexpr FDenGrip FrontGrips[] = { { 0.08f, true, 0.f, 0.f }, { 0.24f, false, 52.f, 40.f }, { 0.36f, false, 76.f, 30.f },
		{ 2.f, false, 104.f, 4.f } };
	// The second pair after them: the lip's lower side, re-set once.
	constexpr FDenGrip SecondGrips[] = { { 0.22f, true, 0.f, 0.f }, { 0.42f, false, 92.f, -26.f }, { 2.f, false, 106.f, -40.f } };

	/** The den's frame: at the mouth's middle, X out of the wall, Z up. */
	FTransform DenFrameOf(const FSpiderDenState& Den)
	{
		return FTransform(FRotationMatrix::MakeFromXZ(Den.Out, FVector::UpVector).ToQuat(), Den.Mouth);
	}

	/** The body along its crawl (Alpha 0..1; no hit's jolt), scaled by its size. */
	FTransform CrawlBody(const FSpiderDenState& Den, float Alpha, float Scale, const FSpiderDenTuning& Tuning)
	{
		const float Time = FMath::Clamp(Alpha, 0.f, 1.f);
		const float Out = SpiderAmbushRules::DenCrawlOut(Time);
		const float X = FMath::Lerp(-Tuning.Inside * Scale, CrawlOut * Scale, Out);
		const float Z = -FSpiderDenTuning::MouthHalfHeight + ThroatRide * Scale;
		// Squeezing out it wriggles (most in the middle), and its nose dips as its head comes out over the lip.
		const float Squeeze = FMath::Sin(UE_PI * Time);
		const float Roll = WriggleRoll * FMath::Sin(2.f * UE_PI * 2.3f * Time) * Squeeze;
		const float Yaw = WriggleYaw * FMath::Sin(2.f * UE_PI * 1.4f * Time + 0.7f) * Squeeze;
		const float Nose = -CrawlNoseDown * FMath::SmoothStep(0.55f, 1.f, Time);
		const FTransform Frame = DenFrameOf(Den);
		return FTransform(Frame.GetRotation() * FRotator(Nose, Yaw, Roll).Quaternion(), Frame.TransformPosition(FVector(X, 0.f, Z)), FVector(Scale));
	}

	/** The body along its tip over the lip onto the face (Alpha 0..1): from the crawl's end, swung out and down, head down. */
	FTransform TipBody(const FSpiderDenState& Den, const FVector& Forward, float Alpha, float Scale, const FSpiderDenTuning& Tuning, float ClingRide)
	{
		const FTransform From = CrawlBody(Den, 1.f, Scale, Tuning);
		const FQuat ToTurn = SpiderAmbushRules::ClingRotation(Den.Normal, Forward);
		const FVector To = Den.ClingPoint + Den.Normal * (ClingRide * Scale);
		const FVector Swing = From.GetLocation() + Den.Out * (TipSwingOut * Scale);
		const float Along = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f));
		// A curve out over the lip and down to the face (a quadratic through Swing's pull).
		const FVector At = From.GetLocation() * FMath::Square(1.f - Along) + Swing * (2.f * (1.f - Along) * Along) + To * FMath::Square(Along);
		return FTransform(FQuat::Slerp(From.GetRotation(), ToTurn, Along).GetNormalized(), At, FVector(Scale));
	}
}

// ---------------------------------------------------------------------------
// Its pose coming out
// ---------------------------------------------------------------------------

FTransform ASpiderCreature::DenBodyNow() const
{
	const FSpiderDenTuning Tuning = FSpiderDenTuning::Live();
	const float Scale = GetSizeScale();
	if (Lurk.Phase == ESpiderLurkPhase::DenTurn)
	{
		return TipBody(Den, Lurk.Forward, Lurk.Clock / SpiderAmbushRules::DenTurnSeconds(Scale, Tuning), Scale, Tuning, FSpiderLurkTuning().ClingRide);
	}
	const float Alpha = Lurk.Phase == ESpiderLurkPhase::DenCrawl ? Lurk.Clock / SpiderAmbushRules::DenCrawlSeconds(Scale, Tuning) : 0.f;
	return CrawlBody(Den, Alpha, Scale, Tuning);
}

void ASpiderCreature::PlanDenFaceSpots()
{
	// As it will hang head down under the mouth, each foot where the cling plants it (SpiderCreatureCling.cpp's PlantOnSurface),
	// so the climb takes over with its feet already in step.
	const float Scale = GetSizeScale();
	const float Ride = FSpiderLurkTuning().ClingRide;
	const FTransform Stance(SpiderAmbushRules::ClingRotation(Den.Normal, Lurk.Forward), Den.ClingPoint + Den.Normal * (Ride * Scale), FVector(Scale));
	for (int32 Index = 0; Index < Legs.Num() && Index < FSpiderDenState::MostLegs; ++Index)
	{
		const FLeg& Leg = Legs[Index];
		Den.FaceSpots[Index] = SurfaceUnder(Stance.TransformPosition(FVector(Leg.Rest.X * GripShare, Leg.Rest.Y * GripShare, -Ride)));
	}
}

FVector ASpiderCreature::DenFootTarget(const FLeg& Leg, int32 Index, float Alpha, const FTransform& Body, bool& bOutGrip) const
{
	const FTransform Frame = DenFrameOf(Den);
	const float Late = Leg.Side < 0.f ? LeftLater : 0.f;
	auto OnLip = [&Frame, &Leg](float Y, float Z)
	{
		return Frame.TransformPosition(FVector(FSpiderDenTuning::LipProud, Y * Leg.Side, Z));
	};
	bOutGrip = true;
	if (Lurk.Phase == ESpiderLurkPhase::DenCrawl && Leg.Pair <= 1)
	{
		// The front pairs grope their way out: their grips in turn (the first still in the throat).
		const TArrayView<const FDenGrip> Grips = Leg.Pair == 0 ? MakeArrayView(FrontGrips) : MakeArrayView(SecondGrips);
		for (const FDenGrip& Grip : Grips)
		{
			if (Alpha < Grip.Until + Late)
			{
				if (!Grip.bInside)
				{
					return OnLip(Grip.Y, Grip.Z);
				}
				break;
			}
		}
	}
	else if (Lurk.Phase == ESpiderLurkPhase::DenTurn)
	{
		// Over the lip each pair steps down onto the face in turn, front first; till then the front pairs keep their grips.
		if (Alpha >= FaceStepAt[FMath::Clamp(Leg.Pair, 0, 3)] + Late && Index < FSpiderDenState::MostLegs)
		{
			return Den.FaceSpots[Index];
		}
		if (Leg.Pair <= 1)
		{
			const FDenGrip& Last = Leg.Pair == 0 ? FrontGrips[UE_ARRAY_COUNT(FrontGrips) - 1] : SecondGrips[UE_ARRAY_COUNT(SecondGrips) - 1];
			return OnLip(Last.Y, Last.Z);
		}
	}
	// Following the body: where it would stand under it, braced on the throat's sides inside, or out on the lip round the mouth.
	bOutGrip = false;
	const FVector Stand = Body.TransformPosition(FVector(Leg.Rest.X * GripShare, Leg.Rest.Y * GripShare, -ThroatRide));
	FVector Local = Frame.InverseTransformPosition(Stand);
	if (Lurk.Phase == ESpiderLurkPhase::DenCrawl)
	{
		// Crawling, only its grips are out: every other foot stays in the throat (a front foot's reach stops short of the mouth).
		Local.X = FMath::Min(Local.X, static_cast<double>(-InThroat));
	}
	if (Local.X < 0.0)
	{
		const float Width = FMath::Lerp(ThroatHalfWidth[0], ThroatHalfWidth[1], FMath::Clamp(static_cast<float>(-Local.X) / ThroatDepth, 0.f, 1.f));
		if (FMath::Abs(Local.Y) > Width)
		{
			Local.Y = FMath::Sign(Local.Y) * Width;
			Local.Z = -FSpiderDenTuning::MouthHalfHeight + BraceHeight;
		}
		else
		{
			Local.Z = -FSpiderDenTuning::MouthHalfHeight;
		}
		return Frame.TransformPosition(Local);
	}
	Local.X = FSpiderDenTuning::LipProud;
	const float Across = static_cast<float>(Local.Y) / FSpiderDenTuning::MouthHalfWidth;
	const float Up = static_cast<float>(Local.Z) / FSpiderDenTuning::MouthHalfHeight;
	const float Edge = FMath::Sqrt(Across * Across + Up * Up);
	if (Edge < LipOutside)
	{
		// Never on the open mouth: out onto the lip round it.
		if (Edge < 0.05f)
		{
			Local.Y = FSpiderDenTuning::MouthHalfWidth * LipOutside * Leg.Side;
			Local.Z = 0.0;
		}
		else
		{
			Local.Y *= LipOutside / Edge;
			Local.Z *= LipOutside / Edge;
		}
	}
	return Frame.TransformPosition(Local);
}

void ASpiderCreature::AnimateDen(float DeltaSeconds)
{
	if (!bRigReady || Lurk.Phase == ESpiderLurkPhase::DenHidden)
	{
		// Unseen in the throat: nothing to pose.
		return;
	}
	ComponentToWorld = GetMesh()->GetComponentTransform();
	const FSpiderDenTuning Tuning = FSpiderDenTuning::Live();
	const float Scale = GetSizeScale();
	const bool bCrawl = Lurk.Phase == ESpiderLurkPhase::DenCrawl;
	const float Seconds = bCrawl ? SpiderAmbushRules::DenCrawlSeconds(Scale, Tuning) : SpiderAmbushRules::DenTurnSeconds(Scale, Tuning);
	const float Alpha = FMath::Clamp(Lurk.Clock / FMath::Max(Seconds, 0.01f), 0.f, 1.f);
	const FTransform Body = DenBodyNow();

	// The body along its beats; a hit jolts and tilts it as on a perch (its springs: SpiderCreatureGait.cpp).
	StepBodySprings(DeltaSeconds);
	AbdomenKick = FMath::FInterpTo(AbdomenKick, 0.f, DeltaSeconds, 8.f);
	const FHitFeelPose Feel = HitFeelPose();
	const FQuat Turned = Body.GetRotation() * FRotator(HurtTilt.X + Feel.Pitch, Feel.Yaw, HurtTilt.Y + Feel.Roll).Quaternion();
	BodyFrame = FTransform(Turned, Body.GetLocation() + HurtOffset, FVector(Scale));
	SetBone(SlotBody, BodyInFrame, BodyFrame);
	// Its fangs working as it feels its way out, wide as it tips; its abdomen squeezed and wobbling through the throat.
	const float WantedFang = bCrawl ? CrawlFang + 5.f * FMath::Sin(AnimTime * 9.f) : TipFang;
	FangOpen = DeltaSeconds > 0.f ? FMath::FInterpTo(FangOpen, WantedFang, DeltaSeconds, 10.f) : WantedFang;
	PoseHeadAndAbdomen(1.f, bCrawl ? 3.f * FMath::Sin(AnimTime * 6.f) : -5.f * Alpha, Feel);

	// The feet: each to its grip or after the body, stepping (posed at once with no time: a start, a thaw).
	const FTransform Frame = DenFrameOf(Den);
	for (int32 Index = 0; Index < Legs.Num(); ++Index)
	{
		FLeg& Leg = Legs[Index];
		bool bGrip = false;
		const FVector Aim = DenFootTarget(Leg, Index, Alpha, Body, bGrip);
		if (DeltaSeconds <= 0.f)
		{
			Leg.Foot = Leg.StepTo = Aim;
			Leg.bStepping = false;
			Leg.StepAlpha = 1.f;
			continue;
		}
		if (Leg.bStepping)
		{
			// A reach out of the throat is longer and higher than a step along it; on the lip or face it lifts out from the wall.
			const bool bOutside = Frame.InverseTransformPosition(Leg.StepTo).X >= 0.0;
			const bool bReach = bOutside && Frame.InverseTransformPosition(Leg.StepFrom).X < 0.0;
			const float Time = FMath::Max((bReach ? ReachSeconds : StepSeconds) * Scale, DeltaSeconds);
			Leg.StepAlpha = FMath::Min(1.f, Leg.StepAlpha + DeltaSeconds / Time);
			const FVector Lift = (bOutside ? Den.Out : FVector::UpVector) * (SpiderGait::StepLift(Leg.StepAlpha) * (bReach ? ReachLift : StepLift) * Scale);
			Leg.Foot = FMath::Lerp(Leg.StepFrom, Leg.StepTo, static_cast<double>(SpiderGait::StepAlong(Leg.StepAlpha))) + Lift;
			if (Leg.StepAlpha >= 1.f)
			{
				Leg.Foot = Leg.StepTo;
				Leg.bStepping = false;
				Leg.LastStepTime = AnimTime;
			}
		}
		else if (FVector::Dist(Leg.Foot, Aim) > (bGrip ? GripMoved : FollowStray * Scale))
		{
			Leg.StepFrom = Leg.Foot;
			Leg.StepTo = Aim;
			Leg.StepAlpha = 0.f;
			Leg.bStepping = true;
		}
	}

	// Each leg solved as walking, the body's back for "up": its knees arch away from its belly.
	const FVector Center = BodyFrame.GetLocation();
	const FVector BodyUp = BodyFrame.GetRotation().GetUpVector();
	for (int32 Index = 0; Index < Legs.Num(); ++Index)
	{
		FLeg& Leg = Legs[Index];
		const FVector Hip = BodyFrame.TransformPosition(Leg.Hip);
		FVector Outward = Hip - Center;
		Outward -= BodyUp * FVector::DotProduct(Outward, BodyUp);
		Outward = Outward.GetSafeNormal(UE_SMALL_NUMBER, BodyFrame.GetRotation().GetRightVector() * Leg.Side);
		const float WantedSpread = WantedKneeSpread(Leg, static_cast<float>(FVector::Dist(Hip, Leg.Foot)) / Scale);
		Leg.KneeSpread = DeltaSeconds > 0.f ? FMath::FInterpTo(Leg.KneeSpread, WantedSpread, DeltaSeconds, KneeRate) : WantedSpread;
		FVector Pole = KneePole(BodyUp, Outward) + Outward * Leg.KneeSpread;
		FVector Knee;
		FVector Foot;
		PoseGaitLeg(Leg, Hip, Leg.Foot, Outward, /*bHeld*/ false, Pole, Knee, Foot);
		Leg.PosedFoot = Foot;
		Leg.HeldVelocity = FVector::ZeroVector;
		SetBone(SlotFirstLeg + Index * 2, Leg.FemurInSegment, SegmentFrame(Hip, Knee, Pole, Scale));
		SetBone(SlotFirstLeg + Index * 2 + 1, Leg.TibiaInSegment, SegmentFrame(Knee, Foot, Pole, Scale));
	}
}

void ASpiderCreature::SpillDenGrit(float Strength)
{
	// Off the mouth's bottom lip: a few stone chips tumbling out and down, a little granite dust sifting after them. The
	// chips are opaque and cost next to nothing; the dust goes through the sentinel's budget and is kept off the camera.
	const FVector Lip = Den.Mouth + Den.Out * (FSpiderDenTuning::LipProud + 6.f) - FVector(0.0, 0.0, FSpiderDenTuning::MouthHalfHeight - 4.f);
	if (UGravemotherFX* Effects = UGravemotherFX::Get(this))
	{
		Effects->Chunks(EGraveChunk::StoneChip, Lip, (Den.Out + FVector(0.0, 0.0, 0.3)).GetSafeNormal(), FMath::RoundToInt(1.f + 2.f * Strength), 130.f, 0.45f);
	}
	FGraveDustBurst Dust;
	Dust.Kind = EGraveDust::Stone;
	Dust.Count = Strength >= 0.9f ? 2 : 1;
	Dust.Spread = 35.f;
	Dust.SpreadUp = 8.f;
	Dust.StartSize = 28.f;
	Dust.EndSize = 85.f;
	Dust.Life = 1.3f;
	Dust.Opacity = 0.26f;
	Dust.Drift = Den.Out * 25.f + FVector(0.0, 0.0, -35.0);
	Dust.Outward = 30.f;
	Dust.Fall = 0.3f;
	StoneSentinelFX::Dust(this, Lip, Dust);
}
