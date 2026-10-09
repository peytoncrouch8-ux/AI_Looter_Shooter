#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/HitReaction.h"
#include "Weapons/WeaponFX.h"
#include "CreatureHitReactionComponent.generated.h"

class ACreatureBase;
class AController;
class UHealthComponent;

/**
 * How a creature takes a hit and a death, on top of its own flinch (OnHurt) and death motion (FHitReaction has the
 * rules and numbers):
 *  - a hit-stop on a crit or the kill: its own time all but stopped for 45-80 ms (CustomTimeDilation on this creature
 *    only, never the world's), so the blow lands with weight;
 *  - a stagger on a heavy hit (a crit, a shotgun blast up close): its brain waits it out (ACreatureBase::TickBrain, so a
 *    wind-up holds too) and it's shoved back along the shot, then can't be staggered again for a while;
 *  - its death: the kind's burst (a spider's shell, a slime's splat, an Unpaid's soul-light; FWeaponFX::SpawnDeathBurst)
 *    with the body's sound, the killer's view punched in (ViewKicks::ForKill), and the corpse knocked a step along the
 *    killing shot (never through a wall or off a drop).
 * A boss gets none of it (Boss rank, or a creature with a UBossComponent: the Gravemother): its fight's show runs its
 * big moments (its own stagger, its slow-motion death). Ticks only while a corpse slides.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UCreatureHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCreatureHitReactionComponent();

	/** Staggered right now: its brain pauses. */
	bool IsStaggered() const;

	/** In a hit-stop right now. */
	bool IsHitStopped() const;

	const FHitReaction& GetReaction() const { return Reaction; }

	/** The death burst a creature of Creature's kind shows (by class, most specific first: Abel is built on the Unpaid). */
	static EDeathBurst DeathBurstOf(const ACreatureBase& Creature);

	/** The burst's color: the spider's chitin, the slime's gel, the Unpaid's coal at its rank. */
	static FLinearColor DeathBurstTint(const ACreatureBase& Creature);

	/** The body's sound for a burst (none for None). */
	static FName DeathBurstCue(EDeathBurst Kind);

	/** A stagger shoves a full-size Basic creature back at this speed (cm/s); its ground friction stops it within about 30 cm. */
	static constexpr float StaggerShove = 360.f;
	/** A corpse is knocked along the killing shot at this speed (a crit's faster), and slides at most this long. */
	static constexpr float CorpseKnock = 260.f;
	static constexpr float CritCorpseKnock = 380.f;
	static constexpr float CorpseKnockSeconds = 0.35f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void HandleDeath(AController* Killer);

	/** Its own time all but stopped until the reaction's hit-stop ends. */
	void StartHitStop();
	void EndHitStop();
	/** Pushes it back along Direction (on the ground, never over a drop), harder for a bigger burst of damage. */
	void Shove(ACreatureBase& Creature, const FVector& Direction, float BurstShare);
	void SpawnDeathBurst(ACreatureBase& Creature);
	void StartCorpseKnock(ACreatureBase& Creature);
	void StepCorpseKnock(float DeltaTime);
	/** The world-static ground under Point (the terrain and placed props, never a volume), within Above and Below. */
	bool FindGround(const FVector& Point, float Above, float Below, FVector& OutGround) const;

	FHitReaction Reaction;
	TWeakObjectPtr<UHealthComponent> Health;
	FTimerHandle HitStopTimer;

	/** The latest hit's way (from the shooter through the wound) and whether it was critical: the death goes with them. */
	FVector LastShotDirection = FVector::ZeroVector;
	bool bLastHitCritical = false;

	/** The corpse's slide: its speed (cm/s, across the ground), seconds slid, and its height over the ground it keeps. */
	FVector KnockVelocity = FVector::ZeroVector;
	float KnockTime = 0.f;
	float KnockGroundOffset = 0.f;
	bool bKnockMeasured = false;
};
