// FBossIntroFreeze: a boss intro's actors held still on their own clocks and given back.

#include "Scenes/BossIntroFreeze.h"
#include "Creatures/CreatureBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

void FBossIntroFreeze::Hold(AActor& Actor, float Dilation)
{
	if (!IsHolding(&Actor))
	{
		FHeld Entry;
		Entry.Actor = &Actor;
		Entry.Dilation = Actor.CustomTimeDilation;
		Held.Add(Entry);
	}
	// Never 0: a tick of no time at all is divided by in places.
	Actor.CustomTimeDilation = FMath::Max(Dilation, KINDA_SMALL_NUMBER);
}

int32 FBossIntroFreeze::HoldCreaturesNear(UWorld& World, const FVector& Center, float Radius, float Dilation)
{
	int32 Count = 0;
	const double RadiusSquared = FMath::Square(static_cast<double>(Radius));
	for (TActorIterator<ACreatureBase> It(&World); It; ++It)
	{
		ACreatureBase* Creature = *It;
		if (!Creature || Creature->IsDead() || IsHolding(Creature)
			|| FVector::DistSquared(Creature->GetActorLocation(), Center) > RadiusSquared)
		{
			continue;
		}
		Hold(*Creature, Dilation);
		++Count;
	}
	return Count;
}

void FBossIntroFreeze::Release(const AActor* Actor)
{
	for (int32 Index = Held.Num() - 1; Index >= 0; --Index)
	{
		if (Held[Index].Actor.Get() == Actor)
		{
			if (AActor* Living = Held[Index].Actor.Get())
			{
				Living->CustomTimeDilation = Held[Index].Dilation;
			}
			Held.RemoveAt(Index);
		}
	}
}

void FBossIntroFreeze::ReleaseAll()
{
	for (const FHeld& Entry : Held)
	{
		if (AActor* Living = Entry.Actor.Get())
		{
			Living->CustomTimeDilation = Entry.Dilation;
		}
	}
	Held.Reset();
}

bool FBossIntroFreeze::IsHolding(const AActor* Actor) const
{
	return Actor && Held.ContainsByPredicate([Actor](const FHeld& Entry) { return Entry.Actor.Get() == Actor; });
}
