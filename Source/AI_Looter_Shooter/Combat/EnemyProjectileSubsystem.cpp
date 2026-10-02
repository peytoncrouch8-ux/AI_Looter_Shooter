#include "Combat/EnemyProjectileSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Combat/CombatRules.h"
#include "Combat/EnemyShotDamageType.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "World/PlayableArea.h"
#include "Components/BrushComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Volume.h"
#include "Kismet/GameplayStatics.h"

// UEnemyProjectileSubsystem: firing, the rules of who a pellet hurts, and its flight. Drawing is in
// EnemyProjectileSubsystemDraw.cpp.

bool UEnemyProjectileSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests (nothing fires there otherwise).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UEnemyProjectileSubsystem::Deinitialize()
{
	Pellets.Reset();
	Charges.Reset();
	Pops.Reset();
	const UWorld* World = GetWorld();
	AActor* Actor = DrawActor.Get();
	if (Actor && (!World || !World->bIsTearingDown))
	{
		Actor->Destroy();
	}
	DrawActor.Reset();
	Instances.Reset();
	DrawnColors.Reset();
	Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Firing
// ---------------------------------------------------------------------------

void UEnemyProjectileSubsystem::Fire(const FEnemyShot& Shot)
{
	const FVector Direction = Shot.Direction.GetSafeNormal();
	if (Direction.IsNearlyZero() || Shot.Speed <= 0.f || Shot.Range <= 0.f)
	{
		return;
	}
	// Over the cap the oldest one still flying goes out (marked, not removed: this may be called from a hit's handlers).
	if (NumShotsInFlight() >= MaxShots)
	{
		for (FPellet& Oldest : Pellets)
		{
			if (!Oldest.bSpent)
			{
				Oldest.bSpent = true;
				break;
			}
		}
	}
	FPellet& Pellet = Pellets.AddDefaulted_GetRef();
	Pellet.Shot = Shot;
	Pellet.Shot.Direction = Direction;
	Pellet.Shot.Radius = FMath::Max(Shot.Radius, 1.f);
	Pellet.Position = Shot.Start;
}

void UEnemyProjectileSubsystem::FireVolley(const FEnemyShot& Pellet, const FVector& Aim, int32 Count, float SpreadDegrees)
{
	for (const FVector& Direction : VolleyDirections(Aim, Count, SpreadDegrees))
	{
		FEnemyShot Shot = Pellet;
		Shot.Direction = Direction;
		Fire(Shot);
	}
}

void UEnemyProjectileSubsystem::ShowCharge(AActor* Shooter, const FVector& LocalOffset, float Seconds, float Radius, const FLinearColor& Color)
{
	if (!Shooter || Seconds <= 0.f)
	{
		return;
	}
	FCharge& Charge = Charges.AddDefaulted_GetRef();
	Charge.Shooter = Shooter;
	Charge.Offset = LocalOffset;
	Charge.Life = Seconds;
	Charge.Radius = FMath::Max(Radius, 1.f);
	Charge.Color = Color;
}

void UEnemyProjectileSubsystem::ClearShotsFrom(const AActor* Shooter)
{
	if (!Shooter)
	{
		return;
	}
	for (FPellet& Pellet : Pellets)
	{
		if (Pellet.Shot.Shooter.Get() == Shooter)
		{
			Pellet.bSpent = true;
		}
	}
	for (FCharge& Charge : Charges)
	{
		if (Charge.Shooter.Get() == Shooter)
		{
			Charge.Age = Charge.Life;
		}
	}
}

int32 UEnemyProjectileSubsystem::NumShotsInFlight() const
{
	int32 Count = 0;
	for (const FPellet& Pellet : Pellets)
	{
		Count += Pellet.bSpent ? 0 : 1;
	}
	return Count;
}

int32 UEnemyProjectileSubsystem::NumShotsFrom(const AActor* Shooter) const
{
	int32 Count = 0;
	for (const FPellet& Pellet : Pellets)
	{
		Count += !Pellet.bSpent && Pellet.Shot.Shooter.Get() == Shooter ? 1 : 0;
	}
	return Count;
}

// ---------------------------------------------------------------------------
// Rules
// ---------------------------------------------------------------------------

bool UEnemyProjectileSubsystem::CanHurt(const AActor* Shooter, const AActor* Victim)
{
	// The same "who can be hunted" rule creatures use: living characters, never creatures (so never the shooter's adds).
	if (!Victim || Victim == Shooter || !Victim->IsA<ACharacter>() || Victim->IsA<ACreatureBase>())
	{
		return false;
	}
	const UHealthComponent* Health = Victim->FindComponentByClass<UHealthComponent>();
	return Health && !Health->IsDead();
}

bool UEnemyProjectileSubsystem::SweepHitsCapsule(const FVector& From, const FVector& To, float Radius, const FVector& CapsuleCenter,
	float CapsuleRadius, float CapsuleHalfHeight, float& OutTime)
{
	// A capsule is the set of points within its radius of its middle segment; a moving ball touches it when its path
	// comes within the two radii of that segment.
	const float Core = FMath::Max(CapsuleHalfHeight - CapsuleRadius, 0.f);
	const FVector Bottom = CapsuleCenter - FVector(0.0, 0.0, Core);
	const FVector Top = CapsuleCenter + FVector(0.0, 0.0, Core);
	FVector OnPath;
	FVector OnCapsule;
	FMath::SegmentDistToSegmentSafe(From, To, Bottom, Top, OnPath, OnCapsule);
	const double Reach = static_cast<double>(Radius) + CapsuleRadius;
	if (FVector::DistSquared(OnPath, OnCapsule) > Reach * Reach)
	{
		return false;
	}
	const double Length = FVector::Dist(From, To);
	OutTime = Length > UE_KINDA_SMALL_NUMBER ? static_cast<float>(FVector::Dist(From, OnPath) / Length) : 0.f;
	return true;
}

TArray<FVector> UEnemyProjectileSubsystem::VolleyDirections(const FVector& Aim, int32 Count, float SpreadDegrees)
{
	TArray<FVector> Directions;
	const FVector Middle = Aim.GetSafeNormal();
	if (Count <= 0 || Middle.IsNearlyZero())
	{
		return Directions;
	}
	Directions.Add(Middle);
	if (Count == 1)
	{
		return Directions;
	}
	// Tilt the middle line up by half the cone (about the axis across it), then turn that copy round the middle line:
	// starting straight above it, so a five-pellet volley is a plus sign the player can read and step through.
	const FVector Across = FMath::Abs(Middle.Z) < 0.9 ? FVector::CrossProduct(Middle, FVector::UpVector).GetSafeNormal()
		: FVector::CrossProduct(Middle, FVector::ForwardVector).GetSafeNormal();
	const FVector Edge = Middle.RotateAngleAxis(SpreadDegrees * 0.5f, Across);
	const int32 Around = Count - 1;
	for (int32 Index = 0; Index < Around; ++Index)
	{
		Directions.Add(Edge.RotateAngleAxis(360.f * static_cast<float>(Index) / static_cast<float>(Around), Middle).GetSafeNormal());
	}
	return Directions;
}

// ---------------------------------------------------------------------------
// Flight
// ---------------------------------------------------------------------------

void UEnemyProjectileSubsystem::Tick(float DeltaTime)
{
	// Nothing flying and nothing left on screen: most of the game, at no cost.
	if (Pellets.IsEmpty() && Charges.IsEmpty() && Pops.IsEmpty() && DrawnColors.IsEmpty())
	{
		return;
	}
	// Last update's spent pellets and finished tells and pops go now (never during an update: a hit's handlers may fire or
	// put out pellets while the list is being walked).
	Pellets.RemoveAll([](const FPellet& Pellet) { return Pellet.bSpent; });
	for (FCharge& Charge : Charges)
	{
		Charge.Age += DeltaTime;
	}
	Charges.RemoveAll([](const FCharge& Charge) { return Charge.Age >= Charge.Life || !Charge.Shooter.IsValid(); });
	for (FPop& Pop : Pops)
	{
		Pop.Age += DeltaTime;
	}
	Pops.RemoveAll([](const FPop& Pop) { return Pop.Age >= PopSeconds; });

	if (!Pellets.IsEmpty())
	{
		const TArray<TWeakObjectPtr<ACharacter>> Victims = GatherVictims();
		const FCollisionQueryParams WorldQuery = MakeWorldQuery();
		// Only the pellets there were as the update began: one fired from a hit's handlers starts flying next update.
		const int32 Count = Pellets.Num();
		for (int32 Index = 0; Index < Count && Index < Pellets.Num(); ++Index)
		{
			if (!Pellets[Index].bSpent)
			{
				Advance(Index, DeltaTime, Victims, WorldQuery);
			}
		}
	}

	Draw();
}

void UEnemyProjectileSubsystem::Advance(int32 Index, float DeltaSeconds, const TArray<TWeakObjectPtr<ACharacter>>& Victims,
	const FCollisionQueryParams& WorldQuery)
{
	UWorld* World = GetWorld();
	// A copy: hurting the player below can run code that adds or puts out pellets.
	const FEnemyShot Shot = Pellets[Index].Shot;
	const FVector From = Pellets[Index].Position;
	const float Step = FMath::Min(Shot.Speed * DeltaSeconds, Shot.Range - Pellets[Index].Traveled);
	if (!World || Step <= 0.f)
	{
		Pellets[Index].bSpent = true;
		return;
	}
	const FVector To = From + Shot.Direction * Step;

	// The world stops it: terrain, rocks and buildings (world-static), never creatures, volumes or walls that only stop pawns.
	FHitResult WorldHit;
	const bool bHitWorld = World->LineTraceSingleByObjectType(WorldHit, From, To, FCollisionObjectQueryParams(ECC_WorldStatic), WorldQuery);
	const float WorldTime = bHitWorld ? WorldHit.Time : 1.f;

	// The player: its path, as far as the world lets it go, passing within its radius of their capsule. The nearest wins.
	const FVector Reach = FMath::Lerp(From, To, WorldTime);
	ACharacter* Victim = nullptr;
	float VictimTime = 1.f;
	for (const TWeakObjectPtr<ACharacter>& Candidate : Victims)
	{
		ACharacter* Character = Candidate.Get();
		const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
		if (!Capsule || !CanHurt(Shot.Shooter.Get(), Character))
		{
			continue;
		}
		float Time = 1.f;
		if (SweepHitsCapsule(From, Reach, Shot.Radius, Capsule->GetComponentLocation(), Capsule->GetScaledCapsuleRadius(),
			Capsule->GetScaledCapsuleHalfHeight(), Time) && (!Victim || Time < VictimTime))
		{
			Victim = Character;
			VictimTime = Time;
		}
	}

	FPellet& Pellet = Pellets[Index];
	if (Victim)
	{
		Pellet.Position = FMath::Lerp(From, Reach, VictimTime);
		Pellet.bSpent = true;
		// Rolled like a bite (LooterCombat's spread), blamed on the shooter. Last: nothing below touches the list.
		const float Damage = LooterCombat::RollHitDamage(Shot.Damage, false);
		UGameplayStatics::ApplyDamage(Victim, Damage, Shot.Instigator.Get(), Shot.Shooter.Get(), UEnemyShotDamageType::StaticClass());
		UE_LOG(LogLooter, Verbose, TEXT("%s's pellet hit %s for %.1f"), *GetNameSafe(Shot.Shooter.Get()), *GetNameSafe(Victim), Damage);
		OnShotHit.Broadcast(Victim, Shot.Shooter.Get(), Damage);
		return;
	}
	if (bHitWorld)
	{
		Pellet.Position = WorldHit.Location;
		Pellet.bSpent = true;
		FPop& Pop = Pops.AddDefaulted_GetRef();
		Pop.Location = WorldHit.Location;
		Pop.Radius = Shot.Radius;
		Pop.Color = Shot.Color;
		return;
	}
	Pellet.Position = To;
	Pellet.Traveled += Step;
	Pellet.bSpent = Pellet.Traveled >= Shot.Range - UE_KINDA_SMALL_NUMBER;
}

TArray<TWeakObjectPtr<ACharacter>> UEnemyProjectileSubsystem::GatherVictims() const
{
	TArray<TWeakObjectPtr<ACharacter>> Victims;
	for (TActorIterator<ACharacter> It(GetWorld()); It; ++It)
	{
		// Who fired doesn't matter here: no creature is ever hurt by a pellet (CanHurt checks each pellet's shooter).
		if (CanHurt(nullptr, *It))
		{
			Victims.Add(*It);
		}
	}
	return Victims;
}

FCollisionQueryParams UEnemyProjectileSubsystem::MakeWorldQuery() const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyShot), /*bTraceComplex*/ false);
	const UWorld* World = GetWorld();
	if (!World)
	{
		return Params;
	}
	// A volume is an invisible box that still answers world-static queries (the meadow's PCG volume is the first thing a
	// trace across the island meets). Only its box is skipped: the trees and rocks it scattered still stop pellets.
	for (TActorIterator<AVolume> It(World); It; ++It)
	{
		if (const UBrushComponent* Brush = It->GetBrushComponent())
		{
			Params.AddIgnoredComponent(Brush);
		}
	}
	// The playable area's invisible walls only stop walking pawns.
	for (TActorIterator<APlayableArea> It(World); It; ++It)
	{
		Params.AddIgnoredActor(*It);
	}
	return Params;
}
