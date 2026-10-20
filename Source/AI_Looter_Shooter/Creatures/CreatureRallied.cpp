#include "Creatures/CreatureRallied.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"

const FName UCreatureRallied::DamageSource(TEXT("Trait.Rally"));

UCreatureRallied::UCreatureRallied()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UCreatureRallied* UCreatureRallied::Find(const ACreatureBase& Creature)
{
	return Creature.FindComponentByClass<UCreatureRallied>();
}

UCreatureRallied* UCreatureRallied::Join(ACreatureBase& Creature, const ACreatureBase& Rallier, float DamageTakenScale, float SpeedScale)
{
	if (Creature.IsDead() || &Creature == &Rallier)
	{
		return nullptr;
	}
	UCreatureRallied* Rallied = Find(Creature);
	if (!Rallied)
	{
		Rallied = NewObject<UCreatureRallied>(&Creature, NAME_None, RF_Transient);
		Creature.AddInstanceComponent(Rallied);
		Rallied->RegisterComponent();
	}
	Rallied->Prune();
	if (!Rallied->IsHeldBy(Rallier))
	{
		Rallied->Ralliers.Add(TWeakObjectPtr<const ACreatureBase>(&Rallier));
	}
	if (!Rallied->bApplied)
	{
		Rallied->Apply(DamageTakenScale, SpeedScale);
	}
	return Rallied;
}

void UCreatureRallied::Leave(ACreatureBase& Creature, const ACreatureBase& Rallier)
{
	UCreatureRallied* Rallied = Find(Creature);
	if (!Rallied)
	{
		return;
	}
	Rallied->Ralliers.RemoveAll([&Rallier](const TWeakObjectPtr<const ACreatureBase>& Each) { return Each.Get() == &Rallier; });
	Rallied->Prune();
	if (Rallied->Ralliers.IsEmpty())
	{
		Rallied->PutBack();
		Rallied->DestroyComponent();
	}
}

int32 UCreatureRallied::NumRalliers() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<const ACreatureBase>& Each : Ralliers)
	{
		Count += Each.IsValid() ? 1 : 0;
	}
	return Count;
}

bool UCreatureRallied::IsHeldBy(const ACreatureBase& Rallier) const
{
	return Ralliers.ContainsByPredicate([&Rallier](const TWeakObjectPtr<const ACreatureBase>& Each) { return Each.Get() == &Rallier; });
}

void UCreatureRallied::Prune()
{
	Ralliers.RemoveAll([](const TWeakObjectPtr<const ACreatureBase>& Each) { return !Each.IsValid(); });
}

void UCreatureRallied::Apply(float DamageTakenScale, float SpeedScale)
{
	ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	if (!Creature)
	{
		return;
	}
	if (UHealthComponent* Health = Creature->FindComponentByClass<UHealthComponent>())
	{
		Health->SetDamageTakenScale(DamageSource, FMath::Clamp(DamageTakenScale, 0.05f, 1.f));
	}
	AppliedSpeedScale = FMath::Max(SpeedScale, 0.01f);
	Creature->ChaseSpeed *= AppliedSpeedScale;
	Creature->WalkSpeed *= AppliedSpeedScale;
	bApplied = true;
}

void UCreatureRallied::PutBack()
{
	if (!bApplied)
	{
		return;
	}
	bApplied = false;
	ACreatureBase* Creature = Cast<ACreatureBase>(GetOwner());
	if (!Creature)
	{
		return;
	}
	if (UHealthComponent* Health = Creature->FindComponentByClass<UHealthComponent>())
	{
		Health->SetDamageTakenScale(DamageSource, 1.f);
	}
	Creature->ChaseSpeed /= AppliedSpeedScale;
	Creature->WalkSpeed /= AppliedSpeedScale;
	AppliedSpeedScale = 1.f;
}

void UCreatureRallied::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Taken away while rallied (the creature removed, the level closing): its pace and its hits go back all the same.
	PutBack();
	Super::EndPlay(EndPlayReason);
}
