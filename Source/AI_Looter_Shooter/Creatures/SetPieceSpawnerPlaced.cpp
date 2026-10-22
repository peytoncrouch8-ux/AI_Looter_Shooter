// ASetPieceSpawner's placed creatures (a balcony's Bell-Ringer, the high ground's Spitters: spots of their own, off its
// ground's spots and probes) and the speakers it shuts while it's on. Springing, beats and its end are in
// SetPieceSpawner.cpp.

#include "Creatures/SetPieceSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/SetPieceRules.h"
#include "Story/SpeakerPointComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------------------
// Placed creatures
// ---------------------------------------------------------------------------

ACreatureBase* ASetPieceSpawner::SpawnPlaced(int32 Entry)
{
	UWorld* World = GetWorld();
	if (!World || !Placed.IsValidIndex(Entry) || !Placed[Entry].CreatureClass)
	{
		return nullptr;
	}
	const FSetPiecePlaced& Each = Placed[Entry];
	ACreatureBase::FRuntimeSpawn Setup;
	Setup.Rank = Each.Rank;
	ACreatureBase* Creature = ACreatureBase::SpawnAtRuntime(World, Each.CreatureClass, Each.Location, Each.Yaw, Setup);
	if (!Creature)
	{
		UE_LOG(LogLooter, Warning, TEXT("Set piece %s: couldn't place its %s at (%.0f, %.0f, %.0f)."), *GetSpawnerId().ToString(),
			*GetNameSafe(Each.CreatureClass.Get()), Each.Location.X, Each.Location.Y, Each.Location.Z);
		return nullptr;
	}
	if (!Creature->HasActorBegunPlay())
	{
		Creature->DispatchBeginPlay();
	}
	// One of the fight as any of its spawned ones: its ground, its tags (the missions count them), the level's caps' count.
	Creature->HuntingGround = MakeHuntingGround();
	for (const FName& Tag : CreatureTags)
	{
		if (!Tag.IsNone())
		{
			Creature->Tags.AddUnique(Tag);
		}
	}
	UEncounterSubsystem* Encounters = GetEncounters();
	if (Encounters)
	{
		Encounters->TrackCreature(Creature);
	}
	Creature->PlayEntranceIfSeen();
	APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	if (bHuntOnSpawn && Player)
	{
		Creature->AlertTo(Player);
	}
	HighestLevel = FMath::Max(HighestLevel, Creature->Level);
	FPlacedLiving& Out = PlacedLiving.AddDefaulted_GetRef();
	Out.Creature = Creature;
	Out.Entry = Entry;
	return Creature;
}

void ASetPieceSpawner::SpawnPlacedForWave(int32 Wave)
{
	for (int32 Entry = 0; Entry < Placed.Num(); ++Entry)
	{
		if (Placed[Entry].Wave == Wave)
		{
			SpawnPlaced(Entry);
		}
	}
}

void ASetPieceSpawner::BringPlacedBack()
{
	const TArray<int32> Coming = MoveTemp(PlacedOwed);
	PlacedOwed.Reset();
	for (const int32 Entry : Coming)
	{
		SpawnPlaced(Entry);
	}
}

void ASetPieceSpawner::RemovePlaced(bool bOweThem)
{
	PrunePlaced();
	for (const FPlacedLiving& Each : PlacedLiving)
	{
		ACreatureBase* Creature = Each.Creature.Get();
		if (!Creature)
		{
			continue;
		}
		if (bOweThem)
		{
			PlacedOwed.Add(Each.Entry);
		}
		// Taken away, not killed: no loot, no experience, no death for the missions.
		Creature->Destroy();
	}
	PlacedLiving.Reset();
}

void ASetPieceSpawner::PrunePlaced()
{
	// Killed (or lost off the world): it never comes back.
	PlacedLiving.RemoveAll([](const FPlacedLiving& Each)
	{
		const ACreatureBase* Creature = Each.Creature.Get();
		return !Creature || Creature->IsDead();
	});
}

int32 ASetPieceSpawner::NumPlacedAlive() const
{
	int32 Alive = 0;
	for (const FPlacedLiving& Each : PlacedLiving)
	{
		const ACreatureBase* Creature = Each.Creature.Get();
		Alive += Creature && !Creature->IsDead() ? 1 : 0;
	}
	return Alive;
}

bool ASetPieceSpawner::IsHeld() const
{
	const UEncounterSubsystem* Encounters = GetEncounters();
	if (!Encounters || WaitForSpawners.IsEmpty())
	{
		return false;
	}
	TArray<EEncounterState, TInlineAllocator<4>> Waited;
	for (const FName& Id : WaitForSpawners)
	{
		if (const AEncounterSpawner* Other = Encounters->FindSpawner(Id))
		{
			Waited.Add(Other->GetState());
		}
	}
	return SetPieceRules::IsHeld(Waited);
}

// ---------------------------------------------------------------------------
// The speakers it shuts
// ---------------------------------------------------------------------------

void ASetPieceSpawner::Hush()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	for (const FName& Tag : HushSpeakerTags)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(World, Tag, Found);
		for (AActor* Actor : Found)
		{
			TArray<USpeakerPointComponent*> Points;
			if (Actor)
			{
				Actor->GetComponents(Points);
			}
			for (USpeakerPointComponent* Point : Points)
			{
				// Only the ones it shut are opened again: a door a mission keeps shut stays shut.
				if (Point && Point->bEnabled)
				{
					Point->bEnabled = false;
					Hushed.Add(Point);
				}
			}
		}
	}
	if (Hushed.Num() > 0)
	{
		UE_LOG(LogLooter, Log, TEXT("Set piece %s: %d speakers won't talk until it's over."), *GetSpawnerId().ToString(), Hushed.Num());
	}
}

void ASetPieceSpawner::Unhush()
{
	for (const TWeakObjectPtr<USpeakerPointComponent>& Each : Hushed)
	{
		if (USpeakerPointComponent* Point = Each.Get())
		{
			Point->bEnabled = true;
		}
	}
	Hushed.Reset();
}
