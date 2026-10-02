#pragma once

// What the mission tests build in their test levels: stand-ins, training dummies that die for good, test missions.

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Combat/TargetDummy.h"
#include "Loot/LootDropComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

namespace MissionTestWorld
{
	/** An actor that is only a place: the player's stand-in, a poster, a speaker point. */
	inline AActor* SpawnMarker(UWorld* World, const FVector& Where, FName Tag = NAME_None)
	{
		AActor* Marker = World->SpawnActor<AActor>();
		if (!Marker)
		{
			return nullptr;
		}
		USceneComponent* Root = NewObject<USceneComponent>(Marker, TEXT("Root"));
		Marker->SetRootComponent(Root);
		Root->RegisterComponent();
		Marker->SetActorLocation(Where);
		if (!Tag.IsNone())
		{
			Marker->Tags.Add(Tag);
		}
		return Marker;
	}

	/** A training dummy started as play starts it, dropping no loot and showing no numbers (a test level has no loot to see). */
	inline ATargetDummy* SpawnDummy(UWorld* World, const FVector& Where, FName Tag = NAME_None)
	{
		ATargetDummy* Dummy = World->SpawnActor<ATargetDummy>(Where, FRotator::ZeroRotator);
		if (!Dummy)
		{
			return nullptr;
		}
		if (!Tag.IsNone())
		{
			Dummy->Tags.Add(Tag);
		}
		if (ULootDropComponent* Loot = Dummy->FindComponentByClass<ULootDropComponent>())
		{
			Loot->bDropOnDeath = false;
		}
		if (UHealthComponent* Health = Dummy->FindComponentByClass<UHealthComponent>())
		{
			Health->bShowDamageNumbers = false;
		}
		Dummy->DispatchBeginPlay();
		return Dummy;
	}

	/** Damage as a shot deals it, credited to By (a player's controller, or nobody). */
	inline void Hurt(AActor* Victim, float Damage, AController* By)
	{
		UGameplayStatics::ApplyDamage(Victim, Damage, By, nullptr, UDamageType::StaticClass());
	}

	/** A mission made in code, in a scratch package so it never meets the project's assets. */
	inline UMissionDefinition* NewMission(UObject* Outer, const TCHAR* Id, EMissionKind Kind, EMissionStart Start, FName Area)
	{
		UMissionDefinition* Mission = NewObject<UMissionDefinition>(Outer, FName(*(FString(UMissionDefinition::AssetPrefix) + Id)), RF_Transient);
		Mission->Id = FName(Id);
		Mission->Title = FText::FromString(FString::Printf(TEXT("Test %s"), Id));
		Mission->Kind = Kind;
		Mission->Start = Start;
		Mission->Area = Area;
		return Mission;
	}

	/** Adds an objective of type T to step Step (made as needed). */
	template <typename T>
	T* AddObjective(UMissionDefinition* Mission, int32 Step)
	{
		while (Mission->Steps.Num() <= Step)
		{
			Mission->Steps.AddDefaulted();
		}
		T* Objective = NewObject<T>(Mission);
		Mission->Steps[Step].Objectives.Add(Objective);
		return Objective;
	}
}

#endif
