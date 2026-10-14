#include "World/AmbientAirSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "World/AmbientAirRules.h"
#include "World/GustSubsystem.h"
#include "World/InteriorVisibility.h"
#include "World/LightingStateSubsystem.h"
#include "World/WorldQueries.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialParameterCollection.h"

namespace
{
	TAutoConsoleVariable<int32> CVarAir(TEXT("Looter.Air"), 1,
		TEXT("Things in the air (motes, pollen, fluff, leaves, moths) and the gusts (0: none, to measure them by difference)."));
	TAutoConsoleVariable<float> CVarAirDensity(TEXT("Looter.Air.Density"), 1.f,
		TEXT("How much of each kind's share of the air's budget may be out (0 to 1)."));

	/** How often (s) the view's shelter and the roof over it are looked at again. */
	constexpr float ShelterSeconds = 0.5f;
	/** A roof this near over the view (cm) counts as cover (a porch, a doorway, a room). */
	constexpr float CoverReach = 1500.f;
	/** How often (s) the light's facts are read again: the lighting states blend the sun round at dusk. */
	constexpr float LightSeconds = 1.f;
	/** How quickly the cost's averages follow each frame. */
	constexpr float CostSmoothing = 0.05f;

	/** A tree's grid cell (cm): the trees near the view are found from the few cells round it. */
	constexpr float TreeCell = 2500.f;
	/** Where in a tree's height its crown starts (a share from its foot to its top). */
	constexpr float CrownStart = 0.45f;
	/** The share of a tree model's half-width its crown reaches. */
	constexpr float CrownShare = 0.8f;

	const FName TreeTag(TEXT("Tree"));
	const FName FlickerTag(TEXT("Flicker"));
	const FName UndergroundTag(TEXT("Underground"));

	FIntPoint CellOf(const FVector& Where)
	{
		return FIntPoint(FMath::FloorToInt(static_cast<float>(Where.X) / TreeCell), FMath::FloorToInt(static_cast<float>(Where.Y) / TreeCell));
	}
}

UAmbientAirSubsystem* UAmbientAirSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UAmbientAirSubsystem>() : nullptr;
}

bool UAmbientAirSubsystem::IsEnabled()
{
	return CVarAir.GetValueOnGameThread() != 0;
}

float UAmbientAirSubsystem::GetDensity()
{
	return FMath::Clamp(CVarAirDensity.GetValueOnGameThread(), 0.f, 1.f);
}

bool UAmbientAirSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::EditorPreview;
}

void UAmbientAirSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UGustSubsystem>();
	Particles.SetNum(AmbientAirRules::Budget);
}

void UAmbientAirSubsystem::Deinitialize()
{
	if (AActor* Actor = Owner.Get())
	{
		Actor->Destroy();
	}
	Owner.Reset();
	Particles.Reset();
	Super::Deinitialize();
}

TStatId UAmbientAirSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAmbientAirSubsystem, STATGROUP_Tickables);
}

bool UAmbientAirSubsystem::IsTickable() const
{
	// Only played worlds move it by the clock (a test's world calls Step); off, it ticks only to put away what's out.
	const UWorld* World = GetWorld();
	return World && World->IsGameWorld() && (IsEnabled() || bDrawn);
}

void UAmbientAirSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const bool bEnabled = IsEnabled();
	if (bEnabled != bWasEnabled)
	{
		bWasEnabled = bEnabled;
		UE_LOG(LogLooter, Display, TEXT("Ambient air %s (Looter.Air)."), bEnabled ? TEXT("on") : TEXT("off"));
		if (bEnabled)
		{
			// Back on: the level is looked at again, in case it changed while the air was off.
			bGathered = false;
		}
	}
	if (!bEnabled)
	{
		Clear();
		return;
	}
	Step(DeltaTime, GatherView(DeltaTime));
}

void UAmbientAirSubsystem::Clear()
{
	for (FAirParticle& Particle : Particles)
	{
		Particle.bLive = false;
	}
	FMemory::Memzero(Counts);
	if (bDrawn)
	{
		Draw(LastView);
	}
}

int32 UAmbientAirSubsystem::CountOf(EAirKind Kind) const
{
	const int32 Index = static_cast<int32>(Kind);
	return Index >= 0 && Index < static_cast<int32>(EAirKind::Count) ? Counts[Index] : 0;
}

int32 UAmbientAirSubsystem::NumLive() const
{
	int32 Total = 0;
	for (int32 Each : Counts)
	{
		Total += Each;
	}
	return Total;
}

void UAmbientAirSubsystem::SetPlaceProbe(TFunction<bool(const FVector&, FAirPlace&)> InProbe)
{
	PlaceProbe = MoveTemp(InProbe);
}

