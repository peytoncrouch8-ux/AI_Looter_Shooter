// AUnpaidCreature's attack: the shriek, which is its lunge's wind-up (a Gravebound one's sends out a ring that slows the
// player, AShriekRing), and the lunge, which flies at its target and lands if it reaches them. Back off during the shriek
// and the lunge falls short. With its fighting kit (UnpaidCreatureMoves.cpp) the wind-up begins with a choice (a rake close
// in), and the lunge starts from farther out, aims where the target will be and bends a little after them in flight.
//
// A gun's hit slows it (UMovementSlowComponent, 55% for 0.12 s a hit): the flight honours the slow, but flies its whole
// way, and its last stretch (Looter.Unpaid.LungeCommit) is committed, the slow held off, so steady fire can't keep every
// lunge short (DEMO 2026-10-28: 59 attacks in the chapel yard, 2 hits). Its slows come once a gap, too (GetHitSlowGap).
// With its kit it attacks only a target it sees (SeesTarget, Looter.Unpaid.Sight): no wind-up, flight or landing through a
// wall (DEMO 2026-10-28: Unpaid outside the chapel nave's wall lunged into it every ~2.6 s, one 21 times, landing none).

#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
#include "Combat/MovementSlowComponent.h"
#include "Creatures/ShriekRing.h"
#include "Creatures/UnpaidFightRules.h"
#include "Creatures/UnpaidFightTuning.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	/** A lunge never flies longer than this past what its distance takes (s): against a wall it gives up. */
	constexpr float LungeGrace = 0.12f;
}

float AUnpaidCreature::GetLungeSpeed() const
{
	return LungeSpeed * Traits.LungeSpeedScale * GetSizeScale();
}

float AUnpaidCreature::GetHitSlowGap() const
{
	return HasFightKit() ? FUnpaidFightTuning::Live().HitSlowGap : 0.f;
}

bool AUnpaidCreature::CanStartAttack() const
{
	// Not mid-cinder or mid-scream either: its turn waits for them; nor with a flank ready (its turn opens with the flank).
	if (IsPhasing() || bLunging || Kit.Action != EUnpaidAction::None || Kit.bFlankArmed)
	{
		return false;
	}
	// Nor right after a blink lands (SinceLastStep restarts as it's whole again): a beat to see where it came back.
	if (HasFightKit() && SinceLastStep < FUnpaidFightTuning::Live().ArrivalHold)
	{
		return false;
	}
	// Nor at a target it can't see (asked last: its trace, kept a moment, only when all else says go).
	const APawn* Victim = GetTarget();
	return !Victim || !NeedsSight() || SeesTarget(*Victim);
}

float AUnpaidCreature::GetAttackStartRange() const
{
	return HasFightKit() ? FMath::Max(FUnpaidFightTuning::Live().LungeStart * GetSizeScale(), GetAttackRange()) : Super::GetAttackStartRange();
}

void AUnpaidCreature::OnAttackStarted()
{
	// The wind-up is the shriek (the pose drops the jaw and spreads the arms through it), or with its kit the rake's arm drawn back.
	CommitLunge(false);
	bLunging = false;
	bLungeLanded = false;
	ChooseKitMove();
	Shriek();
}

AShriekRing* AUnpaidCreature::Shriek()
{
	APawn* Victim = GetTarget();
	ACharacter* Character = Cast<ACharacter>(Victim);
	if (!Traits.bSlowingShriek || !Character || !IsValidTarget(Victim) || SinceSlowingShriek < ShriekCooldown)
	{
		return nullptr;
	}
	SinceSlowingShriek = 0.f;
	const float Scale = GetSizeScale();
	const FVector Feet = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	UE_LOG(LogLooter, Verbose, TEXT("%s shrieks at %s."), *GetName(), *GetNameSafe(Victim));
	return AShriekRing::Spawn(GetWorld(), Feet, ShriekRadius * Scale, GetCoalColor(), Character, ShriekSlowMultiplier, ShriekSlowSeconds);
}

void AUnpaidCreature::Strike()
{
	if (Kit.Move == EUnpaidMove::Rake)
	{
		// Its kit's rake: the first swipe now, the second a beat later (TickRake).
		OnAttackStrike(Swipe(Kit.LeadArm));
		return;
	}
	// The lunge: it flies at its target and the strike lands if it reaches them. Never off a drop: it's cut short where
	// the ground ends, as the slime's hop is. With its kit it flies at where they'll be, and farther.
	const float Scale = GetSizeScale();
	const APawn* Victim = GetTarget();
	const bool bKit = Kit.Move == EUnpaidMove::Lunge;
	const FUnpaidFightTuning& Tuning = FUnpaidFightTuning::Live();
	const float Reach = (bKit ? Tuning.LungeDistance : LungeDistance) * Scale;
	// Never into a wall: its target gone out of its sight through the wind-up (round a corner, behind a door leaf), it holds
	// where it is, its attack spent (looked at now, not a moment ago).
	if (bKit && Victim && Tuning.Sight != 0 && !SeesTarget(*Victim, true))
	{
		bLunging = false;
		LungeLeft = 0.f;
		Kit.LungeReach = Reach;
		OnAttackStrike(false);
		return;
	}
	FVector Aim = GetActorLocation() + GetActorForwardVector() * Reach;
	if (Victim)
	{
		Aim = bKit ? UnpaidFightRules::LeadPoint(GetActorLocation(), Victim->GetActorLocation(), Victim->GetVelocity(), GetLungeSpeed(),
			Tuning.LeadShare, Tuning.LeadMaxSeconds) : Victim->GetActorLocation();
	}
	FVector ToVictim = Aim - GetActorLocation();
	ToVictim.Z = 0.f;
	LungeDirection = ToVictim.IsNearlyZero() ? GetActorForwardVector() : ToVictim.GetSafeNormal();
	float Length = FMath::Clamp(static_cast<float>(ToVictim.Size()) - LungeStopShort * Scale, 0.f, Reach);
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector Ground;
	while (Length > 40.f * Scale && !FindGround(GetActorLocation() + LungeDirection * Length, 50.f * Scale, HalfHeight + 350.f * Scale, Ground))
	{
		Length *= 0.6f;
	}
	LungeLeft = Length > 40.f * Scale ? Length : 0.f;
	LungeTime = 0.f;
	Kit.LungeReach = Reach;
	bLunging = LungeLeft > 0.f;
	// A short flight is committed from its first frame.
	CommitLunge(bLunging && LungeLeft <= GetLungeSpeed() * Tuning.LungeCommit);
	// The rush of cloth and cold air as it flies (or swipes where it stands); the shriek before it was the wind-up's cry.
	Voice->Play(LooterSoundCue::UnpaidLunge);
	if (!bLunging)
	{
		// Already in reach, or nowhere to fly: it strikes where it stands.
		TryLungeHit();
	}
	OnAttackStrike(bLungeLanded);
}

