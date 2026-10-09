// UBreakableDebrisSubsystem: a broken prop's pieces thrown, bouncing, lying and shrinking away, drawn by a pool of
// static mesh components.

#include "World/BreakableDebris.h"
#include "World/BreakableKinds.h"
#include "World/WorldQueries.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	/** A little more than the world's: a short throw still reads as a throw. */
	constexpr float Gravity = 1250.f;
	/** Past this a piece lies wherever it is (one that fell off an edge and never met its ground). */
	constexpr float MostFlight = 5.f;
	/** How far over and under its start the ground is looked for (cm). */
	constexpr double GroundAbove = 40.0;
	constexpr double GroundBelow = 500.0;

	/** A ground steeper than this (its normal's Z) is taken as flat at the point found: a wall's foot, a rock's side. */
	constexpr double SteepestGround = 0.5;
}

double UBreakableDebrisSubsystem::FPiece::GroundZ() const
{
	// The plane through the point found under its start, facing its normal: a slope stays a slope where it lands.
	if (GroundNormal.Z < SteepestGround)
	{
		return GroundPoint.Z;
	}
	return GroundPoint.Z - (GroundNormal.X * (Middle.X - GroundPoint.X) + GroundNormal.Y * (Middle.Y - GroundPoint.Y)) / GroundNormal.Z;
}

UBreakableDebrisSubsystem* UBreakableDebrisSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UBreakableDebrisSubsystem>() : nullptr;
}

bool UBreakableDebrisSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the automated tests.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UBreakableDebrisSubsystem::Deinitialize()
{
	Pieces.Reset();
	Free.Reset();
	Pool.Reset();
	if (IsValid(PoolActor))
	{
		PoolActor->Destroy();
	}
	PoolActor = nullptr;
	Super::Deinitialize();
}

void UBreakableDebrisSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Advance(DeltaTime);
}

bool UBreakableDebrisSubsystem::IsTickable() const
{
	return !Pieces.IsEmpty();
}

int32 UBreakableDebrisSubsystem::NumLying() const
{
	int32 Lying = 0;
	for (const FPiece& Piece : Pieces)
	{
		Lying += Piece.bLying ? 1 : 0;
	}
	return Lying;
}

// ---------------------------------------------------------------------------
// Throwing
// ---------------------------------------------------------------------------

bool UBreakableDebrisSubsystem::Throw(UStaticMesh* Mesh, const FTransform& From, const FVector& Velocity, const FVector& Spin, float Life,
	const AActor* Ignored)
{
	UWorld* World = GetWorld();
	if (!Mesh || !World)
	{
		return false;
	}
	UStaticMeshComponent* Component = TakeComponent();
	if (!Component)
	{
		return false;
	}
	FPiece& Piece = Pieces.AddDefaulted_GetRef();
	Piece.Mesh = Component;
	// Modeled in place: its middle is where it sat in the prop, and it turns about that.
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	Piece.LocalMiddle = Bounds.Origin;
	Piece.Rotation = From.GetRotation();
	Piece.Scale = From.GetScale3D();
	Piece.Middle = From.GetLocation() + Piece.Rotation.RotateVector(Piece.Scale * Piece.LocalMiddle);
	const FVector Extent = Bounds.BoxExtent * Piece.Scale.GetAbs();
	Piece.Rest = FMath::Max(static_cast<float>(Extent.GetMin()), 1.f);
	Piece.Velocity = Velocity;
	Piece.Spin = Spin;
	Piece.Life = FMath::Max(Life, 0.f);
	Piece.BouncesLeft = LooterBreakables::Bounces;
	// The prop stood on the ground at its pivot: that's the fallback when no ground is found under the piece.
	FindGround(Piece.Middle, From.GetLocation().Z, Ignored, Piece.GroundPoint, Piece.GroundNormal);

	Component->SetStaticMesh(Mesh);
	Component->SetVisibility(true);
	Pose(Piece, 1.f);
	return true;
}

void UBreakableDebrisSubsystem::FindGround(const FVector& Middle, double Fallback, const AActor* Ignored, FVector& OutPoint,
	FVector& OutNormal) const
{
	OutPoint = FVector(Middle.X, Middle.Y, Fallback);
	OutNormal = FVector::UpVector;
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("BreakableDebris"), Ignored, false);
	FHitResult Hit;
	if (World->LineTraceSingleByObjectType(Hit, Middle + FVector(0.0, 0.0, GroundAbove), Middle - FVector(0.0, 0.0, GroundBelow),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		OutPoint = Hit.ImpactPoint;
		OutNormal = Hit.ImpactNormal;
	}
}

