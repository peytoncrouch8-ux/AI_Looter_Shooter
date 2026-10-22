// ACreatureBase: its attack's frame (the wind-up, the strike, the recovery) and its blow landing on a victim. Moved out of
// CreatureBase.cpp (2026-10-14) to keep it under the size guide as its reactions came in.

#include "Creatures/CreatureBase.h"
#include "AI_Looter_Shooter.h"
#include "Combat/CombatRules.h"
#include "Combat/LooterDamageTypes.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/**
	 * Nothing solid stands between a creature and its victim: a strike through a wall, a shut door or the eleven's
	 * barricade never lands (the 2026-10-14 play-through: the depot's dead hit the player through the stair door, the
	 * Foreman through his barricade). The world's solid shapes only, as grave salt's sight: volumes are skipped
	 * (StaticGeometryParams), and bodies and loot aren't world-static, so another creature never shields the victim.
	 */
	bool IsStrikeClear(const ACreatureBase& Creature, const APawn& Victim)
	{
		const UWorld* World = Creature.GetWorld();
		if (!World)
		{
			return true;
		}
		FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("CreatureStrike"), &Creature, false);
		Params.AddIgnoredActor(&Victim);
		const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
		const FVector From = Creature.GetActorLocation();
		// The victim's middle, or else its feet: a spider biting at a player stood on a rock's lip reaches the boots
		// over the edge even when the line to the chest crosses the rock.
		const float Half = Victim.GetSimpleCollisionHalfHeight();
		for (const FVector& To : { Victim.GetActorLocation(), Victim.GetActorLocation() - FVector(0.f, 0.f, Half * 0.7f) })
		{
			if (!World->LineTraceTestByObjectType(From, To, Solid, Params))
			{
				return true;
			}
		}
		return false;
	}
}

void ACreatureBase::TickAttack(float DeltaSeconds)
{
	APawn* Victim = Target.Get();
	if (Victim && StateTime < AttackWindup && TracksTargetInWindup())
	{
		// Track the target through the wind-up; commit to the direction once the strike starts (or once a charge has
		// taken its aim).
		FaceToward(Victim->GetActorLocation(), DeltaSeconds);
	}

	if (!bStruck && StateTime >= AttackWindup)
	{
		bStruck = true;
		Strike();
	}

	if (StateTime >= AttackWindup + AttackRecovery)
	{
		CooldownRemaining = AttackCooldown;
		// Its turn is over: back to the ring (or its spot) while the next one comes in (CreatureBaseFight.cpp).
		EndFightTurn();
		ResumeHunt();
	}
}

void ACreatureBase::Strike()
{
	bool bConnected = false;
	APawn* Victim = Target.Get();
	if (IsValidTarget(Victim))
	{
		const FVector ToVictim = Victim->GetActorLocation() - GetActorLocation();
		// Up and down, a small creature still reaches a player standing over it, as a full-size one does.
		const bool bInReach = ToVictim.Size2D() <= GetStrikeReach() && FMath::Abs(static_cast<float>(ToVictim.Z)) < GetStrikeHeight();
		const bool bInFront = FVector::DotProduct(GetActorForwardVector(), ToVictim.GetSafeNormal2D()) > 0.4f;
		if (bInReach && bInFront && IsStrikeClear(*this, *Victim))
		{
			HitWithAttack(Victim, ToVictim.GetSafeNormal2D());
			bConnected = true;
		}
	}
	// The lunge is purely visual (the subclass animates it); a physical shove would push the creature into
	// and past a target standing in melee range.
	OnAttackStrike(bConnected);
}

void ACreatureBase::HitWithAttack(APawn* Victim, const FVector& Push, float Strength)
{
	// Attacks roll in the same damage range as player weapons; a regular creature's hit harder by the fight's difficulty
	// (Looter.Fight.DamageScale; a boss's own moves keep theirs).
	const float Hardness = FMath::Max(Strength, 0.f);
	const float Damage = LooterCombat::RollHitDamage(AttackDamage * Hardness * GetFightDamageScale(), false);
	UGameplayStatics::ApplyDamage(Victim, Damage, GetController(), this, UCreatureAttackDamageType::StaticClass());
	UE_LOG(LogLooter, Verbose, TEXT("%s hit %s for %.1f"), *GetName(), *GetNameSafe(Victim), Damage);
	// Shove the victim back so the hit is felt, not just read on the health bar.
	if (ACharacter* VictimCharacter = Cast<ACharacter>(Victim))
	{
		VictimCharacter->LaunchCharacter((Push * 450.f + FVector(0.f, 0.f, 180.f)) * Hardness, true, false);
	}
}
