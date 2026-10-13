// ABellRingerCreature's body: its arms laid over the Unpaid's pose (LayerPose, the hook a body built on the Unpaid's rig
// uses, as Abel's pose table does) and the props hanging from its fists. The bell's arm (its right) carries the bell
// before it, holds it up while it hunts, raises it high beside its head and shakes it through the tell, flings it out on
// the toll's stroke, is knocked back when the bell is shot, and cocks and swings it for the swipe. The rope's arm (its
// left) hangs low, and opens out for the tell as it calls the dead. The Unpaid's own motion shows through a little on
// the left; both arms go back to the Unpaid's slump as it dies.
//
// Each arm is laid by where its upper arm and forearm point (component space: X forward, Y its right, Z up), eased toward
// what it wants: a bone's turn is in the rest pose's frame under its parent's (Turned = Above * Own), so the upper arm's
// turn takes its rest direction to the wanted one, and the forearm's undoes that first. The bell and the rope hang plumb
// from the point each fist closes on, swinging on damped pendulums the fist's motion drives.

#include "Creatures/BellRingerCreature.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

namespace
{
	FVector Dir(float X, float Y, float Z)
	{
		return FVector(X, Y, Z).GetSafeNormal();
	}

	// The bell's arm (its right: +Y).
	/** Calm: carried before it at its waist, the forearm out level. */
	const FVector CarryUpper = Dir(0.28f, 0.22f, -0.93f);
	const FVector CarryFore = Dir(0.97f, 0.06f, -0.22f);
	/** Hunting: held up before its chest. */
	const FVector ReadyUpper = Dir(0.3f, 0.3f, -0.9f);
	const FVector ReadyFore = Dir(0.85f, 0.22f, 0.3f);
	/** The tell: raised high beside its head, the fist over its shoulder (the bell hangs clear of its head). */
	const FVector TellUpper = Dir(0.42f, 0.78f, 0.46f);
	const FVector TellFore = Dir(0.18f, 0.28f, 0.94f);
	/** The toll's stroke: the forearm flung out to its side. */
	const FVector StrokeFore = Dir(0.3f, 0.88f, 0.36f);
	/** Its bell shot: the arm knocked back and down. */
	const FVector ReelUpper = Dir(-0.25f, 0.85f, -0.45f);
	const FVector ReelFore = Dir(-0.3f, 0.75f, -0.6f);
	/** The swipe: cocked high behind its shoulder, then swung across in front of it. */
	const FVector CockUpper = Dir(-0.3f, 0.7f, 0.65f);
	const FVector CockFore = Dir(-0.05f, 0.2f, 0.98f);
	const FVector SwingUpper = Dir(0.9f, -0.15f, -0.3f);
	const FVector SwingFore = Dir(0.75f, -0.55f, -0.35f);
	// The rope's arm (its left: -Y).
	/** Low at its side, the rope's end trailing from the fist. */
	const FVector RopeUpper = Dir(0.12f, -0.25f, -0.96f);
	const FVector RopeFore = Dir(0.55f, -0.12f, -0.82f);
	/** The tell: opened out to its side, calling the dead. */
	const FVector CallUpper = Dir(0.25f, -0.85f, 0.2f);
	const FVector CallFore = Dir(0.5f, -0.6f, 0.45f);

	/** The toll's stroke lasts this long into the lowering (s); the swipe's swing this long past its wind-up. */
	constexpr float StrokeSeconds = 0.18f;
	constexpr float SwingSeconds = 0.3f;

	/** How quickly an arm eases to where it wants to be (1/s): slowly carried, quickly raised, at once on a stroke. */
	constexpr float EaseCalm = 6.f;
	constexpr float EaseRaise = 7.f;
	constexpr float EaseSnap = 22.f;

	/** The tell's shake: the wrist flexing this far (degrees) this often (Hz), building through the tell's last part. */
	constexpr float ShakeDegrees = 12.f;
	constexpr float ShakeHz = 7.f;

	/** How much of the Unpaid's own arm still shows through the rope's arm (its claws, its drift). */
	constexpr float LeftOwnShare = 0.3f;

