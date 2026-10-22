#include "Weapons/WeaponFX.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"

// FWeaponFX's effects of a shot itself: a round glancing off (a ricochet you can see), a hot muzzle's smoke, and the faint
// smoke thread behind a tracer. Spawning only; WeaponFXDraw.cpp moves and draws every particle.
// The VFX pass (Saved/Vfx/PLAN.md): a ricochet's glint is a starburst; SpawnMuzzleBurst is what a shot's muzzle throws round
// its flash by the gun's kind (VfxRules::MuzzleLook: powder sparks, smoke left hanging, a brake's side jets, a revolver's
// cylinder spit, a heavy gun's shock ring), for AWeaponBase to call beside its flash quads (GUNS-CODE's files).

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

	/** A shot's powder sparks and flash colours (the flash quads' own: AWeaponBase's FlashStarColor family). */
	const FLinearColor MuzzleSparkColor(1.f, 0.6f, 0.22f);
	const FLinearColor MuzzleHotColor(1.f, 0.82f, 0.55f);
	/** Gun smoke for a burst's breath, a little warmer than the wisp's. */
	const FLinearColor MuzzleSmoke(0.66f, 0.65f, 0.63f);
	/** In the player's own view, smoke starts this far out along the barrel (cm): nearer, it fills the screen. */
	constexpr float FirstPersonSmokeOut = 45.f;
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
	// (The pass) a hard glint where it struck: a small starburst reads the bounce at range.
	SpawnStarburst(Location, 22.f * Size, ShotRicochetHeat, 24.f * FVfxTuning::Live().Glow, 0.05f, 4);
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

