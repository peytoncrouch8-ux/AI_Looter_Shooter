#include "Weapons/WeaponFX.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Templates/UnrealTemplate.h"

// FWeaponFX's bullet hits on the world, in the look of what was struck (UBulletSubsystem finds the surface with
// ImpactSurface), and the rings and wakes a hit or a step leaves on water (the pool and its rules are WaterRings.h).
// Spawning only; WeaponFXDraw.cpp moves and draws every particle.
//
// The VFX pass (Saved/Vfx/PLAN.md; Looter.Vfx.Impact.*): soft ground throws a geyser (puffs stacked up the hit's normal,
// fast then slowing, so they read as a column) with a skirt of dust rolling out over the surface and a couple of big clods;
// hard surfaces pop a starburst (stone dust jets, metal pings with a ring and long-bouncing sparks, glass glints); every
// hit's pieces are thinned far off and in a stream of fire into one spot (an LMG's tenth round into a wall).

namespace
{
	// Dust, spray and smoke as the unlit smoke material shows them: about as the afternoon light shows each.
	const FLinearColor ImpactDirtDust(0.5f, 0.41f, 0.31f);
	const FLinearColor ImpactGrassDust(0.5f, 0.48f, 0.37f);
	const FLinearColor ImpactSandDust(0.74f, 0.65f, 0.5f);
	const FLinearColor ImpactStoneDust(0.6f, 0.57f, 0.52f);
	const FLinearColor ImpactSawdust(0.66f, 0.55f, 0.41f);
	const FLinearColor ImpactMetalSmoke(0.42f, 0.42f, 0.44f);
	const FLinearColor ImpactWaterMist(0.82f, 0.86f, 0.9f);
	const FLinearColor ImpactGlassDust(0.82f, 0.84f, 0.86f);
	const FLinearColor ImpactLintDust(0.62f, 0.59f, 0.54f);
	const FLinearColor ImpactHayDust(0.72f, 0.64f, 0.44f);
	const FLinearColor ImpactBoneDust(0.78f, 0.75f, 0.68f);

	// The lit chunks' base colours (linear).
	const FLinearColor ImpactClodColor(0.11f, 0.075f, 0.045f);
	const FLinearColor ImpactTurfColor(0.07f, 0.11f, 0.03f);
	const FLinearColor ImpactSplinterColor(0.5f, 0.34f, 0.18f);
	const FLinearColor ImpactStrawColor(0.62f, 0.5f, 0.2f);
	const FLinearColor ImpactShardColor(0.55f, 0.7f, 0.78f);
	const FLinearColor ImpactDropColor(0.5f, 0.62f, 0.7f);
	const FLinearColor ImpactFlakeColor(0.28f, 0.22f, 0.17f);
	const FLinearColor ImpactBoneColor(0.7f, 0.65f, 0.55f);
	const FLinearColor ImpactLintColor(0.5f, 0.47f, 0.42f);

	// The glows: only a hue (each is multiplied by its strength).
	const FLinearColor ImpactSparkColor(1.f, 0.5f, 0.16f);
	const FLinearColor ImpactHotSparkColor(1.f, 0.7f, 0.35f);
	const FLinearColor ImpactFlashColor(1.f, 0.72f, 0.4f);
	const FLinearColor ImpactGlintColor(0.85f, 0.95f, 1.f);
	const FLinearColor ImpactCritColor(1.f, 0.85f, 0.3f);
	/** A geyser's thrown soil and stone dust: darker than the hanging dust, so the column reads against the ground it rose from. */
	const FLinearColor ImpactGeyserDirt(0.2f, 0.15f, 0.1f);
	const FLinearColor ImpactGeyserStone(0.3f, 0.28f, 0.25f);

	// The pass's numbers (a rifle round's, at Looter.Vfx.Impact.Scale 1).
	/** A geyser's puffs: how many, how fast the first leaves (cm/s; each next a little slower), their sizes and life. */
	constexpr int32 GeyserPuffs = 3;
	constexpr float GeyserSpeed = 340.f;
	/** A skirt's puffs rolling out over the surface, their speed (cm/s). */
	constexpr int32 SkirtPuffs = 3;
	constexpr float SkirtSpeed = 240.f;
	/** A hard hit's starburst (cm across, strength, life). */
	constexpr float HardPopSize = 40.f;
	constexpr float HardPopGlow = 20.f;
	constexpr float HardPopLife = 0.08f;
}

