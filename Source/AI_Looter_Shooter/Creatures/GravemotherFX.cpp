// UGravemotherFX's setup and spawning (GravemotherFX.h): the art's cards and chunks, only near a viewer, capped. Their moving,
// drawing and FWeaponFX's stand-ins are in GravemotherFXDraw.cpp.

#include "Creatures/GravemotherFX.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Creatures/GravemotherAssets.h"
#include "World/WorldQueries.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** Each card carries these floats for its material: tint R, G, B, opacity (or glow), its cell of the sheet. */
	constexpr int32 CardFloats = 5;
	/** How far below a burst its chunks look for a floor (cm). */
	constexpr float FloorReach = 600.f;

	UInstancedStaticMeshComponent* MakeSet(AActor* Owner, const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Material, int32 Floats,
		bool bLit)
	{
		UInstancedStaticMeshComponent* Set = NewObject<UInstancedStaticMeshComponent>(Owner, Name, RF_Transient);
		Set->SetupAttachment(Owner->GetRootComponent());
		Set->SetMobility(EComponentMobility::Movable);
		Set->SetStaticMesh(Mesh);
		if (Material)
		{
			Set->SetMaterial(0, Material);
		}
		Set->SetNumCustomDataFloats(Floats);
		Set->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Set->SetCastShadow(false);
		Set->SetCanEverAffectNavigation(false);
		Set->bReceivesDecals = bLit;
		Set->RegisterComponent();
		return Set;
	}
}

UGravemotherFX* UGravemotherFX::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UGravemotherFX>() : nullptr;
}

bool UGravemotherFX::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor preview worlds too, for the tests (nothing spawns there unless a test asks).
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UGravemotherFX::Deinitialize()
{
	Cards.Reset();
	Chunks_.Reset();
	const UWorld* World = GetWorld();
	if (AActor* Actor = Owner.Get(); Actor && World && !World->bIsTearingDown)
	{
		Actor->Destroy();
	}
	Owner.Reset();
	Super::Deinitialize();
}

void UGravemotherFX::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!Cards.IsEmpty() || !Chunks_.IsEmpty() || bDrawnAnything)
	{
		Advance(DeltaTime);
	}
}

bool UGravemotherFX::EnsureDrawing()
{
	if (Owner.IsValid())
	{
		return true;
	}
	UWorld* World = GetWorld();
	if (bDrawingTried && !Owner.IsValid() && !World)
	{
		return false;
	}
	bDrawingTried = true;
	if (!World)
	{
		return false;
	}
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!Actor)
	{
		return false;
	}
#if WITH_EDITOR
	Actor->SetActorLabel(TEXT("GravemotherFX"));
#endif
	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"), RF_Transient);
	Root->SetMobility(EComponentMobility::Movable);
	Actor->SetRootComponent(Root);
	Root->RegisterComponent();
	Owner = Actor;

	// The cards: one set per material, on the engine plane (100 x 100, facing +Z). A set whose material isn't imported is
	// left empty: its kind uses FWeaponFX's stand-in.
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	const TCHAR* const CardPaths[] = { GravemotherAssets::CardGraveDust, GravemotherAssets::CardFloorDust, GravemotherAssets::CardDustJet,
		GravemotherAssets::CardStoneSpark, GravemotherAssets::CardGraveFlash };
	const TCHAR* const CardNames[] = { TEXT("GraveDust"), TEXT("FloorDust"), TEXT("DustJets"), TEXT("Sparks"), TEXT("Flashes") };
	for (int32 Index = 0; Index < static_cast<int32>(ECardSet::Count); ++Index)
	{
		UMaterialInterface* Material = GravemotherAssets::Load<UMaterialInterface>(CardPaths[Index]);
		CardSets[Index] = Material ? MakeSet(Actor, CardNames[Index], Plane, Material, CardFloats, false) : nullptr;
	}

	// The chunks: one set per mesh, lit, in their own materials.
	ChunkSets.Reset();
	auto AddChunkSet = [this, Actor](const TCHAR* Path, const FString& Name)
	{
		UStaticMesh* Mesh = GravemotherAssets::Load<UStaticMesh>(Path);
		ChunkSets.Add(Mesh ? MakeSet(Actor, *Name, Mesh, nullptr, 0, true) : nullptr);
	};
	for (int32 Index = 1; Index <= 5; ++Index)
	{
		AddChunkSet(GravemotherAssets::DebrisPath(Index), FString::Printf(TEXT("Debris%d"), Index));
	}
	for (int32 Index = 1; Index <= 3; ++Index)
	{
		AddChunkSet(GravemotherAssets::ChipPath(Index), FString::Printf(TEXT("Chip%d"), Index));
	}
	AddChunkSet(GravemotherAssets::ClodSmall, TEXT("Clods"));
	return true;
}

