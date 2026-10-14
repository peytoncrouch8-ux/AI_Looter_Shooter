#include "Weapons/WeaponFX.h"

// FWeaponFX's effects of a shot itself: a round glancing off (a ricochet you can see), a hot muzzle's smoke, and the faint
// smoke thread behind a tracer. Spawning only; WeaponFXDraw.cpp moves and draws every particle.

namespace
{
	// Glows: only a hue (each is multiplied by its strength).
	const FLinearColor ShotRicochetHeat(1.f, 0.92f, 0.75f);
	const FLinearColor ShotRicochetTracer(1.f, 0.72f, 0.38f);
	const FLinearColor ShotRicochetSpark(1.f, 0.6f, 0.22f);
	/** Gun smoke as the unlit smoke material shows it: pale, a little cool. */
	const FLinearColor ShotGunSmoke(0.68f, 0.68f, 0.68f);
	/** A tracer's thread: thinner and paler still. */
	const FLinearColor ShotThreadSmoke(0.74f, 0.73f, 0.71f);

	/** How far a ricochet's tracer reaches at its longest (cm), and how fast it goes (cm/s). */
	constexpr float ShotRicochetLength = 140.f;
	constexpr float ShotRicochetMinSpeed = 7000.f;
	constexpr float ShotRicochetMaxSpeed = 10000.f;
}

void FWeaponFX::SpawnRicochet(const FVector& Location, const FVector& Direction, float Scale)
{
	const FVector Off = Direction.IsNearlyZero() ? FVector::UpVector : Direction.GetSafeNormal();
	const float Size = FMath::Clamp(Scale, 0.4f, 1.6f);
	// A white-hot pinpoint where it struck...
	FParticle& Heat = AddParticle(EParticle::Flash, Location, FVector::ZeroVector, 0.05f);
	Heat.Color = ShotRicochetHeat;
	Heat.Intensity = 34.f;
	Heat.StartSize = 6.f * Size;
	Heat.EndSize = 18.f * Size;
	Heat.Gravity = 0.f;
	// ...the round itself going off at the bounce, a short tracer that grows to its length as it flies...
	FParticle& Tracer = AddParticle(EParticle::Streak, Location, Off * Random.FRandRange(ShotRicochetMinSpeed, ShotRicochetMaxSpeed),
		Random.FRandRange(0.08f, 0.14f));
	Tracer.Color = ShotRicochetTracer;
	Tracer.Intensity = 30.f;
	Tracer.StartSize = 1.6f * Size;
	Tracer.EndSize = ShotRicochetLength * Size;
	Tracer.Gravity = 0.3f;
	// ...and a few sparks thrown with it.
	for (int32 Index = 0; Index < 5; ++Index)
	{
		const FVector Out = (Off + Random.GetUnitVector() * 0.3f).GetSafeNormal();
		FParticle& Spark = AddParticle(EParticle::Spark, Location, Out * Random.FRandRange(900.f, 2000.f), Random.FRandRange(0.1f, 0.2f));
		Spark.Color = ShotRicochetSpark;
		Spark.Intensity = 26.f;
		Spark.StartSize = 1.f;
		Spark.Drag = 2.f;
		Spark.Gravity = 0.4f;
	}
}

void FWeaponFX::SpawnWisp(const FVector& Location, const FVector& Velocity, float Opacity, float StartSize, float EndSize, float Life)
{
	FParticle& Wisp = AddParticle(EParticle::Smoke, Location, Velocity, Life);
	Wisp.Color = ShotGunSmoke;
	Wisp.Intensity = Opacity;
	Wisp.StartSize = StartSize;
	Wisp.EndSize = EndSize;
	// Warm smoke rises slowly and loses its push quickly, so it curls up off the barrel instead of shooting away.
	Wisp.Gravity = -0.06f;
	Wisp.Drag = 1.6f;
}

void FWeaponFX::AddThread(const FVector& Tail, const FVector& Head, float Width, float Opacity, float Life)
{
	// Spreading as it thins, drifting up a little as it goes.
	FParticle& Line = AddParticle(EParticle::Thread, (Tail + Head) * 0.5, FVector(0.0, 0.0, 6.0), Life);
	Line.Extent = (Head - Tail) * 0.5;
	Line.Color = ShotThreadSmoke;
	Line.Intensity = Opacity;
	Line.StartSize = Width;
	Line.EndSize = Width * 3.f;
	Line.Gravity = 0.f;
	Line.Drag = 0.5f;
}
