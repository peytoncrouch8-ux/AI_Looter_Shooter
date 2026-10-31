// ASpiderCreature's weighty gait (the user, 2026-10-31: "their animations need improved upon"), over the stepping SpiderCreature.cpp
// walks: what it reads of its own motion each frame (its turn rate, its speed-up), the body riding its steps
// (SpiderGait::BodyMotion), its legs in the air (a fall, a fling, a leap from its perch: reaching out round the body for the
// ground, the front pair ahead in a pounce) and its landing (every foot reaching for the ground at once, the body dipping by
// the fall), and the springs the body's jolts ride on. The numbers are SpiderGaitRules'.

#include "Creatures/SpiderCreature.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/**
	 * A hit's jolt: its kick holds a moment (OnHurt sets it: SpiderCreatureReact.cpp), then fades at HurtKickFade, and the body
	 * follows on a critically damped spring (HurtOmega, steps of SpringStep): one hit peaks about 90 ms on at 0.84 of its kick,
	 * under a quarter of it in any frame; a kick renewed every frame (the Gravemother sinking as she reels) holds the body at
	 * it. A hit's tilt (pitch and roll away from the shot) rides the same spring.
	 */
	constexpr float HurtOmega = 40.f;
	constexpr float HurtKickFade = 10.f;
	constexpr float SpringStep = 1.f / 120.f;
	/** A landing's dip rides a softer spring (1/s); a kick of LandKickPerCm (cm/s per cm of dip) peaks at about the dip. */
	constexpr float LandOmega = 12.f;
	constexpr float LandKickPerCm = 32.6f;
	/** The turn rate and speed-up are read smoothed at these rates (1/s): a frame's jitter mustn't rock the body. */
	constexpr float YawRateSmoothing = 12.f;
	constexpr float AccelSmoothing = 6.f;

	/**
	 * Its feet in the air, in the body's frame at size 1 by pair (front to back): this far ahead of the hip, the hip's side
	 * spread this many times out, this far down; reaching on a spring of AirOmega (1/s), paddling a little (AirPaddle cm at
	 * AirPaddleHz). Leaping at its prey, the front pair reaches forward and up instead (PounceFront).
	 */
	constexpr float AirAhead[4] = { 90.f, 35.f, -25.f, -75.f };
	constexpr float AirSpread[4] = { 1.9f, 2.3f, 2.3f, 1.9f };
	constexpr float AirLow[4] = { -30.f, -45.f, -50.f, -40.f };
	constexpr float AirOmega = 16.f;
	constexpr float AirPaddle = 6.f;
	constexpr float AirPaddleHz = 5.f;
	const FVector PounceFront(115.f, 1.4f, 10.f);
	/** Down from the air, each foot's step starts this far along (it's already near the ground: a quick, firm plant). */
	constexpr float LandStepStart = 0.55f;
}

float ASpiderCreature::StepThresholdAt(float Speed)
{
	// How far (cm, full size) a foot may get from its spot before its group steps it back, farther at speed (its thaw uses it
	// to judge which feet to put down afresh): the old rule (the gait's own stray is at most a quarter of the leg's reach: SpiderGait::StepStray).
	return 32.f + Speed * 0.07f;
}

bool ASpiderCreature::IsAirborne() const
{
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	return !IsDead() && Movement && Movement->IsFalling();
}

void ASpiderCreature::ReadGaitMotion(float DeltaSeconds)
{
	const float NowYaw = static_cast<float>(GetActorRotation().Yaw);
	const FVector Velocity = GetVelocity();
	if (!Gait.bRead || DeltaSeconds <= 0.f)
	{
		Gait.bRead = true;
		Gait.LastYaw = NowYaw;
		Gait.LastVelocity = Velocity;
		Gait.YawRate = 0.f;
		Gait.ForwardAccel = 0.f;
		return;
	}
	const float Turn = FMath::FindDeltaAngleDegrees(Gait.LastYaw, NowYaw) / DeltaSeconds;
	Gait.YawRate = FMath::FInterpTo(Gait.YawRate, Turn, DeltaSeconds, YawRateSmoothing);
	const FVector Facing = GetActorForwardVector().GetSafeNormal2D();
	const float Accel = static_cast<float>(FVector::DotProduct(FVector(Velocity.X - Gait.LastVelocity.X, Velocity.Y - Gait.LastVelocity.Y, 0.0), Facing)) / DeltaSeconds;
	Gait.ForwardAccel = FMath::FInterpTo(Gait.ForwardAccel, Accel, DeltaSeconds, AccelSmoothing);
	Gait.LastYaw = NowYaw;
	Gait.LastVelocity = Velocity;
	Gait.SinceLanded += DeltaSeconds;
	if (IsAirborne())
	{
		// How fast it's coming down, for its landing's dip.
		Gait.FallSpeed = FMath::Max(0.f, static_cast<float>(-Velocity.Z));
	}
}

