#include "Weapons/WeaponFX.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	const TCHAR* GlowMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Glow.M_FX_Glow");
	const TCHAR* SmokeMaterialPath = TEXT("/Game/Weapons/FX/M_FX_Smoke.M_FX_Smoke");
	/** Used until M_FX_Glow exists: the environment's glow (dimmer seen end-on). */
	const TCHAR* FallbackGlowPath = TEXT("/Game/Environment/Materials/M_StylizedGlow.M_StylizedGlow");
	const TCHAR* SurfaceMaterialPath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");

	constexpr int32 GlowFloats = 5;  // R, G, B, intensity, shape (0 = streak, 1 = round)
	constexpr int32 SmokeFloats = 4; // R, G, B, opacity
	constexpr int32 MaxParticles = 900;
	constexpr float Gravity = 980.f;

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

	/** Any unit vector at right angles to Direction. */
	FVector AnyPerpendicular(const FVector& Direction)
	{
		const FVector Other = FMath::Abs(Direction.Z) < 0.9f ? FVector::UpVector : FVector::ForwardVector;
		return FVector::CrossProduct(Direction, Other).GetSafeNormal();
	}

	/** A quad along Tail -> Head, turned about that line to face the camera. The engine plane is 100 x 100 in XY, facing +Z. */
	FTransform StreakTransform(const FVector& Tail, const FVector& Head, float Width, const FVector& Camera)
	{
		const FVector Axis = Head - Tail;
		const float Length = Axis.Size();
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
	FTransform SpriteTransform(const FVector& Location, float Size, float Roll, const FVector& Camera)
	{
		FVector Facing = Camera - Location;
		if (!Facing.Normalize())
		{
			Facing = FVector::UpVector;
		}
		const FQuat Rotation = FRotationMatrix::MakeFromZ(Facing).ToQuat() * FQuat(FVector::UpVector, Roll);
		return FTransform(Rotation, Location, FVector(Size / 100.f, Size / 100.f, 1.f));
	}

	/** Replaces a component's instances with these (moved in place when the count is unchanged). */
	void Draw(UInstancedStaticMeshComponent* Instances, const TArray<FTransform>& Transforms, const TArray<float>& CustomData)
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
		}
		else if (Transforms.Num() > 0)
		{
			Instances->BatchUpdateInstancesTransforms(0, Transforms, /*bWorldSpace*/ true, /*bMarkRenderStateDirty*/ false, /*bTeleport*/ true);
		}
		if (Transforms.Num() > 0 && CustomData.Num() == Transforms.Num() * Instances->NumCustomDataFloats)
		{
			Instances->SetCustomData(0, Transforms.Num() - 1, CustomData, false);
		}
		Instances->MarkRenderStateDirty();
	}

	void AddGlowData(TArray<float>& Data, const FLinearColor& Color, float Intensity, float Shape)
	{
		Data.Append({ Color.R, Color.G, Color.B, Intensity, Shape });
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

	Glows = MakeInstances(Actor, TEXT("Glows"), Plane, GlowMaterial, GlowFloats);
	// Without its material, smoke would draw as solid squares: leave it out instead.
	Smoke = SmokeMaterial ? MakeInstances(Actor, TEXT("Smoke"), Plane, SmokeMaterial, SmokeFloats) : nullptr;
	Chips = MakeInstances(Actor, TEXT("Chips"), Cube, MakeSurface(Actor, StoneColor, 0.f), 0);
	Droplets = MakeInstances(Actor, TEXT("Droplets"), Sphere, MakeSurface(Actor, IchorColor, 0.35f), 0);
}

void FWeaponFX::Shutdown()
{
	if (AActor* Actor = Owner.Get())
	{
		Actor->Destroy();
	}
	Owner.Reset();
	BitSets.Reset();
	Particles.Reset();
	Streaks.Reset();
}

