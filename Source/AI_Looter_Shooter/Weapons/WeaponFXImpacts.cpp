#include "Weapons/WeaponFX.h"

// FWeaponFX's bullet hits on the world, in the look of what was struck (UBulletSubsystem finds the surface with
// ImpactSurface). Spawning only; WeaponFX.cpp moves and draws every particle.

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
	float Scale, bool bCritical)
{
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
	// A shotgun's pellets throw about half as much each: nine of them land together.
	const float Share = Size < 0.8f ? 0.5f : 1.f;
	auto Many = [Share](int32 Full) { return FMath::Max(1, FMath::RoundToInt32(Full * Share)); };

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

	switch (Material)
	{
	case EImpactMaterial::Dirt:
		// A dull hit: soil thrown up in a column and a wider brown puff, clods tossed out that fall back.
		Flash(ImpactFlashColor, 5.f, 10.f, 18.f, 0.05f);
		Puffs(1, ImpactDirtDust, 0.6f, 10.f, 50.f, 180.f, 260.f, 0.9f, 0.2f);
		Puffs(Many(2), ImpactDirtDust, 0.5f, 14.f, 70.f, 50.f, 110.f, 1.1f, 0.5f);
		Throw(Many(7), BitSetFor(EBitShape::Plate, ImpactClodColor, 0.f), 1.6f, 3.6f, 250.f, 650.f, 0.9f, 0.3f, 0.3f);
		break;
	case EImpactMaterial::Grass:
		// Softer: shreds of green and a little soil, a lighter puff.
		Flash(ImpactFlashColor, 4.f, 9.f, 16.f, 0.05f);
		Puffs(Many(2), ImpactGrassDust, 0.42f, 12.f, 55.f, 60.f, 140.f, 0.9f, 0.4f);
		Throw(Many(6), BitSetFor(EBitShape::Splinter, ImpactTurfColor, 0.f), 1.2f, 2.4f, 200.f, 500.f, 0.8f, 0.3f, 1.2f);
		Throw(Many(3), BitSetFor(EBitShape::Plate, ImpactClodColor, 0.f), 1.2f, 2.6f, 200.f, 480.f, 0.8f, 0.2f, 0.3f);
		break;
	case EImpactMaterial::Gravel:
		// Stones knock sparks off each other; grit and pale stone dust fly.
		Flash(ImpactFlashColor, 9.f, 12.f, 24.f, 0.06f);
		Sparks(Many(3), ImpactSparkColor, 22.f, 400.f, 1000.f, 0.1f, 0.22f);
		Puffs(Many(2), ImpactStoneDust, 0.5f, 10.f, 55.f, 60.f, 130.f, 0.8f, 0.5f);
		StoneChips(Many(7), 1.2f, 2.6f, 300.f, 750.f);
		break;
	case EImpactMaterial::Sand:
		// No hard edge to it: a pale spray that hangs, and fine grains thrown with the shot.
		Puffs(Many(3), ImpactSandDust, 0.55f, 16.f, 80.f, 70.f, 170.f, 1.2f, 0.9f);
		StoneChips(Many(5), 0.6f, 1.2f, 200.f, 520.f);
		break;
	case EImpactMaterial::Stone:
		// The classic: a hot flash, sparks, stone dust and chips.
		Flash(ImpactFlashColor, 16.f, 16.f, 32.f, 0.07f);
		Sparks(Many(7), ImpactSparkColor, 26.f, 450.f, 1300.f, 0.12f, 0.3f);
		Puffs(Many(2), ImpactStoneDust, 0.55f, 10.f, 60.f, 50.f, 120.f, 0.75f, 0.4f);
		StoneChips(Many(5), 1.4f, 2.8f, 250.f, 650.f);
		break;
	case EImpactMaterial::Wood:
		// Pale splinters torn out and a breath of sawdust; wood doesn't spark.
		Flash(ImpactFlashColor, 5.f, 9.f, 18.f, 0.05f);
		Throw(Many(8), BitSetFor(EBitShape::Splinter, ImpactSplinterColor, 0.f), 2.f, 4.5f, 250.f, 700.f, 1.f, 0.4f, 0.4f);
		Puffs(Many(2), ImpactSawdust, 0.45f, 8.f, 45.f, 40.f, 100.f, 0.8f, 0.5f);
		break;
	case EImpactMaterial::Metal:
		// Steel rings out in a shower of hot sparks, a few of them bouncing on, and a wisp of smoke.
		Flash(ImpactFlashColor, 24.f, 18.f, 40.f, 0.07f);
		Sparks(Many(14), ImpactHotSparkColor, 34.f, 600.f, 1700.f, 0.12f, 0.3f);
		Sparks(Many(4), ImpactSparkColor, 24.f, 300.f, 700.f, 0.45f, 0.75f);
		Puffs(1, ImpactMetalSmoke, 0.3f, 8.f, 35.f, 30.f, 70.f, 0.7f, 0.3f);
		break;
	case EImpactMaterial::SheetMetal:
		// Thinner: fewer sparks, and flakes of paint and rust fluttering down.
		Flash(ImpactFlashColor, 20.f, 16.f, 34.f, 0.06f);
		Sparks(Many(10), ImpactHotSparkColor, 30.f, 500.f, 1400.f, 0.1f, 0.25f);
		Throw(Many(4), BitSetFor(EBitShape::Plate, ImpactFlakeColor, 0.f), 1.f, 2.f, 150.f, 400.f, 1.f, 0.3f, 1.8f);
		Puffs(1, ImpactMetalSmoke, 0.25f, 8.f, 30.f, 30.f, 70.f, 0.6f, 0.3f);
		break;
	case EImpactMaterial::Glass:
	{
		// Glinting shards, some on through the pane with the bullet and some back out, a sparkle and a breath of powder.
		Flash(ImpactGlintColor, 10.f, 8.f, 22.f, 0.06f);
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
		// A splash: a column of drops straight up, a crown thrown out round it, and mist.
		const uint8 Drops = BitSetFor(EBitShape::Round, ImpactDropColor, 0.15f);
		for (int32 Index = 0; Index < Many(4); ++Index)
		{
			const FVector Direction = (Up + Random.GetUnitVector() * 0.15f).GetSafeNormal();
			AddBit(Drops, Point, Direction * Random.FRandRange(500.f, 850.f) * Reach, Random.FRandRange(1.6f, 3.f) * Size, Random.FRandRange(0.6f, 0.9f));
		}
		Throw(Many(10), Drops, 1.2f, 2.6f, 260.f, 600.f, 0.75f, 0.3f, 0.2f);
		Puffs(Many(2), ImpactWaterMist, 0.45f, 15.f, 70.f, 40.f, 90.f, 0.8f, 0.2f);
		break;
	}
	case EImpactMaterial::Cloth:
		// A soft puff of lint and a few threads drifting down.
		Puffs(1, ImpactLintDust, 0.35f, 6.f, 28.f, 30.f, 70.f, 0.7f, 0.3f);
		Throw(Many(4), BitSetFor(EBitShape::Plate, ImpactLintColor, 0.f), 0.8f, 1.6f, 80.f, 200.f, 0.9f, 0.3f, 2.5f);
		break;
	case EImpactMaterial::Hay:
		// Straws flung out and a dusty golden puff.
		Puffs(Many(2), ImpactHayDust, 0.45f, 10.f, 50.f, 50.f, 120.f, 0.9f, 0.4f);
		Throw(Many(8), BitSetFor(EBitShape::Splinter, ImpactStrawColor, 0.f), 2.f, 4.f, 150.f, 450.f, 1.1f, 0.3f, 1.2f);
		break;
	case EImpactMaterial::Bone:
		// Brittle: pale chips and a little bone dust.
		Flash(ImpactFlashColor, 6.f, 9.f, 18.f, 0.05f);
		Throw(Many(5), BitSetFor(EBitShape::Plate, ImpactBoneColor, 0.f), 1.f, 2.2f, 250.f, 600.f, 0.8f, 0.3f, 0.3f);
		Puffs(1, ImpactBoneDust, 0.4f, 8.f, 40.f, 40.f, 90.f, 0.7f, 0.3f);
		break;
	default:
		break;
	}

	if (bCritical)
	{
		// Critical hits get a bright pop on top, whatever they hit.
		Flash(ImpactCritColor, 28.f, 26.f, 60.f, 0.1f);
	}
}