FWeaponFX::FParticle& FWeaponFX::AddBit(uint8 Set, const FVector& Location, const FVector& Velocity, float Size, float Life)
{
	FParticle& Bit = AddParticle(EParticle::Bit, Location, Velocity, Life);
	Bit.BitSet = Set;
	Bit.StartSize = Size;
	Bit.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
	Bit.Spin = Random.GetUnitVector() * Random.FRandRange(6.f, 18.f);
	Bit.Drag = 0.3f;
	return Bit;
}

void FWeaponFX::SpawnSurfaceImpact(const FVector& Location, const FVector& Normal, const FVector& ShotDirection, EImpactMaterial Material,
	float Scale, bool bCritical, const FPlane* Floor)
{
	// What it throws lands on the ground under the hit, when it has one (AddParticle gives it to the chunks and sparks).
	TGuardValue<TOptional<FPlane>> FloorDuringSpawn(SpawnFloor, Floor ? TOptional<FPlane>(*Floor) : TOptional<FPlane>());
	if (Material == EImpactMaterial::Unknown)
	{
		// Nothing says what it is: the old sparks, dust and chips.
		SpawnImpact(Location, Normal, ShotDirection, EImpactSurface::World, bCritical);
		return;
	}
	const FVector Shot = ShotDirection.GetSafeNormal();
	const FVector Up = Normal.IsNearlyZero() ? -Shot : Normal.GetSafeNormal();
	// Sit just off the surface so nothing sinks into it; what's thrown glances off along the shot's bounce.
	const FVector Point = Location + Up * 2.f;
	const FVector Bounce = Shot.MirrorByVector(Up);
	const float Size = FMath::Clamp(Scale, 0.4f, 1.6f);
	const float Reach = FMath::Sqrt(Size);
	const bool bPellet = Size < 0.8f;
	// A shotgun's pellets throw about half as much each: nine of them land together.
	const float Share = bPellet ? 0.5f : 1.f;
	// Thinned far off and in a stream of fire into one spot (a blast's pellets are already halved).
	const FVfxTuning Vfx = FVfxTuning::Live();
	const float Detail = DetailAt(Location) * (bPellet ? 1.f : StreamDetailFor(Location));
	auto Many = [Share, Detail](int32 Full) { return FMath::Max(1, FMath::RoundToInt32(Full * Share * Detail)); };
	// The pass's own pieces (Looter.Vfx.Impact.Scale of them; a pellet's half).
	const float Extra = Vfx.ImpactScale * Share * Detail;
	const float Weight = VfxRules::HitWeight(Scale);
	FVector SideA, SideB;
	Up.FindBestAxisVectors(SideA, SideB);

	auto Flash = [this, &Point, Size](const FLinearColor& Color, float Intensity, float From, float To, float Life)
	{
		FParticle& Glow = AddParticle(EParticle::Flash, Point, FVector::ZeroVector, Life);
		Glow.Color = Color;
		Glow.Intensity = Intensity;
		Glow.StartSize = From * Size;
		Glow.EndSize = To * Size;
		Glow.Gravity = 0.f;
	};
	auto Sparks = [this, &Point, &Up, &Bounce](int32 Count, const FLinearColor& Color, float Intensity, float MinSpeed, float MaxSpeed,
		float MinLife, float MaxLife)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Direction = (Bounce * 0.6f + Up * 0.8f + Random.GetUnitVector() * 0.55f).GetSafeNormal();
			FParticle& Spark = AddParticle(EParticle::Spark, Point, Direction * Random.FRandRange(MinSpeed, MaxSpeed), Random.FRandRange(MinLife, MaxLife));
			Spark.Color = Color;
			Spark.Intensity = Intensity * Random.FRandRange(0.7f, 1.2f);
			Spark.StartSize = Random.FRandRange(0.9f, 1.4f);
			Spark.Drag = 1.5f;
		}
	};
	auto Puffs = [this, &Point, &Up, &Bounce, Size, Reach](int32 Count, const FLinearColor& Color, float Opacity, float From, float To,
		float MinRise, float MaxRise, float Life, float Along)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Velocity = (Up * Random.FRandRange(MinRise, MaxRise) + Bounce * Along * Random.FRandRange(60.f, 140.f)) * Reach
				+ Random.GetUnitVector() * 30.f;
			FParticle& Puff = AddParticle(EParticle::Smoke, Point, Velocity, Life * Random.FRandRange(0.8f, 1.2f));
			Puff.Color = Color;
			Puff.Intensity = Opacity;
			Puff.StartSize = From * Size * Random.FRandRange(0.8f, 1.2f);
			Puff.EndSize = To * Size * Random.FRandRange(0.85f, 1.15f);
			Puff.Gravity = -0.04f;
			Puff.Drag = 2.5f;
		}
	};
	auto Throw = [this, &Point, &Up, &Bounce, &Shot, Size, Reach](int32 Count, uint8 Set, float MinSize, float MaxSize, float MinSpeed,
		float MaxSpeed, float Life, float Along, float Drag)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Direction = (Up * 0.9f + (Along >= 0.f ? Bounce : Shot) * FMath::Abs(Along) + Random.GetUnitVector() * 0.55f).GetSafeNormal();
			FParticle& Bit = AddBit(Set, Point, Direction * Random.FRandRange(MinSpeed, MaxSpeed) * Reach, Random.FRandRange(MinSize, MaxSize) * Size,
				Life * Random.FRandRange(0.8f, 1.25f));
			Bit.Drag = Drag;
		}
	};
	auto StoneChips = [this, &Point, &Up, Size, Reach](int32 Count, float MinSize, float MaxSize, float MinSpeed, float MaxSpeed)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Direction = (Up * 0.9f + Random.GetUnitVector() * 0.6f).GetSafeNormal();
			FParticle& Chip = AddParticle(EParticle::Chip, Point, Direction * Random.FRandRange(MinSpeed, MaxSpeed) * Reach, Random.FRandRange(0.5f, 0.8f));
			Chip.StartSize = Random.FRandRange(MinSize, MaxSize) * Size;
			Chip.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Chip.Spin = Random.GetUnitVector() * Random.FRandRange(8.f, 20.f);
		}
	};
	// --- The pass's layers ---
	// A geyser: puffs shot up the normal (a little along the bounce), each slower than the last, so they stack into a column
	// that hangs and spreads: the dirt kicked up by a round, readable at range.
	auto Geyser = [this, &Point, &Up, &Bounce, Size, Reach, Extra, &Vfx](int32 Full, const FLinearColor& Color, float Opacity, float From,
		float To, float Life)
	{
		const int32 Count = VfxRules::Count(Full, Extra);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Slower = 1.f - 0.55f * static_cast<float>(Index) / FMath::Max(Count, 1);
			const FVector Way = (Up + Bounce * 0.15f + Random.GetUnitVector() * 0.12f).GetSafeNormal();
			FParticle& Puff = AddParticle(EParticle::Smoke, Point, Way * GeyserSpeed * Slower * Reach, Life * Random.FRandRange(0.85f, 1.15f));
			Puff.Color = Color;
			Puff.Intensity = FMath::Min(Opacity * Vfx.Smoke, 1.f);
			Puff.StartSize = From * Size * (1.f - 0.3f * (1.f - Slower));
			Puff.EndSize = To * Size * Random.FRandRange(0.85f, 1.15f);
			// Thrown dirt falls back: the column slumps as it spreads.
			Puff.Gravity = 0.35f;
			Puff.Drag = 3.2f;
		}
	};
	// A skirt: low puffs rolling out over the surface round the hit.
	auto Skirt = [this, &Point, &Up, &SideA, &SideB, Size, Reach, Extra, &Vfx](int32 Full, const FLinearColor& Color, float Opacity, float From,
		float To, float Life)
	{
		const int32 Count = VfxRules::Count(Full, Extra);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Angle = UE_TWO_PI * (static_cast<float>(Index) + Random.FRandRange(-0.3f, 0.3f)) / FMath::Max(Count, 1);
			const FVector Out = SideA * FMath::Cos(Angle) + SideB * FMath::Sin(Angle);
			FParticle& Puff = AddParticle(EParticle::Smoke, Point + Out * 6.f * Size, (Out * SkirtSpeed + Up * 30.f) * Reach,
				Life * Random.FRandRange(0.85f, 1.15f));
			Puff.Color = Color;
			Puff.Intensity = FMath::Min(Opacity * Vfx.Smoke, 1.f);
			Puff.StartSize = From * Size;
			Puff.EndSize = To * Size * Random.FRandRange(0.85f, 1.15f);
			Puff.Gravity = -0.02f;
			Puff.Drag = 4.f;
		}
	};
	// A hard hit's pop: a starburst (never thinned by distance: it reads a hit far off).
	auto HardPop = [this, &Point, Weight, bPellet, &Vfx](const FLinearColor& Color, float Strength, int32 Spikes)
	{
		if (Vfx.ImpactScale <= 0.f)
		{
			return;
		}
		SpawnStarburst(Point, HardPopSize * Weight * (bPellet ? 0.6f : 1.f), Color, HardPopGlow * Strength * Vfx.Glow, HardPopLife, Spikes);
	};
	// A few big pieces of what it struck, thrown high (a hit's weight: the chunk you watch fall).
	auto BigChunks = [this, &Point, &Up, &Bounce, Size, Reach, Extra](int32 Full, uint8 Set, float MinSize, float MaxSize)
	{
		const int32 Count = VfxRules::Count(Full, Extra);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Direction = (Up * 1.2f + Bounce * 0.3f + Random.GetUnitVector() * 0.5f).GetSafeNormal();
			FParticle& Chunk = AddBit(Set, Point, Direction * Random.FRandRange(320.f, 560.f) * Reach, Random.FRandRange(MinSize, MaxSize) * Size,
				Random.FRandRange(0.9f, 1.3f));
			Chunk.Drag = 0.2f;
		}
	};

	switch (Material)
	{
	case EImpactMaterial::Dirt:
	{
		// A dull hit: soil thrown up in a column and a wider brown puff, clods tossed out that fall back. The pass: a geyser of
		// soil up the normal, a skirt of dust rolling out over the ground, two big clods thrown high.
		Flash(ImpactFlashColor, 5.f, 10.f, 18.f, 0.05f);
		Puffs(1, ImpactDirtDust, 0.6f, 10.f, 50.f, 180.f, 260.f, 0.9f, 0.2f);
		Puffs(Many(2), ImpactDirtDust, 0.5f, 14.f, 70.f, 50.f, 110.f, 1.1f, 0.5f);
		const uint8 Clods = BitSetFor(EBitShape::Plate, ImpactClodColor, 0.f);
		Throw(Many(7), Clods, 1.6f, 3.6f, 250.f, 650.f, 0.9f, 0.3f, 0.3f);
		Geyser(GeyserPuffs, ImpactGeyserDirt, 0.95f, 20.f, 80.f, 1.2f);
		Skirt(SkirtPuffs, ImpactDirtDust, 0.45f, 14.f, 60.f, 1.2f);
		BigChunks(2, Clods, 3.5f, 5.5f);
		break;
	}
	case EImpactMaterial::Grass:
	{
		// Softer: shreds of green and a little soil, a lighter puff; a short geyser and torn turf thrown high.
		Flash(ImpactFlashColor, 4.f, 9.f, 16.f, 0.05f);
		Puffs(Many(2), ImpactGrassDust, 0.42f, 12.f, 55.f, 60.f, 140.f, 0.9f, 0.4f);
		const uint8 Turf = BitSetFor(EBitShape::Splinter, ImpactTurfColor, 0.f);
		Throw(Many(6), Turf, 1.2f, 2.4f, 200.f, 500.f, 0.8f, 0.3f, 1.2f);
		Throw(Many(3), BitSetFor(EBitShape::Plate, ImpactClodColor, 0.f), 1.2f, 2.6f, 200.f, 480.f, 0.8f, 0.2f, 0.3f);
		Geyser(2, ImpactGeyserDirt, 0.75f, 12.f, 56.f, 1.f);
		BigChunks(2, Turf, 3.f, 4.5f);
		break;
	}
	case EImpactMaterial::Gravel:
		// Stones knock sparks off each other; grit and pale stone dust fly. The pass: a small hard pop, a short jet of grit dust.
		Flash(ImpactFlashColor, 9.f, 12.f, 24.f, 0.06f);
		HardPop(ImpactSparkColor, 0.6f, 6);
		Sparks(Many(3), ImpactSparkColor, 22.f, 400.f, 1000.f, 0.1f, 0.22f);
		Puffs(Many(2), ImpactStoneDust, 0.5f, 10.f, 55.f, 60.f, 130.f, 0.8f, 0.5f);
		Geyser(2, ImpactStoneDust, 0.55f, 8.f, 38.f, 0.7f);
		StoneChips(Many(7), 1.2f, 2.6f, 300.f, 750.f);
		break;
	case EImpactMaterial::Sand:
		// No hard edge to it: a pale spray that hangs, and fine grains thrown with the shot; a tall spray column and a skirt.
		Puffs(Many(3), ImpactSandDust, 0.55f, 16.f, 80.f, 70.f, 170.f, 1.2f, 0.9f);
		Geyser(4, ImpactSandDust, 0.6f, 10.f, 52.f, 1.1f);
		Skirt(SkirtPuffs, ImpactSandDust, 0.4f, 14.f, 64.f, 1.3f);
		StoneChips(Many(5), 0.6f, 1.2f, 200.f, 520.f);
		break;
	case EImpactMaterial::Stone:
		// The classic: a hot flash, sparks, stone dust and chips. The pass: a hard starburst, a jet of stone dust off the face,
		// a couple of big chips thrown high.
		Flash(ImpactFlashColor, 16.f, 16.f, 32.f, 0.07f);
		HardPop(ImpactFlashColor, 1.f, 8);
		Sparks(Many(7), ImpactSparkColor, 26.f, 450.f, 1300.f, 0.12f, 0.3f);
		Puffs(Many(2), ImpactStoneDust, 0.55f, 10.f, 60.f, 50.f, 120.f, 0.75f, 0.4f);
		Geyser(3, ImpactGeyserStone, 0.9f, 16.f, 70.f, 1.1f);
		StoneChips(Many(5), 1.4f, 2.8f, 250.f, 650.f);
		StoneChips(VfxRules::Count(2, Extra), 3.f, 4.5f, 300.f, 520.f);
		break;
	case EImpactMaterial::Wood:
	{
		// Pale splinters torn out and a breath of sawdust; wood doesn't spark. The pass: a jet of sawdust, two long splinters.
		Flash(ImpactFlashColor, 5.f, 9.f, 18.f, 0.05f);
		const uint8 Splinters = BitSetFor(EBitShape::Splinter, ImpactSplinterColor, 0.f);
		Throw(Many(8), Splinters, 2.f, 4.5f, 250.f, 700.f, 1.f, 0.4f, 0.4f);
		Puffs(Many(2), ImpactSawdust, 0.45f, 8.f, 45.f, 40.f, 100.f, 0.8f, 0.5f);
		Geyser(2, ImpactSawdust, 0.5f, 8.f, 36.f, 0.7f);
		BigChunks(2, Splinters, 5.f, 7.5f);
		break;
	}
	case EImpactMaterial::Metal:
		// Steel rings out in a shower of hot sparks, a few of them bouncing on, and a wisp of smoke. The pass: a white-hot
		// starburst and a ring of light pinging out round the hit, facing the eye, and a few sparks that bounce on longer.
		Flash(ImpactFlashColor, 24.f, 18.f, 40.f, 0.07f);
		HardPop(ImpactHotSparkColor, 1.3f, 10);
		if (Extra > 0.f)
		{
			SpawnShockRing(Point, FVector::ZeroVector, 3.f * Weight, 30.f * Weight, 2.5f, ImpactHotSparkColor, 10.f * Vfx.Glow, 0.12f, true);
		}
		Sparks(Many(14), ImpactHotSparkColor, 34.f, 600.f, 1700.f, 0.12f, 0.3f);
		Sparks(Many(4), ImpactSparkColor, 24.f, 300.f, 700.f, 0.45f, 0.75f);
		Sparks(VfxRules::Count(3, Extra), ImpactSparkColor, 22.f, 250.f, 520.f, 0.7f, 1.f);
		Puffs(1, ImpactMetalSmoke, 0.3f, 8.f, 35.f, 30.f, 70.f, 0.7f, 0.3f);
		break;
	case EImpactMaterial::SheetMetal:
		// Thinner: fewer sparks, and flakes of paint and rust fluttering down; the pass's starburst and a smaller ping.
		Flash(ImpactFlashColor, 20.f, 16.f, 34.f, 0.06f);
		HardPop(ImpactHotSparkColor, 1.1f, 8);
		if (Extra > 0.f)
		{
			SpawnShockRing(Point, FVector::ZeroVector, 3.f * Weight, 24.f * Weight, 2.f, ImpactHotSparkColor, 8.f * Vfx.Glow, 0.1f, true);
		}
		Sparks(Many(10), ImpactHotSparkColor, 30.f, 500.f, 1400.f, 0.1f, 0.25f);
		Throw(Many(4), BitSetFor(EBitShape::Plate, ImpactFlakeColor, 0.f), 1.f, 2.f, 150.f, 400.f, 1.f, 0.3f, 1.8f);
		Puffs(1, ImpactMetalSmoke, 0.25f, 8.f, 30.f, 30.f, 70.f, 0.6f, 0.3f);
		break;
	case EImpactMaterial::Glass:
	{
		// Glinting shards, some on through the pane with the bullet and some back out, a sparkle and a breath of powder; the
		// pass's cold starburst.
		Flash(ImpactGlintColor, 10.f, 8.f, 22.f, 0.06f);
		HardPop(ImpactGlintColor, 0.9f, 12);
		for (int32 Index = 0; Index < Many(5); ++Index)
		{
			FParticle& Glint = AddParticle(EParticle::Flash, Point + Random.GetUnitVector() * 6.f * Size, Random.GetUnitVector() * 40.f,
				Random.FRandRange(0.08f, 0.16f));
			Glint.Color = ImpactGlintColor;
			Glint.Intensity = Random.FRandRange(12.f, 22.f);
			Glint.StartSize = Random.FRandRange(2.f, 4.f) * Size;
			Glint.EndSize = 0.5f * Size;
			Glint.Gravity = 0.f;
		}
		const uint8 Shards = BitSetFor(EBitShape::Plate, ImpactShardColor, 0.35f);
		Throw(Many(6), Shards, 1.f, 2.4f, 200.f, 600.f, 0.9f, 0.4f, 0.2f);
		Throw(Many(5), Shards, 1.f, 2.f, 250.f, 650.f, 0.9f, -0.9f, 0.2f);
		Puffs(1, ImpactGlassDust, 0.25f, 6.f, 30.f, 20.f, 60.f, 0.6f, 0.2f);
		break;
	}
	case EImpactMaterial::Water:
	{
		// A splash: a column of drops straight up, a crown thrown out round it, and mist; and rings spreading over the water.
		// The pass: a taller column and a mist that hangs over the rings.
		SpawnWaterRings(Location, Up, Size);
		const uint8 Drops = BitSetFor(EBitShape::Round, ImpactDropColor, 0.15f);
		const int32 Column = Many(4) + VfxRules::Count(3, Extra);
		for (int32 Index = 0; Index < Column; ++Index)
		{
			const FVector Direction = (Up + Random.GetUnitVector() * 0.15f).GetSafeNormal();
			AddBit(Drops, Point, Direction * Random.FRandRange(500.f, 950.f) * Reach, Random.FRandRange(1.6f, 3.f) * Size, Random.FRandRange(0.6f, 0.95f));
		}
		Throw(Many(10), Drops, 1.2f, 2.6f, 260.f, 600.f, 0.75f, 0.3f, 0.2f);
		Puffs(Many(2), ImpactWaterMist, 0.45f, 15.f, 70.f, 40.f, 90.f, 0.8f, 0.2f);
		Geyser(2, ImpactWaterMist, 0.4f, 10.f, 44.f, 0.8f);
		break;
	}
	case EImpactMaterial::Cloth:
		// A soft puff of lint and a few threads drifting down.
		Puffs(1, ImpactLintDust, 0.35f, 6.f, 28.f, 30.f, 70.f, 0.7f, 0.3f);
		Throw(Many(4), BitSetFor(EBitShape::Plate, ImpactLintColor, 0.f), 0.8f, 1.6f, 80.f, 200.f, 0.9f, 0.3f, 2.5f);
		break;
	case EImpactMaterial::Hay:
		// Straws flung out and a dusty golden puff; a jet of chaff (the pass).
		Puffs(Many(2), ImpactHayDust, 0.45f, 10.f, 50.f, 50.f, 120.f, 0.9f, 0.4f);
		Geyser(2, ImpactHayDust, 0.5f, 10.f, 44.f, 1.f);
		Throw(Many(8), BitSetFor(EBitShape::Splinter, ImpactStrawColor, 0.f), 2.f, 4.f, 150.f, 450.f, 1.1f, 0.3f, 1.2f);
		break;
	case EImpactMaterial::Bone:
		// Brittle: pale chips and a little bone dust; a small pale pop.
		Flash(ImpactFlashColor, 6.f, 9.f, 18.f, 0.05f);
		HardPop(ImpactBoneDust, 0.5f, 6);
		Throw(Many(5), BitSetFor(EBitShape::Plate, ImpactBoneColor, 0.f), 1.f, 2.2f, 250.f, 600.f, 0.8f, 0.3f, 0.3f);
		Puffs(1, ImpactBoneDust, 0.4f, 8.f, 40.f, 40.f, 90.f, 0.7f, 0.3f);
		break;
	default:
		break;
	}

	if (bCritical)
	{
		// Critical hits get a bright pop on top, whatever they hit; the pass adds a gold starburst (the crit reads off a wall too).
		Flash(ImpactCritColor, 28.f, 26.f, 60.f, 0.1f);
		if (Vfx.ImpactScale > 0.f)
		{
			SpawnStarburst(Point, 46.f * Weight, ImpactCritColor, 22.f * Vfx.Glow, 0.08f, 10);
		}
	}
}

