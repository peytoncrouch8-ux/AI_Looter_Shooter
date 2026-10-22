#include "Weapons/WeaponFX.h"
#include "Combat/GunfeelReactTuning.h"
#include "Combat/HitReaction.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/PlatformTime.h"

// FWeaponFX's bullet hits on creatures, each in what it spills (ECreatureBlood, picked by UBulletSubsystem from the
// creature's class): the spurt at the wound, Looter.Gunfeel.Spurt.Scale times, and an exit spray thrown on along the shot
// out behind the target (Spurt.Exit). A shotgun blast's pellets share one budget (Spurt.BlastBudget). Spawning only;
// WeaponFXDraw.cpp moves and draws every particle.
//
// The VFX pass (Saved/Vfx/PLAN.md; Looter.Vfx.Hit.*): every hit pops (a dense splat of what it spills that bursts and erodes,
// a pale wet flash; a coal's hot starburst), throws a couple of heavy gobs that arc and land, and its drops fly as streaks of
// spray (WeaponFXDraw.cpp); a crit adds a gold starburst and a ring snapping out round the wound; a heavy round (revolver,
// sniper) a ring of its own and a fuller exit. Far hits and a stream of fire into one spot throw less (DetailAt,
// StreamDetailFor); a hit's pop is never thinned (it's what reads a hit at 40 m).

namespace
{
	// Glows: only a hue (each is multiplied by its strength).
	const FLinearColor BloodIchorFlash(0.85f, 1.f, 0.45f);
	const FLinearColor BloodDarkIchorFlash(0.5f, 0.62f, 0.22f);
	const FLinearColor BloodEmberHeat(1.f, 0.9f, 0.7f);
	const FLinearColor BloodCritColor(1.f, 0.85f, 0.3f);
	/** A wet hit's flash: pale, so it reads as wet light catching the spray, not as sparks. */
	const FLinearColor BloodWetGlint(0.95f, 1.f, 0.9f);
	// Smoke as the unlit smoke material shows it.
	/** The spiders' ichor mist (SpawnImpact's Flesh had it). */
	const FLinearColor BloodIchorMist(0.55f, 0.66f, 0.1f);
	const FLinearColor BloodDarkIchorMist(0.22f, 0.25f, 0.1f);
	/** The grave dust an Unpaid is shaken out of: dry, grey-brown. */
	const FLinearColor BloodGraveDust(0.46f, 0.42f, 0.36f);
	/** A miner's rock dust: paler, the mine's stone. */
	const FLinearColor BloodRockDust(0.6f, 0.57f, 0.52f);
	// Lit chunks (linear).
	const FLinearColor BloodGraveDirt(0.12f, 0.09f, 0.06f);
	/** The spiders' ichor as a lit gob (the shared droplets' colour, a touch deeper). */
	const FLinearColor BloodIchorGob(0.42f, 0.52f, 0.06f);

	/** A shotgun's pellet (its mark's scale under this) spills about half a full hit's, as on the world: nine land together. */
	constexpr float PelletScale = 0.8f;
	constexpr float PelletShare = 0.5f;
	/** Pellets landing within this long and this far (cm) of a blast's first share its budget. */
	constexpr float BlastReach = 600.f;
	/** The exit spray starts this far in along the shot (cm, at a full hit), so it comes out of the far side of the body. */
	constexpr float ExitDepth = 8.f;