	/** The layer gives way to the Unpaid's slump this fast as it dies (1/s). */
	constexpr float DeathFade = 2.5f;

	/** The props' pendulums: from the fist to the weight's middle (cm at size 1), and how fast each settles (1/s). */
	constexpr float BellPendulum = 15.f;
	constexpr float RopePendulum = 35.f;
	constexpr float BellDamping = 2.2f;
	constexpr float RopeDamping = 3.f;
	/** They swing no farther than this off plumb (radians), and a fist jumping farther than this (cm) in a frame moved, not swung. */
	constexpr float MaxSwing = 1.25f;
	constexpr float JumpDistance = 150.f;
	/** The fist's acceleration counts up to this (cm/s^2): a frame's hitch doesn't fling them. */
	constexpr float MaxGripAcceleration = 6000.f;
	constexpr float Gravity = 980.f;

	/** Eases a direction toward Want at Rate (1/s). */
	FVector EaseDirection(const FVector& Now, const FVector& Want, float Rate, float DeltaSeconds)
	{
		const float Alpha = DeltaSeconds > 0.f ? 1.f - FMath::Exp(-Rate * DeltaSeconds) : 1.f;
		const FVector Eased = (Now + (Want - Now) * Alpha).GetSafeNormal();
		return Eased.IsNearlyZero() ? Want : Eased;
	}

	float Ease(float Alpha)
	{
		return FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f));
	}
}

void ABellRingerCreature::SetupArmLayer()
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
	LeftKnuckle = Rig.LeftArm.Fingers.IsValidIndex(2) ? FindPosedBone(Rig.LeftArm.Fingers[2]) : INDEX_NONE;
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

void ABellRingerCreature::WantedArms(FVector& OutUpper, FVector& OutFore, FVector& OutLeftUpper, FVector& OutLeftFore, float& OutWrist) const
{
	const float Time = RingerClock;
	OutWrist = 0.f;
	OutLeftUpper = RopeUpper;
	OutLeftFore = (RopeFore + FVector(0.f, 0.f, 0.06f * FMath::Sin(Time * 0.9f))).GetSafeNormal();

	if (GetCreatureState() == ECreatureState::Attack)
	{
		// The swipe: cocked through the wind-up, swung across at the strike, then back to the ready.
		const float In = GetStateTime();
		if (In < AttackWindup)
		{
			OutUpper = CockUpper;
			OutFore = CockFore;
		}
		else if (In < AttackWindup + SwingSeconds)
		{
			OutUpper = SwingUpper;
			OutFore = SwingFore;
		}
		else
		{
			OutUpper = ReadyUpper;
			OutFore = ReadyFore;
		}
		return;
	}

	switch (Act)
	{
	case EAct::Tell:
	{
		OutUpper = TellUpper;
		OutFore = TellFore;
		OutLeftUpper = CallUpper;
		OutLeftFore = CallFore;
		// Shaken harder as the toll nears, so the clapper rattles before it strikes.
		const float Build = Ease((ActTime / FMath::Max(Tuning.TellSeconds, 0.01f) - 0.35f) / 0.65f);
		OutWrist = ShakeDegrees * Build * FMath::Sin(2.f * UE_PI * ShakeHz * ActTime);
		return;
	}
	case EAct::Lower:
		if (ActTime < StrokeSeconds)
		{
			OutUpper = TellUpper;
			OutFore = StrokeFore;
			return;
		}
		OutUpper = ReadyUpper;
		OutFore = ReadyFore;
		return;
	case EAct::Reel:
		if (ActTime < Tuning.ReelSeconds * 0.5f)
		{
			OutUpper = ReelUpper;
			OutFore = ReelFore;
			return;
		}
		OutUpper = ReadyUpper;
		OutFore = ReadyFore;
		return;
	default:
		break;
	}
	// Carried, with a slow drift of its own; held up once it hunts.
	const bool bHunting = GetTarget() != nullptr;
	OutUpper = bHunting ? ReadyUpper : CarryUpper;
	const FVector Drift(0.f, 0.05f * FMath::Sin(Time * 1.1f), 0.05f * FMath::Sin(Time * 0.8f + 1.3f));
	OutFore = ((bHunting ? ReadyFore : CarryFore) + Drift).GetSafeNormal();
}

