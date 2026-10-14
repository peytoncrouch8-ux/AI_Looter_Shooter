#include "Weapons/WeaponFX.h"
#include "Components/InstancedStaticMeshComponent.h"

// FWeaponFX's moving and drawing: every particle advanced (falling, dragging, tumbling, landing on its impact's floor)
// and drawn as instanced quads (turned to the camera) and lit chunks. The spawns are in WeaponFX.cpp and its topic files.

namespace
{
	constexpr float FXGravity = 980.f;
	/** A chunk lying on its floor goes after this long (seconds, at random between). */
	constexpr float FXRestMin = 0.9f;
	constexpr float FXRestMax = 2.f;
	/** Under this speed (cm/s), or after this many bounces, a landed chunk lies still. */
	constexpr float FXRestSpeed = 45.f;
	constexpr uint8 FXMostBounces = 3;

	/** Any unit vector at right angles to Direction. */
	FVector FXPerpendicular(const FVector& Direction)
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
			Facing = FXPerpendicular(Direction);
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
		bool bCountChanged = false;
		if (Instances->GetInstanceCount() != Transforms.Num())
		{
			bCountChanged = true;
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
		// Moved instances need no render state rebuild: the instance data tracks its own changes and sends them at the end of the
		// frame (FPrimitiveInstanceDataManager), and rebuilding the proxy every frame while any particle lives was the cost. A
		// changed count (a spark born or gone) still takes the full refresh.
		if (bCountChanged)
		{
			Instances->MarkRenderStateDirty();
		}
	}

	void AddGlowData(TArray<float>& Data, const FLinearColor& Color, float Intensity, float Shape)
	{
		Data.Append({ Color.R, Color.G, Color.B, Intensity, Shape });
	}
}

// ---------------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------------

void FWeaponFX::Tick(float DeltaSeconds, const FVector& CameraLocation)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	WaterRings.Tick(Dt);
	for (int32 Index = Particles.Num() - 1; Index >= 0; --Index)
	{
		FParticle& Particle = Particles[Index];
		Particle.Age += Dt;
		if (Particle.Age >= Particle.Life)
		{
			Particles.RemoveAtSwap(Index, 1, EAllowShrinking::No);
			continue;
		}
		if (Particle.bResting)
		{
			// Lying where it landed until it shrinks away.
			continue;
		}
		Particle.Velocity += FVector(0.f, 0.f, -FXGravity * Particle.Gravity * Dt);
		Particle.Velocity *= FMath::Exp(-Particle.Drag * Dt);
		Particle.Location += Particle.Velocity * Dt;
		if (!Particle.Spin.IsNearlyZero())
		{
			const float Angle = Particle.Spin.Size() * Dt;
			Particle.Rotation = FQuat(Particle.Spin.GetSafeNormal(), Angle) * Particle.Rotation;
		}
		if (Particle.bFloor)
		{
			LandOnFloor(Particle);
		}
	}
	Redraw(CameraLocation);
	Streaks.Reset();
}

void FWeaponFX::LandOnFloor(FParticle& Particle)
{
	const FVector Up(Particle.Floor.X, Particle.Floor.Y, Particle.Floor.Z);
	// A chunk comes to lie with its middle a little over the ground; a spark touches it.
	const float Lift = Particle.Type == EParticle::Spark ? 0.3f : Particle.StartSize * 0.25f;
	const double Height = Particle.Floor.PlaneDot(Particle.Location);
	if (Height >= Lift)
	{
		return;
	}
	// A drop (water, gel) splats into the ground: gone.
	const bool bDrop = Particle.Type == EParticle::Droplet
		|| (Particle.Type == EParticle::Bit && BitSets.IsValidIndex(Particle.BitSet) && BitSets[Particle.BitSet].Shape == EBitShape::Round);
	if (bDrop)
	{
		Particle.Life = Particle.Age;
		return;
	}
	Particle.Location += Up * (Lift - Height);
	const double Into = FVector::DotProduct(Particle.Velocity, Up);
	const bool bSpark = Particle.Type == EParticle::Spark;
	if (Into < 0.0)
	{
		// A bounce: it skids on along the ground and hops back up a little (sparks keep more, skittering on).
		const FVector Across = Particle.Velocity - Up * Into;
		Particle.Velocity = Across * (bSpark ? 0.7f : 0.45f) - Up * (Into * (bSpark ? 0.4f : 0.3f));
		Particle.Spin *= 0.5f;
		++Particle.Bounces;
	}
	if (!bSpark && (Particle.Bounces >= FXMostBounces || Particle.Velocity.SizeSquared() < FMath::Square(FXRestSpeed)))
	{
		// Spent: it lies flat where it fell a moment or two, then shrinks away.
		Particle.bResting = true;
		Particle.Velocity = FVector::ZeroVector;
		Particle.Spin = FVector::ZeroVector;
		FVector Along = Particle.Rotation.GetAxisX();
		Along -= Up * FVector::DotProduct(Along, Up);
		if (!Along.Normalize())
		{
			Along = FXPerpendicular(Up);
		}
		Particle.Rotation = FRotationMatrix::MakeFromZX(Up, Along).ToQuat();
		Particle.Life = FMath::Max(Particle.Life, Particle.Age + Random.FRandRange(FXRestMin, FXRestMax));
	}
}

