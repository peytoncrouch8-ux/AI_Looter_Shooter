// AUnpaidCreature's attack: the shriek, which is its lunge's wind-up (a Gravebound one's sends out a ring that slows the
// player, AShriekRing), and the lunge, which flies at its target and lands if it reaches them. Back off during the shriek
// and the lunge falls short. With its fighting kit (UnpaidCreatureMoves.cpp) the wind-up begins with a choice (a rake close
// in), and the lunge starts from farther out, aims where the target will be and bends a little after them in flight.

#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/CreatureVoiceComponent.h"
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

bool AUnpaidCreature::CanStartAttack() const
{
	// Not mid-cinder or mid-scream either: its turn waits for them.
	return !IsPhasing() && !bLunging && Kit.Action == EUnpaidAction::None;
}

float AUnpaidCreature::GetAttackStartRange() const
{
	return HasFightKit() ? FMath::Max(FUnpaidFightTuning::Live().LungeStart * GetSizeScale(), GetAttackRange()) : Super::GetAttackStartRange();
}

void AUnpaidCreature::OnAttackStarted()
{
	// The wind-up is the shriek (the pose drops the jaw and spreads the arms through it), or with its kit the rake's arm drawn back.
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
	LungeLeft -= Speed * DeltaSeconds;
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
	// Up and down, a small one still reaches a player standing over it, as a full-size one does.
	if (ToVictim.Size2D() > LungeHitRadius * Scale || FMath::Abs(ToVictim.Z) > 220.f * FMath::Max(Scale, 1.f))
	{
		return false;
	}
	bLungeLanded = true;
	HitWithAttack(Victim, ToVictim.GetSafeNormal2D());
	return true;
}

void AUnpaidCreature::EndLunge()
{
	// It stops dead where the lunge ends: the dead don't skid.
	bLunging = false;
	TryLungeHit();
	GetCharacterMovement()->StopMovementImmediately();
}
