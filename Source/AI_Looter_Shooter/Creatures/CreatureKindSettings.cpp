#include "Creatures/CreatureKindSettings.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"

UCreatureKindSettings::UCreatureKindSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Creature Kinds");
}

const UCreatureKindSettings& UCreatureKindSettings::Get()
{
	return *GetDefault<UCreatureKindSettings>();
}

const FCreatureKindStats* UCreatureKindSettings::Find(const UClass* Class) const
{
	if (!Class)
	{
		return nullptr;
	}
	const FName Name = Class->GetFName();
	return Kinds.FindByPredicate([Name](const FCreatureKindStats& Kind) { return Kind.ClassName == Name; });
}

float UCreatureKindSettings::HealthFor(const UClass* Class, float ClassValue)
{
	const FCreatureKindStats* Kind = Get().Find(Class);
	return Kind && Kind->Health > 0.f ? Kind->Health : ClassValue;
}

float UCreatureKindSettings::AttackDamageFor(const UClass* Class, float ClassValue)
{
	const FCreatureKindStats* Kind = Get().Find(Class);
	return Kind && Kind->AttackDamage > 0.f ? Kind->AttackDamage : ClassValue;
}

float UCreatureKindSettings::BaseHealthOf(const UClass* Class)
{
	const AActor* Defaults = Class ? Cast<AActor>(Class->GetDefaultObject()) : nullptr;
	const UHealthComponent* Health = Defaults ? Defaults->FindComponentByClass<UHealthComponent>() : nullptr;
	return HealthFor(Class, Health ? Health->MaxHealth : 100.f);
}

float UCreatureKindSettings::BaseAttackDamageOf(const UClass* Class)
{
	const ACreatureBase* Defaults = Class ? Cast<ACreatureBase>(Class->GetDefaultObject()) : nullptr;
	return AttackDamageFor(Class, Defaults ? Defaults->AttackDamage : 0.f);
}