SpiderGait::FBodyMotion ASpiderCreature::GaitMotion() const
{
	const float Speed = static_cast<float>(GetVelocity().Size2D());
	return SpiderGait::BodyMotion(Gait.GroupAlpha, Gait.GroupSide, Speed / FMath::Max(ChaseSpeed, 1.f), Gait.YawRate, Gait.ForwardAccel, Speed);
}

void ASpiderCreature::PoseAirFoot(FLeg& Leg, const FTransform& Body, float DeltaSeconds)
{
	const int32 Pair = FMath::Clamp(Leg.Pair, 0, 3);
	const bool bPounce = Pair == 0 && Lurk.Phase == ESpiderLurkPhase::Leaping;
	FVector Local = bPounce ? FVector(Leg.Hip.X + PounceFront.X, Leg.Hip.Y * PounceFront.Y, PounceFront.Z)
		: FVector(Leg.Hip.X + AirAhead[Pair], Leg.Hip.Y * AirSpread[Pair], AirLow[Pair]);
	// A little paddling, each pair a beat apart, so it isn't a statue in the air.
	Local.Z += AirPaddle * FMath::Sin(2.f * UE_PI * AirPaddleHz * AnimTime + Pair * 1.3f + (Leg.Side > 0.f ? UE_PI : 0.f));
	StepHeldFoot(Leg, Body.TransformPosition(Local), AirOmega, DeltaSeconds);
	Leg.bStepping = false;
	Leg.bNeedsReset = true;
}

void ASpiderCreature::PlantAfterFall(const TArray<FVector, TInlineAllocator<8>>& RestSpots, float StepHeight)
{
	// Every foot reaches for the ground under its spot from where it is in the air, its step begun half way (on the way down:
	// a quick, firm plant). Where the step is taken as starting from is set so it carries on from where the foot is: no pop.
	const float Along = bWeightyGait ? SpiderGait::StepAlong(LandStepStart) : FMath::InterpEaseInOut(0.f, 1.f, LandStepStart, 2.f);
	const float Lift = (bWeightyGait ? SpiderGait::StepLift(LandStepStart) : FMath::Sin(LandStepStart * UE_PI)) * StepHeight;
	for (int32 Index = 0; Index < Legs.Num() && Index < RestSpots.Num(); ++Index)
	{
		FLeg& Leg = Legs[Index];
		Leg.StepTo = GroundUnder(RestSpots[Index]);
		Leg.StepFrom = (Leg.Foot - Leg.StepTo * Along - FVector(0.f, 0.f, Lift)) / FMath::Max(1.f - Along, 0.1f);
		Leg.StepAlpha = LandStepStart;
		Leg.bStepping = true;
		Leg.bNeedsReset = false;
		Leg.HeldVelocity = FVector::ZeroVector;
	}
	SteppingGroup = INDEX_NONE;
	// The body takes the fall: down by the landing's dip (SpiderGait::LandingDip) on a spring, and back up.
	if (bWeightyGait)
	{
		Gait.LandDipSpeed += SpiderGait::LandingDip(Gait.FallSpeed) * LandKickPerCm;
		AbdomenKick = FMath::Max(AbdomenKick, FMath::Min(SpiderGait::LandingDip(Gait.FallSpeed) * 0.6f, 12.f));
	}
	Gait.SinceLanded = 0.f;
	Gait.FallSpeed = 0.f;
}

