#include "Weapons/WeaponFX.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"

// FWeaponFX's looks for the Unpaid's failed-lunge shriek (2026-10-29; Creatures/UnpaidCreatureShriek.cpp plays them, frame by
// frame, at its own rates): cold soul motes drawn into its mouth through the tell, the scream's cone of spectral wisps and soul
// streaks, the ground wave's lifted dust, grass and leaves (UUnpaidPackSubsystem's wave), the embers and soot its coal flings,
// and a breath of frost. Spawning only (WeaponFXDraw.cpp moves and draws); the existing quads, sets and materials, no lights.
//
// Budget: each call thins by DetailAt (x0.35 at 60 m) and Looter.Vfx.Kill.Scale, and holds to a small share of the pool
// (Looter.Vfx.MaxParticles) and a hard cap, so three shrieking at once stay well under a quarter of it.

namespace
{
	/** One call may use at most this share of the particle pool, and never more than this many pieces. */
	constexpr float PoolShare = 0.02f;
	constexpr int32 PieceCap = 16;

	/** The soul's cold light (motes, wisps, streaks), and the frost's breath. */
	const FLinearColor SoulCold(0.72f, 0.86f, 1.f);
	const FLinearColor FrostBreath(0.82f, 0.9f, 1.f);
	/** Embers' white-hot heart; the soot a coal throws (unlit smoke: the colour itself). */
	const FLinearColor EmberHeat(1.f, 0.88f, 0.66f);
	const FLinearColor CoalSoot(0.09f, 0.08f, 0.075f);
	/** Ground dust as full sun shows it (times the light), dry grass blades, a few autumn leaves. */
	const FLinearColor GroundDust(0.42f, 0.36f, 0.28f);
	const FLinearColor GrassBlade(0.24f, 0.31f, 0.11f);
	const FLinearColor LeafAmber(0.46f, 0.27f, 0.08f);

	int32 Allowed(int32 Wanted, int32 Pool)
	{
		if (Wanted <= 0)
		{
			return 0;
		}
		const int32 Share = FMath::Max(1, FMath::RoundToInt32(static_cast<float>(Pool) * PoolShare));
		return FMath::Min3(Wanted, PieceCap, Share);
	}

	/** A way across Way (flat), for swirls. */
	FVector AcrossOf(const FVector& Way)
	{
		const FVector Across = FVector::CrossProduct(Way, FVector::UpVector).GetSafeNormal();
		return Across.IsNearlyZero() ? FVector::RightVector : Across;
	}
}

void FWeaponFX::SpawnSoulInhale(const FVector& Mouth, float Radius, const FLinearColor& Tint, int32 Count)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.KillScale <= 0.f || Count <= 0)
	{
		return;
	}
	const int32 Motes = Allowed(VfxRules::Count(Count, DetailAt(Mouth) * Vfx.KillScale), MaxParticleCount);
	const FLinearColor Cold = FMath::Lerp(SoulCold, Tint, 0.2f);
	for (int32 Index = 0; Index < Motes; ++Index)
	{
		// From a shell round its head, drawn straight into its mouth, arriving as they burn out: half glints, half thin streaks
		// pointing in (the pull reads from any side).
		const FVector From = Mouth + Random.GetUnitVector() * Radius * Random.FRandRange(0.8f, 1.5f) + FVector(0.0, 0.0, Random.FRandRange(-20.f, 30.f));
		const float Life = Random.FRandRange(0.28f, 0.45f);
		const bool bStreak = (Index & 1) == 1;
		FParticle& Mote = AddParticle(bStreak ? EParticle::Spark : EParticle::Flash, From, (Mouth - From) / Life, Life);
		Mote.Color = FMath::Lerp(Cold, FLinearColor::White, Random.FRandRange(0.f, 0.35f));
		Mote.Intensity = Random.FRandRange(10.f, 17.f) * Vfx.Glow;
		Mote.StartSize = bStreak ? Random.FRandRange(0.6f, 1.f) : Random.FRandRange(2.5f, 4.5f);
		Mote.EndSize = bStreak ? Mote.StartSize : 1.2f;
		Mote.Gravity = 0.f;
		Mote.Drag = 0.f;
	}
}

