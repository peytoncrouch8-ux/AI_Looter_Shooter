#include "Weapons/WeaponFX.h"

// FWeaponFX's effects that aren't a gun's: grave dirt (the wake-up's claws), the slide's dust and grit, and a creature's
// death burst (a spider's shell, a slime's splat, an Unpaid's soul-light). Spawning only; WeaponFX.cpp moves and draws
// every particle.

namespace
{
	const FLinearColor GraveDirtColor(0.24f, 0.17f, 0.1f);
	/** The dust a body goes down in (unlit smoke: the dust as afternoon light shows it, a touch darker than a bullet's). */
	const FLinearColor DeathDustColor(0.5f, 0.45f, 0.37f);
	const FLinearColor DeathIchorFlash(0.85f, 1.f, 0.45f);
	/** The white-hot heart of soul-light, which the coal's color is mixed toward. */
	const FLinearColor SoulHeat(1.f, 0.95f, 0.85f);
	/** A ghost's last breath: pale, cool smoke. */
	const FLinearColor SoulWisp(0.5f, 0.55f, 0.6f);

	/** A random direction across the ground. */
	FVector Across(FRandomStream& Random)
	{
		const float Angle = Random.FRandRange(0.f, 2.f * UE_PI);
		return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	}
}

void FWeaponFX::SpawnDirt(const FVector& Location, const FVector& Up, float Strength)
{
	const FVector Rise = Up.IsNearlyZero() ? FVector::UpVector : Up.GetSafeNormal();
	const float Amount = FMath::Clamp(Strength, 0.f, 1.f);
	// Dark puffs that hang over the hole and spread...
	const int32 Puffs = 3 + FMath::RoundToInt32(5.f * Amount);
	for (int32 Index = 0; Index < Puffs; ++Index)
	{
		const FVector Velocity = Rise * Random.FRandRange(40.f, 140.f) * (0.6f + Amount) + Random.GetUnitVector() * 45.f;
		FParticle& Puff = AddParticle(EParticle::Smoke, Location + Random.GetUnitVector() * 12.f, Velocity, Random.FRandRange(0.9f, 1.6f));
		Puff.Color = GraveDirtColor;
		Puff.Intensity = 0.7f;
		Puff.StartSize = Random.FRandRange(14.f, 24.f);
		Puff.EndSize = Random.FRandRange(60.f, 110.f) * (0.7f + 0.5f * Amount);
		Puff.Gravity = 0.02f;
		Puff.Drag = 2.2f;
	}
	// ...and clods tossed up that fall back onto the heap.
	const int32 Clods = 4 + FMath::RoundToInt32(10.f * Amount);
	for (int32 Index = 0; Index < Clods; ++Index)
	{
		const FVector Direction = (Rise + Random.GetUnitVector() * 0.7f).GetSafeNormal();
		const float Speed = Random.FRandRange(180.f, 420.f) * (0.6f + 0.6f * Amount);
		FParticle& Clod = AddParticle(EParticle::Chip, Location, Direction * Speed, Random.FRandRange(0.7f, 1.2f));
		Clod.StartSize = Random.FRandRange(2.5f, 6.f);
		Clod.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
		Clod.Spin = Random.GetUnitVector() * Random.FRandRange(4.f, 12.f);
	}
}

void FWeaponFX::SpawnDustPuff(const FVector& Location, const FVector& Velocity, const FLinearColor& Color, float Opacity, float StartSize,
	float EndSize, float Life)
{
	FParticle& Puff = AddParticle(EParticle::Smoke, Location, Velocity, Life);
	Puff.Color = Color;
	Puff.Intensity = Opacity;
	Puff.StartSize = StartSize;
	Puff.EndSize = EndSize;
	Puff.Gravity = -0.02f;
	Puff.Drag = 2.8f;
}

