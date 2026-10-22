#include "Weapons/WeaponFX.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"
#include "Templates/UnrealTemplate.h"

// FWeaponFX's kills and blasts (the VFX pass, Saved/Vfx/PLAN.md; Looter.Vfx.Kill.*, .Blast.*). Spawning only; WeaponFXDraw.cpp
// moves and draws every particle.
//  - SpawnKillLayers: a creature's kill on top of its kind's death burst (SpawnDeathBurst, which the hit reaction plays):
//    gore thrown with the killing shot by the gun (a shotgun's carries it, a sniper's crit blows it out the back), a mist of
//    what it spilled hanging and settling, a skirt of dust rolling out where it drops, a soul-light's ring racing over the
//    ground and its ash collapsing, a crit's gold flare, and its tier's rings, star and motes. Chunks land on the ground
//    under it and lie a moment (the floor the kill found).
//  - SpawnExplosion: a white crack, fireballs blooming and cooling (white-hot to the fire's colour to a dull ember), a ring
//    racing over the ground and a faint one in the air, a smoke skirt and a rising column (pale for a soul blast), embers
//    arcing, and the ground thrown up in its own look.

namespace
{
	/** Unlit dust and ash as afternoon light shows them (times the kill's dust light). */
	const FLinearColor KillDust(0.5f, 0.45f, 0.37f);
	const FLinearColor KillAsh(0.47f, 0.45f, 0.43f);
	const FLinearColor KillSoot(0.2f, 0.18f, 0.16f);
	const FLinearColor KillCritGold(1.f, 0.85f, 0.3f);
	const FLinearColor KillWhiteHot(1.f, 0.95f, 0.85f);
	const FLinearColor KillRockDust(0.6f, 0.57f, 0.52f);
	/** A blast's smoke: a fire's soot, a soul blast's pale breath. */
	const FLinearColor BlastSoot(0.24f, 0.22f, 0.2f);
	const FLinearColor BlastPale(0.62f, 0.62f, 0.66f);

	/** A random way across a surface whose up is Up. */
	FVector AcrossSurface(const FVector& Up, FRandomStream& Random)
	{
		const FVector Any = Random.GetUnitVector();
		const FVector Flat = (Any - Up * FVector::DotProduct(Any, Up)).GetSafeNormal();
		return Flat.IsNearlyZero() ? FVector::ForwardVector : Flat;
	}
}

