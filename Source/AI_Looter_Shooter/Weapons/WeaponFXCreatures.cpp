#include "Weapons/WeaponFX.h"

// FWeaponFX's bullet hits on creatures, each in what it spills (ECreatureBlood, picked by UBulletSubsystem from the
// creature's class). Spawning only; WeaponFXDraw.cpp moves and draws every particle.

namespace
{
	// Glows: only a hue (each is multiplied by its strength).
	const FLinearColor BloodDarkIchorFlash(0.5f, 0.62f, 0.22f);
	const FLinearColor BloodEmberHeat(1.f, 0.9f, 0.7f);
	const FLinearColor BloodCritColor(1.f, 0.85f, 0.3f);
	// Smoke as the unlit smoke material shows it.
	const FLinearColor BloodDarkIchorMist(0.22f, 0.25f, 0.1f);
	/** The grave dust an Unpaid is shaken out of: dry, grey-brown. */
	const FLinearColor BloodGraveDust(0.46f, 0.42f, 0.36f);
	/** A miner's rock dust: paler, the mine's stone. */
	const FLinearColor BloodRockDust(0.6f, 0.57f, 0.52f);
	// Lit chunks (linear).
	const FLinearColor BloodGraveDirt(0.12f, 0.09f, 0.06f);
}

void FWeaponFX::SpawnCreatureHit(const FVector& Location, const FVector& Normal, const FVector& ShotDirection, ECreatureBlood Blood,
	const FLinearColor& Tint, float Scale, bool bCritical)
{
	// Spiders keep the splash every creature hit had (and anything that isn't a creature gets it too).
	if (Blood == ECreatureBlood::Ichor || Blood == ECreatureBlood::None)
	{
		SpawnImpact(Location, Normal, ShotDirection, EImpactSurface::Flesh, bCritical);
		return;
	}
	const FVector Shot = ShotDirection.GetSafeNormal();
	const FVector Up = Normal.IsNearlyZero() ? -Shot : Normal.GetSafeNormal();
	const FVector Point = Location + Up * 2.f;
	const float Size = FMath::Clamp(Scale, 0.4f, 1.6f);
	const float Reach = FMath::Sqrt(Size);
	// A shotgun's pellets spill about half as much each, as on the world: nine of them land together.
	const float Share = Size < 0.8f ? 0.5f : 1.f;
	auto Many = [Share, bCritical](int32 Full) { return FMath::Max(1, FMath::RoundToInt32(Full * Share * (bCritical ? 1.6f : 1.f))); };

	auto Glow = [this, &Point, Size](const FLinearColor& Color, float Intensity, float From, float To, float Life)
	{
		FParticle& Flash = AddParticle(EParticle::Flash, Point, FVector::ZeroVector, Life);
		Flash.Color = Color;
		Flash.Intensity = Intensity;
		Flash.StartSize = From * Size;
		Flash.EndSize = To * Size;
		Flash.Gravity = 0.f;
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
	// Embers: hot motes in the coal's color thrown out of the wound, floating up as they cool.
	auto Embers = [this, &Point, &Up, &Shot, &Tint, Reach](int32 Count)
	{
		const FLinearColor Hot = FMath::Lerp(Tint, BloodEmberHeat, 0.35f);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Direction = (Up * 0.8f + Random.GetUnitVector() * 0.7f - Shot * 0.25f).GetSafeNormal();
			FParticle& Ember = AddParticle(EParticle::Spark, Point, Direction * Random.FRandRange(160.f, 480.f) * Reach, Random.FRandRange(0.45f, 0.9f));
			Ember.Color = FMath::Lerp(Tint, Hot, Random.FRandRange(0.f, 1.f));
			Ember.Intensity = Random.FRandRange(16.f, 28.f);
			Ember.StartSize = Random.FRandRange(0.7f, 1.2f);
			Ember.Gravity = -0.12f;
			Ember.Drag = 2.2f;
		}
		// A few round motes that hang a moment longer, drifting up.
		for (int32 Index = 0; Index < FMath::Max(1, Count / 3); ++Index)
		{
			FParticle& Mote = AddParticle(EParticle::Flash, Point + Random.GetUnitVector() * 4.f, (Up * 60.f + Random.GetUnitVector() * 50.f) * Reach,
				Random.FRandRange(0.5f, 0.9f));
			Mote.Color = Hot;
			Mote.Intensity = Random.FRandRange(12.f, 20.f);
			Mote.StartSize = Random.FRandRange(3.f, 5.f);
			Mote.EndSize = 0.8f;
			Mote.Gravity = -0.25f;
			Mote.Drag = 1.5f;
		}
	};

	switch (Blood)
	{
	case ECreatureBlood::DarkIchor:
	{
		// Thick and dark: a dull wet flash, heavy drops that fall fast, a dark mist.
		Glow(BloodDarkIchorFlash, 5.f, 16.f, 30.f, 0.07f);
		Fling(Many(9), BitSetFor(EBitShape::Round, Tint, 0.12f), 2.2f, 4.2f, 220.f, 620.f, 0.7f, 0.15f);
		Puffs(Many(1), BloodDarkIchorMist, 0.7f, 12.f, 46.f, 0.45f, 0.2f);
		break;
	}
	case ECreatureBlood::Gel:
	{
		// Gel in the slime's own color: a soft glow of it, blobs flung wide and low, a wet spray.
		Glow(Tint, 6.f, 16.f, 34.f, 0.08f);
		const uint8 Gel = BitSetFor(EBitShape::Round, Tint, 0.45f);
		for (int32 Index = 0; Index < Many(8); ++Index)
		{
			const FVector Direction = (Up * 0.6f + Random.GetUnitVector() * 0.9f - Shot * 0.2f).GetSafeNormal();
			FParticle& Blob = AddBit(Gel, Point, Direction * Random.FRandRange(180.f, 520.f) * Reach, Random.FRandRange(2.f, 4.5f) * Size,
				Random.FRandRange(0.5f, 0.85f));
			Blob.Drag = 0.4f;
		}
		Puffs(Many(1), Tint * 0.9f, 0.4f, 12.f, 42.f, 0.4f, 0.12f);
		break;
	}
	case ECreatureBlood::Embers:
	{
		// What burns in it flares where it's struck: embers and motes in the coal's color, and the grave dust it's made of.
		Glow(FMath::Lerp(Tint, BloodEmberHeat, 0.4f), 14.f, 10.f, 26.f, 0.07f);
		Embers(Many(9));
		Puffs(Many(2), BloodGraveDust, 0.5f, 12.f, 55.f, 0.8f, -0.03f);
		Fling(Many(3), BitSetFor(EBitShape::Plate, BloodGraveDirt, 0.f), 1.2f, 2.6f, 180.f, 420.f, 0.8f, 0.4f);
		break;
	}
	case ECreatureBlood::MinerEmbers:
	{
		// A dead miner: the same embers and grave dust, and the mine's rock dust and grit shaken out of his clothes.
		Glow(FMath::Lerp(Tint, BloodEmberHeat, 0.4f), 12.f, 10.f, 24.f, 0.07f);
		Embers(Many(7));
		Puffs(Many(1), BloodGraveDust, 0.5f, 12.f, 50.f, 0.8f, -0.03f);
		Puffs(Many(1), BloodRockDust, 0.45f, 10.f, 48.f, 0.9f, -0.02f);
		for (int32 Index = 0; Index < Many(4); ++Index)
		{
			const FVector Direction = (Up * 0.9f + Random.GetUnitVector() * 0.6f - Shot * 0.2f).GetSafeNormal();
			FParticle& Chip = AddParticle(EParticle::Chip, Point, Direction * Random.FRandRange(200.f, 520.f) * Reach, Random.FRandRange(0.5f, 0.8f));
			Chip.StartSize = Random.FRandRange(1.2f, 2.4f) * Size;
			Chip.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Chip.Spin = Random.GetUnitVector() * Random.FRandRange(8.f, 20.f);
		}
		break;
	}
	default:
		break;
	}

	if (bCritical)
	{
		// Critical hits get a bright pop on top, whatever they hit.
		Glow(BloodCritColor, 28.f, 26.f, 60.f, 0.1f);
	}
}
