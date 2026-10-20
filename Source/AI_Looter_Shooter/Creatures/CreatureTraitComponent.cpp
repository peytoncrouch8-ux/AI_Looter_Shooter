// STAND-IN (TIER-CORE, 2026-10-20): replaced whole by the TRAITS agent's component.

#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/CreatureBase.h"
#include "Math/RandomStream.h"

UCreatureTraitComponent::UCreatureTraitComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCreatureTraitComponent::SetTraits(TConstArrayView<ECreatureTrait> NewTraits)
{
	TArray<ECreatureTrait> Wanted(NewTraits);
	if (Wanted == Traits)
	{
		return;
	}
	Traits = MoveTemp(Wanted);
	OnTraitsChanged.Broadcast();
}

TArray<ECreatureTrait> UCreatureTraitComponent::RollTraits(int32 Count, bool bEthereal, const ACreatureBase& For, int32 Seed)
{
	TArray<ECreatureTrait> Rolled;
	if (Count <= 0)
	{
		return Rolled;
	}
	if (bEthereal)
	{
		Rolled.Add(ECreatureTrait::Phasing);
	}
	TArray<ECreatureTrait> Pool = { ECreatureTrait::Fleet, ECreatureTrait::Mending, ECreatureTrait::Warded, ECreatureTrait::GraveBurst,
		ECreatureTrait::Rallying, ECreatureTrait::Vengeful };
	FRandomStream Random(Seed);
	while (Rolled.Num() < Count && Pool.Num() > 0)
	{
		Rolled.Add(Pool[Random.RandHelper(Pool.Num())]);
		Pool.RemoveSingle(Rolled.Last());
	}
	return Rolled;
}
