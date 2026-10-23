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

	/**
	 * When its rules find no spot: this far from the player (cm), a lunge and a step off, so they come round the room
	 * rather than into the player's face; and at the last this far, out of reach of a first swing.
	 */
	constexpr float NearClearOfPlayer = 350.f;
	constexpr float LastClearOfPlayer = 200.f;

	/** Closest together its creatures come when its rules find no spot: their bodies apart, and never under this (cm). */
	constexpr float BodyGap = 30.f;
	constexpr float LeastSpacing = 100.f;

	/** Why candidate spots were turned down, counted for the log when none was left. */
	struct FTurnedDown
	{
		int32 SafeZone = 0;
		int32 OffPlayable = 0;
		int32 OffGround = 0;
		int32 NearPlayer = 0;
		int32 NoFloor = 0;
		int32 NotStandable = 0;
		int32 NoRoom = 0;
		int32 OtherLevel = 0;

		FString Describe(int32 Candidates) const
		{
			const int32 Left = Candidates - SafeZone - OffPlayable - OffGround - NearPlayer - NoFloor - NotStandable - NoRoom - OtherLevel;
			return FString::Printf(TEXT("%d candidates: %d in a safe zone, %d off the playable area, %d off its ground, %d by the player, ")
				TEXT("%d with no floor within a step, %d not standable (steep, or on an obstacle), %d with no room, %d on another level, %d ")
				TEXT("too close to another"), Candidates, SafeZone, OffPlayable, OffGround, NearPlayer, NoFloor, NotStandable, NoRoom,
				OtherLevel, FMath::Max(Left, 0));
		}
	};
}

TArray<FEncounterSpotTry> AEncounterSpawner::SpawnSpotTries(float ClearOfPlayer, float Spacing, float BodyRadius)
{
	const float Near = FMath::Min(ClearOfPlayer, NearClearOfPlayer);
	const float Last = FMath::Min(ClearOfPlayer, LastClearOfPlayer);
	const float Close = FMath::Min(Spacing, FMath::Max(BodyRadius * 2.f + BodyGap, LeastSpacing));
	TArray<FEncounterSpotTry> Tries;
	for (const FEncounterSpotTry& Each : { FEncounterSpotTry{ ClearOfPlayer, Spacing }, FEncounterSpotTry{ Near, Spacing },
		FEncounterSpotTry{ Near, Close }, FEncounterSpotTry{ Last, Close } })
	{
		const bool bRepeat = Tries.ContainsByPredicate([&Each](const FEncounterSpotTry& Other)
		{
			return FMath::IsNearlyEqual(Other.ClearOfPlayer, Each.ClearOfPlayer) && FMath::IsNearlyEqual(Other.Spacing, Each.Spacing);
		});
		if (!bRepeat)
		{
			Tries.Add(Each);
		}
	}
	return Tries;
}