void ABellRingerCreature::LayArm(int32 UpperBone, int32 LowerBone, int32 HandBone, const FVector& RestUpper, const FVector& RestFore,
	const FVector& Upper, const FVector& Fore, float Wrist, float Weight)
{
	// The upper arm turned from its rest to Upper; the forearm's own turn undoes the upper arm's first, so it points along
	// Fore whatever the upper arm did; the hand follows the forearm, the wrist flexed by Wrist. The chest's lean and look
	// stay on top (each turn is under its parent's).
	const FQuat UpperTurn = FQuat::FindBetweenNormals(RestUpper, Upper);
	const FQuat ForeTurn = FQuat::FindBetweenNormals(RestFore, Fore);
	const FQuat UpperOwn = UpperTurn;
	const FQuat LowerOwn = UpperTurn.Inverse() * ForeTurn;
	const FQuat HandOwn(FVector::RightVector, FMath::DegreesToRadians(Wrist));
	Bones[UpperBone].Own = FQuat::Slerp(Bones[UpperBone].Own, UpperOwn, Weight).GetNormalized();
	Bones[LowerBone].Own = FQuat::Slerp(Bones[LowerBone].Own, LowerOwn, Weight).GetNormalized();
	Bones[HandBone].Own = FQuat::Slerp(Bones[HandBone].Own, HandOwn, Weight).GetNormalized();
}

void ABellRingerCreature::LayerPose(float DeltaSeconds)
{
	if (!bArmLayerReady)
	{
		return;
	}
	FVector WantUpper;
	FVector WantFore;
	FVector WantLeftUpper;
	FVector WantLeftFore;
	float Wrist = 0.f;
	WantedArms(WantUpper, WantFore, WantLeftUpper, WantLeftFore, Wrist);

	// Snapped on a stroke, a swing or a shot; raised briskly for the tell; otherwise eased.
	const bool bSnap = (Act == EAct::Lower && ActTime < StrokeSeconds * 1.5f) || Act == EAct::Reel
		|| (GetCreatureState() == ECreatureState::Attack && GetStateTime() >= AttackWindup);
	const float Rate = bSnap ? EaseSnap : (Act == EAct::Tell ? EaseRaise : EaseCalm);
	if (!bArmsStarted || DeltaSeconds <= 0.f)
	{
		NowRightUpper = WantUpper;
		NowRightFore = WantFore;
		NowLeftUpper = WantLeftUpper;
		NowLeftFore = WantLeftFore;
		bArmsStarted = true;
	}
	else
	{
		NowRightUpper = EaseDirection(NowRightUpper, WantUpper, Rate, DeltaSeconds);
		NowRightFore = EaseDirection(NowRightFore, WantFore, Rate, DeltaSeconds);
		NowLeftUpper = EaseDirection(NowLeftUpper, WantLeftUpper, Act == EAct::Tell ? EaseRaise : EaseCalm, DeltaSeconds);
		NowLeftFore = EaseDirection(NowLeftFore, WantLeftFore, Act == EAct::Tell ? EaseRaise : EaseCalm, DeltaSeconds);
	}
	// Dying, it lets go: the Unpaid's slump takes the arms back.
	LayerWeight = DeltaSeconds > 0.f ? FMath::FInterpConstantTo(LayerWeight, IsDead() ? 0.f : 1.f, DeltaSeconds, DeathFade) : (IsDead() ? 0.f : 1.f);
	LayArm(RightUpper, RightLower, RightHand, RestRightUpper, RestRightFore, NowRightUpper, NowRightFore, Wrist, LayerWeight);
	LayArm(LeftUpper, LeftLower, LeftHand, RestLeftUpper, RestLeftFore, NowLeftUpper, NowLeftFore, 0.f, LayerWeight * (1.f - LeftOwnShare));
}

// ---------------------------------------------------------------------------
// The props hanging from its fists
// ---------------------------------------------------------------------------

