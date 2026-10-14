// UAmbientAirSubsystem's drawing: each live one drawn as an instance on one of three components (glows, puffs, leaves),
// every component keeping its pool's count of instances so moving them never rebuilds its render state; and the light
// the unlit ones are given (the sun where it reaches, the sky's in shade, the lighting state's tint).

#include "World/AmbientAirSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "World/AmbientAirRules.h"
#include "World/LightingStates.h"
#include "World/LightingStateSubsystem.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include <initializer_list>

namespace
{
	TAutoConsoleVariable<float> CVarAirGlow(TEXT("Looter.Air.Glow"), 1.f,
		TEXT("How bright the motes and moths glow (1: as made; the glow's look depends on the scene's exposure)."));

	/** The game's own effect materials (FWeaponFX's): additive glows and soft translucent puffs, colored by custom data. */
	const TCHAR* GlowMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* SmokeMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Smoke.M_FX_Smoke");
	const TCHAR* FallbackGlowPath = TEXT("/Game/Environment/Materials/M_StylizedGlow.M_StylizedGlow");
	const TCHAR* SurfaceMaterialPath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");
	constexpr int32 GlowFloats = 5;  // R, G, B, strength, shape (0 streak, 1 round)
	constexpr int32 PuffFloats = 4;  // R, G, B, opacity

	/** A mote's glint at full sun, against the sun (times Looter.Air.Glow), and a moth's in its lamp's light. */
	constexpr float MoteGlow = 3.f;
	constexpr float MothGlow = 2.f;
	/** A mote faster than this (cm/s: a gust's) is drawn as a streak this long a cm/s, at most StreakMax (cm). */
	constexpr float StreakSpeed = 60.f;
	constexpr float StreakPerSpeed = 0.05f;
	constexpr float StreakMax = 25.f;
	/** Pollen and fluff opacity at full show. */
	constexpr float PollenOpacity = 0.75f;
	constexpr float FluffOpacity = 0.65f;
	/** A moth is lit by its lamp within this (cm), and its wings beat this often (a second). */
	constexpr float MothLit = 150.f;
	constexpr float WingBeats = 8.f;
	/** A leaf's thickness and width against its length. */
	constexpr float LeafWidth = 0.62f;
	constexpr float LeafThickness = 0.07f;
	/** An unused instance: a speck at the view with nothing shown (a zero scale would make its normals infinite). */
	constexpr float HiddenScale = 0.0001f;

	/** The light on unlit specks (FSlideDust's): about twice their color in full sun, 0.7 in shade under the sky. */
	constexpr float SunLevel = 2.f;
	constexpr float ShadeLevel = 0.7f;
	const FLinearColor SkyColor(0.86f, 0.93f, 1.05f);
	/** How far toward the sun a point is looked at for its light (cm). */
	constexpr float SunCheckDistance = 6000.f;

	/** A light's color with its brightest channel at 1: the hue (its strength is the exposure's business). */
	FLinearColor Hue(const FLinearColor& Color)
	{
		const float Brightest = FMath::Max3(Color.R, Color.G, Color.B);
		return Brightest > 0.01f ? Color / Brightest : FLinearColor::White;
	}

	FVector AnyPerpendicular(const FVector& Direction)
	{
		const FVector Other = FMath::Abs(Direction.Z) < 0.9f ? FVector::UpVector : FVector::ForwardVector;
		return FVector::CrossProduct(Direction, Other).GetSafeNormal();
	}