TArray<FVector> AEncounterSpawner::ChooseSpawnSpots(int32 Wanted, const APawn* Player, int32* OutTry, FString* OutWhy) const
{
	UWorld* World = GetWorld();
	if (!World || Wanted <= 0)
	{
		return TArray<FVector>();
	}
	// Round its spot, or a patrol's point on its route (its own spawn points stay its spot's).
	const FVector Here = GetSpawnCenter();
	// Every spot needs room for the largest body its groups bring, not a man's: a spot a spider fits beside the den's jamb
	// put the Gravemother's legs in the rock (the play-test of 2026-10-08).
	const FEncounterBody Largest = EncounterRules::LargestBody(Groups, FindArea());
	FEncounterGroundProbe Probe(World, this, TEXT("EncounterSpot"), Largest, MaxGroundStep);
	// With no ground anywhere near the spawner (a test level), its creatures stand level with it. A lair on a rock's floor
	// (her den is carved in Den Rock, tagged Obstacle) makes that rock's floor ground for its creatures, under its roof. One
	// under a roof (Tilly's cellar) finds its own floor, and its spots' ground under that roof, never the yard over it.
	Probe.FindHome(Here, HomeLookAbove, HomeLookBelow);
	const double HomeZ = Probe.GetHomeZ();
	if (Probe.HasRoof())
	{
		UE_LOG(LogLooter, Verbose, TEXT("Encounter %s: its ground at %.0f has a roof %.0f cm over it; its spots' ground is looked for under it."),
			*GetSpawnerId().ToString(), HomeZ, Probe.GetRoofZ() - HomeZ);
	}

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
	const FVector PlayerSpot = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	// Each candidate's ground is looked at once, however many looks below ask about it.
	TMap<FVector, FEncounterGroundHit> Grounds;
	FTurnedDown TurnedDown;
	auto GroundAt = [this, &Probe, &Grounds, &TurnedDown, HomeZ](const FVector& Spot)
	{
		const FEncounterGroundHit* Known = Grounds.Find(Spot);
		const FEncounterGroundHit Ground = Known ? *Known : Grounds.Add(Spot, Probe.Look(Spot));
		TurnedDown.NoFloor += Ground.bFound ? 0 : 1;
		TurnedDown.NotStandable += Ground.bFound && !Ground.bStandable ? 1 : 0;
		TurnedDown.NoRoom += Ground.bFound && Ground.bStandable && Ground.Room <= 0.f ? 1 : 0;
		TurnedDown.OtherLevel += Ground.bFound && Ground.bStandable && Ground.Room > 0.f && FMath::Abs(Ground.Point.Z - HomeZ) > MaxGroundStep
			? 1 : 0;
		return Ground;
	};
	double ClearOfPlayerSquared = 0.0;
	auto IsBlocked = [Encounters, Playable, &Turf, &Here, Player, &PlayerSpot, &ClearOfPlayerSquared, &TurnedDown](const FVector& Spot)
	{
		if (Encounters && Encounters->IsInSafeZone(Spot))
		{
			++TurnedDown.SafeZone;
			return true;
		}
		if (Playable && !Playable->Contains(Spot))
		{
			++TurnedDown.OffPlayable;
			return true;
		}
		if (!Turf.ContainsSpot(Spot, Here))
		{
			++TurnedDown.OffGround;
			return true;
		}
		if (Player && FVector::DistSquared2D(Spot, PlayerSpot) < ClearOfPlayerSquared)
		{
			++TurnedDown.NearPlayer;
			return true;
		}
		return false;
	};

	// Its rules first; where they leave no spot at all, nearer the player (the farthest spots first), then closer together.
	// Its ground, its level and the room for its bodies are never given up: a spot it can't stand on is no spot.
	const TArray<FEncounterSpotTry> Tries = SpawnSpotTries(UEncounterSettings::Get().MinSpawnDistanceFromPlayer, Spacing, Largest.Radius);
	TArray<FVector> FarFirst = Candidates;
	if (Player)
	{
		FarFirst.StableSort([&PlayerSpot](const FVector& A, const FVector& B)
		{
			return FVector::DistSquared2D(A, PlayerSpot) > FVector::DistSquared2D(B, PlayerSpot);
		});
	}
	TArray<FVector> Chosen;
	int32 Used = 0;
	for (int32 Try = 0; Try < Tries.Num() && Chosen.IsEmpty(); ++Try)
	{
		// Without a player only the spacing can give: none of the looks that only come nearer the player is needed.
		if (Try > 0 && !Player && FMath::IsNearlyEqual(Tries[Try].Spacing, Tries[Try - 1].Spacing))
		{
			continue;
		}
		TurnedDown = FTurnedDown();
		ClearOfPlayerSquared = FMath::Square(static_cast<double>(Tries[Try].ClearOfPlayer));
		Chosen = EncounterRules::ChooseSpots(Try == 0 ? Candidates : FarFirst, Wanted, HomeZ, MaxGroundStep, Tries[Try].Spacing, Occupied,
			GroundAt, IsBlocked);
		Used = Try;
	}
	if (OutTry)
	{
		*OutTry = Chosen.IsEmpty() ? INDEX_NONE : Used;
	}
	if (OutWhy && Chosen.IsEmpty())
	{
		*OutWhy = TurnedDown.Describe(Candidates.Num());
	}

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