uint8 FWeaponFX::BitSetFor(bool bRound, const FLinearColor& Color, float Glow)
{
	const FColor Key = Color.ToFColor(false);
	// Sets whose components went with an old effects actor are made again.
	BitSets.RemoveAll([](const FBitSet& Set) { return !Set.Instances.IsValid(); });
	for (int32 Index = 0; Index < BitSets.Num(); ++Index)
	{
		if (BitSets[Index].Key == Key && BitSets[Index].bRound == bRound)
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
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, bRound ? TEXT("/Engine/BasicShapes/Sphere.Sphere") : TEXT("/Engine/BasicShapes/Cube.Cube"));
	const FString Name = FString::Printf(TEXT("Bits%d"), BitSets.Num());
	FBitSet& Set = BitSets.AddDefaulted_GetRef();
	Set.Instances = MakeInstances(Actor, *Name, Mesh, MakeSurface(Actor, Color, Glow), 0);
	Set.Key = Key;
	Set.bRound = bRound;
	return static_cast<uint8>(BitSets.Num() - 1);
}

bool FWeaponFX::IsIdle() const
{
	return Particles.IsEmpty() && Streaks.IsEmpty() && !bDrawnAnything;
}

// ---------------------------------------------------------------------------
// Spawning
// ---------------------------------------------------------------------------

void FWeaponFX::AddStreak(const FVector& Tail, const FVector& Head, float Width, const FLinearColor& Color, float Intensity)
{
	Streaks.Add({ Tail, Head, Width, Color, Intensity });
}

FWeaponFX::FParticle& FWeaponFX::AddParticle(EParticle Type, const FVector& Location, const FVector& Velocity, float Life)
{
	if (Particles.Num() >= MaxParticles)
	{
		Particles.RemoveAt(0, 1, EAllowShrinking::No); // the oldest is nearly gone anyway
	}
	FParticle& Particle = Particles.AddDefaulted_GetRef();
	Particle.Type = Type;
	Particle.Location = Location;
	Particle.Velocity = Velocity;
	Particle.Life = FMath::Max(Life, 0.01f);
	Particle.Roll = Random.FRandRange(0.f, 2.f * UE_PI);
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
		Flash(FlashColor, 20.f, 20.f, 40.f, 0.08f);
		Sparks(10, TargetSparkColor, 30.f, 500.f, 1500.f);
		break;
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

// ---------------------------------------------------------------------------
// Simulation and drawing
// ---------------------------------------------------------------------------

void FWeaponFX::Tick(float DeltaSeconds, const FVector& CameraLocation)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	for (int32 Index = Particles.Num() - 1; Index >= 0; --Index)
	{
		FParticle& Particle = Particles[Index];
		Particle.Age += Dt;
		if (Particle.Age >= Particle.Life)
		{
			Particles.RemoveAtSwap(Index, 1, EAllowShrinking::No);
			continue;
		}
		Particle.Velocity += FVector(0.f, 0.f, -Gravity * Particle.Gravity * Dt);
		Particle.Velocity *= FMath::Exp(-Particle.Drag * Dt);
		Particle.Location += Particle.Velocity * Dt;
		if (!Particle.Spin.IsNearlyZero())
		{
			const float Angle = Particle.Spin.Size() * Dt;
			Particle.Rotation = FQuat(Particle.Spin.GetSafeNormal(), Angle) * Particle.Rotation;
		}
	}
	Redraw(CameraLocation);
	Streaks.Reset();
}

