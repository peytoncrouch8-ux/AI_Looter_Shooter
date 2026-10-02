#include "Bosses/BossSeal.h"
#include "World/WorldQueries.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** The curtain's dashes stand this far apart along the line (cm): dense enough to read as a wall, sparse enough to see through. */
	constexpr float DashSpacing = 24.f;
	/** How brightly the curtain glows (M_StylizedSurface's emissive multiplier of its color). */
	constexpr float CurtainGlow = 3.f;
	/** Seconds to grow up out of the ground, and to sink back. */
	constexpr float RiseSeconds = 0.6f;
	constexpr float SinkSeconds = 0.45f;
	/** The seam along the ground: a flat glowing strip this wide and tall (cm). */
	constexpr float SeamWidth = 8.f;
	constexpr float SeamHeight = 3.f;
	/** Spans overlap by this much (cm) past their ends, so no gap opens at a ring's corners for a capsule to squeeze through. */
	constexpr float WallOverlap = 30.f;

	/** Replaces a component's instances with these (moved in place when the count is unchanged). */
	void DrawInstances(UInstancedStaticMeshComponent* Component, const TArray<FTransform>& Transforms)
	{
		if (!Component)
		{
			return;
		}
		if (Component->GetInstanceCount() != Transforms.Num())
		{
			Component->ClearInstances();
			if (Transforms.Num() > 0)
			{
				Component->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
			}
		}
		else if (Transforms.Num() > 0)
		{
			Component->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		}
		Component->MarkRenderStateDirty();
	}

	UInstancedStaticMeshComponent* MakeGlowInstances(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh)
	{
		UInstancedStaticMeshComponent* Component = Owner->CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(Parent);
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetStaticMesh(Mesh);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(false);
		Component->SetCanEverAffectNavigation(false);
		Component->bReceivesDecals = false;
		return Component;
	}
}

ABossSeal::ABossSeal()
{
	PrimaryActorTick.bCanEverTick = true;
	// It only ticks while the curtain stands or moves.
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Surface(TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface"));
	Dashes = MakeGlowInstances(this, Root, TEXT("Dashes"), Cube.Object);
	Seam = MakeGlowInstances(this, Root, TEXT("Seam"), Cube.Object);
	GlowBase = Surface.Object;
}

ABossSeal* ABossSeal::SpawnRing(UWorld* World, const FVector& Center, float RingRadius)
{
	if (!World)
	{
		return nullptr;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.ObjectFlags |= RF_Transient;
	ABossSeal* Seal = World->SpawnActor<ABossSeal>(ABossSeal::StaticClass(), FTransform(Center), Params);
	if (Seal)
	{
		Seal->Shape = EBossSealShape::Ring;
		Seal->Radius = FMath::Max(RingRadius, 200.f);
	}
	return Seal;
}

void ABossSeal::BeginPlay()
{
	Super::BeginPlay();
	// Opaque and emissive (M_StylizedSurface's glow): the light reads without translucency's cost and sorting.
	if (GlowBase)
	{
		GlowMaterial = UMaterialInstanceDynamic::Create(GlowBase, this);
		GlowMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		GlowMaterial->SetScalarParameterValue(TEXT("Glow"), CurtainGlow);
		GlowMaterial->SetScalarParameterValue(TEXT("Variation"), 0.f);
		Dashes->SetMaterial(0, GlowMaterial);
		Seam->SetMaterial(0, GlowMaterial);
	}
}

// ---------------------------------------------------------------------------
// Shape
// ---------------------------------------------------------------------------

TArray<FVector> ABossSeal::GetPath() const
{
	TArray<FVector> Path;
	const FVector Origin = GetActorLocation();
	if (Shape == EBossSealShape::Gate)
	{
		Path.Add(Origin);
		Path.Add(GetActorTransform().TransformPosition(GateEnd));
		return Path;
	}
	// Spans about SpanLength long all round, at least sixteen so a small ring still reads round.
	const int32 Count = FMath::Max(16, FMath::CeilToInt32(2.f * UE_PI * Radius / SpanLength));
	Path.Reserve(Count);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = 2.f * UE_PI * static_cast<float>(Index) / static_cast<float>(Count);
		Path.Add(Origin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Radius);
	}
	return Path;
}

bool ABossSeal::IsInside(const FVector& Point, float Margin) const
{
	const FVector Origin = GetActorLocation();
	if (Shape == EBossSealShape::Gate)
	{
		// Ahead of the gate's line: the arena's side.
		const FVector Ahead = GetActorForwardVector().GetSafeNormal2D();
		return FVector::DotProduct(FVector(Point.X - Origin.X, Point.Y - Origin.Y, 0.0), Ahead) >= Margin;
	}
	return FVector::Dist2D(Point, Origin) <= Radius - Margin;
}

// ---------------------------------------------------------------------------
// Raising and dropping
// ---------------------------------------------------------------------------

void ABossSeal::Raise()
{
	if (bRaised)
	{
		return;
	}
	bRaised = true;
	// Built anew each time, so a seal moved or resized between fights stands where it is now.
	BuildWalls();
	SetWallsBlocking(true);
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		BuildCurtain();
		SetActorTickEnabled(true);
	}
}

