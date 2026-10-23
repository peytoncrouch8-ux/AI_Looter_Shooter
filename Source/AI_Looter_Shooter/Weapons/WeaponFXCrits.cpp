#include "Weapons/WeaponFX.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"

// The gun feel pass (Docs/Work/2026-10-22-Gunfeel/PLAN.md, GF-IMPACT): a crit's own look by what the body spills, on top of
// every crit's warm pop and ring (WeaponFXCreatures.cpp), so a crit on a spider, a slime or an Unpaid each reads as that
// creature breaking (the play-check: crits looked the same on everything). Looter.Vfx.Hit.CritPop scales it; the pieces
// thin far off and with a blast's budget (Amount); the glows are soft discs, capped on screen and sharing the wound's light
// (Light; the play-check, 2026-10-23: no spikes, no white disc over the target), never thinned by distance. Fire glows stay
// under a strength of 3: additive fire over daylight washes to white past it (VFX pass lesson).

namespace
{
	/** A spider's shell: dark chitin and its pale bands (the Brown Spider's; the Spitter's olive reads near enough). */
	const FLinearColor CritChitin(0.09f, 0.075f, 0.06f);
	const FLinearColor CritChitinBand(0.5f, 0.46f, 0.38f);
	/** The shell's crack: a small pale flash. */
	const FLinearColor CritCrackColor(0.92f, 1.f, 0.8f);
	/** A slime core's flash and ring: the gel's colour toward white by this share. */
	constexpr float CritGelWhite = 0.55f;
	/** The Unpaid's soul-light gleaming out through the crack in its coal. */
	const FLinearColor CritSoulGleam(1.f, 0.93f, 0.72f);
	const FLinearColor CritEmberHeat(1.f, 0.9f, 0.7f);
	/** A miner's rock dust. */
	const FLinearColor CritRockDust(0.6f, 0.57f, 0.52f);

	// Pieces at Amount 1 (a rifle round's crit).
	constexpr int32 CritShards = 7;
	constexpr int32 CritBands = 3;
	constexpr int32 CritThickGobs = 4;
	constexpr int32 CritGelDrops = 6;
	constexpr int32 CritEmbers = 10;
	constexpr int32 CritRockChips = 7;
	constexpr int32 CritClods = 5;
}