void FWeaponFX::SpawnMuzzleBurst(const FVfxMuzzle& Muzzle)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.MuzzleScale <= 0.f)
	{
		return;
	}
	const FVfxMuzzleLook Look = VfxRules::MuzzleLook(Muzzle.Kind);
	const FVector Forward = Muzzle.Forward.IsNearlyZero() ? FVector::ForwardVector : Muzzle.Forward.GetSafeNormal();
	FVector Up = Muzzle.Up - Forward * FVector::DotProduct(Muzzle.Up, Forward);
	if (!Up.Normalize())
	{
		FVector Unused;
		Forward.FindBestAxisVectors(Up, Unused);
	}
	const FVector Side = FVector::CrossProduct(Forward, Up);
	const float Scale = FMath::Max(Muzzle.Scale, 0.2f) * Vfx.MuzzleScale;
	const FVector From = Muzzle.Location;
	// A fast gun throws its sparks and smoke on every so many rounds of a burst: every round's would be a cloud.
	const bool bThrows = Look.Every <= 1 || Muzzle.InBurst % Look.Every == 0;

	// Unburnt powder: hot sparks thrown out with the shot in a cone.
	if (bThrows)
	{
		for (int32 Index = 0; Index < Look.Sparks; ++Index)
		{
			const FVector Out = (Forward + Random.GetUnitVector() * 0.3f).GetSafeNormal();
			FParticle& Spark = AddParticle(EParticle::Spark, From, Out * Random.FRandRange(900.f, 1800.f), Random.FRandRange(0.08f, 0.18f));
			Spark.Color = MuzzleSparkColor;
			Spark.Intensity = 20.f * Vfx.Glow;
			Spark.StartSize = Random.FRandRange(0.8f, 1.2f);
			Spark.Drag = 2.f;
			Spark.Gravity = 0.3f;
		}
	}
	// A muzzle brake's ports: two flat jets of flame blown out to the sides, and their smoke rolling away sideways.
	if (Look.SideJets > 0.f)
	{
		for (const float Sign : { -1.f, 1.f })
		{
			const FVector Out = (Side * Sign + Forward * 0.25f).GetSafeNormal();
			FParticle& Jet = AddParticle(EParticle::Streak, From - Forward * 4.f * Scale, Out * 2400.f * Scale, 0.05f);
			Jet.Color = MuzzleHotColor;
			Jet.Intensity = 22.f * Vfx.Glow;
			Jet.StartSize = 7.f * Scale;
			Jet.EndSize = Look.SideJets * Scale;
			Jet.Gravity = 0.f;
			Jet.Drag = 18.f;
			if (!Muzzle.bFirstPerson)
			{
				FParticle& Puff = AddParticle(EParticle::Smoke, From + Out * 12.f * Scale, Out * 260.f * Scale + Up * 30.f, Random.FRandRange(0.8f, 1.2f));
				Puff.Color = MuzzleSmoke * Muzzle.DustLight;
				Puff.Intensity = 0.32f * Vfx.Smoke;
				Puff.StartSize = 10.f * Scale;
				Puff.EndSize = Look.SmokeSize * 0.9f * Scale;
				Puff.Gravity = -0.05f;
				Puff.Drag = 3.5f;
			}
		}
	}
	// A revolver's cylinder gap: a spit of flame to each side, a hand's width back from the muzzle.
	if (Look.GapSpit > 0.f)
	{
		const FVector Gap = From - Forward * 16.f * Scale;
		for (const float Sign : { -1.f, 1.f })
		{
			FParticle& Spit = AddParticle(EParticle::Flash, Gap + Side * (Sign * 2.f * Scale), Side * (Sign * 120.f), 0.04f);
			Spit.Color = MuzzleHotColor;
			Spit.Intensity = 16.f * Vfx.Glow;
			Spit.StartSize = Look.GapSpit * 0.5f * Scale;
			Spit.EndSize = Look.GapSpit * Scale;
			Spit.Gravity = 0.f;
		}
	}
	// Smoke left hanging where it fired, drifting on along the shot and rising as it spreads (clear of the eye in first person).
	if (bThrows)
	{
		const float Out = Muzzle.bFirstPerson ? FirstPersonSmokeOut : 10.f * Scale;
		for (int32 Index = 0; Index < Look.SmokePuffs; ++Index)
		{
			const float Along = Out + static_cast<float>(Index) * 8.f * Scale;
			FParticle& Puff = AddParticle(EParticle::Smoke, From + Forward * Along, Forward * Random.FRandRange(90.f, 220.f) * Scale + Up * 25.f
				+ Random.GetUnitVector() * 20.f, Random.FRandRange(0.8f, 1.4f));
			Puff.Color = MuzzleSmoke * Muzzle.DustLight;
			Puff.Intensity = (Muzzle.bFirstPerson ? 0.2f : 0.34f) * Vfx.Smoke;
			Puff.StartSize = Look.SmokeSize * 0.3f * Scale;
			Puff.EndSize = Look.SmokeSize * Scale * Random.FRandRange(0.85f, 1.15f);
			Puff.Gravity = -0.06f;
			Puff.Drag = 2.6f;
		}
	}
	// A heavy gun's blast ring: smoke thrown out round the muzzle square to the barrel, and a faint ring of hot air with it.
	// Left out in the player's own view, where it would be a cloud over the sights.
	if (Look.Ring > 0.f && !Muzzle.bFirstPerson)
	{
		constexpr int32 RingPuffs = 8;
		const FVector Center = From + Forward * 12.f * Scale;
		for (int32 Index = 0; Index < RingPuffs; ++Index)
		{
			const float Angle = UE_TWO_PI * (static_cast<float>(Index) + Random.FRandRange(-0.2f, 0.2f)) / RingPuffs;
			const FVector Out = Up * FMath::Cos(Angle) + Side * FMath::Sin(Angle);
			FParticle& Puff = AddParticle(EParticle::Smoke, Center + Out * 8.f * Scale, (Out * Look.Ring * 3.f + Forward * 120.f) * Scale,
				Random.FRandRange(0.7f, 1.1f));
			Puff.Color = MuzzleSmoke * Muzzle.DustLight;
			Puff.Intensity = 0.3f * Vfx.Smoke;
			Puff.StartSize = 12.f * Scale;
			Puff.EndSize = Look.SmokeSize * 0.8f * Scale;
			Puff.Gravity = -0.03f;
			Puff.Drag = 5.f;
		}
		SpawnShockRing(Center, Forward, 10.f * Scale, Look.Ring * Scale, 6.f * Scale, MuzzleHotColor, 5.f * Vfx.Glow, 0.16f);
	}
}
