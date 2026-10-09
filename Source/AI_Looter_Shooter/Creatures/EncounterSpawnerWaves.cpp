// AEncounterSpawner's waves: what each brings, spawning them under the caps, and taking them away while the player is
// far. Where they stand is in EncounterSpawnerSpots.cpp; its state, the story and its timer are in EncounterSpawner.cpp.

#include "Creatures/EncounterSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaRulesSubsystem.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/EncounterSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Drawn on a screen this lately (seconds) still counts as seen: it isn't taken away under the player's eyes. */
	constexpr float SeenLatelySeconds = 0.5f;

	/**
	 * Whether a group's kind is under its cap across the level: the group's own, or the class's in the settings (the Unpaid's
	 * 12). When the kind has a cap, OutCount is its running count in KindAlive (counted on first use); otherwise null.
	 */
	bool KindHasRoom(const FEncounterGroup& Group, const UEncounterSettings& Settings, const UEncounterSubsystem* Encounters,
		TMap<const UClass*, int32>& KindAlive, int32*& OutCount)
	{
		const UClass* Counted = Group.CreatureClass;
		int32 KindCap = Group.MaxAliveOfClass;
		if (KindCap <= 0)
		{
			KindCap = Settings.FindClassCap(Group.CreatureClass, &Counted);
		}
		OutCount = nullptr;
		if (KindCap <= 0 || !Counted)
		{
			return true;
		}
		OutCount = KindAlive.Find(Counted);
		if (!OutCount)
		{
			OutCount = &KindAlive.Add(Counted, Encounters ? Encounters->CountAlive(Counted) : 0);
		}
		return EncounterRules::Room(*OutCount, KindCap) > 0;
	}
}

// ---------------------------------------------------------------------------
// Waves
// ---------------------------------------------------------------------------

void AEncounterSpawner::Engage()
{
	SetState(EEncounterState::Engaged);
	// The first wave comes with the first approach; after that, what was owed comes back and timed waves go on.
	if (WavesStarted == 0)
	{
		StartWave();
	}
	else
	{
		SpawnOwed();
	}
}