void ASpiderCreature::StepBodySprings(float DeltaSeconds)
{
	// The hit's jolt follows its fading kick on a stiff spring: no one-frame jump.
	const float SpringSeconds = FMath::Min(DeltaSeconds, 0.5f);
	if (SpringSeconds <= 0.f)
	{
		return;
	}
	const int32 Steps = FMath::Max(1, FMath::CeilToInt32(SpringSeconds / SpringStep - 0.01f));
	const float Step = SpringSeconds / Steps;
	for (int32 Index = 0; Index < Steps; ++Index)
	{
		HurtHold -= Step;
		HurtKick -= HurtHold > 0.f ? FVector::ZeroVector : HurtKick * FMath::Min(HurtKickFade * Step, 1.f);
		HurtVelocity += (HurtOmega * HurtOmega * (HurtKick - HurtOffset) - 2.f * HurtOmega * HurtVelocity) * Step;
		HurtOffset += HurtVelocity * Step;
		HurtTiltKick -= HurtHold > 0.f ? FVector::ZeroVector : HurtTiltKick * FMath::Min(HurtKickFade * Step, 1.f);
		HurtTiltVelocity += (HurtOmega * HurtOmega * (HurtTiltKick - HurtTilt) - 2.f * HurtOmega * HurtTiltVelocity) * Step;
		HurtTilt += HurtTiltVelocity * Step;
		// The hit feel's twist, a shot leg's roll and dip (SpiderCreatureHurt.cpp), on a spring as stiff.
		if (!HitTwist.IsSettled())
		{
			HitTwist.Step(Step, HurtOmega, HurtKickFade);
		}
		// A landing's dip (positive: down), on its softer spring back to nothing.
		Gait.LandDipSpeed += (-LandOmega * LandOmega * Gait.LandDip - 2.f * LandOmega * Gait.LandDipSpeed) * Step;
		Gait.LandDip += Gait.LandDipSpeed * Step;
	}
	if (FMath::Abs(Gait.LandDip) < 0.01f && FMath::Abs(Gait.LandDipSpeed) < 0.1f)
	{
		Gait.LandDip = 0.f;
		Gait.LandDipSpeed = 0.f;
	}
}

void ASpiderCreature::PoseHeadAndAbdomen(float Breathing, float Tilt, const FHitFeelPose& Feel)
{
	// Abdomen: slow breathing sway plus its jolts and tilt.
	const float Breath = FMath::Sin(AnimTime * 1.7f) * Breathing;
	PosePivot(SlotAbdomen, Abdomen, FRotator(Breath * 2.5f + AbdomenKick + Feel.Abdomen + Tilt, FMath::Sin(AnimTime * 0.9f) * 2.f * Breathing, 0.f),
		FVector(1.f, 1.f + Breath * 0.015f, 1.f + Breath * 0.02f));
	const float Fang = FMath::Max(FangOpen, Feel.FangAtLeast) + Feel.FangAdd;
	PosePivot(SlotFangLeft, FangLeft, FRotator(Fang * 0.4f, -Fang, 0.f), FVector::OneVector);
	PosePivot(SlotFangRight, FangRight, FRotator(Fang * 0.4f, Fang, 0.f), FVector::OneVector);
}

bool ASpiderCreature::IsLegHeld(const FLeg& Leg, bool bFrontHeld, bool bAir) const
{
	// Legs the attack, death, the air or a subclass's pose holds don't walk.
	const ECreatureState CurrentState = GetCreatureState();
	return CurrentState == ECreatureState::Dead || bAir || ((CurrentState == ECreatureState::Attack || bFrontHeld) && Leg.Pair == 0);
}