	// The pass's numbers (at Looter.Vfx.Hit.Scale 1, a rifle round's).
	/** The splat that bursts at the wound: from this size to that (cm), this opaque, this long. */
	constexpr float SplatFrom = 8.f;
	constexpr float SplatTo = 46.f;
	constexpr float SplatOpacity = 0.85f;
	constexpr float SplatLife = 0.22f;
	/** The hit's pop (a starburst; cm across at a rifle round), its strength and life. */
	constexpr float PopSize = 30.f;
	constexpr float PopGlow = 14.f;
	constexpr float PopLife = 0.06f;
	/** Gobs a hit throws (heavy, arcing, splatting on the ground), and their size (cm). */
	constexpr int32 HitGobs = 2;
	constexpr float GobSize[2] = { 3.5f, 6.f };
	/** A crit's ring round the wound: from, to (cm at a rifle round), thickness, strength, life. */
	constexpr float CritRingFrom = 6.f;
	constexpr float CritRingTo = 55.f;
	constexpr float CritRingWidth = 5.f;
	constexpr float CritRingGlow = 16.f;
	constexpr float CritRingLife = 0.18f;
	/** A heavy round's exit spray, times its usual share. */
	constexpr float HeavyExit = 1.6f;
}

float FWeaponFX::SpurtAmountFor(const FVector& Location, float Scale, float SpurtScale, float BlastBudget)
{
	if (Scale >= PelletScale)
	{
		return SpurtScale;
	}
	const float Want = PelletShare * SpurtScale;
	const UWorld* World = Owner.IsValid() ? Owner->GetWorld() : nullptr;
	const double Now = World ? World->GetTimeSeconds() : FPlatformTime::Seconds();
	if (Now - SpurtBlastAt > FHitReaction::BurstWindow || FVector::DistSquared(Location, SpurtBlastWhere) > FMath::Square(BlastReach))
	{
		SpurtBlastAt = Now;
		SpurtBlastWhere = Location;
		SpurtBlastSpent = 0.f;
	}
	const float Granted = GunfeelReact::BlastSpurtShare(BlastBudget * SpurtScale, SpurtBlastSpent, Want);
	SpurtBlastSpent += Granted;
	return Granted;
}

