#include "Weapons/WeaponFX.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformTime.h"

// FWeaponFX's shapes (the VFX pass, Saved/Vfx/PLAN.md): shock rings, fireballs and starbursts, one quad each on M_FX_Shape
// (Tools/Unreal/build_vfx_materials.py), or drawn with the glows' streaks and discs until it's in the game (WeaponFXDraw.cpp
// draws both); and how much of a look is thrown by distance and in a stream of fire. Spawning only.

void FWeaponFX::SpawnShockRing(const FVector& Center, const FVector& Axis, float StartRadius, float EndRadius, float Width,
	const FLinearColor& Color, float Intensity, float Life, bool bFacing)
{
	if (Intensity <= 0.f || Life <= 0.f || FMath::Max(StartRadius, EndRadius) <= 0.f)
	{
		return;
	}
	FParticle& Ring = AddParticle(EParticle::Ring, Center, FVector::ZeroVector, Life);
	Ring.Color = Color;
	Ring.Intensity = Intensity;
	Ring.StartSize = FMath::Max(StartRadius, 0.f);
	Ring.EndSize = FMath::Max(EndRadius, 0.f);
	Ring.Width = FMath::Max(Width, 0.5f);
	Ring.Gravity = 0.f;
	Ring.bFacing = bFacing || Axis.IsNearlyZero();
	Ring.Extent = Axis.IsNearlyZero() ? FVector::UpVector : Axis.GetSafeNormal();
	// A ring is never landed: it lies where it was laid (AddParticle gave it no floor: it's neither a chunk nor a spark).
}

void FWeaponFX::SpawnFireball(const FVector& Location, const FVector& Velocity, float StartSize, float EndSize, const FLinearColor& Color,
	float Intensity, float Life)
{
	if (Intensity <= 0.f || Life <= 0.f)
	{
		return;
	}
	FParticle& Ball = AddParticle(EParticle::Fireball, Location, Velocity, Life);
	Ball.Color = Color;
	Ball.Intensity = Intensity;
	Ball.StartSize = FMath::Max(StartSize, 1.f);
	Ball.EndSize = FMath::Max(EndSize, 1.f);
	// Hot air: it rises a little and loses its push fast.
	Ball.Gravity = -0.08f;
	Ball.Drag = 3.f;
}

void FWeaponFX::SpawnStarburst(const FVector& Location, float Size, const FLinearColor& Color, float Intensity, float Life, int32 Spikes)
{
	if (Intensity <= 0.f || Life <= 0.f || Size <= 0.f)
	{
		return;
	}
	FParticle& Star = AddParticle(EParticle::Star, Location, FVector::ZeroVector, Life);
	Star.Color = Color;
	Star.Intensity = Intensity;
	// It snaps open: most of its size at once, the rest as it fades.
	Star.StartSize = Size * 0.7f;
	Star.EndSize = Size;
	Star.Width = static_cast<float>(FMath::Clamp(Spikes, 3, 16));
	Star.Gravity = 0.f;
}

float FWeaponFX::DetailAt(const FVector& Where) const
{
	// Before the first frame is drawn (a test spawning at once) nothing is thinned.
	if (!bHasCamera)
	{
		return 1.f;
	}
	return VfxRules::DetailAt(static_cast<float>(FVector::Dist(Where, LastCamera)), FVfxTuning::Live());
}

float FWeaponFX::StreamDetailFor(const FVector& Location)
{
	const FVfxTuning Tuning = FVfxTuning::Live();
	const UWorld* World = Owner.IsValid() ? Owner->GetWorld() : nullptr;
	const double Now = World ? World->GetTimeSeconds() : FPlatformTime::Seconds();
	return VfxRules::StreamDetail(HitStream.Note(Location, Now, Tuning.StreamWindow, Tuning.StreamReach));
}

int32 FWeaponFX::NumShapes() const
{
	int32 Count = 0;
	for (const FParticle& Particle : Particles)
	{
		Count += (Particle.Type == EParticle::Ring || Particle.Type == EParticle::Fireball || Particle.Type == EParticle::Star) ? 1 : 0;
	}
	return Count;
}
