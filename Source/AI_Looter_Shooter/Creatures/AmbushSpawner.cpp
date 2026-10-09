#include "Creatures/AmbushSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/HuntingGround.h"
#include "Creatures/PackRules.h"
#include "Creatures/UnpaidCreature.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"

namespace
{
	/** The dead rising from their graves, and a spider dropping on its silk. Cues without sounds play nothing (list them in LooterSoundCues.h). */
	const FName RiseCue(TEXT("Creature.Unpaid.Rise"));
	const FName DropCue(TEXT("Creature.Spider.Drop"));

	/** A drop shorter than this (cm) isn't worth the fall: it simply stands there. */
	constexpr float ShortestDrop = 60.f;

	/** It falls from this far (cm) under whatever overhead stopped the look up: never wedged in a limb. */
	constexpr float OverheadClearance = 20.f;

	/** Looks this many intervals apart or less are one watch on the player; a longer gap starts afresh (it was off). */
	constexpr double LooksInAWatch = 4.0;
}

AAmbushSpawner::AAmbushSpawner()
{
	// Sprung, they come at once.
	bHuntOnSpawn = true;
}

bool AAmbushSpawner::IsOnAmbushGround(const FVector& Point) const
{
	if (AmbushCorners.Num() < 3)
	{
		return false;
	}
	if (AmbushMaxRise > 0.f && FMath::Abs(Point.Z - GetActorLocation().Z) > AmbushMaxRise)
	{
		return false;
	}
	return FHuntingGround::IsInsidePolygon(AmbushCorners, FVector2D(Point.X, Point.Y));
}

bool AAmbushSpawner::CheckApproach()
{
	if (AmbushCorners.Num() < 3)
	{
		return Super::CheckApproach();
	}
	const UEncounterSubsystem* Encounters = GetEncounters();
	const APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr;
	const UWorld* World = GetWorld();
	if (!Player || !World)
	{
		bSeenPlayer = false;
		return false;
	}
	const double Now = World->GetTimeSeconds();
	const bool bWatching = bSeenPlayer && Now - LastLookTime <= FMath::Max(static_cast<double>(CheckSeconds), 0.1) * LooksInAWatch;
	const FVector Spot = Player->GetActorLocation();
	const bool bOnGround = IsOnAmbushGround(Spot);
	const bool bWalkedIn = PackRules::IsWalkIn(bWatching, bLastOnGround, bOnGround, LastPlayerSpot, Spot, WalkInStep);
	LastPlayerSpot = Spot;
	bLastOnGround = bOnGround;
	bSeenPlayer = true;
	LastLookTime = Now;
	if (bWalkedIn)
	{
		UE_LOG(LogLooter, Log, TEXT("Ambush %s: the player walked in at (%.0f, %.0f)."), *GetSpawnerId().ToString(), Spot.X, Spot.Y);
	}
	return bWalkedIn;
}

void AAmbushSpawner::OnCreatureSpawned(ACreatureBase& Creature)
{
	switch (Entrance)
	{
	case EAmbushEntrance::Rise:
		// Out of the ground where it stands, its shroud's ends first, shot through until it's mostly there (as Abel's adds).
		if (AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(&Creature))
		{
			Unpaid->RiseIn();
		}
		LooterSound::PlayAt(this, RiseCue, Creature.GetActorLocation());
		break;

	case EAmbushEntrance::Drop:
		Drop(Creature);
		break;

	default:
		break;
	}
}

void AAmbushSpawner::Drop(ACreatureBase& Creature) const
{
	UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = Creature.GetCapsuleComponent();
	if (!World || !Capsule || DropHeight < ShortestDrop)
	{
		return;
	}
	// As high as there's room: its body swept up against what's overhead (a limb, a roof), falling from under it.
	const FVector Feet = Creature.GetActorLocation();
	const FCollisionShape Body = FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("AmbushDrop"), &Creature, /*bTraceComplex*/ false);
	FHitResult Overhead;
	float Lift = DropHeight;
	if (World->SweepSingleByObjectType(Overhead, Feet, Feet + FVector(0.0, 0.0, DropHeight), FQuat::Identity,
		FCollisionObjectQueryParams(ECC_WorldStatic), Body, Params))
	{
		Lift = Overhead.bStartPenetrating ? 0.f : FMath::Max(static_cast<float>(Overhead.Distance) - OverheadClearance, 0.f);
	}
	if (Lift < ShortestDrop)
	{
		return;
	}
	Creature.SetActorLocation(Feet + FVector(0.0, 0.0, Lift), false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Movement = Creature.GetCharacterMovement())
	{
		Movement->SetMovementMode(MOVE_Falling);
	}
	LooterSound::PlayAttached(DropCue, Creature.GetRootComponent());
}
