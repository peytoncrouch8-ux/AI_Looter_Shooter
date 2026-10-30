#include "Weapons/WeaponFX.h"

// FWeaponFX's moving (out of WeaponFXDraw.cpp, which draws): every particle advanced (falling, dragging, tumbling, landing on
// its impact's floor: a bounce, a skitter, then it lies still a moment), then redrawn.

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
}

// ---------------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------------

void FWeaponFX::Tick(float DeltaSeconds, const FVector& CameraLocation)
{
	const float Dt = FMath::Clamp(DeltaSeconds, 0.f, 0.1f);
	// Where looks are seen from (DetailAt thins what's thrown far off).
	LastCamera = CameraLocation;
	bHasCamera = true;
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
		if (Particle.bResting || Particle.Age < 0.f)
		{
			// Lying where it landed until it shrinks away; or not begun yet (a blast's late puff: SpawnFlame's delay).
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