void AUnpaidCreature::TickLunge(float DeltaSeconds)
{
	LungeTime += DeltaSeconds;
	if (TryLungeHit())
	{
		EndLunge();
		return;
	}
	const float Speed = GetLungeSpeed();
	// Its last stretch is committed: the hit slow is held off it. Before that a slow drags its flight (the movement clamps
	// to the held speed), so the way left and the wall-grace count the way it really flew, never the way it meant to.
	if (!bLungeCommitted && LungeLeft <= Speed * FUnpaidFightTuning::Live().LungeCommit)
	{
		CommitLunge(true);
	}
	const UMovementSlowComponent* Slow = bLungeCommitted ? nullptr : FindComponentByClass<UMovementSlowComponent>();
	const float Flown = Slow ? Slow->GetMultiplier() : 1.f;
	LungeLeft -= Speed * Flown * DeltaSeconds;
	LungeTime += (Flown - 1.f) * DeltaSeconds;
	const float Reach = Kit.LungeReach > 0.f ? Kit.LungeReach : LungeDistance * GetSizeScale();
	if (LungeLeft <= 0.f || LungeTime > Reach / FMath::Max(Speed, 1.f) + LungeGrace)
	{
		EndLunge();
		return;
	}
	// With its kit it bends after a target who sidesteps, but only so fast: a late dodge still beats it.
	const APawn* Victim = GetTarget();
	if (Kit.Move == EUnpaidMove::Lunge && Victim)
	{
		LungeDirection = UnpaidFightRules::TurnToward(LungeDirection, Victim->GetActorLocation() - GetActorLocation(),
			FUnpaidFightTuning::Live().LungeHoming * DeltaSeconds);
	}
	// Flown, not walked: full speed at once (the movement keeps it on the ground and off the walls).
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxWalkSpeed = Speed;
	Movement->Velocity = FVector(LungeDirection * Speed) + FVector(0.f, 0.f, Movement->Velocity.Z);
	AddMovementInput(LungeDirection);
}

bool AUnpaidCreature::TryLungeHit()
{
	APawn* Victim = GetTarget();
	if (bLungeLanded || !IsValidTarget(Victim))
	{
		return false;
	}
	const float Scale = GetSizeScale();
	const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
	// With its kit the lunge lands only near its line and hurts its share (a strafe after the tell beats it; DEMO 2026-10-27).
	const bool bKit = Kit.Move == EUnpaidMove::Lunge;
	const FUnpaidFightTuning Tuning = bKit ? FUnpaidFightTuning::Live() : FUnpaidFightTuning();
	const float Radius = bKit ? Tuning.LungeHitRadius : LungeHitRadius;
	// Up and down, a small one still reaches a player standing over it, as a full-size one does.
	if (ToVictim.Size2D() > Radius * Scale || FMath::Abs(ToVictim.Z) > 220.f * FMath::Max(Scale, 1.f))
	{
		return false;
	}
	// Never through a wall (a thin one between, in reach): looked at now, only once it's that near.
	if (bKit && Tuning.Sight != 0 && !SeesTarget(*Victim, true))
	{
		return false;
	}
	bLungeLanded = true;
	HitWithAttack(Victim, ToVictim.GetSafeNormal2D(), bKit ? Tuning.LungeDamageShare : 1.f);
	return true;
}

void AUnpaidCreature::EndLunge()
{
	// It stops dead where the lunge ends: the dead don't skid. Any slow it took on the way holds it again now.
	bLunging = false;
	TryLungeHit();
	GetCharacterMovement()->StopMovementImmediately();
	CommitLunge(false);
}

void AUnpaidCreature::CommitLunge(bool bOn)
{
	if (bOn == bLungeCommitted)
	{
		return;
	}
	bLungeCommitted = bOn;
	// Added now if it has none, so a hit that lands in the committed stretch finds it suspended.
	if (UMovementSlowComponent* Slow = bOn ? UMovementSlowComponent::FindOrAdd(this) : FindComponentByClass<UMovementSlowComponent>())
	{
		Slow->Suspend(bOn);
	}
}
