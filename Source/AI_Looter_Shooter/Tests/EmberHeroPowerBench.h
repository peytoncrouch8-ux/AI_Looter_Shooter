#pragma once

// What the four new heroes' power tests share (EmberOdessaTests.cpp, EmberCraneTests.cpp, EmberGaugeTests.cpp,
// EmberPikeTests.cpp): a floor, the player standing on it facing +X with an ember component, spiders standing about, a
// definition made in the test with the numbers given (the tests don't lean on the data assets), the context a power is
// handed, a hurt as the engine hands it out (a test level has no game mode, so ApplyDamage never lands on a pawn), and
// the frames a test steps a power through. Ellis's three have their own (EmberPowerTests.cpp).

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Core/LooterCharacter.h"
#include "Creatures/SpiderCreature.h"
#include "Embers/EmberComponent.h"
#include "Embers/EmberDamageType.h"
#include "Embers/EmberDefinition.h"
#include "Embers/EmberPower.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "Loot/LootDropComponent.h"
#include "Tests/LocomotionTestWorld.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/Package.h"

namespace EmberHeroBench
{
	inline constexpr float Frame = 1.f / 60.f;

	/** The player on the floor at Feet facing +X, with their ember component (made here if the body has none). */
	struct FBench
	{
		ALooterCharacter* Body = nullptr;
		UEmberComponent* Embers = nullptr;
		UHealthComponent* Health = nullptr;
	};

	inline FBench SpawnBench(UWorld* World, const FVector& Feet)
	{
		FBench Bench;
		UPlayerLocomotionComponent* Locomotion = LocomotionTestWorld::SpawnPlayer(World, Feet);
		Bench.Body = Locomotion ? Cast<ALooterCharacter>(Locomotion->GetOwner()) : nullptr;
		if (!Bench.Body)
		{
			return Bench;
		}
		Bench.Embers = UEmberComponent::Find(Bench.Body);
		if (!Bench.Embers)
		{
			Bench.Embers = NewObject<UEmberComponent>(Bench.Body, TEXT("TestEmbers"));
			Bench.Body->AddInstanceComponent(Bench.Embers);
			Bench.Embers->RegisterComponent();
		}
		Bench.Health = Bench.Body->FindComponentByClass<UHealthComponent>();
		if (Bench.Health)
		{
			Bench.Health->bShowDamageNumbers = false;
		}
		return Bench;
	}

	/** A floor 60 m across with its top at 0. */
	inline void SpawnFloor(UWorld* World)
	{
		LocomotionTestWorld::SpawnBlock(World, FVector(0.0, 0.0, -50.0), FVector(6000.0, 6000.0, 100.0));
	}

	/** A definition made here (the design's numbers; the tests don't lean on the data assets). */
	inline UEmberDefinition* MakeDefinition(FName Id, float Duration, float Damage, float Range, float Radius, float Strength)
	{
		UEmberDefinition* Def = NewObject<UEmberDefinition>(GetTransientPackage());
		Def->Id = Id;
		Def->DurationSeconds = Duration;
		Def->Damage = Damage;
		Def->Range = Range;
		Def->Radius = Radius;
		Def->Strength = Strength;
		Def->Color = FLinearColor(1.f, 0.6f, 0.2f);
		return Def;
	}

	inline FEmberContext MakeContext(const FBench& Bench, const UEmberDefinition* Def)
	{
		FEmberContext Context;
		Context.Character = Bench.Body;
		Context.Component = Bench.Embers;
		Context.Definition = Def;
		Context.LevelScale = 1.f;
		return Context;
	}

	/** A spider standing on the floor at Feet, started as play starts it, able to take a launch, dropping nothing. */
	inline ASpiderCreature* SpawnSpider(UWorld* World, const FVector& Feet)
	{
		ASpiderCreature* Spider = World->SpawnActor<ASpiderCreature>(Feet + FVector(0.0, 0.0, 300.0), FRotator::ZeroRotator);
		if (!Spider)
		{
			return nullptr;
		}
		if (ULootDropComponent* Loot = Spider->FindComponentByClass<ULootDropComponent>())
		{
			Loot->bDropOnDeath = false;
		}
		Spider->DispatchBeginPlay();
		const float HalfHeight = Spider->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Spider->SetActorLocation(Feet + FVector(0.0, 0.0, HalfHeight + 1.0), false, nullptr, ETeleportType::TeleportPhysics);
		UCharacterMovementComponent* Movement = Spider->GetCharacterMovement();
		Movement->SetUpdatedComponent(Spider->GetCapsuleComponent());
		Movement->SetMovementMode(MOVE_Walking);
		Movement->Activate(true);
		return Spider;
	}

	/** A hurt as AActor::TakeDamage hands it to its listeners (its point, then the damage), from Causer. */
	inline void Hurt(AActor* Victim, float Damage, AActor* Causer = nullptr)
	{
		Victim->OnTakeAnyDamage.Broadcast(Victim, Damage, GetDefault<UEmberDamageType>(), nullptr, Causer);
	}

	/** What a power dealt to Target this use (0 when nothing). */
	inline float DealtTo(const UEmberPowerBase& Power, const AActor* Target)
	{
		float Sum = 0.f;
		for (const UEmberPowerBase::FDealt& Each : Power.GetDealt())
		{
			if (Each.Target.Get() == Target)
			{
				Sum += Each.Damage;
			}
		}
		return Sum;
	}

	/** Each of the hero's three icons is generated (Art/Icons/InkedIcons.py) under its IconName. */
	inline bool HasIcon(const TCHAR* Name)
	{
		const LooterUI::FInkedIcon* Icon = LooterUI::FindInkedIcon(Name);
		return Icon && Icon->Shapes.Num() > 0;
	}
}

#endif
