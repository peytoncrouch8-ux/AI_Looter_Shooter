#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Creatures/CreatureTrait.h"
#include "CreatureTraitComponent.generated.h"

class ACreatureBase;

DECLARE_MULTICAST_DELEGATE(FOnCreatureTraitsChanged);

/**
 * STAND-IN (TIER-CORE, 2026-10-20): the contract's API with no behaviour, so the tier core builds and its tests run; the
 * TRAITS agent's component replaces this one whole. ACreatureBase makes one on every creature and sets its traits on
 * each rank change (SetTraits(RollTraits(its tier's TraitCount, Ethereal or not, itself, GetTierSeed()))).
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UCreatureTraitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCreatureTraitComponent();

	void SetTraits(TConstArrayView<ECreatureTrait> NewTraits);
	bool HasTrait(ECreatureTrait Trait) const { return Traits.Contains(Trait); }
	TConstArrayView<ECreatureTrait> GetTraits() const { return Traits; }

	/** After SetTraits changed them. */
	FOnCreatureTraitsChanged OnTraitsChanged;

	/**
	 * Count traits for For from Seed, the same ones every time for the same seed: Phasing always one of an Ethereal's,
	 * the rest drawn without repeats. (The stand-in ignores which kinds each trait fits.)
	 */
	static TArray<ECreatureTrait> RollTraits(int32 Count, bool bEthereal, const ACreatureBase& For, int32 Seed);

private:
	TArray<ECreatureTrait> Traits;
};
