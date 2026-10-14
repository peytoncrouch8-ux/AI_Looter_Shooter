// ABellRingerCreature's place in a fight: it hangs back behind its pack (the creature brain's chase brings it to its
// hold distance, set as its attack range, and stops it there facing its target), backs off a little when its target comes
// too near (slower than a player walks, never off its ground or into a safe zone), and run down, swipes with the bell in
// place of the Unpaid's lunge. While it's at the bell it holds its ground: no steps, no phase-steps.

#include "Creatures/BellRingerCreature.h"
#include "Audio/LooterSound.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Creatures/EncounterSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** It looks this far along a way back before taking it (cm at size 1): a body's length and a half. */
	constexpr float BackOffLook = 180.f;

	/** It turns to keep its eyes on its target this fast (degrees a second) while it stands at the bell or backs off. */
	constexpr float FaceRate = 360.f;

	/** The swipe lands on a target this far in front of it at most (the cosine off its facing). */
	constexpr float SwipeFront = 0.3f;

	/** The swing throws the bell forward on its bail (radians a second). */
	constexpr float SwipeKick = 6.f;
}

void ABellRingerCreature::TickStance(float DeltaSeconds)
{
	SinceBackOff = FMath::Min(SinceBackOff + DeltaSeconds, 1000.f);
	const APawn* Victim = GetTarget();
	// In its notice beat it stands, its bell knocking, before it keeps its distance.
	if (!Victim || Act != EAct::None || IsPhasing() || GetCreatureState() != ECreatureState::Chase || IsNoticing())
	{
		StopBackingOff();
		Stance = EBellRingerStance::Hold;
		return;
	}
	if (bBackingOff)
	{
		BackOffTime += DeltaSeconds;
	}
	const float Distance = static_cast<float>(FVector::Dist2D(GetActorLocation(), Victim->GetActorLocation()));
	Stance = BellRingerRules::ChooseStance(Tuning, Distance, bBackingOff, BackOffTime, SinceBackOff);
	FVector Away;
	if (Stance == EBellRingerStance::BackOff && FindBackOff(Away))
	{
		if (!bBackingOff)
		{
			bBackingOff = true;
			BackOffTime = 0.f;
		}
		// Backing away with its eyes (and its bell) on them: the brain only faces them this near, so the step is its own.
		SetTurnsWithMovement(false);
		GetCharacterMovement()->MaxWalkSpeed = ChaseSpeed * Tuning.BackOffSpeedShare;
		AddMovementInput(Away);
		FaceTarget(DeltaSeconds);
		return;
	}
	// Nowhere to back off to: cornered, it holds (and swipes once it's run down).
	if (Stance == EBellRingerStance::BackOff)
	{
		Stance = EBellRingerStance::Hold;
	}
	StopBackingOff();
}

bool ABellRingerCreature::FindBackOff(FVector& OutDirection) const
{
	const APawn* Victim = GetTarget();
	if (!Victim)
	{
		return false;
	}
	const FVector Here = GetActorLocation();
	const FVector HomeSpot = GetHome().GetLocation();
	const float Look = BackOffLook * GetSizeScale();
	for (const FVector& Way : BellRingerRules::BackOffDirections(Here, Victim->GetActorLocation()))
	{
		// Clear and with ground under it (never off a drop), on its ground, and never over Delia's salt line.
		if (!LookAlong(Way, Look).bClear)
		{
			continue;
		}
		const FVector Ahead = Here + Way * Look;
		if ((HuntingGround.IsSet() && !HuntingGround.Contains(Ahead, HomeSpot)) || UEncounterSubsystem::IsSheltered(this, Ahead))
		{
			continue;
		}
		OutDirection = Way;
		return true;
	}
	return false;
}

void ABellRingerCreature::StopBackingOff()
{
	if (!bBackingOff)
	{
		return;
	}
	bBackingOff = false;
	SinceBackOff = 0.f;
	SetTurnsWithMovement(Act == EAct::None);
}

void ABellRingerCreature::FaceTarget(float DeltaSeconds)
{
	const APawn* Victim = GetTarget();
	const FVector To = Victim ? Victim->GetActorLocation() - GetActorLocation() : FVector::ZeroVector;
	if (To.SizeSquared2D() < 1.0)
	{
		return;
	}
	const FRotator Wanted(0.f, To.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), Wanted, DeltaSeconds, FaceRate));
}

void ABellRingerCreature::SetTurnsWithMovement(bool bTurns)
{
	if (bTurns != bTurnsWithMovement)
	{
		bTurnsWithMovement = bTurns;
		GetCharacterMovement()->bOrientRotationToMovement = bTurns;
	}
}

void ABellRingerCreature::HoldStill()
{
	// The brain's steering this frame goes unused, as the Unpaid's does while it phases...
	ConsumeMovementInputVector();
	// ...and no phase-step carries it off mid-toll: its cooldown is held out of reach until it's done (SetAct puts it back).
	if (HeldPhaseCooldown < 0.f)
	{
		HeldPhaseCooldown = PhaseStep.Cooldown;
		PhaseStep.Cooldown = 1.0e6f;
	}
}

// ---------------------------------------------------------------------------
// The swipe: its attack, only when run down
// ---------------------------------------------------------------------------

float ABellRingerCreature::GetAttackStartRange() const
{
	// Its attack range holds it back (the brain's chase stops at it); it swipes only from much nearer.
	return Tuning.SwipeRange * GetSizeScale();
}

bool ABellRingerCreature::CanStartAttack() const
{
	return Act == EAct::None && !IsPhasing();
}

void ABellRingerCreature::OnAttackStarted()
{
	// No shriek ring and no lunge (the Unpaid's): the wind-up is the bell cocked high behind it (BellRingerCreaturePose.cpp).
	StopBackingOff();
}

void ABellRingerCreature::Strike()
{
	SwipeHit();
	OnAttackStrike(false);
}

void ABellRingerCreature::SwipeHit()
{
	// The bell swung across in front of it: its rush and clank, and it lands on a target in reach before it.
	LooterSound::PlayAttached(LooterSoundCue::BellRinger::Swipe, Bell, NAME_None, 1.f, Voice->GetPitch());
	BellSwing.Speed.X += SwipeKick;
	APawn* Victim = GetTarget();
	if (!IsValidTarget(Victim))
	{
		return;
	}
	const float Scale = GetSizeScale();
	const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
	const bool bInReach = ToVictim.Size2D() <= Tuning.SwipeReach * Scale && FMath::Abs(ToVictim.Z) < 220.f * FMath::Max(Scale, 1.f);
	const bool bInFront = FVector::DotProduct(GetActorForwardVector(), ToVictim.GetSafeNormal2D()) > SwipeFront;
	if (bInReach && bInFront)
	{
		HitWithAttack(Victim, ToVictim.GetSafeNormal2D());
	}
}