void FWeaponFX::SpawnShriekCone(const FVector& Mouth, const FVector& Way, float Reach, const FLinearColor& Tint, int32 Count, float Strength)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.KillScale <= 0.f || Count <= 0 || Strength <= 0.f)
	{
		return;
	}
	const FVector Ahead = Way.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const FVector Across = AcrossOf(Ahead);
	const float Amount = DetailAt(Mouth) * Vfx.KillScale;
	const int32 Pieces = Allowed(VfxRules::Count(Count, Amount), MaxParticleCount);
	const FLinearColor Pale = FMath::Lerp(SoulCold, Tint, 0.15f);
	// Thrown at the speed drag brings to rest about Reach on (distance = speed / drag).
	constexpr float WispDrag = 2.4f;
	constexpr float StreakDrag = 4.f;
	for (int32 Index = 0; Index < Pieces; ++Index)
	{
		const FVector Direction = Random.VRandCone(Ahead, 0.42f);
		if ((Index % 3) != 2)
		{
			// A wisp: a pale ghost of breath torn out of it, curling aside as it slows and fades.
			FParticle& Wisp = AddParticle(EParticle::Smoke, Mouth + Direction * 10.f,
				Direction * Reach * Random.FRandRange(0.6f, 1.f) * WispDrag + Across * Random.FRandRange(-80.f, 80.f), Random.FRandRange(0.55f, 0.85f));
			Wisp.Color = FMath::Lerp(Pale, FLinearColor::White, Random.FRandRange(0.f, 0.3f));
			Wisp.Intensity = Random.FRandRange(0.28f, 0.42f) * Strength * Vfx.Smoke;
			Wisp.StartSize = Random.FRandRange(10.f, 16.f);
			Wisp.EndSize = Random.FRandRange(55.f, 90.f);
			Wisp.Gravity = -0.1f;
			Wisp.Drag = WispDrag;
			continue;
		}
		// A soul streak: a long cold line shot out with the scream.
		FParticle& Streak = AddParticle(EParticle::Streak, Mouth, Direction * Reach * Random.FRandRange(0.7f, 1.1f) * StreakDrag, Random.FRandRange(0.25f, 0.4f));
		Streak.Color = FMath::Lerp(Pale, FLinearColor::White, 0.4f);
		Streak.Intensity = Random.FRandRange(14.f, 22.f) * Strength * Vfx.Glow;
		Streak.StartSize = Random.FRandRange(1.2f, 2.2f);
		Streak.EndSize = Random.FRandRange(40.f, 70.f);
		Streak.Gravity = 0.f;
		Streak.Drag = StreakDrag;
	}
}

void FWeaponFX::SpawnWaveLift(const FVector& Ground, const FVector& Out, const FLinearColor& Light, float Strength)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.KillScale <= 0.f || Strength <= 0.f)
	{
		return;
	}
	const FVector Way = FVector(Out.X, Out.Y, 0.0).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const float Push = FMath::Clamp(Strength, 0.2f, 1.2f);
	// The dust the front shoves along the ground: low, rolling out, spreading as it hangs.
	FParticle& Dust = AddParticle(EParticle::Smoke, Ground + FVector(0.0, 0.0, 8.0),
		Way * Random.FRandRange(220.f, 340.f) * Push + FVector(0.0, 0.0, Random.FRandRange(30.f, 70.f)), Random.FRandRange(0.8f, 1.2f));
	Dust.Color = GroundDust * Light;
	Dust.Intensity = Random.FRandRange(0.3f, 0.45f) * Vfx.Smoke;
	Dust.StartSize = Random.FRandRange(18.f, 28.f);
	Dust.EndSize = Random.FRandRange(80.f, 120.f);
	Dust.Gravity = 0.05f;
	Dust.Drag = 2.6f;
	// Grass blades torn up and flung out and up, tumbling back down; now and then a leaf lifted high, fluttering slowly down.
	// Both land on the ground they came from and lie a moment (the impacts' floor), never sinking through it.
	TGuardValue<TOptional<FPlane>> FloorDuringSpawn(SpawnFloor, TOptional<FPlane>(FPlane(Ground, FVector::UpVector)));
	const uint8 Blades = BitSetFor(EBitShape::Splinter, GrassBlade, 0.f);
	const int32 Count = Random.FRand() < 0.5f ? 2 : 1;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FParticle& Blade = AddBit(Blades, Ground + FVector(0.0, 0.0, 4.0),
			(Way * Random.FRandRange(160.f, 300.f) + FVector(0.0, 0.0, Random.FRandRange(160.f, 300.f))) * Push, Random.FRandRange(2.f, 3.5f),
			Random.FRandRange(0.7f, 1.1f));
		Blade.Drag = 1.2f;
		Blade.Spin = Random.GetUnitVector() * Random.FRandRange(10.f, 22.f);
	}
	if (Random.FRand() < 0.35f)
	{
		const uint8 Leaves = BitSetFor(EBitShape::Plate, LeafAmber, 0.f);
		FParticle& Leaf = AddBit(Leaves, Ground + FVector(0.0, 0.0, 6.0),
			(Way * Random.FRandRange(120.f, 220.f) + FVector(0.0, 0.0, Random.FRandRange(260.f, 380.f))) * Push, Random.FRandRange(3.f, 4.5f),
			Random.FRandRange(1.4f, 2.f));
		// Light and flat: the air holds it up, it spins as it drifts.
		Leaf.Gravity = 0.25f;
		Leaf.Drag = 2.2f;
		Leaf.Spin = Random.GetUnitVector() * Random.FRandRange(4.f, 9.f);
	}
}

