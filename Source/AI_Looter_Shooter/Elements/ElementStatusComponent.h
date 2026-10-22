#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Elements/ElementStatus.h"
#include "Elements/LooterElement.h"
#include "ElementStatusComponent.generated.h"

class AController;
class FWeaponFX;

/**
 * A creature's element statuses in play (Phase 12.3): added at runtime to a creature the first time an element gun's roll
 * comes up on it (TryApply), and kept (a creature comes back for a new life as the same actor). It ticks only while a
 * status lasts. FElementStatus holds the numbers; this deals its pieces of damage (ElementRules::DamageTypeFor, the gun
 * that set the status as the causer, so that gun gets the notch), plays the status's look and sounds, slows a salted
 * creature (UMovementSlowComponent, never a boss), and leaps a smite to the next of the dead.
 *  - Burning: embers rising off it and flame licks, more of both for each burn; Element.Ember.Tick at each piece.
 *  - Smiting: white-gold motes gathering in, a glow at its heart growing until the burst (rays and a flash).
 *  - Salted: pale crystals clinging and dropping off it, a breath of frost; Element.GraveSalt.Tick at each piece.
 * A kill by a status bursts in its look with Element.<Element>.Kill. Looks: ElementStatusComponentLooks.cpp.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UElementStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UElementStatusComponent();

	static UElementStatusComponent* Find(const AActor* Target);
	static UElementStatusComponent* FindOrAdd(AActor& Target);

	/**
	 * An element gun's hit of HitDamage landed on Target (a creature): Roll (0-1) against Chance at Target's weakness
	 * (ElementRules::HitChance) and, if it comes up, sets the status. bCritical: the hit was critical (an Unpaid's coal).
	 * Returns whether it was set. Nothing on a dead creature, or on anything that isn't a creature.
	 */
	static bool TryApply(AActor& Target, ELooterElement Element, float Chance, float HitDamage, bool bCritical, float Roll,
		const FVector& HitLocation, AActor* Causer, AController* Instigator);

	/** Sets Element's status from a hit of HitDamage at Affinity (no roll). */
	void Apply(ELooterElement Element, float HitDamage, EElementAffinity Affinity, const FVector& HitLocation, AActor* Causer,
		AController* Instigator);

	/** Moves the statuses on by DeltaSeconds: their damage, looks and sounds. Its tick calls it; the tests call it. */
	void Advance(float DeltaSeconds);

	const FElementStatus& GetStatus() const { return Status; }
	bool IsBurning() const { return Status.NumBurns() > 0; }
	bool IsSmiting() const { return Status.NumSmites() > 0; }
	bool IsSalted() const { return Status.IsSalted(); }

	/** Damage it has sent so far by Element (what a test level, which deals none, would have dealt). */
	float GetDealt(ELooterElement Element) const;

	/**
	 * A status's damage is being dealt right now (inside its ApplyPointDamage): a listener that shows a hit its own way (the
	 * creature's white hit flash) leaves it to the status's look. A smite's burst isn't a piece: it flashes.
	 */
	static bool IsDealingNow() { return DealingDepth > 0; }

	/** Of the dead nearest Center within Radius (not Except, alive): where a smite leaps. Null for none. */
	static AActor* FindNextOfTheDead(const UWorld* World, const FVector& Center, float Radius, const AActor* Except);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	static constexpr int32 NumElements = 4;
	/** Pieces of damage over time being dealt now (IsDealingNow). */
	static int32 DealingDepth;

	/** Deals one piece to its owner (and a smite's leap); true if that killed it. */
	bool Deal(const FElementTick& Tick);
	/** Sends Damage of Element to Victim as from the status's gun; true if it killed Victim. */
	bool SendDamage(AActor& Victim, ELooterElement Element, float Damage);
	bool IsOwnerAlive() const;
	FVector BodyPoint(float Up) const;
	void GetBody(FVector& OutCenter, float& OutRadius, float& OutHalfHeight) const;
	/** The bullets' pooled effects, ready to draw (null without a world that has them). */
	FWeaponFX* Effects() const;
	void PlayCue(ELooterElement Element, EElementCue Event, const FVector& Where, float Volume = 1.f) const;

	// ElementStatusComponentLooks.cpp
	/** This frame's look of what it carries: the burn's embers, the smite's gathering light, the salt's crystals. */
	void PlayLooks(float DeltaSeconds);
	/** A smite's burst on Where (and its leap's arc to To when set). */
	void PlaySmiteLook(const FVector& Where, const FVector* LeapTo) const;
	/** Its death by Element: its look's burst over the body. */
	void PlayKillLook(ELooterElement Element) const;

	FElementStatus Status;
	/** Who set each element's status (by ELooterElement's number): the gun (causer) and its holder. */
	TWeakObjectPtr<AActor> Causers[NumElements];
	TWeakObjectPtr<AController> Instigators[NumElements];
	float Dealt[NumElements] = {};
	double LastApplyCue[NumElements] = { -1000.0, -1000.0, -1000.0, -1000.0 };
	/** Particles owed to each look (rate x time, spawned in whole ones). */
	float EmberLookOwed = 0.f;
	float SaltLookOwed = 0.f;
	float SmiteLookOwed = 0.f;
	float LookTime = 0.f;
};
