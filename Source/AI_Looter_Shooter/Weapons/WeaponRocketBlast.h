#pragma once

#include "CoreMinimal.h"
#include "Weapons/GunKindsTuning.h"

class AActor;
class AController;
class AWeaponBase;
class UWorld;
struct FHitResult;

/** One body a rocket's burst reaches, and what it does to it. */
struct FRocketBlastTarget
{
	TWeakObjectPtr<AActor> Actor;
	/** The point of the body nearest the burst, and how far that is (cm; 0 for what the rocket struck). */
	FVector Point = FVector::ZeroVector;
	float Distance = 0.f;
	/** The rocket's own shooter (they take a share: FBlastRules::SelfScale). */
	bool bShooter = false;
	/** The damage it takes (falloff and the shooter's share counted). */
	float Damage = 0.f;
};

/** What a burst did: bodies hurt and killed, and whether its shooter was caught in it. */
struct FRocketBlastResult
{
	FVector Center = FVector::ZeroVector;
	int32 Hits = 0;
	int32 Kills = 0;
	float ShooterDamage = 0.f;
};

/**
 * The launcher's rocket bursting (AWeaponRocket calls Detonate), as free functions the tests reach step by step, in the
 * grave-salt burst's ways (GraveSaltBurst: the same reach to a body's nearest side and the same sight line, so nothing is
 * hurt through a wall, rock or the ground):
 *  - its damage at a distance: the gun's damage in full within the burst's heart, falling to its edge's share at its reach
 *    (GunKinds::BlastFalloff); what the rocket struck takes it whole;
 *  - every living thing with health in reach and in sight, its shooter among them at FBlastRules::SelfScale (no other
 *    player); the damage is the gun's (UWeaponDamageType, the launcher its causer, so the kill is the gun's notch and the
 *    creature's reaction knows the launcher) but the shooter's own, which comes from the burst itself;
 *  - the living near the heart thrown out of it (GraveSaltBurst::KnockAway: never a boss, never over an edge); loose
 *    physics bodies shoved; the players' views jolted by distance; the look and the sound (WeaponRocketBlastEffects.cpp).
 */
namespace WeaponRocketBlast
{
	/** The damage a body Distance from the burst takes of Damage (its shooter's share when bShooter). */
	AI_LOOTER_SHOOTER_API float DamageAt(float Damage, float Distance, const GunKinds::FBlastRules& Rules, bool bShooter);

	/** Something a burst can hurt: health, alive; a player's pawn only if it's Shooter's own. */
	AI_LOOTER_SHOOTER_API bool CanHurt(const AActor* Actor, const AActor* Shooter);

	/** Every body the burst at Center reaches, nearest first (Struck, what the rocket hit, at 0 and whatever the sight line). */
	AI_LOOTER_SHOOTER_API TArray<FRocketBlastTarget> FindTargets(const UWorld* World, const FVector& Center, const AActor* Shooter,
		const AActor* Struck, float Damage, const GunKinds::FBlastRules& Rules);

	/**
	 * The whole burst at Center (Up: the way it throws, the surface's normal) for Weapon in Shooter's hands (either may be
	 * gone: the burst still happens), Damage at the heart. Hit is the surface or body it met (null: it burst in the air).
	 */
	AI_LOOTER_SHOOTER_API FRocketBlastResult Detonate(UWorld* World, const FVector& Center, const FVector& Up, AWeaponBase* Weapon,
		AActor* Shooter, AController* Instigator, float Damage, float Impulse, const FHitResult* Hit, AActor* BurstSource);

	// --- WeaponRocketBlastEffects.cpp ---

	/** The burst seen: a white-hot flash, a fireball of embers, a ring of dark smoke rolling out, grit and dirt thrown up. */
	AI_LOOTER_SHOOTER_API void SpawnEffects(UWorld& World, const FVector& Center, const FVector& Up, const FHitResult* Hit);

	/** The launch's back-blast out of the tube's exhaust (Location, the way it faces): a puff of flame and a cone of smoke. */
	AI_LOOTER_SHOOTER_API void SpawnBackblast(UWorld& World, const FVector& Location, const FVector& Direction);

	/** One frame of a rocket's flight seen: the motor's flame at Exhaust (flying along Direction) and, every so often, smoke. */
	AI_LOOTER_SHOOTER_API void DrawFlame(UWorld& World, const FVector& Exhaust, const FVector& Direction);
	AI_LOOTER_SHOOTER_API void PuffSmoke(UWorld& World, const FVector& Exhaust, const FVector& Direction, FRandomStream& Random);

	/** The burst's sound cue (Weapon.Launcher.Explosion). */
	AI_LOOTER_SHOOTER_API FName ExplosionCue();
}
