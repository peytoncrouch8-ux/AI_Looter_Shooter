#pragma once

// What the creature trait tests (CreatureTraitTests.cpp, CreatureTraitPlayTests.cpp) build in their test levels: a floor,
// spiders spawned in play, traits given straight to them, and damage dealt as the game's arrives. A test level never ticks:
// the tests move each trait component on with Advance. Nothing is drawn or heard there (no player to see, no audio).

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Combat/LooterDamageTypes.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTraitComponent.h"
#include "Creatures/CreatureTraitSettings.h"
#include "Creatures/SpiderCreature.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/LocomotionTestWorld.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "UObject/Script.h"

namespace CreatureTraitTestWorld
{
	inline constexpr float Frame = 1.f / 30.f;

	inline const UCreatureTraitSettings& Settings()
	{
		return UCreatureTraitSettings::Get();
	}

	/** A floor the creatures stand on and the blink's looks find ground on (its top at Z 0), 60 m across. */
	inline void SpawnFloor(UWorld* World)
	{
		LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	}

	/** A spider spawned in play at Feet (Regular: no defense, so the sums are the traits' own), dropping no loot. */
	inline ACreatureBase* SpawnSpider(UWorld* World, const FVector& Feet)
	{
		return EncounterTestWorld::SpawnCreature(World, ASpiderCreature::StaticClass(), Feet);
	}

	/** Creature with exactly these traits. */
	inline UCreatureTraitComponent* Give(ACreatureBase& Creature, std::initializer_list<ECreatureTrait> Traits)
	{
		UCreatureTraitComponent* Component = UCreatureTraitComponent::FindOrAdd(Creature);
		Component->SetTraits(TArray<ECreatureTrait>(Traits));
		return Component;
	}

	/** Damage as AActor::TakeDamage hands it to its listeners (BossTestWorld::Hurt), a crit if asked. */
	inline void Hurt(AActor* Victim, float Damage, bool bCritical = false)
	{
		if (!Victim)
		{
			return;
		}
		FEditorScriptExecutionGuard RunActorEvents;
		const UDamageType* Type = bCritical ? static_cast<const UDamageType*>(GetDefault<UWeaponCritDamageType>()) : GetDefault<UDamageType>();
		Victim->OnTakeAnyDamage.Broadcast(Victim, Damage, Type, nullptr, nullptr);
	}

	inline UHealthComponent* HealthOf(const AActor* Actor)
	{
		return Actor ? Actor->FindComponentByClass<UHealthComponent>() : nullptr;
	}
}

#endif