	/** A quad along Tail -> Head, turned about that line to face the camera (the engine plane: 100 x 100 in XY, facing +Z). */
	FTransform StreakTransform(const FVector& Tail, const FVector& Head, float Width, const FVector& Camera)
	{
		const FVector Axis = Head - Tail;
		const float Length = static_cast<float>(Axis.Size());
		const FVector Direction = Length > UE_KINDA_SMALL_NUMBER ? Axis / Length : FVector::ForwardVector;
		const FVector Middle = (Tail + Head) * 0.5f;
		const FVector ToCamera = Camera - Middle;
		FVector Facing = ToCamera - Direction * FVector::DotProduct(ToCamera, Direction);
		if (!Facing.Normalize())
		{
			Facing = AnyPerpendicular(Direction);
		}
		const FQuat Rotation = FRotationMatrix::MakeFromXZ(Direction, Facing).ToQuat();
		return FTransform(Rotation, Middle, FVector(FMath::Max(Length, 0.1f) / 100.f, Width / 100.f, 1.f));
	}

	/** A round quad facing the camera. */
	FTransform SpriteTransform(const FVector& Location, float Size, const FVector& Camera)
	{
		FVector Facing = Camera - Location;
		if (!Facing.Normalize())
		{
			Facing = FVector::UpVector;
		}
		return FTransform(FRotationMatrix::MakeFromZ(Facing).ToQuat(), Location, FVector(Size / 100.f, Size / 100.f, 1.f));
	}

	UInstancedStaticMeshComponent* MakeInstances(AActor* Owner, const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Material, int32 CustomFloats)
	{
		UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(Owner, Name, RF_Transient);
		Instances->SetupAttachment(Owner->GetRootComponent());
		Instances->SetMobility(EComponentMobility::Movable);
		Instances->SetStaticMesh(Mesh);
		Instances->SetMaterial(0, Material);
		Instances->SetNumCustomDataFloats(CustomFloats);
		Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Instances->SetCastShadow(false);
		Instances->SetCanEverAffectNavigation(false);
		Instances->bReceivesDecals = false;
		Instances->bAffectDistanceFieldLighting = false;
		Instances->RegisterComponent();
		return Instances;
	}

	/**
	 * Gives a component exactly these instances: moved in place while the count holds (the pools never change size while
	 * anything's out), added afresh only when it doesn't (the first frame, and after everything went).
	 */
	void DrawPool(UInstancedStaticMeshComponent* Instances, const TArray<FTransform>& Transforms, const TArray<float>& CustomData)
	{
		if (!Instances)
		{
			return;
		}
		if (Instances->GetInstanceCount() != Transforms.Num())
		{
			Instances->ClearInstances();
			if (Transforms.Num() > 0)
			{
				Instances->AddInstances(Transforms, /*bShouldReturnIndices*/ false, /*bWorldSpace*/ true, /*bUpdateNavigation*/ false);
			}
			Instances->MarkRenderStateDirty();
		}
		else if (Transforms.Num() > 0)
		{
			Instances->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		}
		if (Transforms.Num() > 0 && CustomData.Num() == Transforms.Num() * Instances->NumCustomDataFloats && CustomData.Num() > 0)
		{
			Instances->SetCustomData(0, Transforms.Num() - 1, CustomData, false);
		}
	}
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

void UAmbientAirSubsystem::MakeComponents()
{
	bComponentsMade = true;
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.ObjectFlags |= RF_Transient;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Params);
	if (!Actor)
	{
		return;
	}
#if WITH_EDITOR
	Actor->SetActorLabel(TEXT("AmbientAir"));
#endif
	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"), RF_Transient);
	Root->SetMobility(EComponentMobility::Movable);
	Actor->SetRootComponent(Root);
	Root->RegisterComponent();
	Owner = Actor;

	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* GlowMaterial = LoadObject<UMaterialInterface>(nullptr, GlowMaterialPath);
	if (!GlowMaterial)
	{
		GlowMaterial = LoadObject<UMaterialInterface>(nullptr, FallbackGlowPath);
	}
	UMaterialInterface* SmokeMaterial = LoadObject<UMaterialInterface>(nullptr, SmokeMaterialPath);
	Glows = MakeInstances(Actor, TEXT("AirGlows"), Plane, GlowMaterial, GlowFloats);
	// Without its material a puff would draw as a solid square: pollen and fluff stay away instead.
	Puffs = SmokeMaterial ? MakeInstances(Actor, TEXT("AirPuffs"), Plane, SmokeMaterial, PuffFloats) : nullptr;
	// Leaves are lit flakes in the late summer's olive gold, so the sun and the dusk light them as the trees.
	UMaterialInterface* Surface = LoadObject<UMaterialInterface>(nullptr, SurfaceMaterialPath);
	UMaterialInstanceDynamic* Leaf = Surface ? UMaterialInstanceDynamic::Create(Surface, Actor) : nullptr;
	if (Leaf)
	{
		Leaf->SetVectorParameterValue(TEXT("Color"), AmbientAirRules::Albedo(EAirKind::Leaf));
		Leaf->SetScalarParameterValue(TEXT("Variation"), 0.25f);
		Leaf->SetScalarParameterValue(TEXT("Glow"), 0.f);
	}
	Leaves = Surface ? MakeInstances(Actor, TEXT("AirLeaves"), Cube, Leaf ? static_cast<UMaterialInterface*>(Leaf) : Surface, 0) : nullptr;
	if (!Puffs.IsValid())
	{
		UE_LOG(LogLooter, Warning, TEXT("Ambient air: no %s, so no pollen or fluff."), SmokeMaterialPath);
	}
}

