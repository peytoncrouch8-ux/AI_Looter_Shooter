#include "Weapons/WeaponFX.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

// FWeaponFX's setup and the guns' own spawns (a hit's sparks and splash, a scene's gunfire). WeaponFXDraw.cpp moves and
// draws every particle.

namespace
{
	const TCHAR* GlowMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* SmokeMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Smoke.M_FX_Smoke");
	/** Used until M_FX_Glow exists: the environment's glow (dimmer seen end-on). */
	const TCHAR* FallbackGlowPath = TEXT("/Game/Environment/Materials/M_StylizedGlow.M_StylizedGlow");
	const TCHAR* SurfaceMaterialPath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");
	/**
	 * The VFX pass's masters (Tools/Unreal/build_vfx_materials.py): M_FX_Shape draws rings, fireballs and starbursts on one
	 * quad each (additive); M_FX_Puff is the smoke broken up by noise and eroding as it thins (translucent). Each is used
	 * only once it's in the game, so the code runs before the script has.
	 */
	const TCHAR* ShapeMaterialPath = TEXT("/Game/Art/Materials/Masters/M_FX_Shape.M_FX_Shape");
	const TCHAR* PuffMaterialPath = TEXT("/Game/Art/Materials/Masters/M_FX_Puff.M_FX_Puff");
	/** VFX-2's alpha-blended fire rolling into soot (SpawnFlame): fire that keeps its colour in full sun. */
	const TCHAR* FireMaterialPath = TEXT("/Game/Art/Materials/Masters/M_FX_Fire.M_FX_Fire");
	/** The muzzle rework's master: its soft tapered streak draws sparks and tracers (SoftStreaks; the play-check, 2026-10-22). */
	const TCHAR* SoftStreakMaterialPath = TEXT("/Game/Art/Materials/Masters/M_FX_Muzzle.M_FX_Muzzle");

	constexpr int32 GlowFloats = 5;  // R, G, B, intensity, shape (0 = streak, 1 = round, 2 = six-pointed star)
	constexpr int32 SmokeFloats = 4; // R, G, B, opacity
	constexpr int32 PuffFloats = 6;  // R, G, B, opacity, seed (below 0: a plain soft disc), erosion (0 fresh .. 1 thinned away)
	constexpr int32 ShapeFloats = 7; // R, G, B, intensity, shape (EVfxShape), param (thickness, heat or spikes), seed
	constexpr int32 FireFloats = 10; // fire R, G, B, smoke R, G, B, opacity, heat (1 white-hot .. 0 smoke), seed, erosion
	constexpr int32 SoftStreakFloats = 8; // R, G, B, intensity, shape (0 streak), length share, seed, crosshair hole (0)

	UMaterialInterface* LoadQuietly(const TCHAR* Path)
	{
		return LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	// Palette (linear). Glows are multiplied by their intensity, so they only need a hue.
	const FLinearColor SparkColor(1.f, 0.5f, 0.16f);
	const FLinearColor FlashColor(1.f, 0.72f, 0.4f);
	const FLinearColor TargetSparkColor(1.f, 0.78f, 0.45f);
	const FLinearColor DustColor(0.58f, 0.54f, 0.47f);
	const FLinearColor StoneColor(0.34f, 0.31f, 0.27f);
	const FLinearColor IchorColor(0.55f, 0.66f, 0.1f);
	const FLinearColor IchorFlashColor(0.85f, 1.f, 0.45f);
	const FLinearColor CritFlashColor(1.f, 0.85f, 0.3f);

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
		// Effects light nothing and are in no distance field or ray-traced scene (VFX-2: a rebuilt component was re-added there).
		Instances->bAffectDistanceFieldLighting = false;
		Instances->bAffectDynamicIndirectLighting = false;
		Instances->bVisibleInRayTracing = false;
		Instances->RegisterComponent();
		return Instances;
	}