void FWeaponFX::SpawnKillLayers(const FVfxKill& Kill)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.KillScale <= 0.f)
	{
		return;
	}
	const float Weight = VfxRules::KillWeight(Kill.Gun, Kill.bCritical, Kill.Share) * Vfx.KillScale;
	const float Carry = VfxRules::KillCarry(Kill.Gun);
	const float Scale = FMath::Clamp(Kill.Size / 100.f, 0.3f, 4.f);
	const float Reach = FMath::Sqrt(Scale);
	const float Amount = Weight * DetailAt(Kill.Center);
	auto N = [Amount](int32 Full) { return VfxRules::Count(Full, Amount); };
	const FVector Up = Kill.bGround ? Kill.GroundNormal.GetSafeNormal() : FVector::UpVector;
	const FVector Ground = Kill.bGround ? Kill.Ground : Kill.Center - FVector(0.0, 0.0, 40.0 * Scale);
	// The killing shot's way along the ground: what's thrown goes with it (never into the ground).
	const FVector Shot = Kill.Direction.GetSafeNormal2D();
	// What's thrown lands on the ground under it and lies there a moment.
	TGuardValue<TOptional<FPlane>> FloorDuringSpawn(SpawnFloor, Kill.bGround ? TOptional<FPlane>(FPlane(Ground, Up)) : TOptional<FPlane>());

	// Gore thrown with the shot: Count pieces of Set, flung from the body up and along the shot by the gun's carry.
	auto Gore = [this, &Kill, &Shot, Carry, Scale, Reach](int32 Count, uint8 Set, float MinSize, float MaxSize, float MinSpeed, float MaxSpeed)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector Direction = (FVector::UpVector * Random.FRandRange(0.5f, 1.1f) + Shot * (Carry * 1.6f) + Random.GetUnitVector() * 0.6f).GetSafeNormal();
			FParticle& Bit = AddBit(Set, Kill.Center, Direction * Random.FRandRange(MinSpeed, MaxSpeed) * Reach, Random.FRandRange(MinSize, MaxSize) * Scale,
				Random.FRandRange(0.9f, 1.4f));
			Bit.Drag = 0.25f;
		}
	};
	// A mist of what it spilled: hanging over the body, spreading and settling slowly.
	auto Mist = [this, &Kill, &Shot, Scale, Reach, &Vfx](int32 Count, const FLinearColor& Color, float Opacity, float To, float Life)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FParticle& Puff = AddParticle(EParticle::Smoke, Kill.Center + Random.GetUnitVector() * 15.f * Scale,
				(Shot * 70.f + FVector(0.0, 0.0, 40.0) + Random.GetUnitVector() * 50.f) * Reach, Life * Random.FRandRange(0.85f, 1.15f));
			Puff.Color = Color * Kill.DustLight;
			Puff.Intensity = FMath::Min(Opacity * Vfx.Smoke, 1.f);
			Puff.StartSize = 30.f * Scale;
			Puff.EndSize = To * Scale * Random.FRandRange(0.85f, 1.15f);
			Puff.Gravity = 0.04f;
			Puff.Drag = 2.5f;
		}
	};
	// A skirt of dust (or spray) rolling out over the ground where it drops.
	FVector SideA, SideB;
	Up.FindBestAxisVectors(SideA, SideB);
	auto Skirt = [this, &Ground, &Up, &SideA, &SideB, &Kill, Scale, Reach, &Vfx](int32 Count, const FLinearColor& Color, float Opacity, float To)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float Angle = UE_TWO_PI * (static_cast<float>(Index) + Random.FRandRange(-0.3f, 0.3f)) / FMath::Max(Count, 1);
			const FVector Out = SideA * FMath::Cos(Angle) + SideB * FMath::Sin(Angle);
			FParticle& Puff = AddParticle(EParticle::Smoke, Ground + Up * 10.f * Scale + Out * 25.f * Scale,
				(Out * Random.FRandRange(220.f, 340.f) + Up * 25.f) * Reach, Random.FRandRange(1.f, 1.5f));
			Puff.Color = Color * Kill.DustLight;
			Puff.Intensity = FMath::Min(Opacity * Vfx.Smoke, 1.f);
			Puff.StartSize = 22.f * Scale;
			Puff.EndSize = To * Scale * Random.FRandRange(0.85f, 1.15f);
			Puff.Gravity = -0.02f;
			Puff.Drag = 3.2f;
		}
	};

	switch (Kill.Blood)
	{
	case ECreatureBlood::Ichor:
	case ECreatureBlood::DarkIchor:
	{
		// The body bursts with the shot: gobs of ichor, shards of shell and a leg piece or two, a wet mist and the dust it drops in.
		const uint8 Gobs = BitSetFor(EBitShape::Round, Kill.Spill, 0.25f);
		const uint8 Shards = BitSetFor(EBitShape::Plate, Kill.Tint, 0.f);
		const uint8 Legs = BitSetFor(EBitShape::Splinter, Kill.Tint * 0.6f, 0.f);
		Gore(N(8), Gobs, 3.f, 6.f, 260.f, 560.f);
		Gore(N(6), Shards, 4.f, 8.f, 300.f, 620.f);
		Gore(N(3), Legs, 6.f, 10.f, 240.f, 480.f);
		Mist(N(3), Kill.Spill, 0.55f, 150.f, 1.4f);
		Skirt(N(6), KillDust, 0.45f, 110.f);
		break;
	}
	case ECreatureBlood::Gel:
	{
		// The jelly goes everywhere: fat blobs with the shot, a gel mist, a low spray rolling out over the ground.
		const uint8 Blobs = BitSetFor(EBitShape::Round, Kill.Tint, 0.45f);
		Gore(N(8), Blobs, 4.f, 8.f, 220.f, 520.f);
		Mist(N(2), Kill.Tint * 0.85f, 0.45f, 130.f, 1.1f);
		Skirt(N(5), Kill.Tint * 0.8f, 0.4f, 100.f);
		break;
	}
	case ECreatureBlood::Clay:
	{
		// A placeholder comes apart in clods of its clay (its spill's colour), a puff of its dust and a skirt of it on the ground.
		const uint8 Clods = BitSetFor(EBitShape::Round, Kill.Spill, 0.15f);
		Gore(N(9), Clods, 4.f, 9.f, 200.f, 480.f);
		Mist(N(2), Kill.Spill * 0.8f, 0.45f, 130.f, 1.2f);
		Skirt(N(5), Kill.Spill * 0.75f, 0.45f, 105.f);
		break;
	}
	case ECreatureBlood::Embers:
	case ECreatureBlood::MinerEmbers:
	{
		// Its soul goes: a ring of its coal's light racing over the ground, its ash collapsing in a heap of dust, embers
		// fountaining up out of it.
		if (Vfx.KillRing > 0.f)
		{
			SpawnShockRing(Ground + Up * 4.f, Up, 30.f * Scale, 230.f * Scale * FMath::Sqrt(Weight), 12.f * Reach, Kill.Tint, 14.f * Vfx.Glow * Vfx.KillRing,
				0.42f);
		}
		for (int32 Index = 0; Index < N(6); ++Index)
		{
			FParticle& Ash = AddParticle(EParticle::Smoke, Kill.Center + Random.GetUnitVector() * 25.f * Scale,
				FVector(0.0, 0.0, -60.0) + AcrossSurface(Up, Random) * Random.FRandRange(60.f, 140.f), Random.FRandRange(1.2f, 1.8f));
			Ash.Color = KillAsh * Kill.DustLight;
			Ash.Intensity = 0.5f * Vfx.Smoke;
			Ash.StartSize = 30.f * Scale;
			Ash.EndSize = Random.FRandRange(110.f, 160.f) * Scale;
			Ash.Gravity = 0.12f;
			Ash.Drag = 2.f;
		}
		const FLinearColor Hot = FMath::Lerp(Kill.Tint, KillWhiteHot, 0.35f);
		for (int32 Index = 0; Index < N(14); ++Index)
		{
			const FVector Way = (FVector::UpVector * 1.4f + Random.GetUnitVector() * 0.7f + Shot * 0.3f).GetSafeNormal();
			FParticle& Ember = AddParticle(EParticle::Spark, Kill.Center, Way * Random.FRandRange(220.f, 520.f) * Reach, Random.FRandRange(0.8f, 1.4f));
			Ember.Color = FMath::Lerp(Kill.Tint, Hot, Random.FRandRange(0.f, 1.f));
			Ember.Intensity = Random.FRandRange(14.f, 24.f) * Vfx.Glow;
			Ember.StartSize = Random.FRandRange(0.8f, 1.4f);
			Ember.Gravity = -0.18f;
			Ember.Drag = 1.6f;
		}
		if (Kill.Blood == ECreatureBlood::MinerEmbers)
		{
			// A miner's rock dust and a few stones out of his pockets.
			Mist(N(2), KillRockDust, 0.4f, 120.f, 1.3f);
			for (int32 Index = 0; Index < N(4); ++Index)
			{
				const FVector Direction = (FVector::UpVector + Random.GetUnitVector() * 0.6f + Shot * 0.4f).GetSafeNormal();
				FParticle& Chip = AddParticle(EParticle::Chip, Kill.Center, Direction * Random.FRandRange(220.f, 480.f) * Reach, Random.FRandRange(0.9f, 1.3f));
				Chip.StartSize = Random.FRandRange(2.5f, 4.5f) * Scale;
				Chip.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
				Chip.Spin = Random.GetUnitVector() * Random.FRandRange(6.f, 16.f);
			}
		}
		break;
	}
	default:
		break;
	}

	// The gun's own mark on the kill: a rocket's soot and cinders; a crit's gold flare at the wound.
	if (Kill.Gun == EWeaponKind::Launcher)
	{
		Mist(N(3), KillSoot, 0.55f, 160.f, 1.8f);
	}
	if (Kill.bCritical)
	{
		const FVector Wound = Kill.HitAt.IsNearlyZero() ? Kill.Center : Kill.HitAt;
		SpawnStarburst(Wound, 90.f * Reach, KillCritGold, 26.f * Vfx.Glow, 0.12f, 12);
		SpawnShockRing(Wound, FVector::ZeroVector, 10.f * Reach, 110.f * Reach, 7.f, KillCritGold, 14.f * Vfx.Glow, 0.24f, true);
	}

	// Its tier: rings of its colour racing over the ground, a star of it on the body, motes rising (small under a top tier's
	// own show: UCreatureTierFXSubsystem plays the big moment).
	const FVfxTierLayers Tier = VfxRules::TierLayers(Kill.Rank);
	if (Tier.Star > 0.f)
	{
		SpawnStarburst(Kill.Center, 70.f * Tier.Star * Reach, Kill.TierColor, 20.f * Vfx.Glow, 0.14f, 10);
	}
	for (int32 Ring = 0; Ring < Tier.Rings && Vfx.KillRing > 0.f; ++Ring)
	{
		// The second slower and wider: two rings racing out, one after the other.
		const float Later = 1.f + 0.6f * static_cast<float>(Ring);
		SpawnShockRing(Ground + Up * (5.f + 2.f * Ring), Up, 20.f * Scale, 100.f * Tier.RingReach * Scale * Later, 9.f * Reach, Kill.TierColor,
			16.f * Vfx.Glow * Vfx.KillRing / Later, 0.38f * Later);
	}
	for (int32 Index = 0; Index < Tier.Motes; ++Index)
	{
		FParticle& Mote = AddParticle(EParticle::Flash, Kill.Center + Random.GetUnitVector() * 30.f * Scale,
			AcrossSurface(Up, Random) * Random.FRandRange(30.f, 90.f) + FVector(0.0, 0.0, Random.FRandRange(120.f, 240.f)), Random.FRandRange(0.9f, 1.5f));
		Mote.Color = Kill.TierColor;
		Mote.Intensity = Random.FRandRange(14.f, 22.f) * Vfx.Glow;
		Mote.StartSize = Random.FRandRange(5.f, 9.f) * Reach;
		Mote.EndSize = 1.f;
		Mote.Gravity = -0.2f;
		Mote.Drag = 1.4f;
	}
}