void UAmbientAirSubsystem::Draw(const FAirView& View)
{
	const bool bAnything = NumLive() > 0;
	if (!bAnything && !bDrawn)
	{
		return;
	}
	if (bAnything && !bComponentsMade)
	{
		MakeComponents();
	}
	if (!bAnything)
	{
		// Everything's gone: the components are emptied once and left so until something's out again.
		for (const TWeakObjectPtr<UInstancedStaticMeshComponent>& Each : { Glows, Puffs, Leaves })
		{
			if (UInstancedStaticMeshComponent* Instances = Each.Get())
			{
				Instances->ClearInstances();
				Instances->MarkRenderStateDirty();
			}
		}
		bDrawn = false;
		return;
	}

	const FVector Eye = View.Location;
	const float Glow = FMath::Max(CVarAirGlow.GetValueOnGameThread(), 0.f);
	const FLinearColor MoteColor = AmbientAirRules::Albedo(EAirKind::Mote) * SunHue * StateTint;
	TArray<FTransform> GlowTransforms, PuffTransforms, LeafTransforms;
	TArray<float> GlowData, PuffData;
	const int32 GlowPool = AmbientAirRules::Cap(EAirKind::Mote) + AmbientAirRules::Cap(EAirKind::Moth);
	const int32 PuffPool = AmbientAirRules::Cap(EAirKind::Pollen) + AmbientAirRules::Cap(EAirKind::Fluff);
	const int32 LeafPool = AmbientAirRules::Cap(EAirKind::Leaf);
	GlowTransforms.Reserve(GlowPool);
	GlowData.Reserve(GlowPool * GlowFloats);
	PuffTransforms.Reserve(PuffPool);
	PuffData.Reserve(PuffPool * PuffFloats);
	LeafTransforms.Reserve(LeafPool);

	for (const FAirParticle& Particle : Particles)
	{
		if (!Particle.bLive)
		{
			continue;
		}
		const float Level = static_cast<float>(FVector::Dist2D(Particle.Location, Eye));
		const float ToEye = static_cast<float>(FVector::Dist(Particle.Location, Eye));
		const float Leaving = Particle.Leaving > 0.f ? FMath::Clamp(Particle.Leaving, 0.f, 1.f) : 1.f;
		const float Show = AmbientAirRules::AgeFade(Particle.Age, Particle.Life)
			* AmbientAirRules::DistanceFade(Level, AmbientAirRules::Reach(Particle.Kind, View.bUnderCover), ToEye) * Leaving;
		switch (Particle.Kind)
		{
		case EAirKind::Mote:
		{
			if (GlowTransforms.Num() >= GlowPool)
			{
				break;
			}
			// A glint, brightest against the sun (light scattered forward off the dust), none out of it.
			const FVector Out = ToEye > 1.f ? (Particle.Location - Eye) / ToEye : FVector::ForwardVector;
			const float Against = FMath::Pow(FMath::Clamp(static_cast<float>(FVector::DotProduct(Out, ToSun)), 0.f, 1.f), 3.f);
			const float Strength = MoteGlow * Glow * (Particle.bInSun ? 1.f : 0.f) * SunHeight * (0.25f + 0.75f * Against) * Show;
			const float Speed = static_cast<float>(Particle.Velocity.Size());
			if (Speed > StreakSpeed)
			{
				// Racing in a gust: a streak along its way.
				const FVector Tail = Particle.Location - Particle.Velocity / Speed * FMath::Min(Speed * StreakPerSpeed, StreakMax);
				GlowTransforms.Add(StreakTransform(Tail, Particle.Location, Particle.Size, Eye));
				GlowData.Append({ MoteColor.R, MoteColor.G, MoteColor.B, Strength, 0.f });
			}
			else
			{
				GlowTransforms.Add(SpriteTransform(Particle.Location, Particle.Size, Eye));
				GlowData.Append({ MoteColor.R, MoteColor.G, MoteColor.B, Strength, 1.f });
			}
			break;
		}
		case EAirKind::Moth:
		{
			if (GlowTransforms.Num() >= GlowPool)
			{
				break;
			}
			// Lit by its lamp, brightest close to it; its wings' beat flickers its size.
			const FVector Lamp = Lamps.IsValidIndex(Particle.Anchor) ? Lamps[Particle.Anchor].Location : Particle.Location;
			const float Near = FMath::Square(FMath::Clamp(1.f - static_cast<float>(FVector::Dist(Particle.Location, Lamp)) / MothLit, 0.f, 1.f));
			const float Strength = MothGlow * Glow * (0.15f + 0.85f * Near) * Show;
			const float Beat = FMath::Abs(FMath::Sin(Particle.Age * UE_TWO_PI * WingBeats + static_cast<float>(Particle.Phase.X)));
			const FLinearColor Color = AmbientAirRules::Albedo(EAirKind::Moth);
			GlowTransforms.Add(SpriteTransform(Particle.Location, Particle.Size * (0.65f + 0.35f * Beat), Eye));
			GlowData.Append({ Color.R, Color.G, Color.B, Strength, 1.f });
			break;
		}
		case EAirKind::Pollen:
		case EAirKind::Fluff:
		{
			if (PuffTransforms.Num() >= PuffPool)
			{
				break;
			}
			// Unlit, so given the light where it is: the sun's, or the sky's in shade, dimmed at dusk.
			const FLinearColor Color = AmbientAirRules::Albedo(Particle.Kind) * LightFor(Particle.bInSun ? 1.f : 0.f);
			const float Opacity = (Particle.Kind == EAirKind::Pollen ? PollenOpacity : FluffOpacity) * Show;
			const float Speed = static_cast<float>(Particle.Velocity.Size());
			if (Particle.Kind == EAirKind::Fluff && Speed > 150.f)
			{
				// A tuft racing in a gust smears a little along its way.
				const FVector Tail = Particle.Location - Particle.Velocity / Speed * FMath::Min(Speed * 0.02f, Particle.Size * 3.f);
				PuffTransforms.Add(StreakTransform(Tail, Particle.Location, Particle.Size, Eye));
			}
			else
			{
				PuffTransforms.Add(SpriteTransform(Particle.Location, Particle.Size, Eye));
			}
			PuffData.Append({ Color.R, Color.G, Color.B, Opacity });
			break;
		}
		case EAirKind::Leaf:
		{
			if (LeafTransforms.Num() >= LeafPool)
			{
				break;
			}
			// Opaque and lit: it shrinks away rather than fading. The engine cube is 100 across.
			const float Length = Particle.Size * FMath::Clamp(Show * 1.5f, 0.f, 1.f) / 100.f;
			LeafTransforms.Add(FTransform(Particle.Rotation, Particle.Location,
				FVector(FMath::Max(Length, HiddenScale), FMath::Max(Length * LeafWidth, HiddenScale), FMath::Max(Length * LeafThickness, HiddenScale))));
			break;
		}
		default:
			break;
		}
	}

	// Each pool keeps its count: the slots nobody's in are a speck at the eye, showing nothing.
	const FTransform Hidden(FQuat::Identity, Eye, FVector(HiddenScale));
	while (GlowTransforms.Num() < GlowPool)
	{
		GlowTransforms.Add(Hidden);
		GlowData.Append({ 0.f, 0.f, 0.f, 0.f, 1.f });
	}
	while (PuffTransforms.Num() < PuffPool)
	{
		PuffTransforms.Add(Hidden);
		PuffData.Append({ 0.f, 0.f, 0.f, 0.f });
	}
	while (LeafTransforms.Num() < LeafPool)
	{
		LeafTransforms.Add(Hidden);
	}
	DrawPool(Glows.Get(), GlowTransforms, GlowData);
	DrawPool(Puffs.Get(), PuffTransforms, PuffData);
	DrawPool(Leaves.Get(), LeafTransforms, {});
	bDrawn = true;
}