void UAmbientAirSubsystem::SetTrees(const TArray<FAirTree>& InTrees)
{
	bTreesGiven = true;
	Trees = InTrees;
	TreeCells.Reset();
	for (int32 Index = 0; Index < Trees.Num(); ++Index)
	{
		TreeCells.FindOrAdd(CellOf(Trees[Index].Base)).Add(Index);
	}
	NearIn = 0.f;
	NearFrom = FVector(TNumericLimits<float>::Max());
}

void UAmbientAirSubsystem::SetLamps(const TArray<FAirLamp>& InLamps)
{
	bLampsGiven = true;
	Lamps = InLamps;
	NearIn = 0.f;
	NearFrom = FVector(TNumericLimits<float>::Max());
}

// ---------------------------------------------------------------------------
// A frame
// ---------------------------------------------------------------------------

void UAmbientAirSubsystem::Step(float DeltaSeconds, const FAirView& View)
{
	const uint64 StartCycles = FPlatformTime::Cycles64();
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	if (bLightingKnown && View.Lighting != LastLighting && !bLampsGiven)
	{
		// The light switched (behind the camera's fade): the lamps lit for dusk may be new, so the level is looked at again.
		bGathered = false;
	}
	if (!bGathered)
	{
		GatherLevel();
	}
	// The view's own look at the roof (GatherView) counts toward the frame's traces.
	TracesLeft = FMath::Max(MaxTracesPerFrame - TracesThisFrame, 0);
	if (View.bSheltered != LastView.bSheltered)
	{
		// Into the mine (or out): what's out leaves now, not at the next look round.
		NearIn = 0.f;
	}
	LastView = View;

	// The lighting changed (dusk came): the light is read again, and what no longer belongs leaves (Thin).
	if (!bLightingKnown || View.Lighting != LastLighting)
	{
		bLightingKnown = true;
		LastLighting = View.Lighting;
		LightIn = 0.f;
		NearIn = 0.f;
	}
	LightIn -= Dt;
	if (LightIn <= 0.f)
	{
		LightIn = LightSeconds;
		RefreshLight();
	}

	if (View.bValid)
	{
		RefreshSurroundings(View, Dt);
		Place(View);
	}
	Move(View, Dt);
	RecheckSun();
	Draw(View);

	const float Milliseconds = static_cast<float>(FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - StartCycles));
	AverageMilliseconds = FMath::Lerp(AverageMilliseconds, Milliseconds, CostSmoothing);
	AverageTraces = FMath::Lerp(AverageTraces, static_cast<float>(TracesThisFrame), CostSmoothing);
	TracesThisFrame = 0;
}

FAirView UAmbientAirSubsystem::GatherView(float DeltaSeconds)
{
	FAirView View;
	UWorld* World = GetWorld();
	const APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
	if (!Player || !Player->PlayerCameraManager)
	{
		return View;
	}
	if (!bGathered)
	{
		GatherLevel();
	}
	View.bValid = true;
	View.Location = Player->PlayerCameraManager->GetCameraLocation();
	View.Direction = Player->PlayerCameraManager->GetCameraRotation().Vector();
	View.FieldOfView = Player->PlayerCameraManager->GetFOVAngle();
	if (const ULightingStateSubsystem* Lighting = World->GetSubsystem<ULightingStateSubsystem>())
	{
		View.Lighting = Lighting->GetState();
	}
	if (const UGustSubsystem* Gusts = World->GetSubsystem<UGustSubsystem>())
	{
		View.BreezeDirection = Gusts->GetPrevailingDirection();
		View.GustDirection = Gusts->GetDirection();
		View.GustStrength = Gusts->GetStrengthNow();
	}
	// Shelter and cover a couple of times a second: a trace up for the roof, boxes for the mine.
	ShelterIn -= DeltaSeconds;
	if (ShelterIn <= 0.f)
	{
		ShelterIn = ShelterSeconds;
		bSheltered = IsShelteredAt(View.Location);
		FHitResult Roof;
		bUnderCover = !bSheltered && World->LineTraceSingleByObjectType(Roof, View.Location, View.Location + FVector(0.0, 0.0, CoverReach),
			FCollisionObjectQueryParams(ECC_WorldStatic), GroundParams);
		++TracesThisFrame;
	}
	View.bSheltered = bSheltered;
	View.bUnderCover = bUnderCover;
	return View;
}