void ABossSeal::Drop()
{
	if (!bRaised)
	{
		return;
	}
	bRaised = false;
	// Open at once; the curtain sinks away over a moment (Tick).
	SetWallsBlocking(false);
}

void ABossSeal::BuildWalls()
{
	for (UBoxComponent* Wall : Walls)
	{
		if (Wall)
		{
			Wall->DestroyComponent();
		}
	}
	Walls.Reset();

	const TArray<FVector> Path = GetPath();
	const int32 Spans = IsClosed() ? Path.Num() : Path.Num() - 1;
	for (int32 Index = 0; Index < Spans; ++Index)
	{
		const FVector& From = Path[Index];
		const FVector& To = Path[(Index + 1) % Path.Num()];
		const FVector Along = FVector(To.X - From.X, To.Y - From.Y, 0.0);
		const double Length = Along.Size();
		if (Length < 1.0)
		{
			continue;
		}
		// From Depth under its lower end to Height over its higher one.
		const double Bottom = FMath::Min(From.Z, To.Z) - Depth;
		const double Top = FMath::Max(From.Z, To.Z) + Height;
		const FVector Center((From.X + To.X) * 0.5, (From.Y + To.Y) * 0.5, (Bottom + Top) * 0.5);

		UBoxComponent* Wall = NewObject<UBoxComponent>(this, MakeUniqueObjectName(this, UBoxComponent::StaticClass(),
			*FString::Printf(TEXT("SealWall_%d"), Index)), RF_Transient);
		Wall->SetMobility(EComponentMobility::Movable);
		// In world space like the line, whatever the actor's own transform.
		Wall->SetUsingAbsoluteLocation(true);
		Wall->SetUsingAbsoluteRotation(true);
		Wall->SetUsingAbsoluteScale(true);
		Wall->SetRelativeLocation_Direct(Center);
		Wall->SetRelativeRotation_Direct(FRotator(0.f, static_cast<float>(Along.Rotation().Yaw), 0.f));
		Wall->InitBoxExtent(FVector(Length * 0.5 + WallOverlap, Thickness * 0.5, (Top - Bottom) * 0.5));
		Wall->SetupAttachment(GetRootComponent());
		// Walking pawns only: world-dynamic, so ground traces (world-static) never land on top of it, and every other
		// channel (bullets, the camera, sight, pellets' world trace) passes through.
		Wall->SetCollisionObjectType(ECC_WorldDynamic);
		Wall->SetCollisionResponseToAllChannels(ECR_Ignore);
		Wall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Wall->SetHiddenInGame(true);
		Wall->SetCanEverAffectNavigation(false);
		Wall->SetGenerateOverlapEvents(false);
		// Nobody climbs it: a character that bumps into it never tries to step up onto it.
		Wall->CanCharacterStepUpOn = ECB_No;
		Wall->RegisterComponent();
		Walls.Add(Wall);
	}
}