// ---------------------------------------------------------------------------
// The light
// ---------------------------------------------------------------------------

void UAmbientAirSubsystem::RefreshLight()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (!Sun.IsValid())
	{
		// The sun is the light the sky atmosphere sets by; any directional light will do in a level without one.
		const UDirectionalLightComponent* Fallback = nullptr;
		for (TActorIterator<ADirectionalLight> It(World); It; ++It)
		{
			// GetLightComponent, not GetComponent: that one is editor-only (a packaged game can't compile it).
			const UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(It->GetLightComponent());
			if (!Light || !Light->IsVisible())
			{
				continue;
			}
			if (Light->bAtmosphereSunLight)
			{
				Sun = Light;
				break;
			}
			Fallback = Fallback ? Fallback : Light;
		}
		if (!Sun.IsValid())
		{
			Sun = Fallback;
		}
	}
	const UDirectionalLightComponent* Light = Sun.Get();
	ToSun = Light ? -Light->GetDirection() : FVector::UpVector;
	SunHue = Light ? Hue(Light->GetLightColor()) : FLinearColor::White;
	// A sun low over the horizon lights them less; one under it, not at all.
	SunHeight = Light && Light->Intensity > 0.f ? FMath::Clamp(static_cast<float>(ToSun.Z) * 4.f, 0.f, 1.f) : 0.f;
	// What the lighting state does to things that can't see the light themselves: white by day, darker and warmer at dusk.
	StateTint = FLinearColor::White;
	if (!bLightingCollectionLooked)
	{
		bLightingCollectionLooked = true;
		LightingCollection = LoadObject<UMaterialParameterCollection>(nullptr, ALightingStates::DefaultCollectionPath);
	}
	const UMaterialParameterCollectionInstance* Instance = LightingCollection ? World->GetParameterCollectionInstance(LightingCollection) : nullptr;
	FLinearColor Tint;
	if (Instance && Instance->GetVectorParameterValue(ULightingStateSubsystem::BackdropTintParameter, Tint))
	{
		StateTint = Tint;
	}
}

FLinearColor UAmbientAirSubsystem::LightFor(float SunShare) const
{
	return (SkyColor * ShadeLevel + SunHue * ((SunLevel - ShadeLevel) * FMath::Clamp(SunShare, 0.f, 1.f) * SunHeight)) * StateTint;
}

bool UAmbientAirSubsystem::IsInSun(const FVector& Point) const
{
	const UWorld* World = GetWorld();
	if (!World || SunHeight <= 0.f || ToSun.Z <= 0.0)
	{
		return false;
	}
	return !World->LineTraceTestByChannel(Point, Point + ToSun * SunCheckDistance, ECC_Visibility, SunParams);
}
