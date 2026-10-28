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
 * How a creature takes a hit and a death, on top of its own flinch (OnHurt, sized by the hit: FCreatureHit) and death
 * motion (FHitReaction has the rules; the numbers are Looter.Gunfeel.* cvars, FGunfeelReactTuning):
 *  - a hit-stop on a crit, a heavy hit or the kill: its own time all but stopped for 35-100 ms (CustomTimeDilation on this
 *    creature only, never the world's), so the blow lands with weight; a revolver's crit longest, with a ring for its
 *    shooter (GunshotSound::PlayCritRing);
 *  - a brief slow on any hit (UMovementSlowComponent), so a charging creature visibly catches the bullet;
 *  - a stagger on a heavy hit (a crit, a shotgun blast up close) or the rifle's pressure: its brain waits it out
 *    (ACreatureBase::TickBrain, so a wind-up holds too) and it's shoved back along the shot, then can't be staggered again
 *    for a while; a close shotgun blast knocks a small one back instead (UCreatureKnockComponent::Knock);
 *  - its death: the kind's burst (a spider's shell, a slime's splat, an Unpaid's soul-light; FWeaponFX::SpawnDeathBurst;
 *    a revolver's crit kill bigger, at the crit spot, with a spurt) with the body's sound, the killer's view punched in
 *    (ViewKicks::ForKill), and the corpse knocked along the killing shot by the gun (a shotgun's thrown with a lift, a
 *    rifle's crumpling where it stands), never through a wall or off a drop. A kill by an element's status (a burn, a smite,
 *    the salt) has none of those: no gun dealt it, and the element's own death plays instead (KnocksCorpse).
 * A boss gets none of it but the smaller flinch (Boss rank, or a creature with a UBossComponent: the Gravemother): its
 * fight's show runs its big moments (its own stagger, its slow-motion death). Ticks only while a corpse slides.
 * ACreatureBase::HandleDamaged hands it each hit (TakeHit) before the kind's flinch, so the flinch knows the burst.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UCreatureHitReactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCreatureHitReactionComponent();

	/**
	 * The owner took a hit (ACreatureBase::HandleDamaged, its health already down): its hit-stop, stagger or knock, slow
	 * and the revolver's ring. Returns the hit as its flinch and death go by (not yet fresh).
	 */
	FCreatureHit TakeHit(float Damage, bool bCritical, const FVector& HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	/** Staggered right now: its brain pauses. */
	bool IsStaggered() const;

	/** In a hit-stop right now. */
	bool IsHitStopped() const;

	const FHitReaction& GetReaction() const { return Reaction; }

	/** The latest hit it took (the creature's GetLastHit is protected): the tests read what a kill was. */
	const FCreatureHit& GetLastTakenHit() const { return LastHit; }

	/** The gun that dealt a hit (a bullet's DamageCauser is the AWeaponBase that fired it); None for anything else. */
	static EWeaponKind KindOf(const AActor* DamageCauser);

	/**
	 * How fast the corpse is knocked along the killing shot (cm/s) by the gun that killed it: a shotgun's Kill.ShotgunKnock,
	 * a rifle's Kill.RifleKnock, anything else today's (CorpseKnock); a crit's CritCorpseKnock where that's larger.
	 */
	static float CorpseKnockFor(EWeaponKind Kind, bool bCritical, const FGunfeelReactTuning& Tuning);
	/** How high the corpse is lifted at its slide's middle (cm): a shotgun kill's Kill.ShotgunLift, else none. */
	static float CorpseLiftFor(EWeaponKind Kind, const FGunfeelReactTuning& Tuning);

	/**
	 * A killing hit is played the gun's way (its corpse knock, the kind's own death burst) unless a status dealt it: the
	 * element's own death is the whole of that kill (CreatureKillLook, ElementDeathLook), and a burn is not a shotgun blast.
	 */
	static bool KnocksCorpse(const FCreatureHit& Hit) { return Hit.Element == ELooterElement::None; }
	static bool BurstsLikeItsKind(const FCreatureHit& Hit) { return Hit.Element == ELooterElement::None; }

	/** The death burst a creature of Creature's kind shows (by class, most specific first: Abel is built on the Unpaid). */
	static EDeathBurst DeathBurstOf(const ACreatureBase& Creature);

	/** The burst's color: the spider's chitin, the slime's gel, the Unpaid's coal at its rank. */
	static FLinearColor DeathBurstTint(const ACreatureBase& Creature);

	/** The body's sound for a burst (none for None). */
	static FName DeathBurstCue(EDeathBurst Kind);

	/** A corpse is knocked along the killing shot at this speed (a crit's faster) by anything but a shotgun or a rifle, and slides at most this long. */
	static constexpr float CorpseKnock = 260.f;
	static constexpr float CritCorpseKnock = 380.f;
	static constexpr float CorpseKnockSeconds = 0.35f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleDeath(AController* Killer);

	/** Its own time all but stopped until the reaction's hit-stop ends. */
	void StartHitStop();
	void EndHitStop();
	/** Pushes it back along Direction (on the ground, never over a drop), harder for a bigger burst of damage. */
	void Shove(ACreatureBase& Creature, const FVector& Direction, float BurstShare, float ShoveSpeed);
	/** The shotgun's power on a small one: thrown back along Direction (UCreatureKnockComponent, its shove style). */
	void KnockBack(ACreatureBase& Creature, const FVector& Direction, const FGunfeelReactTuning& Tuning);
	void SpawnDeathBurst(ACreatureBase& Creature, const FGunfeelReactTuning& Tuning);
	void StartCorpseKnock(ACreatureBase& Creature, const FGunfeelReactTuning& Tuning);
	void StepCorpseKnock(float DeltaTime);
	/** The slide over: a corpse still lifted is set down on its ground. */
	void EndCorpseKnock();
	/** The world-static ground under Point (the terrain and placed props, never a volume), within Above and Below. */
	bool FindGround(const FVector& Point, float Above, float Below, FVector& OutGround) const;

	FHitReaction Reaction;
	TWeakObjectPtr<UHealthComponent> Health;
	FTimerHandle HitStopTimer;

	/** The latest hit (its way from the shooter through the wound, crit, gun, spot): the death goes with it. */
	FCreatureHit LastHit;
	/** When the creature last took the hit slow (world seconds): a kind with a gap between slows isn't slowed again sooner. */
	double LastSlowAt = -1.0e9;

	/** The corpse's slide: its speed (cm/s, across the ground), seconds slid, and its height over the ground it keeps. */
	FVector KnockVelocity = FVector::ZeroVector;
	float KnockTime = 0.f;
	float KnockGroundOffset = 0.f;
	bool bKnockMeasured = false;
	/** A shotgun kill's lift: the arc's top (cm), and how high it is lifted now. */
	float KnockLift = 0.f;
	float KnockLiftNow = 0.f;
};