void FWeaponFX::SpawnCoalFling(const FVector& Coal, const FVector& Out, const FLinearColor& Tint, int32 Count, bool bSoot)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.KillScale <= 0.f || Count <= 0)
	{
		return;
	}
	const FVector Ahead = Out.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const int32 Embers = Allowed(VfxRules::Count(Count, DetailAt(Coal) * Vfx.KillScale), MaxParticleCount);
	const FLinearColor Hot = FMath::Lerp(Tint, EmberHeat, 0.45f);
	for (int32 Index = 0; Index < Embers; ++Index)
	{
		// Flung out of its chest and up, floating as embers do, cooling as they go.
		const FVector Direction = (Random.VRandCone(Ahead, 0.9f) + FVector(0.0, 0.0, 0.35)).GetSafeNormal();
		FParticle& Ember = AddParticle(EParticle::Spark, Coal + Direction * 6.f, Direction * Random.FRandRange(220.f, 560.f), Random.FRandRange(0.45f, 0.9f));
		Ember.Color = FMath::Lerp(Tint, Hot, Random.FRandRange(0.f, 1.f));
		Ember.Intensity = Random.FRandRange(14.f, 26.f) * Vfx.Glow;
		Ember.StartSize = Random.FRandRange(0.7f, 1.3f);
		Ember.Gravity = 0.25f;
		Ember.Drag = 1.6f;
	}
	if (bSoot && VfxRules::Count(1, DetailAt(Coal) * Vfx.KillScale) > 0)
	{
		FParticle& Soot = AddParticle(EParticle::Smoke, Coal + Ahead * 8.f,
			Ahead * Random.FRandRange(60.f, 120.f) + FVector(0.0, 0.0, Random.FRandRange(50.f, 90.f)), Random.FRandRange(0.9f, 1.3f));
		Soot.Color = FMath::Lerp(CoalSoot, Tint, 0.08f);
		Soot.Intensity = 0.42f * Vfx.Smoke;
		Soot.StartSize = 10.f;
		Soot.EndSize = Random.FRandRange(40.f, 60.f);
		Soot.Gravity = -0.08f;
		Soot.Drag = 2.f;
	}
}

void FWeaponFX::SpawnFrostBreath(const FVector& Mouth, const FVector& Way, float Size)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.KillScale <= 0.f || VfxRules::Count(1, DetailAt(Mouth) * Vfx.KillScale) <= 0)
	{
		return;
	}
	const float Scale = FMath::Clamp(Size / 40.f, 0.3f, 3.f);
	const FVector Ahead = Way.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	// Cold breath: a pale puff out of the mouth that sinks as it spreads (cold air falls), and a glint of frost in it.
	FParticle& Breath = AddParticle(EParticle::Smoke, Mouth + Ahead * 6.f, Random.VRandCone(Ahead, 0.35f) * Random.FRandRange(110.f, 200.f),
		Random.FRandRange(0.9f, 1.3f));
	Breath.Color = FrostBreath;
	Breath.Intensity = Random.FRandRange(0.25f, 0.36f) * Vfx.Smoke;
	Breath.StartSize = 8.f * Scale;
	Breath.EndSize = Random.FRandRange(40.f, 60.f) * Scale;
	Breath.Gravity = 0.12f;
	Breath.Drag = 2.2f;
	if (Random.FRand() < 0.5f)
	{
		FParticle& Glint = AddParticle(EParticle::Flash, Mouth + Ahead * 12.f + Random.GetUnitVector() * 8.f * Scale,
			Ahead * Random.FRandRange(60.f, 120.f), Random.FRandRange(0.3f, 0.5f));
		Glint.Color = FLinearColor::White;
		Glint.Intensity = Random.FRandRange(6.f, 10.f) * Vfx.Glow;
		Glint.StartSize = 1.5f;
		Glint.EndSize = 0.5f;
		Glint.Gravity = 0.05f;
		Glint.Drag = 1.5f;
	}
}