	UMaterialInterface* MakeSurface(UObject* Outer, const FLinearColor& Color, float Glow)
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, SurfaceMaterialPath);
		UMaterialInstanceDynamic* Instance = Base ? UMaterialInstanceDynamic::Create(Base, Outer) : nullptr;
		if (!Instance)
		{
			return Base;
		}
		Instance->SetVectorParameterValue(TEXT("Color"), Color);
		Instance->SetScalarParameterValue(TEXT("Variation"), 0.08f);
		Instance->SetScalarParameterValue(TEXT("Glow"), Glow);
		return Instance;
	}
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void FWeaponFX::Initialize(UWorld* World)
{
	if (Owner.IsValid() || !World)
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
	Actor->SetActorLabel(TEXT("WeaponFX"));
#endif
	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"), RF_Transient);
	Root->SetMobility(EComponentMobility::Movable);
	Actor->SetRootComponent(Root);
	Root->RegisterComponent();
	Owner = Actor;

	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* GlowMaterial = LoadObject<UMaterialInterface>(nullptr, GlowMaterialPath);
	if (!GlowMaterial)
	{
		GlowMaterial = LoadObject<UMaterialInterface>(nullptr, FallbackGlowPath);
	}
	UMaterialInterface* SmokeMaterial = LoadObject<UMaterialInterface>(nullptr, SmokeMaterialPath);
	const FVfxTuning Tuning = FVfxTuning::Live();
	MaxParticleCount = Tuning.MaxParticles;
	// The noise-broken puffs once they're in the game (Looter.Vfx.Puffs); the plain soft smoke until then.
	UMaterialInterface* PuffMaterial = Tuning.bPuffs ? LoadQuietly(PuffMaterialPath) : nullptr;
	UMaterialInterface* ShapeMaterial = Tuning.bShapes ? LoadQuietly(ShapeMaterialPath) : nullptr;

	Glows = MakeInstances(Actor, TEXT("Glows"), Plane, GlowMaterial, GlowFloats);
	// Without its material, smoke would draw as solid squares: leave it out instead. Redraw writes as many floats a puff as
	// the component takes (the puffs' six, the plain smoke's four).
	if (PuffMaterial)
	{
		Smoke = MakeInstances(Actor, TEXT("Smoke"), Plane, PuffMaterial, PuffFloats);
	}
	else
	{
		Smoke = SmokeMaterial ? MakeInstances(Actor, TEXT("Smoke"), Plane, SmokeMaterial, SmokeFloats) : nullptr;
	}
	// Rings, fireballs and starbursts on their own quads; without M_FX_Shape they're drawn on the glows (Redraw's stand-ins).
	Shapes = ShapeMaterial ? MakeInstances(Actor, TEXT("Shapes"), Plane, ShapeMaterial, ShapeFloats) : nullptr;
	// Fire and its soot, alpha-blended and drawn over the dust (a blast's skirt rolls out under its fire).
	if (UMaterialInterface* FireMaterial = Tuning.bFire ? LoadQuietly(FireMaterialPath) : nullptr)
	{
		UInstancedStaticMeshComponent* FireInstances = MakeInstances(Actor, TEXT("Fire"), Plane, FireMaterial, FireFloats);
		FireInstances->SetTranslucentSortPriority(1);
		FireQuads = FireInstances;
	}
	// Sparks and tracers as soft tapered streaks once M_FX_Muzzle is in the game (else on the glows).
	if (UMaterialInterface* SoftMaterial = Tuning.bSoftStreaks ? LoadQuietly(SoftStreakMaterialPath) : nullptr)
	{
		SoftStreaks = MakeInstances(Actor, TEXT("SoftStreaks"), Plane, SoftMaterial, SoftStreakFloats);
	}
	Chips = MakeInstances(Actor, TEXT("Chips"), Cube, MakeSurface(Actor, StoneColor, 0.f), 0);
	// The spiders' ichor drops: lit, barely glowing (Looter.Vfx.Hit.DropGlow; at 0.35 they read as lime chips, 2026-10-23).
	Droplets = MakeInstances(Actor, TEXT("Droplets"), Sphere, MakeSurface(Actor, IchorColor, Tuning.HitDropGlow), 0);
}

