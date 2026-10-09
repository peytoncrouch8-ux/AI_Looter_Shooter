#include "Creatures/PatrolSpawner.h"
#include "AI_Looter_Shooter.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreaturePackComponent.h"
#include "Creatures/EncounterGroup.h"

namespace
{
	/** A pace for a pack whose kinds can't be read (no class): about the Unpaid's stroll. */
	constexpr float FallbackWalkSpeed = 150.f;

	bool IsHunting(const ACreatureBase& Creature)
	{
		const ECreatureState Doing = Creature.GetCreatureState();
		return Doing == ECreatureState::Chase || Doing == ECreatureState::Attack;
	}
}

APatrolSpawner::APatrolSpawner()
{
	// They walk their road until someone comes near: no rush at the player as they appear.
	bHuntOnSpawn = false;
	// They appear together round the pack's point, not over a camp's ground.
	SpawnRadius = 400.f;
}

void APatrolSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (PatrolRoute.Num() < 2)
	{
		UE_LOG(LogLooter, Warning, TEXT("Patrol %s has %d route points: it stands where it is (it needs two or more)."), *GetSpawnerId().ToString(),
			PatrolRoute.Num());
	}
}

TArray<FVector> APatrolSpawner::GetWorldRoute() const
{
	TArray<FVector> World;
	const FTransform& Frame = GetActorTransform();
	for (const FVector& Local : PatrolRoute)
	{
		World.Add(Frame.TransformPositionNoScale(Local));
	}
	return World;
}

FVector APatrolSpawner::GetPatrolPoint() const
{
	const TArray<FVector> World = GetWorldRoute();
	return World.IsEmpty() ? GetActorLocation() : PackRules::PointAlong(World, bLoop, Walk.Along);
}

FVector APatrolSpawner::GetSpawnCenter() const
{
	return GetPatrolPoint();
}

float APatrolSpawner::GetPace() const
{
	float Slowest = TNumericLimits<float>::Max();
	for (const ACreatureBase* Creature : GetAliveCreatures())
	{
		Slowest = FMath::Min(Slowest, Creature->WalkSpeed);
	}
	if (Slowest == TNumericLimits<float>::Max())
	{
		// None out: the point walks on at the pace its kinds would keep.
		for (const FEncounterGroup& Group : Groups)
		{
			if (Group.CreatureClass)
			{
				Slowest = FMath::Min(Slowest, Group.CreatureClass->GetDefaultObject<ACreatureBase>()->WalkSpeed);
			}
		}
	}
	if (Slowest == TNumericLimits<float>::Max())
	{
		Slowest = FallbackWalkSpeed;
	}
	return Slowest * PaceShare;
}

void APatrolSpawner::UpdatePack(float DeltaSeconds)
{
	const TArray<FVector> WorldRoute = GetWorldRoute();
	if (WorldRoute.Num() < 2)
	{
		bWalking = false;
		return;
	}
	// A fight holds the walk, and so does a straggler (one walking back from a chase, or held up on the way).
	bool bHold = false;
	const double CatchUpSquared = FMath::Square(static_cast<double>(CatchUpDistance));
	for (const ACreatureBase* Creature : GetAliveCreatures())
	{
		const UCreaturePackComponent* Pack = Creature->GetPack();
		if (IsHunting(*Creature)
			|| (Pack && Pack->HasRoamAnchor() && FVector::DistSquared2D(Creature->GetActorLocation(), Pack->GetRoamAnchor()) > CatchUpSquared))
		{
			bHold = true;
			break;
		}
	}
	bWalking = !bHold && PackRules::AdvancePatrol(WorldRoute, bLoop, GetPace(), DeltaSeconds, PauseSeconds, Walk);
	PlaceFile(WorldRoute);
}

void APatrolSpawner::PlaceFile(const TArray<FVector>& WorldRoute)
{
	TArray<ACreatureBase*> Members = GetAliveCreatures();
	// The strongest leads (its rank shows first), then the rest in the order they came, the same order at every look.
	Members.StableSort([](const ACreatureBase& A, const ACreatureBase& B)
	{
		if (A.GetRank() != B.GetRank())
		{
			return static_cast<uint8>(A.GetRank()) > static_cast<uint8>(B.GetRank());
		}
		return A.GetUniqueID() < B.GetUniqueID();
	});
	const FVector Point = PackRules::PointAlong(WorldRoute, bLoop, Walk.Along);
	const FVector Heading = PackRules::PatrolHeading(WorldRoute, bLoop, Walk);
	for (int32 Index = 0; Index < Members.Num(); ++Index)
	{
		if (UCreaturePackComponent* Pack = Members[Index]->GetPack())
		{
			Pack->SetRoamAnchor(PackRules::FormationSpot(Point, Heading, Index, FileSpacing), bWalking);
		}
	}
}
