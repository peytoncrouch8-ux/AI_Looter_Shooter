#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CreatureRallied.generated.h"

class ACreatureBase;

/**
 * A creature rallied by a Rallying one near it (UCreatureTraitComponent, ECreatureTrait::Rallying): while any rallier
 * holds it, it takes less of each hit (its health's damage-taken scale, source "Trait.Rally") and walks and chases
 * faster. Added by the first rallier to reach it and gone with the last: several ralliers count once, never stacking.
 * Its pace goes back by division, so whatever else changed the creature's speed meanwhile (a Bell-Ringer's steadying, a
 * trait of its own) stays. No tick: the ralliers join and leave it.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UCreatureRallied : public UActorComponent
{
	GENERATED_BODY()

public:
	UCreatureRallied();

	/**
	 * Rallier holds Creature (a living one): from the first rallier on, each hit times DamageTakenScale and its walk and
	 * chase times SpeedScale. A rallier already holding it changes nothing. Returns its rallying, or null.
	 */
	static UCreatureRallied* Join(ACreatureBase& Creature, const ACreatureBase& Rallier, float DamageTakenScale, float SpeedScale);

	/** Rallier lets go of Creature; with no rallier left, its pace and its hits go back and the component goes. */
	static void Leave(ACreatureBase& Creature, const ACreatureBase& Rallier);

	/** Creature's rallying, if it has one. */
	static UCreatureRallied* Find(const ACreatureBase& Creature);

	bool IsRallied() const { return bApplied; }

	/** How many ralliers hold it now. */
	int32 NumRalliers() const;

	/** Whether Rallier is one of them. */
	bool IsHeldBy(const ACreatureBase& Rallier) const;

	/** Its health's damage-taken source (UHealthComponent::SetDamageTakenScale). */
	static const FName DamageSource;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Apply(float DamageTakenScale, float SpeedScale);
	/** Puts its pace and its hits back (once). */
	void PutBack();
	void Prune();

	TArray<TWeakObjectPtr<const ACreatureBase>> Ralliers;
	float AppliedSpeedScale = 1.f;
	bool bApplied = false;
};