void FWeaponFX::Shutdown()
{
	if (AActor* Actor = Owner.Get())
	{
		Actor->Destroy();
	}
	Owner.Reset();
	Shapes.Reset();
	FireQuads.Reset();
	SoftStreaks.Reset();
	BitSets.Reset();
	Particles.Reset();
	Streaks.Reset();
	WaterRings.Reset();
	SpawnFloor.Reset();
	HitStream.Reset();
	KillStream.Reset();
	bHasCamera = false;
}

uint8 FWeaponFX::BitSetFor(bool bRound, const FLinearColor& Color, float Glow)
{
	return BitSetFor(bRound ? EBitShape::Round : EBitShape::Plate, Color, Glow);
}

uint8 FWeaponFX::BitSetFor(EBitShape Shape, const FLinearColor& Color, float Glow)
{
	const FColor Key = Color.ToFColor(false);
	// Sets whose components went with an old effects actor are made again.
	BitSets.RemoveAll([](const FBitSet& Set) { return !Set.Instances.IsValid(); });
	for (int32 Index = 0; Index < BitSets.Num(); ++Index)
	{
		if (BitSets[Index].Key == Key && BitSets[Index].Shape == Shape)
		{
			return static_cast<uint8>(Index);
		}
	}
	AActor* Actor = Owner.Get();
	if (!Actor || BitSets.Num() >= MaxBitSets)
	{
		// No room for another color: the last set's will do (a stray chunk in the wrong shade beats none).
		return static_cast<uint8>(FMath::Max(BitSets.Num() - 1, 0));
	}
	// A component per color: the surface material takes one color per instance of it, not per chunk.
	const bool bRound = Shape == EBitShape::Round;
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, bRound ? TEXT("/Engine/BasicShapes/Sphere.Sphere") : TEXT("/Engine/BasicShapes/Cube.Cube"));
	const FString Name = FString::Printf(TEXT("Bits%d"), BitSets.Num());
	FBitSet& Set = BitSets.AddDefaulted_GetRef();
	Set.Instances = MakeInstances(Actor, *Name, Mesh, MakeSurface(Actor, Color, Glow), 0);
	Set.Key = Key;
	Set.Shape = Shape;
	return static_cast<uint8>(BitSets.Num() - 1);
}

bool FWeaponFX::IsIdle() const
{
	return Particles.IsEmpty() && Streaks.IsEmpty() && WaterRings.IsEmpty() && !bDrawnAnything;
}

// ---------------------------------------------------------------------------
// Spawning
// ---------------------------------------------------------------------------

void FWeaponFX::AddStreak(const FVector& Tail, const FVector& Head, float Width, const FLinearColor& Color, float Intensity, EStreakLook Look)
{
	Streaks.Add({ Tail, Head, Width, Color, Intensity, false, Look });
}

void FWeaponFX::AddGlow(const FVector& Location, float Size, const FLinearColor& Color, float Intensity)
{
	Streaks.Add({ Location, Location, Size, Color, Intensity, true });
}

FWeaponFX::FParticle& FWeaponFX::AddParticle(EParticle Type, const FVector& Location, const FVector& Velocity, float Life)
{
	if (Particles.Num() >= MaxParticleCount)
	{
		Particles.RemoveAt(0, 1, EAllowShrinking::No); // the oldest is nearly gone anyway
	}
	FParticle& Particle = Particles.AddDefaulted_GetRef();
	Particle.Type = Type;
	Particle.Location = Location;
	Particle.Velocity = Velocity;
	Particle.Life = FMath::Max(Life, 0.01f);
	Particle.Roll = Random.FRandRange(0.f, 2.f * UE_PI);
	// An impact with ground under it: what it throws lands there (sparks skitter, chunks bounce and lie, drops splat).
	if (SpawnFloor.IsSet() && (Type == EParticle::Spark || Type == EParticle::Chip || Type == EParticle::Droplet || Type == EParticle::Bit))
	{
		Particle.bFloor = true;
		Particle.Floor = SpawnFloor.GetValue();
	}
	return Particle;
}