void FWeaponFX::SpawnBloodCrit(ECreatureBlood Blood, const FVector& Point, const FVector& Up, const FVector& Shot, const FLinearColor& Tint,
	float Size, float Amount, float Light)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	const float Strength = Vfx.HitCritPop * Vfx.Glow * Light;
	const float Reach = FMath::Sqrt(FMath::Max(Size, 0.1f));
	if (Strength <= 0.f && Amount <= 0.f)
	{
		return;
	}
	// Flung out of the wound, a little back along the shot (they come out the side it went in).
	auto Out = [this, &Up, &Shot](float Lift, float Wild) { return (Up * Lift + Random.GetUnitVector() * Wild - Shot * 0.3f).GetSafeNormal(); };
	auto Fling = [this, &Point, &Out, Size, Reach](int32 Count, uint8 Set, float MinSize, float MaxSize, float MinSpeed, float MaxSpeed, float Gravity)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FParticle& Bit = AddBit(Set, Point, Out(0.9f, 0.8f) * Random.FRandRange(MinSpeed, MaxSpeed) * Reach,
				Random.FRandRange(MinSize, MaxSize) * Size, Random.FRandRange(0.8f, 1.1f));
			Bit.Drag = 0.25f;
			Bit.Gravity = Gravity;
			Bit.Spin = Random.GetUnitVector() * Random.FRandRange(10.f, 24.f);
		}
	};
	auto Flash = [this, &Point, Size](const FLinearColor& Color, float Intensity, float From, float To, float Life)
	{
		const float Big = WoundFlashSize(Point, To * Size);
		if (Intensity <= 0.f || Big <= 0.f)
		{
			return;
		}
		FParticle& Glow = AddParticle(EParticle::Flash, Point, FVector::ZeroVector, Life);
		Glow.Color = Color;
		Glow.Intensity = Intensity;
		Glow.StartSize = Big * From / FMath::Max(To, 0.01f);
		Glow.EndSize = Big;
		Glow.Gravity = 0.f;
	};

	switch (Blood)
	{
	case ECreatureBlood::Ichor:
		// The shell cracks: a small pale flash, shards of dark chitin and its pale bands flung hard.
		Flash(CritCrackColor, 12.f * Strength, 8.f, 22.f, 0.05f);
		Fling(VfxRules::Count(CritShards, Amount), BitSetFor(EBitShape::Plate, CritChitin, 0.f), 2.5f, 5.f, 380.f, 820.f, 1.f);
		Fling(VfxRules::Count(CritBands, Amount), BitSetFor(EBitShape::Plate, CritChitinBand, 0.f), 2.f, 3.5f, 340.f, 700.f, 1.f);
		break;
	case ECreatureBlood::DarkIchor:
	{
		// The Gravemother: thick gobs of her dark ichor lobbed up and falling heavily, and a dark gout of it.
		Fling(VfxRules::Count(CritThickGobs, Amount), BitSetFor(EBitShape::Round, Tint, 0.12f), 7.f, 11.f, 300.f, 520.f, 1.4f);
		if (Amount > 0.f)
		{
			FParticle& Gout = AddParticle(EParticle::Smoke, Point + Up * 4.f, (Up * 70.f - Shot * 40.f) * Size, 0.35f);
			Gout.Color = Tint * 0.6f;
			Gout.Intensity = FMath::Min(0.9f * Vfx.Smoke, 1.f);
			Gout.StartSize = 20.f * Size;
			Gout.EndSize = 80.f * Size;
			Gout.Gravity = 0.3f;
			Gout.Drag = 5.f;
		}
		break;
	}
	case ECreatureBlood::Gel:
	{
		// Through the gel to the core: a flash of the core and a ring of its light snapping out, a spray of gel.
		const FLinearColor Core = FMath::Lerp(Tint, FLinearColor::White, CritGelWhite);
		Flash(Core, 24.f * Strength, 16.f, 40.f, 0.08f);
		SpawnShockRing(Point, FVector::ZeroVector, 5.f * Size, WoundFlashSize(Point, 96.f * Size) * 0.5f, 3.f, Core, 14.f * Strength, 0.14f, true);
		Fling(VfxRules::Count(CritGelDrops, Amount), BitSetFor(EBitShape::Round, Tint, 0.45f), 2.f, 4.f, 260.f, 620.f, 1.f);
		break;
	}
	case ECreatureBlood::Embers:
	{
		// The coal flares where it's struck and the soul-light gleams out through the crack: a fireball of its colour, a soft
		// pale gleam, embers thrown up.
		SpawnFireball(Point + Up * 4.f, Up * 60.f, 14.f * Size, WoundFlashSize(Point, 52.f * Size), FMath::Lerp(Tint, CritEmberHeat, 0.3f),
			FMath::Min(2.6f * Strength, 3.f), 0.2f);
		Flash(CritSoulGleam, 16.f * Strength, 10.f, 30.f, 0.07f);
		const FLinearColor Hot = FMath::Lerp(Tint, CritEmberHeat, 0.4f);
		for (int32 Index = 0; Index < VfxRules::Count(CritEmbers, Amount); ++Index)
		{
			FParticle& Ember = AddParticle(EParticle::Spark, Point, Out(1.2f, 0.7f) * Random.FRandRange(240.f, 620.f) * Reach, Random.FRandRange(0.5f, 1.f));
			Ember.Color = FMath::Lerp(Tint, Hot, Random.FRandRange(0.f, 1.f));
			Ember.Intensity = Random.FRandRange(18.f, 30.f);
			Ember.StartSize = Random.FRandRange(0.8f, 1.3f);
			Ember.Gravity = -0.12f;
			Ember.Drag = 2.f;
		}
		break;
	}
	case ECreatureBlood::MinerEmbers:
	{
		// A dead miner: a smaller coal flare, and the mine's rock knocked out of him in chips and dust.
		SpawnFireball(Point + Up * 4.f, Up * 50.f, 10.f * Size, WoundFlashSize(Point, 36.f * Size), FMath::Lerp(Tint, CritEmberHeat, 0.3f),
			FMath::Min(2.2f * Strength, 3.f), 0.16f);
		for (int32 Index = 0; Index < VfxRules::Count(CritRockChips, Amount); ++Index)
		{
			FParticle& Chip = AddParticle(EParticle::Chip, Point, Out(0.9f, 0.7f) * Random.FRandRange(300.f, 700.f) * Reach, Random.FRandRange(0.6f, 0.9f));
			Chip.StartSize = Random.FRandRange(2.5f, 4.5f) * Size;
			Chip.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Chip.Spin = Random.GetUnitVector() * Random.FRandRange(8.f, 20.f);
		}
		if (Amount > 0.f)
		{
			FParticle& Dust = AddParticle(EParticle::Smoke, Point, (Up * 60.f + Random.GetUnitVector() * 30.f) * Reach, 0.9f);
			Dust.Color = CritRockDust;
			Dust.Intensity = FMath::Min(0.5f * Vfx.Smoke, 1.f);
			Dust.StartSize = 14.f * Size;
			Dust.EndSize = 64.f * Size;
			Dust.Gravity = -0.02f;
			Dust.Drag = 3.f;
		}
		break;
	}
	case ECreatureBlood::Clay:
		// A placeholder's clay: clods of it knocked out, falling heavily.
		Flash(FMath::Lerp(Tint, FLinearColor::White, 0.4f), 8.f * Strength, 14.f, 34.f, 0.07f);
		Fling(VfxRules::Count(CritClods, Amount), BitSetFor(EBitShape::Round, Tint, 0.15f), 4.f, 7.f, 260.f, 520.f, 1.3f);
		break;
	default:
		break;
	}
}

float FWeaponFX::WoundGlowFor(const FVector& Location)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	return VfxRules::GlowShare(WoundGlowStream.Note(Location, EffectsNow(), Vfx.HitGlowWindow, Vfx.HitGlowReach));
}

float FWeaponFX::WoundFlashSize(const FVector& Location, float Size) const
{
	// Before any camera is known (a test, the first frame) as given; else no wider on screen than Looter.Vfx.Hit.FlashMax.
	if (!bHasCamera)
	{
		return Size;
	}
	return VfxRules::HitFlashSizeAt(Size, static_cast<float>(FVector::Dist(Location, LastCamera)), FVfxTuning::Live().HitFlashMax);
}
