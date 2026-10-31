#include "Creatures/AmbushSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/EncounterSubsystem.h"
#include "Creatures/HuntingGround.h"
#include "Creatures/PackRules.h"
#include "Creatures/SpiderAmbushRules.h"
#include "Creatures/SpiderCreature.h"
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

	/** Den spiders spawned within this (s) of each other are one spawn's, taking turns out of the dens. */
	constexpr double DenSpawnTogether = 0.5;
}

AAmbushSpawner::AAmbushSpawner()
{
	// Sprung, they come at once.
	bHuntOnSpawn = true;
}

void AAmbushSpawner::BeginPlay()
{
	// Lurkers wait on their perches for the player to pass (each springs on its own: ASpiderCreature::BeginLurk), so they're
	// never set on the player as they're brought out, nor recalled as stragglers out of a fight they haven't joined.
	if (Entrance == EAmbushEntrance::Lurk)
	{
		bHuntOnSpawn = false;
	}
	Super::BeginPlay();
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
	case EAmbushEntrance::RiseAround:
		if (AUnpaidCreature* Unpaid = Cast<AUnpaidCreature>(&Creature))
		{
			// Out of the ground, staggered across the pack: the ground stirring over each first, then a hand, the haul and the
			// stand, each with its own sounds (UnpaidCreatureRise.cpp); where it stands, or round the player who sprang it.
			const UEncounterSubsystem* Encounters = GetEncounters();
			Unpaid->RiseAmbush(Encounters ? Encounters->GetPlayer() : nullptr, Entrance == EAmbushEntrance::RiseAround, RiseRadius);
		}
		else
		{
			// Not one of the dead: out of the ground its kind's way (a spider climbing out of the dirt), if it's seen.
			Creature.PlayEntranceIfSeen();
			LooterSound::PlayAt(this, RiseCue, Creature.GetActorLocation());
		}
		break;

	case EAmbushEntrance::Drop:
		Drop(Creature);
		break;

	case EAmbushEntrance::Lurk:
		SetOnPerch(Creature);
		break;

	case EAmbushEntrance::Den:
		SendIntoDen(Creature);
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

void AAmbushSpawner::SetOnPerch(ACreatureBase& Creature)
{
	ASpiderCreature* Spider = Cast<ASpiderCreature>(&Creature);
	if (!Spider || FSpiderLurkTuning::Live().Enabled == 0)
	{
		// Not a spider, or the lurk turned off (Looter.Spider.Lurk 0): it drops in and comes for the player at once.
		if (Spider)
		{
			Drop(Creature);
		}
		else
		{
			Creature.PlayEntranceIfSeen();
		}
		const UEncounterSubsystem* Encounters = GetEncounters();
		if (APawn* Player = Encounters ? Encounters->GetPlayer() : nullptr)
		{
			Creature.AlertTo(Player);
		}
		return;
	}
	// The nearest free perch to where it was spawned (its spot is the ground under its perch).
	PerchHolders.SetNum(Perches.Num());
	int32 Best = INDEX_NONE;
	double BestDistance = TNumericLimits<double>::Max();
	for (int32 Index = 0; Index < Perches.Num(); ++Index)
	{
		const ACreatureBase* Holder = PerchHolders[Index].Get();
		if (Holder && !Holder->IsDead() && Holder != &Creature)
		{
			continue;
		}
		const double Distance = FVector::DistSquared2D(Perches[Index].Spot, Creature.GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Index;
		}
	}
	if (Best == INDEX_NONE)
	{
		// More spiders than perches: it lurks crouched on the ground where it stands, as still.
		FSpiderPerchSpot Ground;
		Ground.Kind = ESpiderPerchKind::Ledge;
		Ground.Spot = Creature.GetActorLocation();
		Ground.FaceYaw = static_cast<float>(Creature.GetActorRotation().Yaw);
		Spider->BeginLurk(Ground, this);
		return;
	}
	PerchHolders[Best] = &Creature;
	Spider->BeginLurk(Perches[Best], this);
}

void AAmbushSpawner::SendIntoDen(ACreatureBase& Creature)
{
	ASpiderCreature* Spider = Cast<ASpiderCreature>(&Creature);
	const UWorld* World = GetWorld();
	if (!Spider || !World || Dens.IsEmpty() || FSpiderDenTuning::Live().Enabled == 0)
	{
		// Not a spider, no dens, or the dens turned off (Looter.Spider.Den 0): it comes its kind's way where it was spawned.
		Creature.PlayEntranceIfSeen();
		return;
	}
	// A wave's spiders are spawned together: each takes the next turn, round the dens, a little after the one before. One
	// spawned on its own later (owed, brought back) starts a turn afresh.
	const double Now = World->GetTimeSeconds();
	if (Now - LastDenSpawn > DenSpawnTogether)
	{
		DenTurn = 0;
	}
	LastDenSpawn = Now;
	const int32 Turn = DenTurn++;
	const FSpiderPerchSpot& Den = Dens[SpiderAmbushRules::DenFor(Turn, Dens.Num())];
	if (!Spider->BeginDenEmerge(Den, SpiderAmbushRules::DenDelay(Turn, FMath::FRand()), this))
	{
		// No wall at its den (a rebuilt wall, a den set wrong): it climbs out of the dirt on its floor spot instead.
		UE_LOG(LogLooter, Warning, TEXT("Ambush %s: no wall at den (%.0f, %.0f, %.0f) for %s: it appears on its floor spot."),
			*GetSpawnerId().ToString(), Den.Spot.X, Den.Spot.Y, Den.Spot.Z, *Spider->GetName());
		Creature.PlayEntranceIfSeen();
	}
}

void AAmbushSpawner::NoteLurkerSprang(const ASpiderCreature& Spider, APawn* Prey)
{
	const FSpiderLurkTuning Tuning = FSpiderLurkTuning::Live();
	for (ACreatureBase* Each : GetAliveCreatures())
	{
		ASpiderCreature* Mate = Cast<ASpiderCreature>(Each);
		if (!Mate || Mate == &Spider || Mate->GetLurkPhase() != ESpiderLurkPhase::Perched)
		{
			continue;
		}
		// Only those who can reach the player come now; the rest keep waiting for them to pass.
		if (Prey && FVector::Dist(Mate->GetActorLocation(), Prey->GetActorLocation()) > Tuning.MateReach * Mate->GetSizeScale())
		{
			continue;
		}
		Mate->NoteMateSprang(Prey, SpiderAmbushRules::MateDelay(FMath::FRand(), Tuning));
	}
}
