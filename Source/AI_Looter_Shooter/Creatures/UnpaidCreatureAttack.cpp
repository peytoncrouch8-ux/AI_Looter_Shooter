// AUnpaidCreature's attack: the shriek, which is its lunge's wind-up (a Gravebound one's sends out a ring that slows the
// player, AShriekRing), and the lunge, which flies at its target and lands if it reaches them. Back off during the shriek
// and the lunge falls short.

#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/ShriekRing.h"
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
	return !IsPhasing() && !bLunging;
}

void AUnpaidCreature::OnAttackStarted()
{
	// The wind-up is the shriek (the pose drops the jaw and spreads the arms through it).
	bLunging = false;
	bLungeLanded = false;
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
	// The lunge: it flies at its target and the strike lands if it reaches them. Never off a drop: it's cut short where
	// the ground ends, as the slime's hop is.
	const float Scale = GetSizeScale();
	const APawn* Victim = GetTarget();
	FVector ToVictim = Victim ? Victim->GetActorLocation() - GetActorLocation() : GetActorForwardVector() * (LungeDistance * Scale);
	ToVictim.Z = 0.f;
	LungeDirection = ToVictim.IsNearlyZero() ? GetActorForwardVector() : ToVictim.GetSafeNormal();
	float Length = FMath::Clamp(static_cast<float>(ToVictim.Size()) - LungeStopShort * Scale, 0.f, LungeDistance * Scale);
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector Ground;
	while (Length > 40.f * Scale && !FindGround(GetActorLocation() + LungeDirection * Length, 50.f * Scale, HalfHeight + 350.f * Scale, Ground))
	{
		Length *= 0.6f;
	}
	LungeLeft = Length > 40.f * Scale ? Length : 0.f;
	LungeTime = 0.f;
	bLunging = LungeLeft > 0.f;
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
	if (LungeLeft <= 0.f || LungeTime > LungeDistance * GetSizeScale() / FMath::Max(Speed, 1.f) + LungeGrace)
	{
		EndLunge();
		return;
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