UStaticMeshComponent* UBreakableDebrisSubsystem::TakeComponent()
{
	while (!Free.IsEmpty())
	{
		if (UStaticMeshComponent* Component = Free.Pop())
		{
			return Component;
		}
	}
	UWorld* World = GetWorld();
	if (Pool.Num() < MaxPieces && World)
	{
		if (!IsValid(PoolActor))
		{
			FActorSpawnParameters Spawn;
			Spawn.ObjectFlags |= RF_Transient;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			PoolActor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Spawn);
			if (!PoolActor)
			{
				return nullptr;
			}
			USceneComponent* Root = NewObject<USceneComponent>(PoolActor, TEXT("Root"));
			Root->SetMobility(EComponentMobility::Movable);
			PoolActor->SetRootComponent(Root);
			Root->RegisterComponent();
		}
		UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(PoolActor);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetCanEverAffectNavigation(false);
		Component->bReceivesDecals = false;
		// Placed in the world by each piece's own transform, never by the pool actor's.
		Component->SetUsingAbsoluteLocation(true);
		Component->SetUsingAbsoluteRotation(true);
		Component->SetUsingAbsoluteScale(true);
		Component->SetupAttachment(PoolActor->GetRootComponent());
		Component->RegisterComponent();
		PoolActor->AddInstanceComponent(Component);
		Pool.Add(Component);
		return Component;
	}
	// The pool is full: the piece that has lain longest gives up its component (else the oldest of all).
	int32 Oldest = INDEX_NONE;
	for (int32 Index = 0; Index < Pieces.Num(); ++Index)
	{
		const FPiece& Piece = Pieces[Index];
		if (Oldest == INDEX_NONE || (Piece.bLying && (!Pieces[Oldest].bLying || Piece.LyingFor > Pieces[Oldest].LyingFor))
			|| (!Piece.bLying && !Pieces[Oldest].bLying && Piece.Age > Pieces[Oldest].Age))
		{
			Oldest = Index;
		}
	}
	if (Oldest == INDEX_NONE)
	{
		return nullptr;
	}
	UStaticMeshComponent* Taken = Pieces[Oldest].Mesh.Get();
	Pieces.RemoveAtSwap(Oldest);
	return Taken;
}

// ---------------------------------------------------------------------------
// Flying, lying, shrinking
// ---------------------------------------------------------------------------

void UBreakableDebrisSubsystem::Advance(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f)
	{
		return;
	}
	for (int32 Index = Pieces.Num() - 1; Index >= 0; --Index)
	{
		FPiece& Piece = Pieces[Index];
		if (!Piece.Mesh.IsValid())
		{
			Pieces.RemoveAtSwap(Index);
			continue;
		}
		Piece.Age += DeltaSeconds;
		float Shrink = 1.f;
		if (!Piece.bLying)
		{
			Piece.Velocity.Z -= Gravity * DeltaSeconds;
			Piece.Middle += Piece.Velocity * DeltaSeconds;
			const float Turn = static_cast<float>(Piece.Spin.Size()) * DeltaSeconds;
			if (Turn > UE_KINDA_SMALL_NUMBER)
			{
				Piece.Rotation = (FQuat(Piece.Spin.GetSafeNormal(), Turn) * Piece.Rotation).GetNormalized();
			}
			const double Floor = Piece.GroundZ() + Piece.Rest;
			if (Piece.Middle.Z <= Floor && Piece.Velocity.Z < 0.f)
			{
				Piece.Middle.Z = Floor;
				if (Piece.BouncesLeft > 0)
				{
					// A knock off the ground: up a little, slower across, turning less.
					--Piece.BouncesLeft;
					Piece.Velocity.Z = -Piece.Velocity.Z * LooterBreakables::Bounce;
					Piece.Velocity.X *= LooterBreakables::BounceSlide;
					Piece.Velocity.Y *= LooterBreakables::BounceSlide;
					Piece.Spin *= 0.5f;
				}
				else
				{
					Piece.bLying = true;
				}
			}
			if (Piece.Age > MostFlight)
			{
				Piece.bLying = true;
			}
			if (Piece.bLying)
			{
				Piece.Velocity = FVector::ZeroVector;
				Piece.Spin = FVector::ZeroVector;
			}
		}
		else
		{
			Piece.LyingFor += DeltaSeconds;
			Shrink = 1.f - FMath::Clamp((Piece.LyingFor - Piece.Life) / LooterBreakables::ShrinkSeconds, 0.f, 1.f);
			if (Shrink <= 0.f)
			{
				Release(Piece);
				Pieces.RemoveAtSwap(Index);
				continue;
			}
			// Shrinking toward the ground, not into the air.
			Piece.Middle.Z = Piece.GroundZ() + Piece.Rest * Shrink;
		}
		Pose(Piece, Shrink);
	}
}

void UBreakableDebrisSubsystem::Pose(const FPiece& Piece, float Shrink)
{
	UStaticMeshComponent* Component = Piece.Mesh.Get();
	if (!Component)
	{
		return;
	}
	// Turned about its own middle: the mesh's origin (the prop's pivot) goes wherever that puts it.
	const FVector Scale = Piece.Scale * FMath::Max(Shrink, 0.01f);
	const FVector Origin = Piece.Middle - Piece.Rotation.RotateVector(Scale * Piece.LocalMiddle);
	Component->SetWorldTransform(FTransform(Piece.Rotation, Origin, Scale));
}

void UBreakableDebrisSubsystem::Release(FPiece& Piece)
{
	if (UStaticMeshComponent* Component = Piece.Mesh.Get())
	{
		Component->SetVisibility(false);
		Component->SetStaticMesh(nullptr);
		Free.Add(Component);
	}
	Piece.Mesh.Reset();
}
