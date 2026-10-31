// AStoneSentinelShockwave's hurt (StoneSentinelShockwave.h): whom its front catches as it runs out (on the ground, on its level,
// not in a gap or behind where a wall or a step stopped it), how it's heard coming, and the throw and shake it gives. Split
// from StoneSentinelShockwave.cpp by topic.

#include "Creatures/StoneSentinelShockwave.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bosses/BossCameraShake.h"
#include "Combat/CombatRules.h"
#include "Combat/EnemyProjectileSubsystem.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/FightDirectorSubsystem.h"
#include "Creatures/FightEvent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** It's heard coming this far (cm, at size 1) before it reaches a player. */
	constexpr float HeardAhead = 300.f;
	/** A hit's shake on their view, and the faint one a jump over it or a gap still gives. */
	constexpr float HitShake = 0.35f;
	constexpr float NearShake = 0.12f;
}

void AStoneSentinelShockwave::SweepPlayers(float FromRadius, float ToRadius)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FSentinelShockTuning& Shock = Launch.Tuning;
	const float Scale = FMath::Max(Launch.Size, 0.05f);
	const AActor* Causer = Launch.Causer.Get();
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PC = It->Get();
		ACharacter* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
		const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
		if (!Capsule || !UEnemyProjectileSubsystem::CanHurt(Causer ? Causer : this, Character) || Struck.Contains(Character))
		{
			continue;
		}
		const FVector Middle = Capsule->GetComponentLocation();
		const float Distance = static_cast<float>(FVector::Dist2D(Launch.Center, Middle));
		const float BodyRadius = Capsule->GetScaledCapsuleRadius();
		const float Bearing = BearingOf(Middle);
		const FSector& Sector = Sectors.IsEmpty() ? FSector() : Sectors[SectorOf(Bearing)];
		const bool bBlocked = Sector.bStopped && Distance - BodyRadius > Sector.StopRadius;
		// Heard as it comes: stone punching up out of the ground on their side, a moment before it reaches them.
		if (!Heard.Contains(Character) && !bBlocked && Distance - BodyRadius - ToRadius <= HeardAhead * Scale)
		{
			Heard.Add(Character);
			const float Radians = FMath::DegreesToRadians(Bearing);
			const FVector Toward(FMath::Cos(Radians), FMath::Sin(Radians), 0.f);
			const FVector Spot = Launch.Center + Toward * FMath::Min(Distance, Shock.MaxRadius * Scale);
			LooterSound::PlayAt(this, FName(StoneSentinelCue::ShockSpikes), FVector(Spot.X, Spot.Y, Sector.Ground.Z));
		}
		if (bBlocked || !StoneSentinelMoves::FrontCrosses(Distance, BodyRadius, FromRadius, ToRadius, Shock.Band * Scale))
		{
			continue;
		}
		const bool bInGap = StoneSentinelMoves::IsInGap(Bearing, Launch.Gaps, Shock.GapDegrees);
		const float GroundZ = Sector.bHasGround ? static_cast<float>(Sector.Ground.Z) : static_cast<float>(Launch.Center.Z);
		const float FeetAbove = static_cast<float>(Middle.Z) - Capsule->GetScaledCapsuleHalfHeight() - GroundZ;
		const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		const bool bFalling = Movement && Movement->IsFalling();
		if (bInGap || !StoneSentinelMoves::Catches(bFalling, FeetAbove, Shock))
		{
			// Passed under them, or by them: felt, not hurt. Once.
			Struck.Add(Character);
			BossCameraShake::Kick(this, Middle, NearShake, 0.3f, 600.f);
			continue;
		}
		Struck.Add(Character);
		++PlayersHit;
		const float Dealt = LooterCombat::RollHitDamage(Launch.Damage, false);
		AActor* Blame = Launch.Causer.IsValid() ? Launch.Causer.Get() : this;
		UGameplayStatics::ApplyDamage(Character, Dealt, Launch.Instigator.Get(), Blame, UCreatureAttackDamageType::StaticClass());
		if (const ACreatureBase* Maker = Cast<ACreatureBase>(Launch.Causer.Get()))
		{
			UFightDirectorSubsystem::Note(*Maker, EFightEvent::AttackLanded, Dealt);
		}
		FVector Away = Middle - Launch.Center;
		Away.Z = 0.0;
		Away = Away.IsNearlyZero() ? FVector::ForwardVector : Away.GetSafeNormal();
		Character->LaunchCharacter(Away * Shock.ThrowBack + FVector(0.0, 0.0, Shock.ThrowUp), true, true);
		BossCameraShake::Kick(this, Middle, HitShake, 0.45f, 600.f);
		UE_LOG(LogLooter, Log, TEXT("%s's shockwave caught %s (%.0f) %.0f cm out."), *GetNameSafe(Launch.Causer.Get()), *Character->GetName(), Dealt, Distance);
	}
}

