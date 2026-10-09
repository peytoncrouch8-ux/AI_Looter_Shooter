#include "Bosses/BossSeal.h"
#include "AI_Looter_Shooter.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** How brightly the seam glows (M_StylizedSurface's emissive multiplier of its color). */
	constexpr float CurtainGlow = 3.f;
	/** Seconds to grow up out of the ground, and to sink back. */
	constexpr float RiseSeconds = 0.6f;
	constexpr float SinkSeconds = 0.45f;
	/** Spans overlap by this much (cm) past their ends, so no gap opens at a ring's corners for a capsule to squeeze through. */
	constexpr float WallOverlap = 30.f;

	/** The game's soft smoke puff and its additive glow (FWeaponFX's), colored and faded per instance by custom data. */
	const TCHAR* SmokePath = TEXT("/Game/Weapons/FX/M_FX_Smoke.M_FX_Smoke");
	const TCHAR* LightPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	/** Custom data per instance: R, G, B, opacity (smoke); R, G, B, strength, shape (glow). */
	constexpr int32 SmokeFloats = 4;
	constexpr int32 LightFloats = 5;

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
	// After the camera has its place for the frame: the fog's quads face where it is now.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Surface(TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface"));
	Fog = MakeGlowInstances(this, Root, TEXT("Fog"), Plane.Object);
	Fog->SetNumCustomDataFloats(SmokeFloats);
	Glow = MakeGlowInstances(this, Root, TEXT("Glow"), Plane.Object);
	Glow->SetNumCustomDataFloats(LightFloats);
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
	// The seam is opaque and emissive (M_StylizedSurface's glow): it reads without translucency's cost and sorting.
	if (GlowBase)
	{
		GlowMaterial = UMaterialInstanceDynamic::Create(GlowBase, this);
		GlowMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		GlowMaterial->SetScalarParameterValue(TEXT("Glow"), CurtainGlow);
		GlowMaterial->SetScalarParameterValue(TEXT("Variation"), 0.f);
		Seam->SetMaterial(0, GlowMaterial);
	}
	// The fog and the light take their color per instance from the game's own FX materials. Without one the quads would draw
	// as solid squares, so they're left out instead.
	UMaterialInterface* Smoke = LoadObject<UMaterialInterface>(nullptr, SmokePath);
	UMaterialInterface* Light = LoadObject<UMaterialInterface>(nullptr, LightPath);
	if (!Smoke || !Light)
	{
		UE_LOG(LogLooter, Warning, TEXT("Boss seal: %s is missing, so the fog wall shows without its %s."),
			!Smoke ? SmokePath : LightPath, !Smoke ? TEXT("fog") : TEXT("light"));
	}
	Fog->SetMaterial(0, Smoke);
	Fog->SetVisibility(Smoke != nullptr);
	Glow->SetMaterial(0, Light);
	Glow->SetVisibility(Light != nullptr);
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
