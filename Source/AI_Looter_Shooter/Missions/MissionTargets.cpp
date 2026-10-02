#include "Missions/MissionTargets.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"

// --- FMissionActorFilter ---

bool FMissionActorFilter::Matches(const AActor* Candidate) const
{
	return Candidate && IsSet() && (!ActorClass || Candidate->IsA(ActorClass)) && (ActorTag.IsNone() || Candidate->ActorHasTag(ActorTag));
}

FString FMissionActorFilter::Describe() const
{
	if (!ActorTag.IsNone())
	{
		return FName::NameToDisplayString(ActorTag.ToString(), /*bIsBool*/ false);
	}
	if (ActorClass)
	{
		// A Blueprint's class is "BP_Thing_C": people read "Thing".
		FString Name = ActorClass->GetName();
		Name.RemoveFromStart(TEXT("BP_"));
		Name.RemoveFromEnd(TEXT("_C"));
		return FName::NameToDisplayString(Name, /*bIsBool*/ false);
	}
	return FString();
}

// --- FMissionPlace ---

TOptional<FVector> FMissionPlace::Resolve(const UWorld* World, const TOptional<FVector>& From) const
{
	if (!Actor.IsSet())
	{
		return Location;
	}
	const AActor* Nearest = MissionTargets::FindNearest(World, Actor, From, /*bLivingOnly*/ false);
	return Nearest ? TOptional<FVector>(Nearest->GetActorLocation()) : TOptional<FVector>();
}

bool FMissionPlace::Contains(const FVector& Middle, const FVector& Where) const
{
	const double Distance = bIgnoreHeight ? FVector::Dist2D(Middle, Where) : FVector::Dist(Middle, Where);
	return Distance <= Radius;
}

bool FMissionPlace::ContainsInWorld(const UWorld* World, const FVector& Where) const
{
	const TOptional<FVector> Middle = Resolve(World, Where);
	return Middle.IsSet() && Contains(*Middle, Where);
}

// --- MissionTargets ---

namespace MissionTargets
{
	void ForEach(const UWorld* World, const FMissionActorFilter& Filter, TFunctionRef<void(AActor&)> Visit)
	{
		if (!World || !Filter.IsSet())
		{
			return;
		}
		// A class narrows the walk to that class's actors; a tag alone has to look at every actor.
		const TSubclassOf<AActor> Walk = Filter.ActorClass ? Filter.ActorClass : TSubclassOf<AActor>(AActor::StaticClass());
		for (TActorIterator<AActor> It(World, Walk); It; ++It)
		{
			AActor* Candidate = *It;
			if (Candidate && !Candidate->IsActorBeingDestroyed() && Filter.Matches(Candidate))
			{
				Visit(*Candidate);
			}
		}
	}

	bool Any(const UWorld* World, const FMissionActorFilter& Filter)
	{
		bool bFound = false;
		ForEach(World, Filter, [&bFound](AActor&) { bFound = true; });
		return bFound;
	}

	AActor* FindNearest(const UWorld* World, const FMissionActorFilter& Filter, const TOptional<FVector>& From, bool bLivingOnly)
	{
		AActor* Nearest = nullptr;
		double NearestDistance = TNumericLimits<double>::Max();
		ForEach(World, Filter, [&](AActor& Candidate)
		{
			if (bLivingOnly && !IsAlive(Candidate))
			{
				return;
			}
			// Without a player to measure from, the first one will do (a level usually has one weapon rack, one door).
			const double Distance = From.IsSet() ? FVector::DistSquared2D(Candidate.GetActorLocation(), *From) : 0.0;
			if (!Nearest || Distance < NearestDistance)
			{
				Nearest = &Candidate;
				NearestDistance = Distance;
			}
		});
		return Nearest;
	}

	TOptional<FVector> FindCenter(const UWorld* World, const FMissionActorFilter& Filter)
	{
		FVector Sum = FVector::ZeroVector;
		int32 Found = 0;
		ForEach(World, Filter, [&Sum, &Found](AActor& Candidate)
		{
			Sum += Candidate.GetActorLocation();
			++Found;
		});
		return Found > 0 ? TOptional<FVector>(Sum / Found) : TOptional<FVector>();
	}

	bool IsAlive(const AActor& Candidate)
	{
		if (const ACreatureBase* Creature = Cast<ACreatureBase>(&Candidate))
		{
			return !Creature->IsDead();
		}
		const UHealthComponent* Health = Candidate.FindComponentByClass<UHealthComponent>();
		return !Health || !Health->IsDead();
	}
}
