// AUnpaidCreature's moods: what each moment asks of its pose (PoseTargets: the channels UnpaidCreatureRig.cpp eases toward
// and lays on its bones), and its reactions (the 2026-10-14 polish, "creatures that react"): the notice beat (its head snaps
// round to its prey and its coal flares bright, it starts back with its jaw open), its rise out of the ground (it comes up
// from under the dirt clawing, the dirt thrown up), its idle life (a slow sway, now and then a look over its shoulder, each
// on its own dice), its crumple and the ash it breaks into as it dies, and the dust its shroud drags as it rushes on.

#include "Creatures/UnpaidCreature.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactRules.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	float UnpaidEase(float Alpha)
	{
		return FMath::InterpEaseInOut(0.f, 1.f, FMath::Clamp(Alpha, 0.f, 1.f), 2.f);
	}

	/** How far it looks round at its target before its body has to turn (degrees). */
	constexpr float LookYawLimit = 60.f;
	constexpr float LookPitchLimit = 35.f;

	/** Its notice beat: it starts back this much (the shriek's rear), its arms flare this much, its coal flares this bright. */
	constexpr float NoticeRear = 0.45f;
	constexpr float NoticeSpread = 0.3f;
	constexpr float NoticeJaw = 22.f;
	constexpr float NoticeFlare = 0.9f;

	/** Rising out of the ground it starts this far under its hang (cm at size 1) and holds its brain this share of the rise. */
	constexpr float RiseDepth = 115.f;
	constexpr float RiseHoldShare = 0.7f;

	/** Calm, it sways this far (degrees of roll), and looks over its shoulder every so often, this far, this long. */
	constexpr float SwayRoll = 2.5f;
	constexpr float GlanceEvery[2] = { 3.5f, 8.f };
	constexpr float GlanceYawRange[2] = { 30.f, 55.f };
	constexpr float GlanceHold[2] = { 0.9f, 2.f };

	/** Rushing on, its shroud drags a puff of dust every this many seconds (cm behind its middle, at size 1). */
	constexpr float ShroudDustSeconds = 0.3f;
	constexpr float ShroudDustBehind = 55.f;
	constexpr float ShroudDustPace = 0.55f;

	/** Dying, ash comes off it this fast (motes a second) while it breaks up, from these of its bones. */
	constexpr float AshPerSecond = 20.f;
}

// ---------------------------------------------------------------------------
// What each moment asks of the pose
// ---------------------------------------------------------------------------