void FWeaponFX::SpawnExplosion(const FVfxExplosion& Blast)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.BlastScale <= 0.f)
	{
		return;
	}
	const float Radius = FMath::Max(Blast.Radius * Vfx.BlastScale, 50.f);
	const FVfxExplosionLook Look = VfxRules::ExplosionLook(Radius);
	const FVector Up = Blast.Up.IsNearlyZero() ? FVector::UpVector : Blast.Up.GetSafeNormal();
	const FVector Center = Blast.Center;
	const float Detail = DetailAt(Center);
	auto N = [Detail](int32 Full) { return VfxRules::Count(Full, Detail); };
	const FLinearColor Fire = Blast.Fire;
	const FLinearColor SmokeColor = (Blast.bSoul ? BlastPale : BlastSoot) * Blast.DustLight;
	TGuardValue<TOptional<FPlane>> FloorDuringSpawn(SpawnFloor, Blast.bOnGround ? TOptional<FPlane>(FPlane(Center, Up)) : TOptional<FPlane>());

	// The crack: a white starburst and a flash, gone in a blink.
	SpawnStarburst(Center + Up * Radius * 0.1f, Radius * 0.95f, KillWhiteHot, 18.f * Vfx.Glow, 0.09f, 14);
	FParticle& Crack = AddParticle(EParticle::Flash, Center + Up * Radius * 0.1f, FVector::ZeroVector, 0.07f);
	Crack.Color = KillWhiteHot;
	Crack.Intensity = 18.f * Vfx.Glow;
	Crack.StartSize = Radius * 0.35f;
	Crack.EndSize = Radius * 0.8f;
	Crack.Gravity = 0.f;

	// The fireballs: a cluster blooming out and up from the heart, each cooling white to its colour to a dull ember.
	for (int32 Index = 0; Index < Look.Fireballs; ++Index)
	{
		const FVector Out = (Up * Random.FRandRange(0.3f, 1.f) + Random.GetUnitVector() * 0.8f).GetSafeNormal();
		const float Life = Look.FireballLife * Random.FRandRange(0.8f, 1.2f);
		SpawnFireball(Center + Out * Radius * Random.FRandRange(0.05f, 0.25f), Out * Radius * Random.FRandRange(0.6f, 1.1f) + Up * Radius * 0.4f,
			Radius * Random.FRandRange(0.18f, 0.28f), Radius * Look.FireballSize * Random.FRandRange(0.8f, 1.2f), Fire, 2.f * Vfx.Glow, Life);
	}

	// The ring racing over the ground, and a faint one in the air (the shock).
	if (Blast.bOnGround)
	{
		SpawnShockRing(Center + Up * 5.f, Up, Radius * 0.15f, Radius * Look.RingReach, Radius * 0.07f, FMath::Lerp(Fire, KillWhiteHot, 0.3f),
			(Blast.bSoul ? 18.f : 14.f) * Vfx.Glow, Look.RingLife);
	}
	SpawnShockRing(Center + Up * Radius * 0.2f, FVector::ZeroVector, Radius * 0.1f, Radius * 1.2f, Radius * 0.04f, KillWhiteHot, 6.f * Vfx.Glow,
		0.18f, true);

	// The smoke: a skirt rolling out over the ground and a column rising off the spot.
	for (int32 Index = 0; Index < N(Look.SmokeRing); ++Index)
	{
		const FVector Out = AcrossSurface(Up, Random);
		FParticle& Puff = AddParticle(EParticle::Smoke, Center + Out * Radius * 0.08f, Out * Radius * Random.FRandRange(0.9f, 1.6f) + Up * Radius * Random.FRandRange(0.1f, 0.3f),
			Random.FRandRange(1.2f, 1.9f));
		Puff.Color = SmokeColor;
		Puff.Intensity = 0.6f * Vfx.Smoke;
		Puff.StartSize = Radius * 0.12f;
		Puff.EndSize = Radius * Random.FRandRange(0.45f, 0.65f);
		Puff.Gravity = -0.02f;
		Puff.Drag = 2.4f;
	}
	for (int32 Index = 0; Index < N(Blast.bSoul ? Look.SmokeColumn / 2 : Look.SmokeColumn); ++Index)
	{
		const FVector Drift = AcrossSurface(Up, Random) * Random.FRandRange(10.f, 40.f) + Up * Radius * Random.FRandRange(0.2f, 0.45f);
		FParticle& Puff = AddParticle(EParticle::Smoke, Center + Up * Radius * Random.FRandRange(0.05f, 0.25f), Drift, Random.FRandRange(2.4f, 3.4f));
		Puff.Color = SmokeColor;
		Puff.Intensity = 0.45f * Vfx.Smoke;
		Puff.StartSize = Radius * 0.25f;
		Puff.EndSize = Radius * Random.FRandRange(0.7f, 0.95f);
		Puff.Gravity = -0.05f;
		Puff.Drag = 1.6f;
	}

	// Embers arcing out and down (a soul blast's motes rise instead).
	for (int32 Index = 0; Index < N(Look.Embers); ++Index)
	{
		const FVector Out = (Up * Random.FRandRange(0.3f, 1.2f) + Random.GetUnitVector() * 0.9f).GetSafeNormal();
		FParticle& Ember = AddParticle(EParticle::Spark, Center + Up * 10.f, Out * Radius * Random.FRandRange(1.5f, 3.f), Random.FRandRange(0.6f, 1.3f));
		Ember.Color = FMath::Lerp(Fire, KillWhiteHot, Random.FRandRange(0.f, 0.5f));
		Ember.Intensity = Random.FRandRange(18.f, 28.f) * Vfx.Glow;
		Ember.StartSize = Random.FRandRange(1.f, 2.f);
		Ember.Gravity = Blast.bSoul ? -0.2f : 0.9f;
		Ember.Drag = 1.2f;
	}

	// The ground thrown up in its own look, and its debris raining back.
	if (Blast.bOnGround && Blast.Ground != EImpactMaterial::Unknown)
	{
		const FPlane Floor(Center, Up);
		SpawnSurfaceImpact(Center, Up, -Up, Blast.Ground, 1.6f, false, &Floor);
		for (int32 Index = 0; Index < N(Look.Debris); ++Index)
		{
			const FVector Direction = (Up * Random.FRandRange(0.6f, 1.3f) + Random.GetUnitVector() * 0.8f).GetSafeNormal();
			FParticle& Clod = AddParticle(EParticle::Chip, Center + Up * 6.f, Direction * Radius * Random.FRandRange(1.2f, 2.2f), Random.FRandRange(1.f, 1.6f));
			Clod.StartSize = Random.FRandRange(3.f, 7.f);
			Clod.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Clod.Spin = Random.GetUnitVector() * Random.FRandRange(5.f, 14.f);
		}
	}
}
