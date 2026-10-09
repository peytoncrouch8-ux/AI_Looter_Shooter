#pragma once

#include "CoreMinimal.h"
#include "Combat/GraveSaltRules.h"
#include "Engine/HitResult.h"

class AActor;
class APawn;
class UWorld;

/** One body a burst reaches, and what it does to it. */
struct FGraveSaltTarget
{
	TWeakObjectPtr<AActor> Actor;
	/** The point of the body nearest the burst, and how far that is (cm). */
	FVector Point = FVector::ZeroVector;
	float Distance = 0.f;
	/** Salt burns the dead: half as much again, and a stagger. */
	bool bUnpaid = false;
	/** A boss (Boss rank, or a creature a boss fight is built round): its share capped, never knocked. */
	bool bBoss = false;
	/** The damage it takes (falloff, level, salt and the boss's cap all counted). */
	float Damage = 0.f;
};

/** What a burst did: the hits for the thrower's hit marker, and the count of bodies hurt and killed. */
struct FGraveSaltBurstResult
{
	FVector Center = FVector::ZeroVector;
	int32 Hits = 0;
	int32 Kills = 0;
	/** Each body hurt, as a hit at its nearest point, and the damage dealt there (same order). */
	TArray<FHitResult> HitResults;
	TArray<float> Damages;
};

/**
 * The grave-salt grenade's burst (FGraveSaltRules has its numbers), as free functions so the tests reach every step:
 *  - FindTargets: every living thing with health within the radius (measured to its nearest side) that the burst can see
 *    (CanSee: no wall, rock or ground between), never the thrower or any player;
 *  - Detonate: the look and the sound first (GraveSaltBurstEffects.cpp), then each body's damage (UGraveSaltDamageType,
 *    from the thrower, so the gun in their hand counts the kill as with a melee strike; the Unpaid through a melee scope,
 *    which gives them the strike's heavy stagger and hit-stop), the knock away from it (KnockAway), and the jolt on the
 *    players' views by distance (KickViews).
 */
namespace GraveSaltBurst
{
	/** The body the burst measures to: a creature's capsule, or the box round anything else's solid parts. */
	AI_LOOTER_SHOOTER_API FGraveSaltBody BodyOf(const AActor& Actor);

	/** Something the burst can hurt: health, alive, not Thrower and not a player's pawn. */
	AI_LOOTER_SHOOTER_API bool CanHurt(const AActor* Actor, const AActor* Thrower);

	/** The Unpaid (and anything built on them, Abel among them): salt burns them. */
	AI_LOOTER_SHOOTER_API bool IsUnpaid(const AActor* Actor);

	/** A boss: Boss rank, or a creature with a boss fight's component (the Gravemother). */
	AI_LOOTER_SHOOTER_API bool IsBoss(const AActor* Actor);

	/**
	 * The burst at Center sees some of Body: its nearest point, its middle or its top, past no wall, rock or ground (grass,
	 * volumes and other creatures never shield it). Target and Thrower are never in the way.
	 */
	AI_LOOTER_SHOOTER_API bool CanSee(const UWorld& World, const FVector& Center, const FGraveSaltBody& Body, const AActor* Target, const AActor* Thrower);

	/**
	 * Every body a burst at Center reaches, nearest first, with its damage at LevelScale (each rolled from Random in the
	 * +/-10% spread). Nothing without a world.
	 */
	AI_LOOTER_SHOOTER_API TArray<FGraveSaltTarget> FindTargets(const UWorld* World, const FVector& Center, const AActor* Thrower, float LevelScale,
		FRandomStream& Random);

	/**
	 * Knocks Target away from Center with the melee strike's hop (UPlayerMeleeComponent::KnockBack: never over an edge,
	 * nothing for a boss, a Soulfed monster, a corpse or anything that isn't a creature). Returns the launch given.
	 */
	AI_LOOTER_SHOOTER_API FVector KnockAway(AActor& Target, const FVector& Center);

	/**
	 * The whole burst at Center (Up: the way the burst rises, the surface's up) for Thrower (may be null: no one credited)
	 * at LevelScale. Returns what it did.
	 */
	AI_LOOTER_SHOOTER_API FGraveSaltBurstResult Detonate(UWorld* World, const FVector& Center, const FVector& Up, APawn* Thrower, float LevelScale);

	// --- GraveSaltBurstEffects.cpp ---

	/** The white salt-and-ember burst, drawn in code through FWeaponFX: a flash, embers rising, salt flung, a salt cloud. */
	AI_LOOTER_SHOOTER_API void SpawnEffects(UWorld& World, const FVector& Center, const FVector& Up);

	/** The burst's sound at Center, and the salt searing each Unpaid it burned (at most a few, so it never smears). */
	AI_LOOTER_SHOOTER_API void PlaySounds(UWorld& World, const FVector& Center, const TArray<FGraveSaltTarget>& Targets);

	/** Every local player's view jolted and shaken by how far they are from Center (FGraveSaltRules::KickShare). */
	AI_LOOTER_SHOOTER_API void KickViews(UWorld& World, const FVector& Center, FRandomStream& Random);
}