int32 FWeaponFX::NumResting() const
{
	int32 Count = 0;
	for (const FParticle& Particle : Particles)
	{
		Count += Particle.bResting ? 1 : 0;
	}
	return Count;
}

float FWeaponFX::LowestOverFloor() const
{
	bool bAny = false;
	double Lowest = 0.0;
	for (const FParticle& Particle : Particles)
	{
		// Not a drop that just splatted (gone next update) and not a spark (it only skitters).
		if (Particle.bFloor && Particle.Type != EParticle::Spark && Particle.Age < Particle.Life)
		{
			const double Height = Particle.Floor.PlaneDot(Particle.Location);
			Lowest = bAny ? FMath::Min(Lowest, Height) : Height;
			bAny = true;
		}
	}
	return static_cast<float>(Lowest);
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

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

	// Chunks shrink away over the end of their life: the last fifth of a short one, the last 0.4 s of one that lay a while.
	auto Shrink = [](const FParticle& Particle)
	{
		const float Window = FMath::Max(FMath::Min(Particle.Life * 0.2f, 0.4f), 0.01f);
		return FMath::Clamp((Particle.Life - Particle.Age) / Window, 0.f, 1.f);
	};

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
		case EParticle::Streak:
		{
			// A ricochet's tracer: it grows from where the round struck to its full length (EndSize) as it flies off.
			const float Speed = Particle.Velocity.Size();
			const float Length = FMath::Min(Speed * Particle.Age + 2.f, Particle.EndSize);
			const FVector Tail = Particle.Location - Particle.Velocity.GetSafeNormal() * Length;
			GlowTransforms.Add(StreakTransform(Tail, Particle.Location, Particle.StartSize, Camera));
			AddGlowData(GlowData, Particle.Color, Particle.Intensity * FMath::Pow(1.f - T, 1.2f), 0.f);
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
		case EParticle::Thread:
		{
			// A soft line of smoke along a tracer's path, widening as it thins; faded where it passes near the eye.
			const FVector Tail = Particle.Location - Particle.Extent;
			const FVector Head = Particle.Location + Particle.Extent;
			const float Width = FMath::Lerp(Particle.StartSize, Particle.EndSize, FMath::Sqrt(T));
			SmokeTransforms.Add(StreakTransform(Tail, Head, Width, Camera));
			const float Near = FMath::Clamp((static_cast<float>(FMath::PointDistToSegment(Camera, Tail, Head)) - 30.f) / 90.f, 0.f, 1.f);
			const float Opacity = Particle.Intensity * FMath::Pow(1.f - T, 1.4f) * FMath::Clamp(T * 8.f, 0.f, 1.f) * Near;
			SmokeData.Append({ Particle.Color.R, Particle.Color.G, Particle.Color.B, Opacity });
			break;
		}
		case EParticle::Chip:
		case EParticle::Droplet:
		{
			// The engine cube and sphere are 100 across.
			const float Size = Particle.StartSize * Shrink(Particle) / 100.f;
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
			// Round bits (gel, water) stretch along their flight like droplets; splinters (wood, straw) are long and thin; the
			// rest are thin plates (chitin, chips of paint and glass). Splinters and plates tumble.
			const float Size = Particle.StartSize * Shrink(Particle) / 100.f;
			const EBitShape Shape = BitSets[Particle.BitSet].Shape;
			const bool bRound = Shape == EBitShape::Round;
			const FVector Scale = bRound ? FVector(Size * 1.35f, Size, Size * 0.9f)
				: (Shape == EBitShape::Splinter ? FVector(Size * 2.4f, Size * 0.3f, Size * 0.22f) : FVector(Size, Size * 0.8f, Size * 0.28f));
			const FQuat Rotation = bRound && !Particle.Velocity.IsNearlyZero() ? FRotationMatrix::MakeFromX(Particle.Velocity).ToQuat() : Particle.Rotation;
			BitTransforms[Particle.BitSet].Add(FTransform(Rotation, Particle.Location, Scale));
			break;
		}
		}
	}

	// Rings and wakes on the water: flat dashes of the same soft smoke quad (WaterRings.h).
	WaterRings.Draw(Camera, SmokeTransforms, SmokeData);

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