void ASpiderCreature::PlanSteps(const TArray<FVector, TInlineAllocator<8>>& RestSpots, float GaitSpeed, float Speed, bool bFrontHeld, bool bAir)
{
	const FTransform& Body = BodyFrame;
	const float Scale = GetSizeScale();
	const FVector Velocity(GetVelocity().X, GetVelocity().Y, 0.f);
	auto IsHeld = [this, bFrontHeld, bAir](const FLeg& Leg) { return IsLegHeld(Leg, bFrontHeld, bAir); };
	bool bGroupWants[2] = { false, false };
	for (int32 Index = 0; Index < Legs.Num(); ++Index)
	{
		FLeg& Leg = Legs[Index];
		const FVector& Rest = RestSpots[Index];
		if (IsHeld(Leg) || Leg.bStepping)
		{
			continue;
		}
		const float Offset = static_cast<float>(FVector::Dist2D(Leg.Foot, Rest));
		if (Offset > 450.f * Scale)
		{
			Leg.Foot = GroundUnder(Rest); // teleported (respawn)
			continue;
		}
		const bool bSettle = Speed < 10.f && Offset > 12.f * Scale && AnimTime - Leg.LastStepTime > 0.6f;
		// A foot left trailing near the end of its leg's reach (its group waited on the other's step while the body ran on)
		// steps with its group's next turn, and near the very end steps at once: a leg drawn out straight reads as a spike,
		// and past full reach the IK would pull it off its spot, sliding.
		const FVector Hip = Body.TransformPosition(Leg.Hip);
		const float Reach = (Leg.FemurLength + Leg.TibiaLength) * Scale;
		const float Stretch = static_cast<float>(FVector::Dist(Hip, Leg.Foot));
		const bool bTrailing = FVector::DotProduct(Leg.Foot - Hip, Velocity) < 0.0;
		// Standing, a foot the body has pulled toward the end of its reach (rearing for a bite, settling on a slope) steps
		// back under it the same way: no velocity says it trails, and it would only slide. One on its spot already stays.
		const bool bPulled = bTrailing || (Speed < 10.f && Offset > 8.f * Scale);
		// (Every spider, a boss too: the solve never poses a leg past SpiderGait::MaxReachShare, so its feet step before that.)
		const float Severe = SpiderGait::SevereOverreachShare;
		const float Over = SpiderGait::OverreachShare;
		if (bPulled && Stretch > Severe * Reach)
		{
			Leg.StepFrom = Leg.Foot;
			Leg.StepTo = GroundUnder(Rest);
			Leg.StepAlpha = 0.f;
			Leg.bStepping = true;
			Leg.bNeedsReset = false;
			continue;
		}
		Leg.bNeedsReset |= bPulled && Stretch > Over * Reach;
		const float Stray = (bWeightyGait ? SpiderGait::StepStray(GaitSpeed, Leg.FemurLength + Leg.TibiaLength) : 32.f + Speed * 0.07f) * Scale;
		bGroupWants[Leg.Group] |= Offset > Stray || bSettle || Leg.bNeedsReset;
	}

	// Standing about, now and then it shifts its weight: a leg or two lifted and set down a hand's width off (idle life:
	// SpiderCreatureReact.cpp picks them).
	FVector ShiftOffset = FVector::ZeroVector;
	const int32 Shifting = IdleShiftLeg(ShiftOffset);
	if (Shifting != INDEX_NONE && Legs.IsValidIndex(Shifting) && !Legs[Shifting].bStepping && !IsHeld(Legs[Shifting]))
	{
		FLeg& Leg = Legs[Shifting];
		Leg.StepFrom = Leg.Foot;
		Leg.StepTo = GroundUnder(RestSpots[Shifting] + ShiftOffset);
		Leg.StepAlpha = 0.f;
		Leg.bStepping = true;
		++ShiftDone;
	}

	// Tetrapod gait: the two groups of alternate legs take turns. Once one group has all its feet down, the other lifts
	// every foot that is off its spot, together. (Letting each leg go whenever the other group was down let one group
	// step on and on while the other's feet dragged behind.)
	if (SteppingGroup != INDEX_NONE && !Legs.ContainsByPredicate([](const FLeg& Leg) { return Leg.bStepping; }))
	{
		LastGroup = SteppingGroup;
		SteppingGroup = INDEX_NONE;
		// The group's feet strike: the body's weight comes down on them.
		Gait.PlantDip = FMath::Max(Gait.PlantDip, SpiderGait::PlantDip(Speed / FMath::Max(ChaseSpeed, 1.f)));
	}
	if (SteppingGroup == INDEX_NONE && (bGroupWants[0] || bGroupWants[1]))
	{
		SteppingGroup = bGroupWants[0] && bGroupWants[1] ? 1 - LastGroup : (bGroupWants[0] ? 0 : 1);
		for (int32 Index = 0; Index < Legs.Num(); ++Index)
		{
			FLeg& Leg = Legs[Index];
			if (Leg.Group == SteppingGroup && !IsHeld(Leg) && !Leg.bStepping
				&& (Leg.bNeedsReset || FVector::Dist2D(Leg.Foot, RestSpots[Index]) > 8.f * Scale))
			{
				Leg.StepFrom = Leg.Foot;
				Leg.StepTo = GroundUnder(RestSpots[Index]);
				Leg.StepAlpha = 0.f;
				Leg.bStepping = true;
				Leg.bNeedsReset = false;
			}
		}
	}
	// The stepping group's progress and its front leg's side, for the body's ride (SpiderGait::BodyMotion).
	Gait.GroupAlpha = -1.f;
	if (SteppingGroup != INDEX_NONE)
	{
		float AlphaSum = 0.f;
		int32 Stepping = 0;
		for (const FLeg& Leg : Legs)
		{
			if (Leg.Group == SteppingGroup && Leg.bStepping)
			{
				AlphaSum += Leg.StepAlpha;
				++Stepping;
			}
			if (Leg.Group == SteppingGroup && Leg.Pair == 0)
			{
				Gait.GroupSide = Leg.Side;
			}
		}
		Gait.GroupAlpha = Stepping > 0 ? AlphaSum / Stepping : -1.f;
	}
}