void FWeaponFX::SpawnImpact(const FVector& Location, const FVector& Normal, const FVector& ShotDirection, EImpactSurface Surface, bool bCritical)
{
	const FVector Up = Normal.IsNearlyZero() ? -ShotDirection.GetSafeNormal() : Normal.GetSafeNormal();
	// Sit just off the surface so nothing sinks into it.
	const FVector Point = Location + Up * 2.f;
	// Sparks glance off along the bounce of the shot as much as straight out of the surface.
	const FVector Bounce = ShotDirection.GetSafeNormal().MirrorByVector(Up);

	auto Flash = [this, &Point](const FLinearColor& Color, float Intensity, float From, float To, float Life)
	{
		FParticle& Particle = AddParticle(EParticle::Flash, Point, FVector::ZeroVector, Life);
		Particle.Color = Color;
		Particle.Intensity = Intensity;
		Particle.StartSize = From;
		Particle.EndSize = To;
		Particle.Gravity = 0.f;
	};
	auto Sparks = [this, &Point, &Up, &Bounce](int32 Count, const FLinearColor& Color, float Intensity, float MinSpeed, float MaxSpeed)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Direction = (Bounce * 0.6f + Up * 0.8f + Random.GetUnitVector() * 0.55f).GetSafeNormal();
			FParticle& Spark = AddParticle(EParticle::Spark, Point, Direction * Random.FRandRange(MinSpeed, MaxSpeed), Random.FRandRange(0.12f, 0.3f));
			Spark.Color = Color;
			Spark.Intensity = Intensity * Random.FRandRange(0.7f, 1.2f);
			Spark.StartSize = Random.FRandRange(0.9f, 1.4f); // streak width
			Spark.Drag = 1.5f;
		}
	};

	switch (Surface)
	{
	case EImpactSurface::World:
	{
		Flash(FlashColor, 16.f, 16.f, 32.f, 0.07f);
		Sparks(7, SparkColor, 26.f, 450.f, 1300.f);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			FParticle& Puff = AddParticle(EParticle::Smoke, Point, Up * Random.FRandRange(50.f, 120.f) + Random.GetUnitVector() * 25.f, Random.FRandRange(0.55f, 0.85f));
			Puff.Color = DustColor;
			Puff.Intensity = 0.55f;
			Puff.StartSize = Random.FRandRange(10.f, 16.f);
			Puff.EndSize = Random.FRandRange(45.f, 70.f);
			Puff.Gravity = -0.04f;
			Puff.Drag = 2.5f;
		}
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector Direction = (Up * 0.9f + Random.GetUnitVector() * 0.6f).GetSafeNormal();
			FParticle& Chip = AddParticle(EParticle::Chip, Point, Direction * Random.FRandRange(250.f, 650.f), Random.FRandRange(0.5f, 0.8f));
			Chip.StartSize = Random.FRandRange(1.4f, 2.8f);
			Chip.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Chip.Spin = Random.GetUnitVector() * Random.FRandRange(8.f, 20.f);
		}
		break;
	}
	case EImpactSurface::Target:
	{
		// The plated dummies ring: a hard starburst and a ping of light round the plate (the pass's hit pop, as on metal).
		Flash(FlashColor, 20.f, 20.f, 40.f, 0.08f);
		Sparks(10, TargetSparkColor, 30.f, 500.f, 1500.f);
		const FVfxTuning Tuning = FVfxTuning::Live();
		if (Tuning.ImpactScale > 0.f)
		{
			SpawnStarburst(Point, 34.f, TargetSparkColor, 26.f * Tuning.Glow, 0.07f, 6);
			SpawnShockRing(Point, Up, 4.f, 26.f, 3.f, TargetSparkColor, 10.f * Tuning.Glow, 0.16f, true);
		}
		break;
	}
	case EImpactSurface::Flesh:
	{
		Flash(IchorFlashColor, 7.f, 14.f, 26.f, 0.06f);
		const int32 Drops = bCritical ? 12 : 7;
		for (int32 Index = 0; Index < Drops; ++Index)
		{
			const FVector Direction = (Up + Random.GetUnitVector() * 0.7f - ShotDirection.GetSafeNormal() * 0.2f).GetSafeNormal();
			FParticle& Drop = AddParticle(EParticle::Droplet, Point, Direction * Random.FRandRange(200.f, bCritical ? 750.f : 550.f), Random.FRandRange(0.45f, 0.7f));
			Drop.StartSize = Random.FRandRange(1.6f, bCritical ? 3.6f : 2.8f);
			Drop.Spin = Random.GetUnitVector() * Random.FRandRange(4.f, 10.f);
		}
		for (int32 Index = 0; Index < (bCritical ? 2 : 1); ++Index)
		{
			FParticle& Puff = AddParticle(EParticle::Smoke, Point, Up * Random.FRandRange(30.f, 80.f), Random.FRandRange(0.3f, 0.45f));
			Puff.Color = IchorColor;
			Puff.Intensity = 0.65f;
			Puff.StartSize = 10.f;
			Puff.EndSize = bCritical ? 50.f : 36.f;
			Puff.Gravity = 0.15f;
			Puff.Drag = 3.f;
		}
		break;
	}
	}

	if (bCritical)
	{
		// Critical hits get a bright pop on top, whatever they hit.
		Flash(CritFlashColor, 28.f, 26.f, 60.f, 0.1f);
	}
}

