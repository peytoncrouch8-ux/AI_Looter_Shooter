// AEncounterSpawner's spots: where its creatures stand as they spawn. Its waves and spawning are in
// EncounterSpawnerWaves.cpp, its state and the story in EncounterSpawner.cpp.

#include "Creatures/EncounterSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterGroundProbe.h"
#include "Creatures/EncounterRules.h"
#include "Creatures/EncounterSettings.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/HuntingGround.h"
#include "World/PlayableArea.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** Home's ground is looked for from this far over the spawner to this far under it (cm). */
	constexpr double HomeLookAbove = 300.0;
	constexpr double HomeLookBelow = 2000.0;
}

TArray<FVector> AEncounterSpawner::ChooseSpawnSpots(int32 Wanted, const APawn* Player) const
{
	UWorld* World = GetWorld();
	if (!World || Wanted <= 0)
	{
		return TArray<FVector>();
	}
	const FVector Here = GetActorLocation();
	// Every spot needs room for the largest body its groups bring, not a man's: a spot a spider fits beside the den's jamb
	// put the Gravemother's legs in the rock (the play-test of 2026-10-08).
	const FEncounterBody Largest = EncounterRules::LargestBody(Groups, FindArea());
	FEncounterGroundProbe Probe(World, this, TEXT("EncounterSpot"), Largest, MaxGroundStep);
	// With no ground anywhere near the spawner (a test level), its creatures stand level with it. A lair on a rock's floor
	// (her den is carved in Den Rock, tagged Obstacle) makes that rock's floor ground for its creatures, under its roof.
	Probe.FindHome(Here, HomeLookAbove, HomeLookBelow);
	const double HomeZ = Probe.GetHomeZ();

	TArray<FVector> Points;
	for (const FVector& Local : SpawnPoints)
	{
		Points.Add(GetActorTransform().TransformPositionNoScale(Local));
	}
	const TArray<FVector> Candidates = EncounterRules::CandidateSpots(FVector(Here.X, Here.Y, HomeZ), SpawnRadius, Points, Wanted,
		Spacing, static_cast<float>(Rolls.FRandRange(0.0, 360.0)));
	TArray<FVector> Occupied;
	for (const FLivingCreature& Each : Living)
	{
		if (const ACreatureBase* Creature = Each.Creature.Get())
		{
			Occupied.Add(Creature->GetActorLocation());
		}
	}

	const FHuntingGround Turf = MakeHuntingGround();
	const UEncounterSubsystem* Encounters = GetEncounters();
	const APlayableArea* Playable = APlayableArea::Find(World);
	const double ClearOfPlayerSquared = FMath::Square(static_cast<double>(UEncounterSettings::Get().MinSpawnDistanceFromPlayer));
	auto GroundAt = [&Probe](const FVector& Spot)
	{
		return Probe.Look(Spot);
	};
	auto IsBlocked = [Encounters, Playable, &Turf, &Here, Player, ClearOfPlayerSquared](const FVector& Spot)
	{
		return (Encounters && Encounters->IsInSafeZone(Spot))
			|| (Playable && !Playable->Contains(Spot))
			|| !Turf.ContainsSpot(Spot, Here)
			|| (Player && FVector::DistSquared2D(Spot, Player->GetActorLocation()) < ClearOfPlayerSquared);
	};
	const TArray<FVector> Chosen = EncounterRules::ChooseSpots(Candidates, Wanted, HomeZ, MaxGroundStep, Spacing, Occupied, GroundAt,
		IsBlocked);

	// Nowhere on its ground fits the whole of its largest body: it stands in the roomiest spot, but the level should give it
	// more room (or the spawner a better point).
	for (const FVector& Spot : Chosen)
	{
		const float Share = Probe.Look(Spot).Room;
		if (Share < 1.f)
		{
			UE_LOG(LogLooter, Warning, TEXT("Encounter %s: no spot on its ground has room for its largest body (%.0f cm wide) whole; ")
				TEXT("one stands at (%.0f, %.0f, %.0f), with room for %.0f%% of it."), *GetSpawnerId().ToString(), Largest.Radius * 2.f,
				Spot.X, Spot.Y, Spot.Z, Share * 100.f);
		}
	}
	return Chosen;
}
