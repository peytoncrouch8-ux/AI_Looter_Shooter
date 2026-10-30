#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Creatures/CreatureTrait.h"
#include "Elements/LooterElement.h"
#include "Embers/Powers/EmberFX.h"
#include "Math/RandomStream.h"
#include "CreatureTraitComponent.generated.h"

class ACreatureBase;
class AController;
class APawn;
class FWeaponFX;
class UAudioComponent;
class UHealthComponent;

/** A creature's traits changed (SetTraits): the tag's trait line follows. */
DECLARE_MULTICAST_DELEGATE(FOnCreatureTraitsChanged);

/** One body a Grave-burst caught: what it was dealt and how far it stood (to its nearest side). For tests and logs. */
struct FCreatureBurstVictim
{
	TWeakObjectPtr<AActor> Actor;
	float Damage = 0.f;
	float Distance = 0.f;
};

/**
 * A high-tier creature's traits (ECreatureTrait; the enemy tiers: Gravebound one, Hallowed two, Ethereal three) and
 * everything they do, each with a tell before anything that hurts and a way to beat it (numbers: UCreatureTraitSettings):
 *  - Fleet: walk and chase x1.25 (by multiplying, so others' changes stay), each attack's wind-up x0.85 from the frame it
 *    starts (put back as it ends, unless the creature set its own meanwhile); wind streaks trail it, a glint as it winds up.
 *  - Mending: 3 s unhit, 3% of its health a second, green motes rising and its shimmer looping; any hit (ward hits too)
 *    breaks it off with the motes flung away.
 *  - Warded: its health's Shield (UHealthComponent): a ward of 30% of its health takes each hit first, typed by an element
 *    (GetWardElement: its own element x2 into it, Wardbreak x1.5 on any other, the larger alone), then a crit counting
 *    twice; the rest of a breaking hit spills over. A turning cage of light in the ward's colour fading as it wears down,
 *    flickering near the end, a ripple and a ping where each hit lands; broken: a pop of shards and light in its colour,
 *    always a stagger (UCreatureHitReactionComponent::WardBreak) and a 0.8 s daze.
 *  - Grave-burst: dead, the body swells 1 s (it grows and shivers, its heart glows, its 3 m ring pulses on the ground
 *    faster and faster, the swell's sound), then bursts: 1.5x its attack within 1.2 m, 0.6x at 3 m, players only, a knock
 *    and a jolt; the corpse is gone in the blast.
 *  - Rallying: creatures within 8 m (never a boss, never itself) are held by it (UCreatureRallied: 15% less damage, 10%
 *    faster) while it lives; in a fight its ring lies on the ground, tethers of light run to each one it holds, and a
 *    rallying cry pulses through the ring every 6 s. Dead, the tethers snap.
 *  - Vengeful: under 30% health it enrages for good: damage and pace x1.25, a red flare and the enrage cry, embers rising,
 *    its tier glow doubled (GetGlowScale, for the look).
 *  - Phasing: 8% of its health taken within 1 s (never mid-attack, every 4 s at most): it blinks 4-6 m aside of whoever
 *    hit it, to ground it can stand on in their sight: a shimmer where it left, a streak of light, 0.25 s gone while light
 *    gathers where it'll be, a shimmer as it's back.
 * Nothing it applies outlives it: a new life (a respawn) starts every trait afresh, and SetTraits or EndPlay undo it all.
 * Looks are code-drawn (FWeaponFX streaks and glows, FEmberSparks), never the overlay slot (the hit flash's and the
 * Ethereal's ghost-rim are there), only when a player is near enough to read them. Topic files: CreatureTraitComponent.cpp
 * (life, events, shared measures), ...Pace.cpp (Fleet, Vengeful), ...Ward.cpp (Warded, Mending), ...Rally.cpp,
 * ...Burst.cpp (Grave-burst), ...Phase.cpp (Phasing).
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UCreatureTraitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCreatureTraitComponent();

	/**
	 * Gives it exactly these traits (each once, in the order given): whatever the old ones applied is undone first, and the
	 * new ones start fresh (a full ward, calm). The same traits again change nothing. Before play, they start with it.
	 */
	void SetTraits(TConstArrayView<ECreatureTrait> NewTraits);

	bool HasTrait(ECreatureTrait Trait) const { return Traits.Contains(Trait); }
	TConstArrayView<ECreatureTrait> GetTraits() const { return Traits; }

	/** SetTraits gave it others (not on a new life with the same ones). */
	FOnCreatureTraitsChanged OnTraitsChanged;

	/**
	 * Count traits for For from Seed (CreatureTraitRules::Roll for its kind): an Ethereal's first is always Phasing; only
	 * traits that fit its kind (CreatureTraitRules::Fits). The same seed always gives the same traits.
	 */
	static TArray<ECreatureTrait> RollTraits(int32 Count, bool bEthereal, const ACreatureBase& For, int32 Seed);

	/** Actor's traits, if it has the component. */
	static UCreatureTraitComponent* Find(const AActor* Actor);
	/** Creature's traits, the component added (and begun) if it had none (tests, the dev command). */
	static UCreatureTraitComponent* FindOrAdd(ACreatureBase& Creature);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Moves every trait on by DeltaSeconds of the creature's own time. Its tick calls it; the tests call it. */
	void Advance(float DeltaSeconds);

	// --- What the look reads (the tag's ward bar and trait line, the tier glow) ---

	/** The ward left, as a share of a full one (0: none, or broken). */
	float GetWardFraction() const { return WardFraction; }
	bool IsWardUp() const { return WardFraction > 0.f; }
	/**
	 * The ward's element (typed wards, the user's 2026-10-29 night): a Touched creature's own, else rolled from its tier seed
	 * (Thunder 0.5, Wardbreak 0.2, Cinder, Hallow and Rot 0.1 each: CreatureTraitRules::RollWardElement). None without a
	 * ward (not Warded, or broken). Its matching element breaks it x2, Wardbreak any other x1.5; its cage, shards and bar
	 * take the element's colour.
	 */
	ELooterElement GetWardElement() const { return IsWardUp() ? WardElement : ELooterElement::None; }
	/** Picks the ward's element again from its owner as it is now (its touch may have changed with its rank); the ward left stays. */
	void RefreshWardElement();
	bool IsMending() const { return bMending; }
	bool IsEnraged() const { return bEnraged; }
	/** Gone between a blink's two shimmers. */
	bool IsBlinking() const { return bBlinking; }
	/** Dead and swelling toward its burst. */
	bool IsSwelling() const { return bSwelling; }
	/** How many creatures its Rallying holds now. */
	int32 GetRalliedCount() const;
	/**
	 * What its tier glow (eyes, core, coal, lamp) is times now: 1 normally; Vengeful's x2 enraged and brighter still in its
	 * flare; brightening as a Grave-burst swells; a soft breath while mending.
	 */
	float GetGlowScale() const;

	// --- For the tests ---

	/** The bodies its Grave-burst caught (empty until it bursts). */
	const TArray<FCreatureBurstVictim>& GetLastBurst() const { return LastBurst; }
	bool HasBurst() const { return bBurst; }
	int32 GetBlinkCount() const { return BlinkCount; }
	/** Health mended this life. */
	float GetMended() const { return Mended; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// --- Life and events (CreatureTraitComponent.cpp) ---
	void Bind();
	void Unbind();
	/**
	 * Starts every trait afresh (a new life, new traits); EndTraits undoes all they applied (a dead body's swell plays on,
	 * unless bEndingPlay). Both safe to call twice.
	 */
	void BeginTraits();
	void EndTraits(bool bEndingPlay = false);
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);
	UFUNCTION()
	void HandleDeath(AController* Killer);
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);
	void UpdateTickEnabled();

	ACreatureBase* GetCreature() const;
	/** The world's code-drawn effects, when a player is near enough to see this creature's (null in tests and far away). */
	FWeaponFX* GetEffects() const;
	/** Its standing looks (cage, ring, streaks) show: effects near, and its body posed (not frozen far out of view). */
	bool ShowsLooks() const;
	/** Its body's middle as drawn, its width (radius) and height (half), at its size. */
	FVector BodyCenter() const;
	float BodyRadius() const;
	float BodyHalfHeight() const;
	FVector Feet() const;
	float MaxHealthNow() const;
	/** Its walk and chase times Scale (and back with 1 / Scale). */
	static void ScaleSpeed(ACreatureBase& Creature, float Scale);

	// --- Fleet and Vengeful (CreatureTraitComponentPace.cpp) ---
	void BeginFleet();
	void EndFleet();
	void TickFleet(float DeltaSeconds, FWeaponFX* Effects);
	void EndWindup();
	void TickVengeance(float DeltaSeconds, FWeaponFX* Effects);
	void CheckEnrage();
	void Enrage();
	void Calm();

	// --- Warded and Mending (CreatureTraitComponentWard.cpp) ---
	/** Its health's Shield: the ward takes what it can of each hit first; returns what's left for health. */
	float AbsorbIntoWard(float Damage, bool bCritical, const FVector& HitLocation);
	void RaiseWard();
	void DropWard();
	void NoteWardHit(const FVector& HitLocation, bool bCritical);
	void BreakWard(const FVector& HitLocation);
	/** The ward's element for its owner now: its touch, or its seed's roll. */
	ELooterElement PickWardElement() const;
	/** The ward's colour (its element's look colour; the tuning's for none). */
	FLinearColor WardColor() const;
	void TickWard(float DeltaSeconds, FWeaponFX* Effects);
	void DrawWardCage(FWeaponFX& Effects) const;
	void TickMending(float DeltaSeconds, FWeaponFX* Effects);
	void StartMending();
	void StopMending(bool bBroken);

	// --- Rallying (CreatureTraitComponentRally.cpp) ---
	void TickRally(float DeltaSeconds, FWeaponFX* Effects);
	void ScanRally();
	void ReleaseRally(bool bSnap);
	void PulseRally();
	void DrawRally(FWeaponFX& Effects);

	// --- Grave-burst (CreatureTraitComponentBurst.cpp) ---
	void StartSwell();
	void TickSwell(float DeltaSeconds, FWeaponFX* Effects);
	void Burst();
	void CancelSwell();
	void HurtWithBurst(const FVector& Center);
	void KickViews(const FVector& Center) const;

	// --- Phasing (CreatureTraitComponentPhase.cpp) ---
	void NoteHitForPhase(float Damage);
	void TickPhasing(float DeltaSeconds, FWeaponFX* Effects);
	APawn* FindAttacker() const;
	bool FindBlinkSpot(FVector& OutFeet) const;
	void StartBlink(const FVector& ToFeet);
	void StepBlink(float DeltaSeconds, FWeaponFX* Effects);
	void Arrive();
	void EndBlink();
	void Shimmer(const FVector& Center, bool bArriving);

	UPROPERTY(Transient)
	TArray<ECreatureTrait> Traits;

	TWeakObjectPtr<UHealthComponent> Health;
	/** Bound to its health's events and its Shield. */
	bool bBound = false;
	/** BeginTraits ran since the last EndTraits. */
	bool bBegun = false;
	/** It died and hasn't come back yet: its coming back is a new life. */
	bool bDeadSeen = false;
	/** Its own clock (its time, dilations and all) and how long since anything hit it. */
	float Clock = 0.f;
	float SinceHit = 1000.f;
	/** Whoever hit it last (a blink goes aside of them). */
	TWeakObjectPtr<APawn> LastAttacker;
	/** Every trait's sparks, shards, motes and embers. */
	EmberFX::FEmberSparks Sparks;
	FRandomStream Random;

	// Fleet
	float FleetSpeedApplied = 1.f;
	bool bWindupScaled = false;
	bool bWasAttacking = false;
	float SavedWindup = 0.f;
	float WrittenWindup = 0.f;
	float GlintLeft = 0.f;

	// Mending
	bool bMending = false;
	/** Its shimmer's loop while it mends. */
	TWeakObjectPtr<UAudioComponent> MendSound;
	float MendMoteIn = 0.f;
	float Mended = 0.f;

	// Warded
	float WardFraction = 0.f;
	/** The ward's element (GetWardElement), picked as it's raised. */
	ELooterElement WardElement = ELooterElement::Thunder;
	/** A hit's flare on the cage (1 at the hit, fading), and the break's flash and rings (seconds since). */
	float WardFlare = 0.f;
	float WardBreakAge = 1000.f;
	bool bDazePending = false;
	float SinceWardSound = 1000.f;
	struct FWardRipple
	{
		FVector Location = FVector::ZeroVector;
		FVector Normal = FVector::UpVector;
		float Age = 0.f;
	};
	TArray<FWardRipple, TInlineAllocator<6>> Ripples;

	// Rallying
	TArray<TWeakObjectPtr<ACreatureBase>> Rallied;
	float RallyScanIn = 0.f;
	float RallyPulseIn = 0.f;
	float RallyPulseAge = 1000.f;
	bool bRallyFighting = false;
	/** Its ring laid on the ground, and where it was laid (laid again as it moves). */
	TArray<FVector> RallyRing;
	FVector RallyRingAt = FVector::ZeroVector;
	float RallyRingIn = 0.f;

	// Vengeful
	bool bEnraged = false;
	float EnrageSpeedApplied = 1.f;
	float EnrageDamageApplied = 1.f;
	/** The attack damage it wrote: its stats applied again since (a level change) get the rage again. */
	float WrittenDamage = 0.f;
	float FlareLeft = 0.f;
	float EmberIn = 0.f;

	// Phasing
	struct FRecentHit
	{
		float Time = 0.f;
		float Damage = 0.f;
	};
	TArray<FRecentHit, TInlineAllocator<16>> RecentHits;
	float SinceBlink = 1000.f;
	/** A hard hit it couldn't blink from (mid-attack): it blinks once that's over, within this many seconds. */
	float BlinkPendingLeft = 0.f;
	bool bBlinking = false;
	float BlinkTime = 0.f;
	int32 BlinkCount = 0;
	/** The light streak from where it left to where it comes back, and the after-image where it left (seconds since). */
	FVector TrailFrom = FVector::ZeroVector;
	FVector TrailTo = FVector::ZeroVector;
	float TrailAge = 1000.f;
	bool bRightFirst = true;

	// Grave-burst
	bool bSwelling = false;
	bool bBurst = false;
	float SwellTime = 0.f;
	FVector BaseMeshScale = FVector::OneVector;
	/** Its ring on the ground (laid as it dies), and the blast's ring going out (seconds since). */
	TArray<FVector> BurstRing;
	FVector BurstCenter = FVector::ZeroVector;
	float BurstAge = 1000.f;
	TArray<FCreatureBurstVictim> LastBurst;
};