void FWeaponFX::SpawnCreatureHit(const FVector& Location, const FVector& Normal, const FVector& ShotDirection, ECreatureBlood Blood,
	const FLinearColor& Tint, float Scale, bool bCritical)
{
	// Anything that isn't a creature keeps the splash every hit on flesh had.
	if (Blood == ECreatureBlood::None)
	{
		SpawnImpact(Location, Normal, ShotDirection, EImpactSurface::Flesh, bCritical);
		return;
	}
	const FGunfeelReactTuning Tuning = FGunfeelReactTuning::Live();
	const FVfxTuning Vfx = FVfxTuning::Live();
	const FVector Shot = ShotDirection.GetSafeNormal();
	const FVector Up = Normal.IsNearlyZero() ? -Shot : Normal.GetSafeNormal();
	const FVector Point = Location + Up * 2.f;
	const float Size = FMath::Clamp(Scale, 0.4f, 1.6f);
	const float Reach = FMath::Sqrt(Size);
	const bool bPellet = Scale < PelletScale;
	const bool bHeavy = VfxRules::IsHeavyRound(Scale);
	const float Weight = VfxRules::HitWeight(Scale);
	// How much it spills: Spurt.Scale full hits' worth, a pellet half that, a blast's pellets sharing one budget.
	const float Amount = SpurtAmountFor(Location, Scale, Tuning.SpurtScale, Tuning.SpurtBlastBudget);
	// ...thinned far off and in a stream of fire into one spot (a blast's pellets have their budget instead).
	const float Detail = DetailAt(Location) * (bPellet ? 1.f : StreamDetailFor(Location));
	auto Many = [Amount, Detail, bCritical](int32 Full)
	{
		return Amount <= 0.f ? 0 : FMath::Max(1, FMath::RoundToInt32(Full * Amount * Detail * (bCritical ? 1.6f : 1.f)));
	};
	// The exit spray: its share of what the wound throws (none when Spurt.Exit is 0), fuller for a heavy round.
	auto ExitMany = [&Many, &Tuning, bHeavy](int32 Full)
	{
		return Tuning.SpurtExit <= 0.f ? 0 : FMath::RoundToInt32(Many(Full) * Tuning.SpurtExit * (bHeavy ? HeavyExit : 1.f));
	};
	// The pass's own pieces: none past a blast's budget, Looter.Vfx.Hit.Scale of them.
	const float Extra = Amount > 0.f ? Vfx.HitScale * Detail : 0.f;
	const FVector ExitPoint = Location + Shot * (ExitDepth * Size);

	auto Pop = [this, &Point, Size](const FLinearColor& Color, float Intensity, float From, float To, float Life)
	{
		FParticle& Flash = AddParticle(EParticle::Flash, Point, FVector::ZeroVector, Life);
		Flash.Color = Color;
		Flash.Intensity = Intensity;
		Flash.StartSize = From * Size;
		Flash.EndSize = To * Size;
		Flash.Gravity = 0.f;
	};
	// The wound's own flash: none for a pellet past its blast's budget.
	auto Glow = [&Pop, Amount](const FLinearColor& Color, float Intensity, float From, float To, float Life)
	{
		if (Amount > 0.f)
		{
			Pop(Color, Intensity, From, To, Life);
		}
	};
	auto Puffs = [this, &Point, &Up, Size, Reach](int32 Count, const FLinearColor& Color, float Opacity, float From, float To, float Life,
		float Fall)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FParticle& Puff = AddParticle(EParticle::Smoke, Point, (Up * Random.FRandRange(30.f, 90.f) + Random.GetUnitVector() * 30.f) * Reach,
				Life * Random.FRandRange(0.8f, 1.2f));
			Puff.Color = Color;
			Puff.Intensity = Opacity;
			Puff.StartSize = From * Size;
			Puff.EndSize = To * Size * Random.FRandRange(0.85f, 1.15f);
			Puff.Gravity = Fall;
			Puff.Drag = 3.f;
		}
	};
	// The splat: a dense puff of what it spills bursting out of the wound and eroding at once (M_FX_Puff), the hit's wet pop.
	auto Splat = [this, &Point, &Up, &Shot, Size, Weight, Extra, &Vfx](const FLinearColor& Color, float Opacity)
	{
		if (Extra <= 0.f)
		{
			return;
		}
		FParticle& Burst = AddParticle(EParticle::Smoke, Point + Up * 3.f, (Up * 60.f - Shot * 40.f) * Size, SplatLife * (0.9f + 0.2f * Weight));
		Burst.Color = Color;
		Burst.Intensity = FMath::Min(Opacity * Vfx.Smoke, 1.f);
		Burst.StartSize = SplatFrom * Weight;
		Burst.EndSize = SplatTo * Weight * FMath::Sqrt(Vfx.HitScale);
		Burst.Gravity = 0.25f;
		Burst.Drag = 6.f;
	};
	// The pop that reads at range: a starburst (never thinned by distance; a blast's spent pellets have none).
	auto StarPop = [this, &Point, Weight, Amount, &Vfx, bPellet](const FLinearColor& Color, float Strength, int32 Spikes)
	{
		if (Amount <= 0.f || Vfx.HitPop <= 0.f)
		{
			return;
		}
		SpawnStarburst(Point, PopSize * Weight * (bPellet ? 0.6f : 1.f), Color, PopGlow * Strength * Vfx.HitPop * Vfx.Glow, PopLife, Spikes);
	};
	// Lit chunks or drops flung out of the wound, a little back along the shot (they come out the side it went in).
	auto Fling = [this, &Point, &Up, &Shot, Size, Reach](int32 Count, uint8 Set, float MinSize, float MaxSize, float MinSpeed, float MaxSpeed,
		float Life, float Drag)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Direction = (Up + Random.GetUnitVector() * 0.75f - Shot * 0.2f).GetSafeNormal();
			FParticle& Bit = AddBit(Set, Point, Direction * Random.FRandRange(MinSpeed, MaxSpeed) * Reach, Random.FRandRange(MinSize, MaxSize) * Size,
				Life * Random.FRandRange(0.8f, 1.2f));
			Bit.Drag = Drag;
		}
	};
	// Gobs: a couple of heavy pieces of it lobbed up and out, arcing down (what gives a hit its weight in the air).
	auto Gobs = [this, &Point, &Up, &Shot, Size, Reach, Extra, bCritical](uint8 Set)
	{
		const int32 Count = VfxRules::Count(HitGobs + (bCritical ? 1 : 0), Extra);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Direction = (Up * 1.1f + Random.GetUnitVector() * 0.6f - Shot * 0.35f).GetSafeNormal();
			FParticle& Gob = AddBit(Set, Point, Direction * Random.FRandRange(260.f, 480.f) * Reach,
				Random.FRandRange(GobSize[0], GobSize[1]) * Size, Random.FRandRange(0.7f, 1.f));
			Gob.Drag = 0.2f;
			Gob.Gravity = 1.3f;
		}
	};
	// The spiders' ichor drops (the shared droplet component is ichor-coloured), from From along Way.
	auto IchorDrops = [this, Size, Reach](int32 Count, const FVector& From, const FVector& Way, float Spread, float MaxSpeed)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Direction = (Way + Random.GetUnitVector() * Spread).GetSafeNormal();
			FParticle& Drop = AddParticle(EParticle::Droplet, From, Direction * Random.FRandRange(200.f, MaxSpeed) * Reach, Random.FRandRange(0.45f, 0.7f));
			Drop.StartSize = Random.FRandRange(1.6f, 2.8f) * Size;
			Drop.Spin = Random.GetUnitVector() * Random.FRandRange(4.f, 10.f);
		}
	};
	// Embers: hot motes in the coal's color, from From mostly along Way, floating up as they cool.
	auto Embers = [this, &Tint, Reach](int32 Count, const FVector& From, const FVector& Way, bool bMotes)
	{
		const FLinearColor Hot = FMath::Lerp(Tint, BloodEmberHeat, 0.35f);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Direction = (Way + Random.GetUnitVector() * 0.7f).GetSafeNormal();
			FParticle& Ember = AddParticle(EParticle::Spark, From, Direction * Random.FRandRange(160.f, 480.f) * Reach, Random.FRandRange(0.45f, 0.9f));
			Ember.Color = FMath::Lerp(Tint, Hot, Random.FRandRange(0.f, 1.f));
			Ember.Intensity = Random.FRandRange(16.f, 28.f);
			Ember.StartSize = Random.FRandRange(0.7f, 1.2f);
			Ember.Gravity = -0.12f;
			Ember.Drag = 2.2f;
		}
		if (!bMotes)
		{
			return;
		}
		// A few round motes that hang a moment longer, drifting up.
		for (int32 Index = 0; Index < FMath::Max(1, Count / 3); ++Index)
		{
			FParticle& Mote = AddParticle(EParticle::Flash, From + Random.GetUnitVector() * 4.f, (FVector::UpVector * 60.f + Random.GetUnitVector() * 50.f) * Reach,
				Random.FRandRange(0.5f, 0.9f));
			Mote.Color = Hot;
			Mote.Intensity = Random.FRandRange(12.f, 20.f);
			Mote.StartSize = Random.FRandRange(3.f, 5.f);
			Mote.EndSize = 0.8f;
			Mote.Gravity = -0.25f;
			Mote.Drag = 1.5f;
		}
	};
	// A coal's flare where it's struck: a small fireball of its colour blooming and cooling in a blink.
	auto CoalFlare = [this, &Point, &Up, &Tint, Weight, Extra, &Vfx]()
	{
		if (Extra <= 0.f)
		{
			return;
		}
		SpawnFireball(Point + Up * 4.f, Up * 40.f, 10.f * Weight, 34.f * Weight, FMath::Lerp(Tint, BloodEmberHeat, 0.25f), 3.f * Vfx.Glow,
			0.16f);
	};
	// The exit spray: thrown on along the shot out of the body's far side, fast and in a narrow cone.
	auto ExitBits = [this, &ExitPoint, &Shot, Size, Reach](int32 Count, uint8 Set, float MinSize, float MaxSize)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Direction = (Shot + Random.GetUnitVector() * 0.35f + FVector::UpVector * 0.1f).GetSafeNormal();
			FParticle& Bit = AddBit(Set, ExitPoint, Direction * Random.FRandRange(260.f, 700.f) * Reach, Random.FRandRange(MinSize, MaxSize) * Size,
				Random.FRandRange(0.5f, 0.8f));
			Bit.Drag = 0.3f;
		}
	};
	const FVector EmberWay = (Up * 0.8f - Shot * 0.25f).GetSafeNormal();

	switch (Blood)
	{
	case ECreatureBlood::Ichor:
	{
		// A spider's yellow-green splash: a wet flash, drops thrown out of the wound, a breath of ichor mist; a splat bursting
		// out of the wound and a couple of heavy gobs (the pass).
		Glow(BloodIchorFlash, 7.f, 14.f, 26.f, 0.06f);
		StarPop(FMath::Lerp(BloodIchorFlash, BloodWetGlint, 0.5f), 0.6f, 5);
		Splat(BloodIchorMist, SplatOpacity);
		IchorDrops(Many(7), Point, Up - Shot * 0.2f, 0.7f, bCritical ? 750.f : 550.f);
		Gobs(BitSetFor(EBitShape::Round, BloodIchorGob, 0.3f));
		Puffs(Many(1), BloodIchorMist, 0.65f, 10.f, bCritical ? 50.f : 36.f, 0.38f, 0.15f);
		IchorDrops(ExitMany(7), ExitPoint, Shot, 0.35f, 700.f);
		break;
	}
	case ECreatureBlood::DarkIchor:
	{
		// Thick and dark: a dull wet flash, heavy drops that fall fast, a dark mist; a dark splat and heavier gobs.
		Glow(BloodDarkIchorFlash, 5.f, 16.f, 30.f, 0.07f);
		StarPop(BloodDarkIchorFlash, 0.5f, 5);
		Splat(BloodDarkIchorMist, SplatOpacity);
		const uint8 Dark = BitSetFor(EBitShape::Round, Tint, 0.12f);
		Fling(Many(9), Dark, 2.2f, 4.2f, 220.f, 620.f, 0.7f, 0.15f);
		Gobs(Dark);
		Puffs(Many(1), BloodDarkIchorMist, 0.7f, 12.f, 46.f, 0.45f, 0.2f);
		ExitBits(ExitMany(9), Dark, 2.f, 3.8f);
		break;
	}
	case ECreatureBlood::Gel:
	{
		// Gel in the slime's own color: a soft glow of it, blobs flung wide and low, a wet spray; a gel splat, two fat blobs.
		Glow(Tint, 6.f, 16.f, 34.f, 0.08f);
		StarPop(FMath::Lerp(Tint, BloodWetGlint, 0.6f), 0.55f, 5);
		Splat(Tint * 0.9f, SplatOpacity * 0.8f);
		const uint8 Gel = BitSetFor(EBitShape::Round, Tint, 0.45f);
		for (int32 Index = 0; Index < Many(8); ++Index)
		{
			const FVector Direction = (Up * 0.6f + Random.GetUnitVector() * 0.9f - Shot * 0.2f).GetSafeNormal();
			FParticle& Blob = AddBit(Gel, Point, Direction * Random.FRandRange(180.f, 520.f) * Reach, Random.FRandRange(2.f, 4.5f) * Size,
				Random.FRandRange(0.5f, 0.85f));
			Blob.Drag = 0.4f;
		}
		Gobs(Gel);
		Puffs(Many(1), Tint * 0.9f, 0.4f, 12.f, 42.f, 0.4f, 0.12f);
		ExitBits(ExitMany(8), Gel, 1.8f, 4.f);
		break;
	}
	case ECreatureBlood::Embers:
	{
		// What burns in it flares where it's struck: embers and motes in the coal's color, and the grave dust it's made of;
		// the coal's hot starburst and a blink of fire (the pass).
		Glow(FMath::Lerp(Tint, BloodEmberHeat, 0.4f), 14.f, 10.f, 26.f, 0.07f);
		StarPop(FMath::Lerp(Tint, BloodEmberHeat, 0.5f), 1.f, 7);
		CoalFlare();
		Embers(Many(9), Point, EmberWay, true);
		Puffs(Many(2), BloodGraveDust, 0.5f, 12.f, 55.f, 0.8f, -0.03f);
		Splat(BloodGraveDust, 0.6f);
		Fling(Many(3), BitSetFor(EBitShape::Plate, BloodGraveDirt, 0.f), 1.2f, 2.6f, 180.f, 420.f, 0.8f, 0.4f);
		// Out of its back: a spray of embers along the shot.
		Embers(ExitMany(9), ExitPoint, Shot, false);
		break;
	}
	case ECreatureBlood::MinerEmbers:
	{
		// A dead miner: the same embers and grave dust, and the mine's rock dust and grit shaken out of his clothes.
		Glow(FMath::Lerp(Tint, BloodEmberHeat, 0.4f), 12.f, 10.f, 24.f, 0.07f);
		StarPop(FMath::Lerp(Tint, BloodEmberHeat, 0.5f), 0.9f, 7);
		CoalFlare();
		Embers(Many(7), Point, EmberWay, true);
		Puffs(Many(1), BloodGraveDust, 0.5f, 12.f, 50.f, 0.8f, -0.03f);
		Puffs(Many(1), BloodRockDust, 0.45f, 10.f, 48.f, 0.9f, -0.02f);
		Splat(BloodRockDust, 0.6f);
		for (int32 Index = 0; Index < Many(4); ++Index)
		{
			const FVector Direction = (Up * 0.9f + Random.GetUnitVector() * 0.6f - Shot * 0.2f).GetSafeNormal();
			FParticle& Chip = AddParticle(EParticle::Chip, Point, Direction * Random.FRandRange(200.f, 520.f) * Reach, Random.FRandRange(0.5f, 0.8f));
			Chip.StartSize = Random.FRandRange(1.2f, 2.4f) * Size;
			Chip.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Chip.Spin = Random.GetUnitVector() * Random.FRandRange(8.f, 20.f);
		}
		Embers(ExitMany(7), ExitPoint, Shot, false);
		break;
	}
	default:
		break;
	}

	if (Amount > 0.f && Extra > 0.f && bHeavy && !bCritical)
	{
		// A heavy round's thump: a pale ring snapping out round the wound, facing the eye.
		SpawnShockRing(Point, FVector::ZeroVector, 4.f * Weight, 34.f * Weight, 3.5f, BloodWetGlint, 7.f * Vfx.Glow, 0.14f, true);
	}
	if (bCritical)
	{
		// Critical hits get a bright pop on top, whatever they hit (even a pellet past its blast's budget)...
		Pop(BloodCritColor, 28.f, 26.f, 60.f, 0.1f);
		// ...and (the pass) a gold starburst and a ring snapping out round the wound: the crit reads at any range.
		if (Vfx.HitScale > 0.f)
		{
			SpawnStarburst(Point, 52.f * Weight, BloodCritColor, 24.f * Vfx.Glow * Vfx.HitPop, 0.09f, 10);
			SpawnShockRing(Point, FVector::ZeroVector, CritRingFrom * Weight, CritRingTo * Weight, CritRingWidth, BloodCritColor,
				CritRingGlow * Vfx.Glow, CritRingLife, true);
		}
	}
}