bool UGravemotherFX::ShouldSpawn(const FVector& Where) const
{
	if (bIgnoreViewers)
	{
		return true;
	}
	UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(this);
	return Reacts && Reacts->IsNearAViewer(Where, SeenWithin);
}

bool UGravemotherFX::HasCards(ECardSet Set) const
{
	return !bForceFallback && CardSets[static_cast<int32>(Set)].IsValid();
}

bool UGravemotherFX::HasChunks(EGraveChunk Kind) const
{
	if (bForceFallback)
	{
		return false;
	}
	const int32 First = Kind == EGraveChunk::Debris ? 0 : (Kind == EGraveChunk::StoneChip ? 5 : 8);
	const int32 Count = Kind == EGraveChunk::Debris ? 5 : (Kind == EGraveChunk::StoneChip ? 3 : 1);
	for (int32 Index = First; Index < First + Count; ++Index)
	{
		if (ChunkSets.IsValidIndex(Index) && ChunkSets[Index].IsValid())
		{
			return true;
		}
	}
	return false;
}

UGravemotherFX::FCard& UGravemotherFX::AddCard(ECardSet Set)
{
	if (Cards.Num() >= MaxCards)
	{
		// The oldest is nearly gone anyway.
		Cards.RemoveAt(0, 1, EAllowShrinking::No);
	}
	FCard& Card = Cards.AddDefaulted_GetRef();
	Card.Set = Set;
	Card.Roll = Random.FRandRange(0.f, 2.f * UE_PI);
	return Card;
}

FLinearColor UGravemotherFX::DustTint(EGraveDust Kind)
{
	// The sheets are painted in their own dust's colour: the stone's dust is the earth's sheet lightened to the granite's.
	if (Kind != EGraveDust::Stone)
	{
		return FLinearColor::White;
	}
	const FLinearColor Earth = GravemotherAssets::EarthDustColor();
	const FLinearColor Stone = GravemotherAssets::StoneDustColor();
	return FLinearColor(Stone.R / FMath::Max(Earth.R, 0.01f), Stone.G / FMath::Max(Earth.G, 0.01f), Stone.B / FMath::Max(Earth.B, 0.01f));
}

float UGravemotherFX::FloorUnder(const FVector& At) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return -1.0e9f;
	}
	FHitResult Hit;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("GravemotherFX"));
	if (World->LineTraceSingleByObjectType(Hit, At + FVector(0.0, 0.0, 40.0), At - FVector(0.0, 0.0, FloorReach),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		return static_cast<float>(Hit.ImpactPoint.Z);
	}
	return -1.0e9f;
}

// ---------------------------------------------------------------------------
// Spawning
// ---------------------------------------------------------------------------

void UGravemotherFX::Puffs(const FVector& Center, const FGraveDustBurst& Burst)
{
	if (Burst.Count <= 0 || !ShouldSpawn(Center))
	{
		return;
	}
	EnsureDrawing();
	const ECardSet Set = Burst.Kind == EGraveDust::Floor ? ECardSet::Floor : ECardSet::Dust;
	if (!HasCards(Set))
	{
		FallbackPuffs(Center, Burst);
		return;
	}
	const FLinearColor Tint = DustTint(Burst.Kind);
	for (int32 Index = 0; Index < Burst.Count; ++Index)
	{
		const FVector Out = FVector(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f), 0.f).GetSafeNormal();
		FCard& Card = AddCard(Set);
		Card.Location = Center + Out * Random.FRandRange(0.f, Burst.Spread) + FVector(0.0, 0.0, Random.FRandRange(0.f, Burst.SpreadUp));
		Card.Velocity = Burst.Drift * Random.FRandRange(0.7f, 1.2f) + (Out + FVector(0.0, 0.0, 0.25)) * (Burst.Outward * Random.FRandRange(0.5f, 1.1f));
		Card.Life = Burst.Life * Random.FRandRange(0.8f, 1.25f);
		Card.StartSize = Burst.StartSize * Random.FRandRange(0.8f, 1.2f);
		Card.EndSize = Burst.EndSize * Random.FRandRange(0.8f, 1.2f);
		Card.Opacity = Burst.Opacity * Random.FRandRange(0.85f, 1.1f);
		Card.Cell = static_cast<float>(Random.RandRange(0, 3));
		Card.Fall = Burst.Fall;
		Card.Drag = 1.6f;
		Card.Tint = Tint;
	}
}