// ---------------------------------------------------------------------------
// Water: rings and wakes
// ---------------------------------------------------------------------------

FLinearColor FWeaponFX::WaterTint(const FVector& Where)
{
	const AActor* Actor = Owner.Get();
	const UWorld* World = Actor ? Actor->GetWorld() : nullptr;
	if (!World)
	{
		return FWaterRings::FoamAlbedo() * 1.2f;
	}
	// The sun's trace is the cost: once a second is plenty for the light on a creek.
	const double Now = World->GetTimeSeconds();
	if (Now - WaterLightAt >= WaterLightSeconds || Now < WaterLightAt)
	{
		WaterLight = WaterDustLight.At(*World, Where + FVector(0.0, 0.0, 40.0), *Actor);
		WaterLightAt = Now;
	}
	return FWaterRings::FoamAlbedo() * WaterLight;
}

int32 FWeaponFX::SpawnWaterRings(const FVector& Location, const FVector& Normal, float Size, const FLinearColor* Light)
{
	const FVector Up = Normal.IsNearlyZero() ? FVector::UpVector : Normal.GetSafeNormal();
	if (!FWaterRings::LiesFlat(Up))
	{
		return 0;
	}
	const FLinearColor Color = Light ? FWaterRings::FoamAlbedo() * *Light : WaterTint(Location);
	// Just over the surface, so the dashes never sink into it.
	return WaterRings.AddRings(Location + Up * 1.5, Up, Size, FWaterRings::RingsFor(Size), Color);
}

bool FWeaponFX::SpawnWake(const FVector& Location, const FVector& Normal, const FVector& Direction, float Speed, float Scale,
	const FLinearColor* Light)
{
	const FVector Up = Normal.IsNearlyZero() ? FVector::UpVector : Normal.GetSafeNormal();
	if (!FWaterRings::LiesFlat(Up))
	{
		return false;
	}
	const FLinearColor Color = Light ? FWaterRings::FoamAlbedo() * *Light : WaterTint(Location);
	return WaterRings.AddWake(Location + Up * 1.5, Up, Direction, Speed, Scale, Color);
}