void FWeaponFX::Redraw(const FVector& Camera)
{
	TArray<FTransform> GlowTransforms, SmokeTransforms, ChipTransforms, DropTransforms;
	TArray<float> GlowData, SmokeData;
	TArray<TArray<FTransform>, TInlineAllocator<MaxBitSets>> BitTransforms;
	BitTransforms.SetNum(BitSets.Num());

	for (const FStreak& Streak : Streaks)
	{
		GlowTransforms.Add(StreakTransform(Streak.Tail, Streak.Head, Streak.Width, Camera));
		AddGlowData(GlowData, Streak.Color, Streak.Intensity, 0.f);
	}

	for (const FParticle& Particle : Particles)
	{
		const float T = Particle.Age / Particle.Life;
		switch (Particle.Type)
		{
		case EParticle::Spark:
		{
			// A short streak trailing behind the spark, fading as it cools.
			const float Length = FMath::Clamp(Particle.Velocity.Size() * 0.025f, 3.f, 40.f);
			const FVector Tail = Particle.Location - Particle.Velocity.GetSafeNormal() * Length;
			GlowTransforms.Add(StreakTransform(Tail, Particle.Location, Particle.StartSize, Camera));
			AddGlowData(GlowData, Particle.Color, Particle.Intensity * FMath::Pow(1.f - T, 1.5f), 0.f);
			break;
		}
		case EParticle::Flash:
		{
			const float Size = FMath::Lerp(Particle.StartSize, Particle.EndSize, FMath::Sqrt(T));
			GlowTransforms.Add(SpriteTransform(Particle.Location, Size, Particle.Roll, Camera));
			AddGlowData(GlowData, Particle.Color, Particle.Intensity * FMath::Square(1.f - T), 1.f);
			break;
		}
		case EParticle::Smoke:
		{
			const float Size = FMath::Lerp(Particle.StartSize, Particle.EndSize, 1.f - FMath::Square(1.f - T));
			SmokeTransforms.Add(SpriteTransform(Particle.Location, Size, Particle.Roll, Camera));
			// Thinned as the camera gets into it: a puff drifting past the eye (a slide's dust) would otherwise fill the view.
			const float Near = FMath::Clamp((static_cast<float>(FVector::Dist(Particle.Location, Camera)) - Size * 0.25f) / 40.f, 0.f, 1.f);
			const float Opacity = Particle.Intensity * FMath::Pow(1.f - T, 1.5f) * FMath::Clamp(T * 12.f, 0.f, 1.f) * Near;
			SmokeData.Append({ Particle.Color.R, Particle.Color.G, Particle.Color.B, Opacity });
			break;
		}
		case EParticle::Chip:
		case EParticle::Droplet:
		{
			// Shrink away over the last fifth of their life. The engine cube and sphere are 100 across.
			const float Size = Particle.StartSize * FMath::Clamp((1.f - T) * 5.f, 0.f, 1.f) / 100.f;
			const FVector Scale = Particle.Type == EParticle::Droplet ? FVector(Size * 1.3f, Size, Size) : FVector(Size, Size * 0.7f, Size * 0.5f);
			// Droplets stretch along their flight.
			const FQuat Rotation = Particle.Type == EParticle::Droplet && !Particle.Velocity.IsNearlyZero()
				? FRotationMatrix::MakeFromX(Particle.Velocity).ToQuat() : Particle.Rotation;
			(Particle.Type == EParticle::Chip ? ChipTransforms : DropTransforms).Add(FTransform(Rotation, Particle.Location, Scale));
			break;
		}
		case EParticle::Bit:
		{
			if (!BitTransforms.IsValidIndex(Particle.BitSet))
			{
				break;
			}
			// Round bits (gel) stretch along their flight like droplets; the rest are thin plates (chitin) that tumble.
			const float Size = Particle.StartSize * FMath::Clamp((1.f - T) * 5.f, 0.f, 1.f) / 100.f;
			const bool bRound = BitSets[Particle.BitSet].bRound;
			const FVector Scale = bRound ? FVector(Size * 1.35f, Size, Size * 0.9f) : FVector(Size, Size * 0.8f, Size * 0.28f);
			const FQuat Rotation = bRound && !Particle.Velocity.IsNearlyZero() ? FRotationMatrix::MakeFromX(Particle.Velocity).ToQuat() : Particle.Rotation;
			BitTransforms[Particle.BitSet].Add(FTransform(Rotation, Particle.Location, Scale));
			break;
		}
		}
	}

	bool bAnything = !GlowTransforms.IsEmpty() || !SmokeTransforms.IsEmpty() || !ChipTransforms.IsEmpty() || !DropTransforms.IsEmpty();
	for (const TArray<FTransform>& Set : BitTransforms)
	{
		bAnything |= !Set.IsEmpty();
	}
	if (bAnything || bDrawnAnything)
	{
		Draw(Glows.Get(), GlowTransforms, GlowData);
		Draw(Smoke.Get(), SmokeTransforms, SmokeData);
		Draw(Chips.Get(), ChipTransforms, {});
		Draw(Droplets.Get(), DropTransforms, {});
		for (int32 Index = 0; Index < BitSets.Num(); ++Index)
		{
			Draw(BitSets[Index].Instances.Get(), BitTransforms[Index], {});
		}
	}
	bDrawnAnything = bAnything;
}