AUnpaidCreature::FPoseChannels AUnpaidCreature::PoseTargets(float Speed, float Time) const
{
	FPoseChannels Goal;
	const float Moving = FMath::Min(Speed, 1.f);
	// Drifting or hanging still: a little hunched, leaning into its drift, the jaw slack and twitching now and then.
	Goal.Lean = 4.f + 10.f * Moving;
	Goal.Jaw = 6.f + 2.5f * FMath::Pow(FMath::Max(FMath::Sin(Time * 0.9f), 0.f), 4.f) * FMath::Sin(Time * 11.f);
	Goal.Curl = 25.f;
	Goal.Splay = 4.f;

	const float InState = GetStateTime();
	switch (GetCreatureState())
	{
	case ECreatureState::Chase:
	{
		// Hungry: thrust forward, mouth open, the arms up and clawing at the air (UnpaidCreatureArms.cpp).
		Goal.Lean = 6.f + 16.f * Moving;
		Goal.Jaw = 12.f;
		Goal.Curl = 30.f;
		Goal.Splay = 8.f;
		Goal.Hunt = 1.f;
		// Its notice beat: it starts back from what it has seen, its arms flaring, its jaw dropping, before the hunger takes it.
		const float Notice = GetNoticeStrength();
		if (Notice > 0.f)
		{
			Goal.Rear = NoticeRear * Notice;
			Goal.Spread = NoticeSpread * Notice;
			Goal.Lean = FMath::Lerp(Goal.Lean, -4.f, Notice);
			Goal.Jaw = FMath::Lerp(Goal.Jaw, NoticeJaw, Notice);
			Goal.Splay = FMath::Lerp(Goal.Splay, 16.f, Notice);
			Goal.Hunt = 1.f - 0.6f * Notice;
		}
		break;
	}

	case ECreatureState::Attack:
		if (InState < AttackWindup)
		{
			// The shriek: it rears back with its arms flung wide and its fingers splayed, the jaw dropping as far as it goes.
			const float Rise = UnpaidEase(InState / FMath::Max(AttackWindup, 0.01f));
			Goal.Rear = Rise;
			Goal.Spread = Rise;
			Goal.Lean = -8.f * Rise;
			Goal.Jaw = FMath::Lerp(12.f, JawOpenDegrees, Rise);
			Goal.Curl = -5.f;
			Goal.Splay = 18.f;
		}
		else
		{
			// The lunge: thrown forward, arms reaching long and clawing, the shroud snapped straight; the recovery eases back.
			const float After = (InState - AttackWindup) / FMath::Max(AttackRecovery, 0.01f);
			const float Reach = bLunging ? 1.f : 1.f - UnpaidEase((After - 0.3f) / 0.7f);
			Goal.Reach = Reach;
			Goal.Spread = 0.15f * Reach;
			Goal.Lean = 6.f + 22.f * Reach;
			Goal.Jaw = FMath::Lerp(12.f, JawOpenDegrees * 0.75f, Reach);
			Goal.Curl = FMath::Lerp(30.f, 48.f, Reach);
			Goal.Splay = 12.f * Reach;
			Goal.Snap = bLunging ? 1.f : 0.f;
		}
		break;

	case ECreatureState::Dead:
		// It slumps as it goes: head down, arms hanging, hands curled, the jaw fallen open; and crumples to the ground (a
		// boss's death is its fight's show).
		Goal.Death = 1.f;
		Goal.Lean = 35.f;
		Goal.Jaw = 25.f;
		Goal.Curl = 50.f;
		Goal.Splay = 0.f;
		Goal.Crumple = IsBossCreature() ? 0.f : 1.f;
		break;

	default:
		break;
	}

	// Rising out of the ground: clawing up out of the dirt, arms reaching and raking, its head thrown back, mouth open.
	if (RiseTime >= 0.f)
	{
		const float Up = 1.f - UnpaidEase(RiseTime / RiseSeconds);
		Goal.Reach = FMath::Max(Goal.Reach, 0.7f * Up);
		Goal.Hunt = FMath::Max(Goal.Hunt, Up);
		Goal.Rear = FMath::Max(Goal.Rear, 0.35f * Up);
		Goal.Jaw = FMath::Max(Goal.Jaw, 20.f * Up);
		Goal.Curl = FMath::Lerp(Goal.Curl, 45.f, Up);
	}

	// It looks at whoever it's after, as far as its neck turns; otherwise it looks about, slowly, now and then over its
	// shoulder, and sways where it hangs.
	if (const APawn* Victim = GetTarget(); Victim && !IsDead())
	{
		const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, 0.7f * GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		const FVector To = FRotator(0.0, GetActorRotation().Yaw, 0.0).UnrotateVector(Victim->GetActorLocation() + FVector(0.f, 0.f, 60.f) - Eye);
		Goal.LookYaw = FMath::Clamp(static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(To.Y, To.X))), -LookYawLimit, LookYawLimit);
		Goal.LookPitch = FMath::Clamp(static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(To.Z, To.Size2D()))), -LookPitchLimit, LookPitchLimit);
	}
	else if (!IsDead())
	{
		Goal.LookYaw = FMath::Clamp(25.f * FMath::Sin(Time * 0.35f) + GlanceYaw, -LookYawLimit, LookYawLimit);
		Goal.LookPitch = -5.f + 4.f * FMath::Sin(Time * 0.23f);
		Goal.Sway = IsBossCreature() ? 0.f : SwayRoll * FMath::Sin(Time * 0.55f) * (1.f - FMath::Min(Speed, 1.f));
	}
	return Goal;
}

// ---------------------------------------------------------------------------
// Its reactions frame by frame
// ---------------------------------------------------------------------------

float AUnpaidCreature::RiseDrop() const
{
	if (RiseTime < 0.f)
	{
		return 0.f;
	}
	// Up out of the dirt quickly at first, settling into its hang (the mesh's space: the actor's size scales it).
	return -RiseDepth * (1.f - FMath::InterpEaseOut(0.f, 1.f, FMath::Clamp(RiseTime / RiseSeconds, 0.f, 1.f), 2.f));
}