void FWeaponFX::SpawnFlash(const FVector& Location, const FVector& Direction, float Scale)
{
	const FVector Along = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal();
	const float Size = FMath::Max(Scale, 0.1f);
	// A hard star on top (the pass: a round glow alone read as a soft blob), turned at random, a shot's worth.
	if (Size >= 0.5f)
	{
		SpawnStarburst(Location + Along * 6.f * Size, 40.f * Size, FlashColor, 22.f * FVfxTuning::Live().Glow, 0.06f, 7);
	}
	// The gun's own flash in round glows: a hot core at the muzzle and a wider burst a little ahead of it.
	FParticle& Core = AddParticle(EParticle::Flash, Location, FVector::ZeroVector, 0.06f);
	Core.Color = FlashColor;
	Core.Intensity = 30.f;
	Core.StartSize = 14.f * Size;
	Core.EndSize = 26.f * Size;
	Core.Gravity = 0.f;
	FParticle& Burst = AddParticle(EParticle::Flash, Location + Along * 12.f * Size, FVector::ZeroVector, 0.09f);
	Burst.Color = SparkColor;
	Burst.Intensity = 18.f;
	Burst.StartSize = 30.f * Size;
	Burst.EndSize = 64.f * Size;
	Burst.Gravity = 0.f;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const FVector Out = (Along + Random.GetUnitVector() * 0.35f).GetSafeNormal();
		FParticle& Spark = AddParticle(EParticle::Spark, Location, Out * Random.FRandRange(900.f, 1700.f), Random.FRandRange(0.08f, 0.18f));
		Spark.Color = SparkColor;
		Spark.Intensity = 22.f;
		Spark.StartSize = 1.f;
		Spark.Drag = 2.f;
		Spark.Gravity = 0.3f;
	}
	// A breath of smoke left hanging where it fired.
	FParticle& Puff = AddParticle(EParticle::Smoke, Location + Along * 20.f * Size, Along * 60.f, 0.9f);
	Puff.Color = DustColor;
	Puff.Intensity = 0.4f;
	Puff.StartSize = 12.f * Size;
	Puff.EndSize = 60.f * Size;
	Puff.Gravity = -0.03f;
	Puff.Drag = 2.5f;
}