void FWeaponFX::SpawnGrit(const FVector& Location, const FVector& Velocity, float Size, float Life)
{
	FParticle& Grain = AddParticle(EParticle::Chip, Location, Velocity, Life);
	Grain.StartSize = Size;
	Grain.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
	Grain.Spin = Random.GetUnitVector() * Random.FRandRange(8.f, 22.f);
	Grain.Drag = 0.3f;
}

void FWeaponFX::SpawnDeathBurst(EDeathBurst Kind, const FVector& Center, const FVector& ShotDirection, float Size, const FLinearColor& Tint)
{
	if (Kind == EDeathBurst::None)
	{
		return;
	}
	// Sizes scale with the body; speeds only with its square root, so a giant's burst is bigger but not absurdly far-flung.
	const float Scale = FMath::Clamp(Size / 100.f, 0.3f, 4.f);
	const float Reach = FMath::Sqrt(Scale);
	const FVector Up = FVector::UpVector;
	// The killing shot carries the burst a little its way (along the ground: never into it).
	const FVector Shot = ShotDirection.GetSafeNormal2D();

	auto Glow = [this, &Center](const FLinearColor& Color, float Intensity, float From, float To, float Life)
	{
		FParticle& Flash = AddParticle(EParticle::Flash, Center, FVector::ZeroVector, Life);
		Flash.Color = Color;
		Flash.Intensity = Intensity;
		Flash.StartSize = From;
		Flash.EndSize = To;
		Flash.Gravity = 0.f;
	};

	switch (Kind)
	{
	case EDeathBurst::Shell:
	{
		// The shell gives with a wet crack of light...
		Glow(DeathIchorFlash, 9.f, 18.f * Scale, 70.f * Scale, 0.1f);
		// ...the dust it was standing in rolls out round it...
		for (int32 Index = 0; Index < 5; ++Index)
		{
			const FVector Out = Across(Random);
			const FVector Velocity = Out * Random.FRandRange(90.f, 170.f) * Reach + Up * Random.FRandRange(30.f, 80.f) + Shot * 60.f;
			FParticle& Puff = AddParticle(EParticle::Smoke, Center + Out * 20.f * Scale - Up * 15.f * Scale, Velocity, Random.FRandRange(0.9f, 1.4f));
			Puff.Color = DeathDustColor;
			Puff.Intensity = 0.6f;
			Puff.StartSize = 20.f * Scale;
			Puff.EndSize = Random.FRandRange(90.f, 140.f) * Scale;
			Puff.Gravity = -0.03f;
			Puff.Drag = 2.4f;
		}
		// ...and plates of chitin fly, tumbling, with a splash of ichor.
		const uint8 Chitin = BitSetFor(false, Tint, 0.f);
		for (int32 Index = 0; Index < 14; ++Index)
		{
			const FVector Direction = (Up * Random.FRandRange(0.6f, 1.2f) + Random.GetUnitVector() * 0.7f + Shot * 0.5f).GetSafeNormal();
			FParticle& Plate = AddParticle(EParticle::Bit, Center, Direction * Random.FRandRange(280.f, 620.f) * Reach, Random.FRandRange(0.9f, 1.5f));
			Plate.BitSet = Chitin;
			Plate.StartSize = Random.FRandRange(3.f, 7.5f) * Scale;
			Plate.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Plate.Spin = Random.GetUnitVector() * Random.FRandRange(6.f, 18.f);
			Plate.Drag = 0.2f;
		}
		for (int32 Index = 0; Index < 8; ++Index)
		{
			const FVector Direction = (Up + Random.GetUnitVector() * 0.8f + Shot * 0.3f).GetSafeNormal();
			FParticle& Drop = AddParticle(EParticle::Droplet, Center, Direction * Random.FRandRange(220.f, 560.f) * Reach, Random.FRandRange(0.45f, 0.8f));
			Drop.StartSize = Random.FRandRange(2.f, 4.f) * Scale;
		}
		break;
	}
	case EDeathBurst::Gel:
	{
		// The jelly lets go all at once: a soft glow, gel flung out every way, and a wet spray low over the ground.
		Glow(Tint, 6.f, 26.f * Scale, 100.f * Scale, 0.12f);
		const uint8 Gel = BitSetFor(true, Tint, 0.45f);
		for (int32 Index = 0; Index < 18; ++Index)
		{
			const FVector Direction = (Across(Random) + Up * Random.FRandRange(0.35f, 1.1f) + Shot * 0.35f).GetSafeNormal();
			FParticle& Blob = AddParticle(EParticle::Bit, Center, Direction * Random.FRandRange(240.f, 560.f) * Reach, Random.FRandRange(0.7f, 1.1f));
			Blob.BitSet = Gel;
			Blob.StartSize = Random.FRandRange(3.f, 7.f) * Scale;
			Blob.Drag = 0.4f;
		}
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector Out = Across(Random);
			FParticle& Spray = AddParticle(EParticle::Smoke, Center - Up * 20.f * Scale, Out * Random.FRandRange(160.f, 260.f) + Up * 20.f,
				Random.FRandRange(0.45f, 0.7f));
			Spray.Color = Tint * 0.9f;
			Spray.Intensity = 0.45f;
			Spray.StartSize = 16.f * Scale;
			Spray.EndSize = Random.FRandRange(70.f, 100.f) * Scale;
			Spray.Gravity = 0.1f;
			Spray.Drag = 4.f;
		}
		break;
	}
	case EDeathBurst::SoulLight:
	{
		// What held it here goes up in light: a white-hot heart in a wider halo of its coal's color...
		const FLinearColor Hot = FMath::Lerp(Tint, SoulHeat, 0.45f);
		Glow(Hot, 36.f, 26.f * Scale, 120.f * Scale, 0.13f);
		Glow(Tint, 20.f, 60.f * Scale, 240.f * Scale, 0.32f);
		// ...motes of it rising and drifting apart as they dwindle...
		for (int32 Index = 0; Index < 16; ++Index)
		{
			const FVector Velocity = Across(Random) * Random.FRandRange(50.f, 160.f) * Reach + Up * Random.FRandRange(110.f, 240.f) * Reach;
			FParticle& Mote = AddParticle(EParticle::Flash, Center + Random.GetUnitVector() * 30.f * Scale, Velocity, Random.FRandRange(0.8f, 1.4f));
			Mote.Color = FMath::Lerp(Tint, Hot, Random.FRandRange(0.f, 0.5f));
			Mote.Intensity = Random.FRandRange(16.f, 28.f);
			Mote.StartSize = Random.FRandRange(7.f, 13.f) * Scale;
			Mote.EndSize = 1.5f * Scale;
			Mote.Gravity = -0.22f;
			Mote.Drag = 1.5f;
		}
		// ...sparks thrown off, mostly with the shot...
		for (int32 Index = 0; Index < 8; ++Index)
		{
			const FVector Direction = (Random.GetUnitVector() + Up * 0.3f + Shot * 0.4f).GetSafeNormal();
			FParticle& Spark = AddParticle(EParticle::Spark, Center, Direction * Random.FRandRange(550.f, 1050.f), Random.FRandRange(0.15f, 0.3f));
			Spark.Color = Tint;
			Spark.Intensity = 22.f;
			Spark.StartSize = 1.2f;
			Spark.Drag = 2.f;
		}
		// ...and a pale wisp of it left hanging.
		for (int32 Index = 0; Index < 3; ++Index)
		{
			FParticle& Wisp = AddParticle(EParticle::Smoke, Center, Up * Random.FRandRange(40.f, 90.f) + Random.GetUnitVector() * 25.f,
				Random.FRandRange(1.f, 1.5f));
			Wisp.Color = SoulWisp;
			Wisp.Intensity = 0.35f;
			Wisp.StartSize = 30.f * Scale;
			Wisp.EndSize = Random.FRandRange(120.f, 170.f) * Scale;
			Wisp.Gravity = -0.06f;
			Wisp.Drag = 2.f;
		}
		break;
	}
	default:
		break;
	}
}