void AUnpaidCreature::TickReactions(float DeltaSeconds)
{
	if (RiseTime >= 0.f)
	{
		RiseTime += DeltaSeconds;
		if (RiseTime >= RiseSeconds)
		{
			RiseTime = -1.f;
		}
	}
	if (IsDead())
	{
		return;
	}
	// Its notice beat: its coal flares bright (the material's Heat, and the coal swells with it).
	const float Notice = GetNoticeStrength();
	if (Notice > 0.f)
	{
		Heat = FMath::Max(Heat, NoticeFlare * Notice);
	}
	UCreatureReactComponent* Reacts = GetReact();
	if (!Reacts)
	{
		return;
	}
	FRandomStream& Dice = Reacts->GetRandom();
	// Calm, now and then it looks over its shoulder a while (PoseTargets eases its head there and back).
	const ECreatureState BrainState = GetCreatureState();
	const bool bCalm = !GetTarget() && (BrainState == ECreatureState::Idle || BrainState == ECreatureState::Wander) && !IsBossCreature();
	if (NextGlance < 0.f)
	{
		NextGlance = Dice.FRandRange(1.f, GlanceEvery[1]);
	}
	if (GlanceLeft > 0.f)
	{
		GlanceLeft -= DeltaSeconds;
		if (GlanceLeft <= 0.f || !bCalm)
		{
			GlanceLeft = 0.f;
			GlanceYaw = 0.f;
		}
	}
	else if (bCalm)
	{
		NextGlance -= DeltaSeconds;
		if (NextGlance <= 0.f)
		{
			GlanceYaw = (Dice.FRand() < 0.5f ? -1.f : 1.f) * Dice.FRandRange(GlanceYawRange[0], GlanceYawRange[1]);
			GlanceLeft = Dice.FRandRange(GlanceHold[0], GlanceHold[1]);
			NextGlance = Dice.FRandRange(GlanceEvery[0], GlanceEvery[1]);
		}
	}
	// Rushing at its prey, its shroud's tail drags a low puff of dust off the ground behind it.
	const float Pace = static_cast<float>(GetVelocity().Size2D());
	if (BrainState == ECreatureState::Chase && !IsPhasing() && !IsPoseFrozen() && Pace > ChaseSpeed * ShroudDustPace)
	{
		ShroudDustClock -= DeltaSeconds;
		if (ShroudDustClock <= 0.f)
		{
			ShroudDustClock = ShroudDustSeconds;
			const float Scale = GetSizeScale();
			const FVector Feet = GetActorLocation() - FVector(0.0, 0.0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
			Reacts->KickFootfall(Feet - GetActorForwardVector() * (ShroudDustBehind * Scale), GetVelocity(), Scale * 1.1f);
		}
	}
}

void AUnpaidCreature::OnNoticed()
{
	// Its coal flares (TickReactions) with a breath drawn in, as its head snaps round to them (AnimateBody's quicker look).
	Voice->Play(NoticeCue.IsNone() ? FName(CreatureReactCue::UnpaidNotice) : NoticeCue);
}

void AUnpaidCreature::PlayEntrance()
{
	// Up through the dirt where it stands, its brain waiting most of the rise (RiseIn's fade, claw and dirt).
	RiseIn();
	HoldBrain(RiseSeconds * RiseHoldShare);
	LooterSound::PlayAt(this, FName(CreatureReactCue::UnpaidRise), GetActorLocation(), 1.f, Voice->GetPitch());
}

void AUnpaidCreature::ShedDeathAsh(float DeltaSeconds)
{
	UCreatureReactComponent* Reacts = GetReact();
	if (!Reacts || PhaseAmount <= 0.f || PhaseAmount >= 1.f || DeltaSeconds <= 0.f)
	{
		// Not breaking up (below 0: the next time it starts to, its crumble is heard first).
		AshOwed = -1.f;
		return;
	}
	if (AshOwed < 0.f)
	{
		AshOwed = 0.f;
		Voice->Play(FName(CreatureReactCue::UnpaidAsh));
	}
	AshOwed += AshPerSecond * DeltaSeconds;
	const int32 Count = FMath::FloorToInt32(AshOwed);
	AshOwed -= Count;
	if (Count <= 0)
	{
		return;
	}
	// From about its body as it lies now (a random bone of the posed rig), drifting up and off downwind of its fall.
	const FTransform& MeshXf = GetMesh()->GetComponentTransform();
	FRandomStream& Dice = Reacts->GetRandom();
	const FVector Drift = GetActorForwardVector() * 20.f + FVector(Dice.FRandRange(-15.f, 15.f), Dice.FRandRange(-15.f, 15.f), 0.f);
	for (int32 Mote = 0; Mote < Count; ++Mote)
	{
		const FVector From = bRigReady && Bones.Num() > 0
			? MeshXf.TransformPosition(Bones[Dice.RandRange(0, Bones.Num() - 1)].Posed.GetLocation())
			: GetActorLocation() + FVector(0.0, 0.0, Dice.FRandRange(-40.f, 40.f));
		Reacts->ShedAsh(From, Drift, GetSizeScale(), 1);
	}
}