void ABossSeal::SetWallsBlocking(bool bBlocking)
{
	for (UBoxComponent* Wall : Walls)
	{
		if (Wall)
		{
			Wall->SetCollisionEnabled(bBlocking ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		}
	}
}

// ---------------------------------------------------------------------------
// The curtain
// ---------------------------------------------------------------------------

void ABossSeal::BuildCurtain()
{
	Curtain.Reset();
	SeamPieces.Reset();
	UWorld* World = GetWorld();
	const TArray<FVector> Path = GetPath();
	if (!World || Path.Num() < 2)
	{
		return;
	}
	// The ground under each column, so the curtain stands on uneven ground (world-static only: never grass or volumes).
	const FCollisionQueryParams Ground = LooterWorld::StaticGeometryParams(World, TEXT("BossSealGround"), this);
	auto GroundUnder = [World, &Ground](const FVector& Point) -> FVector
	{
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(Hit, Point + FVector(0.0, 0.0, 400.0), Point - FVector(0.0, 0.0, 600.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), Ground))
		{
			return Hit.ImpactPoint;
		}
		return Point;
	};

	// The same random look every time it rises (seeded by its place, so two seals differ).
	FRandomStream Random(static_cast<int32>(GetActorLocation().X * 0.37 + GetActorLocation().Y * 1.13));
	const int32 Spans = IsClosed() ? Path.Num() : Path.Num() - 1;
	for (int32 Index = 0; Index < Spans; ++Index)
	{
		const FVector From = GroundUnder(Path[Index]);
		const FVector To = GroundUnder(Path[(Index + 1) % Path.Num()]);
		const double Length = FVector::Dist2D(From, To);
		const int32 Columns = FMath::Max(1, FMath::RoundToInt32(static_cast<float>(Length) / DashSpacing));
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			const float Alpha = (static_cast<float>(Column) + Random.FRandRange(0.2f, 0.8f)) / static_cast<float>(Columns);
			FDash& Dash = Curtain.AddDefaulted_GetRef();
			Dash.Foot = GroundUnder(FMath::Lerp(Path[Index], Path[(Index + 1) % Path.Num()], Alpha));
			Dash.Length = Random.FRandRange(70.f, 190.f);
			Dash.Speed = Random.FRandRange(80.f, 170.f);
			Dash.Width = Random.FRandRange(2.5f, 5.f);
			Dash.Phase = Random.FRandRange(0.f, Height + Dash.Length);
		}
		// The seam: a flat strip from end to end of the span, along the ground.
		const FVector Along = To - From;
		const float Span = static_cast<float>(Along.Size());
		if (Span > 1.f)
		{
			SeamPieces.Add(FTransform(FRotationMatrix::MakeFromX(Along).ToQuat(), (From + To) * 0.5 + FVector(0.0, 0.0, SeamHeight * 0.5),
				FVector((Span + SeamWidth) / 100.f, SeamWidth / 100.f, SeamHeight / 100.f)));
		}
	}
}

void ABossSeal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Clock += DeltaSeconds;
	Rise = FMath::FInterpConstantTo(Rise, bRaised ? 1.f : 0.f, DeltaSeconds, 1.f / (bRaised ? RiseSeconds : SinkSeconds));
	if (!bRaised && Rise <= 0.f)
	{
		ClearCurtain();
		SetActorTickEnabled(false);
		return;
	}
	DrawCurtain();
}

void ABossSeal::DrawCurtain()
{
	// Each dash climbs its column and starts again at the foot; the curtain's top is its height times how far it has risen,
	// so it grows up out of the ground and sinks back. A dash out of sight keeps its instance at no size, so the count
	// never changes and the instances are only moved.
	const float Top = Height * FMath::InterpEaseOut(0.f, 1.f, Rise, 2.f);
	TArray<FTransform> Transforms;
	Transforms.Reserve(Curtain.Num());
	for (const FDash& Dash : Curtain)
	{
		const float Head = FMath::Fmod(Dash.Phase + Clock * Dash.Speed, Height + Dash.Length);
		const float Low = FMath::Max(Head - Dash.Length, 0.f);
		const float High = FMath::Min(Head, Top);
		const float Shown = High - Low;
		if (Shown < 2.f)
		{
			Transforms.Add(FTransform(FQuat::Identity, Dash.Foot, FVector::ZeroVector));
			continue;
		}
		// The engine cube is 100 cm a side, centered.
		Transforms.Add(FTransform(FQuat::Identity, Dash.Foot + FVector(0.0, 0.0, (Low + High) * 0.5f),
			FVector(Dash.Width / 100.f, Dash.Width / 100.f, Shown / 100.f)));
	}
	DrawInstances(Dashes, Transforms);

	// The seam shows whenever the curtain does, flattening into the ground as it sinks.
	TArray<FTransform> Pieces = SeamPieces;
	for (FTransform& Piece : Pieces)
	{
		FVector Scale = Piece.GetScale3D();
		Scale.Z *= Rise;
		Piece.SetScale3D(Scale);
	}
	DrawInstances(Seam, Pieces);
}

void ABossSeal::ClearCurtain()
{
	DrawInstances(Dashes, TArray<FTransform>());
	DrawInstances(Seam, TArray<FTransform>());
}