void UGravemotherFX::Jet(const FVector& Base, float Width, float Height, float Life)
{
	if (!ShouldSpawn(Base))
	{
		return;
	}
	EnsureDrawing();
	if (!HasCards(ECardSet::Jet))
	{
		// A stand-in: a short column of rising puffs.
		FGraveDustBurst Column;
		Column.Count = 3;
		Column.Spread = Width * 0.2f;
		Column.SpreadUp = Height * 0.4f;
		Column.StartSize = Width * 0.8f;
		Column.EndSize = Width * 1.8f;
		Column.Life = Life;
		Column.Opacity = 0.3f;
		Column.Drift = FVector(0.0, 0.0, Height * 1.2f);
		Column.Outward = 10.f;
		FallbackPuffs(Base, Column);
		return;
	}
	FCard& Card = AddCard(ECardSet::Jet);
	Card.Location = Base;
	Card.Velocity = FVector(0.0, 0.0, Height * 0.35f);
	Card.Life = Life;
	Card.StartSize = Width * 0.7f;
	Card.EndSize = Width * 1.3f;
	Card.Length = Height;
	Card.Opacity = 0.8f;
	Card.Cell = static_cast<float>(Random.RandRange(0, 1));
	Card.Drag = 2.f;
	Card.Roll = 0.f;
}

void UGravemotherFX::Sparks(const FVector& At, const FVector& Normal, const FVector& ShotDirection, int32 Count)
{
	if (Count <= 0 || !ShouldSpawn(At))
	{
		return;
	}
	EnsureDrawing();
	const FVector Up = Normal.IsNearlyZero() ? -ShotDirection.GetSafeNormal() : Normal.GetSafeNormal();
	const FVector Bounce = ShotDirection.GetSafeNormal().MirrorByVector(Up);
	if (!HasCards(ECardSet::Spark))
	{
		FallbackSparks(At, Up, ShotDirection);
		return;
	}
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FCard& Card = AddCard(ECardSet::Spark);
		Card.Location = At + Up * 2.f;
		Card.Velocity = (Bounce * 0.6f + Up * 0.5f + Random.GetUnitVector() * 0.45f).GetSafeNormal() * Random.FRandRange(500.f, 1100.f);
		Card.Life = Random.FRandRange(0.12f, 0.26f);
		Card.StartSize = Random.FRandRange(1.f, 5.f);
		Card.EndSize = Card.StartSize * 0.5f;
		Card.Length = Random.FRandRange(20.f, 55.f);
		Card.Opacity = Random.FRandRange(6.f, 12.f);
		Card.Fall = 0.6f;
		Card.Drag = 3.f;
	}
}

void UGravemotherFX::Flash(const FVector& At, float Size)
{
	if (!ShouldSpawn(At))
	{
		return;
	}
	EnsureDrawing();
	if (!HasCards(ECardSet::Flash))
	{
		return;
	}
	FCard& Card = AddCard(ECardSet::Flash);
	Card.Location = At;
	Card.Life = 0.09f;
	Card.StartSize = Size * 0.6f;
	Card.EndSize = Size;
	Card.Opacity = 8.f;
	Card.Drag = 0.f;
}

void UGravemotherFX::Chunks(EGraveChunk Kind, const FVector& At, const FVector& Up, int32 Count, float Speed, float Size)
{
	if (Count <= 0 || !ShouldSpawn(At))
	{
		return;
	}
	EnsureDrawing();
	if (!HasChunks(Kind))
	{
		FallbackChunks(Kind, At, Up, Count, Speed, Size);
		return;
	}
	const int32 First = Kind == EGraveChunk::Debris ? 0 : (Kind == EGraveChunk::StoneChip ? 5 : 8);
	const int32 Meshes = Kind == EGraveChunk::Debris ? 5 : (Kind == EGraveChunk::StoneChip ? 3 : 1);
	const float Floor = FloorUnder(At);
	const FVector Lift = Up.IsNearlyZero() ? FVector::UpVector : Up.GetSafeNormal();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		// A mesh of its kind that's imported (a missing one's turn goes to the next).
		int32 Mesh = First + Random.RandRange(0, Meshes - 1);
		for (int32 Try = 0; Try < Meshes && !(ChunkSets.IsValidIndex(Mesh) && ChunkSets[Mesh].IsValid()); ++Try)
		{
			Mesh = First + (Mesh - First + 1) % Meshes;
		}
		if (Chunks_.Num() >= MaxChunks)
		{
			Chunks_.RemoveAt(0, 1, EAllowShrinking::No);
		}
		FChunk& Chunk = Chunks_.AddDefaulted_GetRef();
		Chunk.Kind = Kind;
		Chunk.Mesh = Mesh;
		Chunk.Location = At + Random.GetUnitVector() * 8.f;
		const FVector Out = FVector(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f), 0.f).GetSafeNormal();
		Chunk.Velocity = (Lift * Random.FRandRange(0.6f, 1.2f) + Out * Random.FRandRange(0.3f, 0.9f)).GetSafeNormal() * Speed * Random.FRandRange(0.6f, 1.15f);
		Chunk.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
		Chunk.Spin = Random.GetUnitVector() * Random.FRandRange(4.f, 12.f);
		Chunk.Size = Size * Random.FRandRange(0.7f, 1.15f);
		Chunk.Life = Random.FRandRange(1.8f, 3.2f);
		Chunk.FloorZ = Floor;
	}
}