void AEncounterSpawner::StartWave()
{
	if (!HasWavesLeft())
	{
		return;
	}
	const int32 Wave = ++WavesStarted;
	WaveClock = 0.f;
	const UAreaDefinition* Area = FindArea();
	int32 Brought = 0;
	for (int32 GroupIndex = 0; GroupIndex < Groups.Num(); ++GroupIndex)
	{
		const FEncounterGroup& Group = Groups[GroupIndex];
		if (!Group.CreatureClass || !EncounterRules::JoinsWave(Group, Wave))
		{
			continue;
		}
		for (int32 Each = 0; Each < Group.Count && (MaxTotal <= 0 || TotalQueued < MaxTotal); ++Each)
		{
			// Ranks are rolled as the wave is called, so one taken away and brought back keeps its own.
			FOwedCreature& Entry = Owed.AddDefaulted_GetRef();
			Entry.Group = GroupIndex;
			Entry.Rank = EncounterRules::PickRank(Group, Area, Rolls.FRand());
			++TotalQueued;
			++Brought;
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Encounter %s: wave %d brings %d."), *GetSpawnerId().ToString(), Wave, Brought);
	SpawnOwed();
}

bool AEncounterSpawner::HasWavesLeft() const
{
	return EncounterRules::HasWavesLeft(WavesStarted, NumWaves, TotalQueued, MaxTotal);
}

bool AEncounterSpawner::HasTimedWaves() const
{
	return bWaitForClear || WaveInterval > 0.f;
}

const UAreaDefinition* AEncounterSpawner::FindArea() const
{
	const UWorld* World = GetWorld();
	const UAreaRulesSubsystem* Rules = World ? World->GetSubsystem<UAreaRulesSubsystem>() : nullptr;
	return Rules ? Rules->GetArea() : nullptr;
}

// ---------------------------------------------------------------------------
// Spawning under the caps
// ---------------------------------------------------------------------------

int32 AEncounterSpawner::SpawnOwed()
{
	if (Owed.IsEmpty() || !GetWorld())
	{
		return 0;
	}
	PruneLiving();
	UEncounterSubsystem* Encounters = GetEncounters();
	const UEncounterSettings& Settings = UEncounterSettings::Get();
	APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	const FVector PlayerSpot = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	const double NearSquared = FMath::Square(static_cast<double>(Settings.NearPlayerRadius));

	// The caps, counted once here and kept up as it spawns: its own, the creatures near the player, and each kind's.
	int32 OwnAlive = NumAlive();
	int32 NearAlive = Encounters && Player ? Encounters->CountAliveNear(PlayerSpot, Settings.NearPlayerRadius) : 0;
	TMap<const UClass*, int32> KindAlive;

	// Nothing can come while its own cap is full, or every kind it owes is at its cap: leave before looking for spots (up to 240
	// ground probes), which it would otherwise repeat at every check while its creatures wait here for room.
	if (EncounterRules::Room(OwnAlive, MaxAlive) <= 0)
	{
		return 0;
	}
	bool bSomeKindFits = false;
	for (const FOwedCreature& Entry : Owed)
	{
		const FEncounterGroup* Group = Groups.IsValidIndex(Entry.Group) ? &Groups[Entry.Group] : nullptr;
		int32* KindCount = nullptr;
		// A group taken out of the data since is cleared out below, so it counts as one that fits.
		if (!Group || !Group->CreatureClass || KindHasRoom(*Group, Settings, Encounters, KindAlive, KindCount))
		{
			bSomeKindFits = true;
			break;
		}
	}
	if (!bSomeKindFits)
	{
		return 0;
	}

	// Spots for all of them at once, so they spread over its ground instead of piling on the first free one.
	const TArray<FVector> Spots = ChooseSpawnSpots(Owed.Num(), Player);
	int32 NextSpot = 0;
	int32 Spawned = 0;
	int32 Index = 0;
	while (Index < Owed.Num() && NextSpot < Spots.Num() && EncounterRules::Room(OwnAlive, MaxAlive) > 0)
	{
		const FOwedCreature Entry = Owed[Index];
		const FEncounterGroup* Group = Groups.IsValidIndex(Entry.Group) ? &Groups[Entry.Group] : nullptr;
		if (!Group || !Group->CreatureClass)
		{
			// Its group was taken out of the data since: nothing to bring.
			Owed.RemoveAt(Index);
			continue;
		}

		// Its kind's cap across the level: its group's own, or the class's in the settings (the Unpaid's 12).
		int32* KindCount = nullptr;
		if (!KindHasRoom(*Group, Settings, Encounters, KindAlive, KindCount))
		{
			// One of another kind may still fit; this one waits its turn.
			++Index;
			continue;
		}

		// At most 16 of any kind near the player: a spot near them waits for room, one far off doesn't add to it.
		const FVector Feet = Spots[NextSpot];
		const bool bNear = Player && FVector::DistSquared(Feet, PlayerSpot) <= NearSquared;
		if (bNear && EncounterRules::Room(NearAlive, Settings.MaxCreaturesNearPlayer) <= 0)
		{
			break;
		}
		++NextSpot;
		Owed.RemoveAt(Index);
		if (!SpawnOne(Entry, Feet, Player))
		{
			UE_LOG(LogLooter, Warning, TEXT("Encounter %s: couldn't spawn a %s; it's dropped."), *GetSpawnerId().ToString(),
				*GetNameSafe(Group->CreatureClass.Get()));
			continue;
		}
		++Spawned;
		++OwnAlive;
		NearAlive += bNear ? 1 : 0;
		if (KindCount)
		{
			++*KindCount;
		}
	}

	if (Owed.Num() > 0 && Spots.IsEmpty() && !bWarnedNoRoom)
	{
		bWarnedNoRoom = true;
		UE_LOG(LogLooter, Warning, TEXT("Encounter %s: its creatures find nowhere to stand (every spot is on another level, in a safe ")
			TEXT("zone, off its ground or the playable area, on an obstacle, or by the player)."), *GetSpawnerId().ToString());
	}
	if (Spawned > 0)
	{
		UE_LOG(LogLooter, Log, TEXT("Encounter %s: %d spawned, %d alive, %d still to come."), *GetSpawnerId().ToString(), Spawned,
			NumAlive(), Owed.Num());
	}
	return Spawned;
}

ACreatureBase* AEncounterSpawner::SpawnOne(const FOwedCreature& Entry, const FVector& Feet, APawn* Player)
{
	const FEncounterGroup& Group = Groups[Entry.Group];
	ACreatureBase::FRuntimeSpawn Setup;
	Setup.Rank = Entry.Rank;
	Setup.Level = Group.Level;
	Setup.BodyScale = Group.BodyScale;
	Setup.HealthScale = Group.HealthScale;
	// Facing the player when there is one (they come for the corpse), else any way.
	const float Yaw = Player ? static_cast<float>((Player->GetActorLocation() - Feet).Rotation().Yaw)
		: static_cast<float>(Rolls.FRandRange(-180.0, 180.0));
	ACreatureBase* Creature = ACreatureBase::SpawnAtRuntime(GetWorld(), Group.CreatureClass, Feet, Yaw, Setup);
	if (!Creature)
	{
		return nullptr;
	}
	// In a level that isn't playing yet (a test level), it starts as play would start it.
	if (!Creature->HasActorBegunPlay())
	{
		Creature->DispatchBeginPlay();
	}
	// It fights on the spawner's ground, and carries its tags for the missions that count its kind.
	Creature->HuntingGround = MakeHuntingGround();
	for (const FName& Tag : CreatureTags)
	{
		if (!Tag.IsNone())
		{
			Creature->Tags.AddUnique(Tag);
		}
	}
	// A Legendary lair's monster: its death is noted in the session, for its return (USessionSubsystem::IsLegendaryBack).
	if (!LegendaryId.IsNone())
	{
		if (UHealthComponent* Life = Creature->FindComponentByClass<UHealthComponent>())
		{
			Life->OnDeath.AddDynamic(this, &AEncounterSpawner::HandleCreatureDeath);
		}
	}
	FLivingCreature& Out = Living.AddDefaulted_GetRef();
	Out.Creature = Creature;
	Out.Group = Entry.Group;
	if (UEncounterSubsystem* Encounters = GetEncounters())
	{
		Encounters->TrackCreature(Creature);
	}
	// Its kind of encounter's own entrance (an ambush's dead rising, its spiders dropping), before it comes for anyone.
	OnCreatureSpawned(*Creature);
	if (bHuntOnSpawn && Player)
	{
		// Refused, as any call is, when the player isn't on its hunting ground or is in a safe zone.
		Creature->AlertTo(Player);
	}
	return Creature;
}

// ---------------------------------------------------------------------------
// Killed, and taken away
// ---------------------------------------------------------------------------

void AEncounterSpawner::PruneLiving()
{
	for (int32 Index = Living.Num() - 1; Index >= 0; --Index)
	{
		const ACreatureBase* Creature = Living[Index].Creature.Get();
		if (!Creature || Creature->IsDead())
		{
			// Killed, or lost off the world: it never comes back, and its body sinks away on its own.
			Living.RemoveAtSwap(Index);
			++Killed;
		}
	}
}

int32 AEncounterSpawner::RemoveLiving(bool bOweThem)
{
	PruneLiving();
	int32 Taken = 0;
	for (const FLivingCreature& Each : Living)
	{
		ACreatureBase* Creature = Each.Creature.Get();
		if (!Creature)
		{
			continue;
		}
		if (bOweThem)
		{
			// Not killed: owed, and back as it was (its rank) when the player comes back.
			FOwedCreature& Entry = Owed.AddDefaulted_GetRef();
			Entry.Group = Each.Group;
			Entry.Rank = Creature->GetRank();
		}
		// Taken away, not killed: no loot, no experience, no death for the missions to count.
		Creature->Destroy();
		++Taken;
	}
	Living.Reset();
	return Taken;
}

int32 AEncounterSpawner::Despawn()
{
	const int32 Taken = RemoveLiving(/*bOweThem*/ true);
	if (State == EEncounterState::Engaged)
	{
		SetState(EEncounterState::Waiting);
	}
	if (Taken > 0)
	{
		UE_LOG(LogLooter, Log, TEXT("Encounter %s: took away %d while the player is far; they come back with the player."),
			*GetSpawnerId().ToString(), Taken);
	}
	UpdateTimer();
	return Taken;
}

bool AEncounterSpawner::ShouldDespawn() const
{
	const UEncounterSubsystem* Encounters = GetEncounters();
	const APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	if (!Player || DespawnRadius <= 0.f || IsPlayerWithin(DespawnRadius))
	{
		return false;
	}
	// Far away, but never under the player's eyes or in the middle of a fight; nor while one is still near the player (it
	// chased them off and is walking home).
	if (IsAnyInView() || IsAnyFighting())
	{
		return false;
	}
	const FVector PlayerSpot = Player->GetActorLocation();
	const double NearSquared = FMath::Square(static_cast<double>(ActivationRadius));
	for (const ACreatureBase* Creature : GetAliveCreatures())
	{
		if (FVector::DistSquared(Creature->GetActorLocation(), PlayerSpot) <= NearSquared)
		{
			return false;
		}
	}
	return true;
}

bool AEncounterSpawner::IsAnyInView() const
{
	const UWorld* World = GetWorld();
	const UEncounterSubsystem* Encounters = GetEncounters();
	for (const ACreatureBase* Creature : GetAliveCreatures())
	{
		const USkeletalMeshComponent* Body = Creature->GetMesh();
		const float BodyRadius = Body ? static_cast<float>(Body->Bounds.SphereRadius) : 100.f;
		const bool bInView = Encounters && Encounters->IsInPlayersView(Creature->GetActorLocation(), BodyRadius);
		const bool bDrawnLately = World && Body && World->TimeSince(Body->GetLastRenderTimeOnScreen()) <= SeenLatelySeconds;
		if (bInView || bDrawnLately)
		{
			return true;
		}
	}
	return false;
}

bool AEncounterSpawner::IsAnyFighting() const
{
	for (const ACreatureBase* Creature : GetAliveCreatures())
	{
		const ECreatureState Doing = Creature->GetCreatureState();
		if (Doing == ECreatureState::Chase || Doing == ECreatureState::Attack)
		{
			return true;
		}
	}
	return false;
}
