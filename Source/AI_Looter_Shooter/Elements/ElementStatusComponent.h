#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Elements/ElementStatus.h"
#include "Elements/ElementStatusIcons.h"
#include "Elements/LooterElement.h"
#include "ElementStatusComponent.generated.h"

class AController;
class FWeaponFX;

/** A status was set on Target (Apply): its element, and whether it's the first of that element on this creature this life (the first-time callout). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnElementStatusSet, AActor* /*Target*/, ELooterElement, bool /*bFirstOnThisCreature*/);

/**
 * A creature's element statuses in play (Phase 12.3): added at runtime to a creature the first time an element gun's roll
 * comes up on it (TryApply), and kept (a creature comes back for a new life as the same actor). It ticks only while a
 * status lasts. FElementStatus holds the numbers; this deals its pieces of damage (ElementRules::DamageTypeFor, the gun
 * that set the status as the causer, so that gun gets the notch), plays the status's look and sounds, slows a salted
 * creature (UMovementSlowComponent, never a boss), and leaps a smite to the next of the dead.
 *  - Burning: embers rising off it and flame licks, more of both for each burn; Element.Ember.Tick at each piece.
 *  - Smiting: white-gold motes gathering in, a glow at its heart growing until the burst (rays and a flash).
 *  - Salted: pale crystals clinging and dropping off it, a breath of frost; Element.GraveSalt.Tick at each piece.
 *  - Jolting (Thunder): sparks crawling over it until the burst: a flash, a stagger (UCreatureHitReactionComponent::Jolt) and
 *    an arc to the nearest living creature (FindNextOfTheLiving) for a share of it.
 *  - Rotting: drips and a dim green glow; Element.Rot.Tick at each piece. Its death spreads the rot to the living near it
 *    (SpreadRot, once a death).
 * A kill by a status bursts in its look with Element.<Element>.Kill. Looks: ElementStatusComponentLooks.cpp.
 * Nothing sets on a creature immune to the element (ElementRules::Affinity), nor through a standing ward (the ward takes
 * the hit); an immune creature is skipped by a jolt's arc, a smite's leap and a rot's spread.
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

	/**
	 * Sets Element's status from a hit of HitDamage at Affinity (no roll); nothing when Affinity is Immune. Broadcasts
	 * OnStatusSet.
	 */
	void Apply(ELooterElement Element, float HitDamage, EElementAffinity Affinity, const FVector& HitLocation, AActor* Causer,
		AController* Instigator);

	/** The delegate's type by the component's name too (UElementStatusComponent::FOnElementStatusSet). */
	using FOnElementStatusSet = ::FOnElementStatusSet;

	/** Every status set on any creature (the damage numbers' first-time callout: BURNING, JOLTED...). Game thread. */
	static FOnElementStatusSet OnStatusSet;

	/** What it carries now, one icon per element in the enum's order (the tag's status icons); Out is emptied first. */
	void GetStatusIcons(TArray<FCreatureStatusIcon>& Out) const;

	/** Moves the statuses on by DeltaSeconds: their damage, looks and sounds. Its tick calls it; the tests call it. */
	void Advance(float DeltaSeconds);

	const FElementStatus& GetStatus() const { return Status; }
	bool IsBurning() const { return Status.NumBurns() > 0; }
	bool IsSmiting() const { return Status.NumSmites() > 0; }
	bool IsSalted() const { return Status.IsSalted(); }
	bool IsJolting() const { return Status.NumJolts() > 0; }
	bool IsRotting() const { return Status.IsRotting(); }

	/** Damage it has sent so far by Element (what a test level, which deals none, would have dealt). */
	float GetDealt(ELooterElement Element) const;

	/**
	 * A status's damage is being dealt right now (inside its ApplyPointDamage): a listener that shows a hit its own way (the
	 * creature's white hit flash) leaves it to the status's look. A smite's burst isn't a piece: it flashes.
	 */
	static bool IsDealingNow() { return DealingDepth > 0; }

	/**
	 * The element whose status is dealing damage right now (a burn's piece, a smite's burst or its leap, the salt's piece), None
	 * otherwise: inside that damage's own events. The hit reaction reads it (a status's kill is the element's death, with
	 * no gun's knock) and so does the damage number (it takes the element's colour).
	 */
	static ELooterElement DealingElement() { return DealingElementNow; }

	/** Marks the damage dealt inside it as Element's status's (SendDamage's; the tests deal a status's damage the same way). */
	struct FDealingScope
	{
		explicit FDealingScope(ELooterElement Element)
			: Previous(DealingElementNow)
		{
			DealingElementNow = Element;
		}
		~FDealingScope() { DealingElementNow = Previous; }

	private:
		ELooterElement Previous;
	};

	/** Of the dead nearest Center within Radius (not Except, alive): where a smite leaps. Null for none. */
	static AActor* FindNextOfTheDead(const UWorld* World, const FVector& Center, float Radius, const AActor* Except);
	/** The living creature (not of the dead) nearest Center within Radius (not Except, alive): where a jolt arcs. Null for none. */
	static AActor* FindNextOfTheLiving(const UWorld* World, const FVector& Center, float Radius, const AActor* Except);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Its owner died: the next life's statuses are first ones again. */
	UFUNCTION()
	void HandleOwnerDeath(AController* Killer);

	static constexpr int32 NumElements = 6;
	/** Pieces of damage over time being dealt now (IsDealingNow). */
	static int32 DealingDepth;
	/** SendDamage's element while it deals (DealingElement). */
	static ELooterElement DealingElementNow;

	/** Deals one piece to its owner (and a smite's leap, a jolt's arc and stagger); true if that killed it. */
	bool Deal(const FElementTick& Tick);
	/** Its owner is dying while rotting: the rot spreads to the living near it, once for this death. */
	void SpreadRot();
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
	/** A jolt's burst on Where (and its arc to LeapTo when set). */
	void PlayJoltLook(const FVector& Where, const FVector* LeapTo) const;
	/** The rot spreading from a death at From to each of To. */
	void PlayRotSpreadLook(const FVector& From, TConstArrayView<FVector> To) const;
	/** Its death by Element: its look's burst over the body. */
	void PlayKillLook(ELooterElement Element) const;

	FElementStatus Status;
	/** Who set each element's status (by ELooterElement's number): the gun (causer) and its holder. */
	TWeakObjectPtr<AActor> Causers[NumElements];
	TWeakObjectPtr<AController> Instigators[NumElements];
	float Dealt[NumElements] = {};
	/** The elements set on it this life (MaskOf bits): OnStatusSet's bFirstOnThisCreature. */
	uint8 SetThisLife = 0;
	double LastApplyCue[NumElements] = { -1000.0, -1000.0, -1000.0, -1000.0, -1000.0, -1000.0 };
	/** The strongest hit that rotted it (what its death spreads), and whether this death has spread it. */
	float RotHitDamage = 0.f;
	bool bRotSpread = false;
	/** Particles owed to each look (rate x time, spawned in whole ones). */
	float EmberLookOwed = 0.f;
	float SaltLookOwed = 0.f;
	float SmiteLookOwed = 0.f;
	float ThunderLookOwed = 0.f;
	float RotLookOwed = 0.f;
	float LookTime = 0.f;
};