void ABellRingerCreature::StepSwing(FSwing& Swing, const FVector& GripWorld, float Length, float Damping, float DeltaSeconds) const
{
	// A fist that jumped (a phase-step, a frozen spell thawed) moved the prop with it: it starts hanging still again there.
	if (!Swing.bStarted || DeltaSeconds <= 0.f || FVector::Dist(GripWorld, Swing.LastGrip) > JumpDistance)
	{
		Swing.LastGrip = GripWorld;
		Swing.LastVelocity = FVector::ZeroVector;
		Swing.bStarted = true;
		return;
	}
	const FVector Velocity = (GripWorld - Swing.LastGrip) / DeltaSeconds;
	const FVector Acceleration = ((Velocity - Swing.LastVelocity) / DeltaSeconds).GetClampedToMaxSize(MaxGripAcceleration);
	Swing.LastGrip = GripWorld;
	Swing.LastVelocity = Velocity;
	// In its own frame: forward, to its right, up. The fist's lift pulls harder on the weight, its drop eases it.
	const FVector Local = GetActorRotation().UnrotateVector(Acceleration);
	const float Pull = FMath::Max(Gravity + static_cast<float>(Local.Z), 200.f);
	const float Reach = FMath::Max(Length * GetSizeScale(), 1.f);
	const float Push[2] = { static_cast<float>(Local.X), static_cast<float>(Local.Y) };
	// Short steps: the bell's pendulum swings over a second, and a long frame mustn't throw it.
	const int32 Steps = FMath::Clamp(FMath::CeilToInt32(DeltaSeconds * 120.f), 1, 8);
	const float Step = DeltaSeconds / Steps;
	for (int32 Each = 0; Each < Steps; ++Each)
	{
		for (int32 Axis = 0; Axis < 2; ++Axis)
		{
			double& Angle = Axis == 0 ? Swing.Angle.X : Swing.Angle.Y;
			double& Speed = Axis == 0 ? Swing.Speed.X : Swing.Speed.Y;
			// Gravity pulls it back to plumb; the fist's acceleration leaves it behind; the air and the bail settle it.
			const double Accel = -(Pull * FMath::Sin(Angle) + Push[Axis] * FMath::Cos(Angle)) / Reach - Damping * Speed;
			Speed += Accel * Step;
			Angle += Speed * Step;
			if (FMath::Abs(Angle) > MaxSwing)
			{
				Angle = FMath::Sign(Angle) * MaxSwing;
				Speed = 0.f;
			}
		}
	}
}

void ABellRingerCreature::UpdateProps(float DeltaSeconds)
{
	if (!bArmLayerReady)
	{
		return;
	}
	const FTransform& ToWorld = GetMesh()->GetComponentTransform();
	auto Hang = [&](UStaticMeshComponent* Prop, int32 Hand, int32 Knuckle, FSwing& Swing, float Length, float Damping)
	{
		if (!Prop || !Bones.IsValidIndex(Hand))
		{
			return;
		}
		// This frame's pose, solved already: the mesh takes the same pose this frame, so the prop sits where it's set here.
		const FTransform& HandPose = Bones[Hand].Posed;
		const FVector KnuckleAt = Bones.IsValidIndex(Knuckle) ? Bones[Knuckle].Posed.GetLocation() : HandPose.GetLocation();
		const FVector Grip = BellRingerRules::GripPoint(HandPose.GetLocation(), KnuckleAt);
		StepSwing(Swing, ToWorld.TransformPosition(Grip), Length, Damping, DeltaSeconds);
		// Plumb, then swung: forward is a turn back about its right-hand axis (as a hanging limb's), its right a roll.
		const FQuat Hung = FQuat(FVector::RightVector, -Swing.Angle.X) * FQuat(FVector::ForwardVector, Swing.Angle.Y);
		Prop->SetRelativeTransform(FTransform(Hung, Grip).GetRelativeTransform(HandPose));
	};
	Hang(Bell, RightHand, RightKnuckle, BellSwing, BellPendulum, BellDamping);
	Hang(Rope, LeftHand, LeftKnuckle, RopeSwing, RopePendulum, RopeDamping);
}