bool UAmbientAirSubsystem::IsShelteredAt(const FVector& Location) const
{
	for (const FBox& Zone : UndergroundZones)
	{
		if (Zone.IsInsideOrOn(Location))
		{
			return true;
		}
	}
	for (const TWeakObjectPtr<AInteriorVisibility>& Weak : Interiors)
	{
		const AInteriorVisibility* Interior = Weak.Get();
		if (Interior && Interior->GetView() == EInteriorView::Inside)
		{
			return true;
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// The level, looked at once
// ---------------------------------------------------------------------------

void UAmbientAirSubsystem::GatherLevel()
{
	bGathered = true;
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const double Start = FPlatformTime::Seconds();
	// Traces for the ground skip the volumes and the boundary walls (and the scatter's trees: they find the terrain).
	GroundParams = LooterWorld::StaticGeometryParams(World, TEXT("AmbientAirGround"), nullptr, true);
	GroundParams.bReturnFaceIndex = true;
	SunParams = FCollisionQueryParams(SCENE_QUERY_STAT(AmbientAirSun), false);
	UndergroundZones.Reset();
	Interiors.Reset();
	if (!bTreesGiven)
	{
		Trees.Reset();
		TreeCells.Reset();
	}
	if (!bLampsGiven)
	{
		Lamps.Reset();
	}

	// One pass over the level: its underground spaces, its interiors, its broadleaf trees (instances or placed models tagged
	// Tree, known by their model's name) and its lamps (lights tagged Flicker, or on an actor tagged so).
	TMap<const UStaticMesh*, bool> Broadleaf;
	auto IsLeafy = [&Broadleaf](const UStaticMesh* Mesh)
	{
		if (!Mesh)
		{
			return false;
		}
		if (const bool* Known = Broadleaf.Find(Mesh))
		{
			return *Known;
		}
		return Broadleaf.Add(Mesh, AmbientAirRules::IsBroadleaf(Mesh->GetName()));
	};
	auto AddTree = [this](const FTransform& Where, const UStaticMesh& Mesh)
	{
		const FBox Box = Mesh.GetBoundingBox();
		const FVector Scale = Where.GetScale3D().GetAbs();
		FAirTree& Tree = Trees.AddDefaulted_GetRef();
		Tree.Base = Where.GetLocation();
		Tree.CrownRadius = static_cast<float>(FMath::Max(Box.GetExtent().X, Box.GetExtent().Y) * FMath::Max(Scale.X, Scale.Y)) * CrownShare;
		Tree.CrownTop = static_cast<float>(Tree.Base.Z + Box.Max.Z * Scale.Z);
		Tree.CrownBottom = static_cast<float>(Tree.Base.Z + FMath::Lerp(Box.Min.Z, Box.Max.Z, static_cast<double>(CrownStart)) * Scale.Z);
		TreeCells.FindOrAdd(CellOf(Tree.Base)).Add(Trees.Num() - 1);
	};
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}
		if (Actor->ActorHasTag(UndergroundTag))
		{
			const FBox Zone = Actor->GetComponentsBoundingBox(/*bNonColliding*/ true);
			if (Zone.IsValid)
			{
				UndergroundZones.Add(Zone);
			}
		}
		if (AInteriorVisibility* Interior = Cast<AInteriorVisibility>(Actor))
		{
			Interiors.Add(Interior);
		}
		const bool bTreeActor = Actor->ActorHasTag(TreeTag);
		const bool bFlickerActor = Actor->ActorHasTag(FlickerTag);
		for (UActorComponent* Component : Actor->GetComponents())
		{
			if (UInstancedStaticMeshComponent* Instances = Cast<UInstancedStaticMeshComponent>(Component))
			{
				const UStaticMesh* Mesh = Instances->GetStaticMesh();
				if (!bTreesGiven && Mesh && Instances->ComponentHasTag(TreeTag) && IsLeafy(Mesh))
				{
					for (int32 Index = 0; Index < Instances->GetInstanceCount(); ++Index)
					{
						FTransform Where;
						if (Instances->GetInstanceTransform(Index, Where, /*bWorldSpace*/ true))
						{
							AddTree(Where, *Mesh);
						}
					}
				}
			}
			else if (UStaticMeshComponent* Model = Cast<UStaticMeshComponent>(Component))
			{
				const UStaticMesh* Mesh = Model->GetStaticMesh();
				if (!bTreesGiven && bTreeActor && Mesh && IsLeafy(Mesh))
				{
					AddTree(Model->GetComponentTransform(), *Mesh);
				}
			}
			else if (ULightComponent* Light = Cast<ULightComponent>(Component))
			{
				if (!bLampsGiven && (bFlickerActor || Light->ComponentHasTag(FlickerTag)))
				{
					FAirLamp& Lamp = Lamps.AddDefaulted_GetRef();
					Lamp.Location = Light->GetComponentLocation();
					Lamp.Light = Light;
				}
			}
		}
	}
	NearIn = 0.f;
	NearFrom = FVector(TNumericLimits<float>::Max());
	UE_LOG(LogLooter, Log, TEXT("Ambient air: %d broadleaf trees, %d lamps, %d underground spaces, %d interiors (%.1f ms)."),
		Trees.Num(), Lamps.Num(), UndergroundZones.Num(), Interiors.Num(), (FPlatformTime::Seconds() - Start) * 1000.0);
}
