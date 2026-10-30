#include "Weapons/WeaponFX.h"
#include "Elements/ElementRules.h"
#include "Weapons/DeathSplats.h"
#include "Weapons/ElementVfxTuning.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxSubsystem.h"
#include "Weapons/VfxTuning.h"
#include "GameFramework/Actor.h"

// FWeaponFX's bigger element deaths ("Elements matter" lane F, 2026-10-29; the user's pick: "bigger element deaths (ash, a shock
// pop, a melt, crystallised, a burst of light)"), laid by ElementDeathLook::Play over its aura and burst when a status kills:
//  - Cinder: a heat burst of fire rolling up (M_FX_Fire, alpha-blended: additive fire washes white in daylight), a scorched
//    ring on the ground, ash flakes thrown up and fluttering down with a few embers among them, a scorch splat.
//  - Thunder: a shock pop (a white starburst and a facing ring), a ring of crooked arcs round the body, bolts to the ground.
//  - Rot: the body melts (drips running off it), a low puddle-burst of gobs over the ground, bubbles popping on a green puddle
//    (a gel splat in the rot's colour), a cloud rolling out low.
//  - Wardbreak: it crystallises (salt plates set still over the body, a cold sheen) and a moment later shatters (the plates
//    flung out, glints, a crack of light, a ring and a cold puff).
//  - Hallow: a burst of light lifting: a gold starburst, a column of light and motes rising, a ground ring, a halo lifting off.
// Spawning only; counts thin with distance (DetailAt) and with Amount (ElementDeathLook halves it on a full pool).

namespace
{
	const FLinearColor EmberHot(1.f, 0.62f, 0.16f);
	const FLinearColor SoulWhite(1.f, 0.97f, 0.9f);
	const FLinearColor FrostColor(0.72f, 0.82f, 0.88f);
	const FLinearColor RotCloud(0.22f, 0.34f, 0.1f);
	/** WeaponFXElements.cpp's gob colour and glow (one bit set); the puddle splat a darker wet green. */
	const FLinearColor RotGob(0.3f, 0.52f, 0.12f);
	constexpr float RotGobGlow = 0.2f;
	const FLinearColor RotPuddle(0.2f, 0.32f, 0.07f);
	/** Ash flakes: lit, near black-grey; their soot. */
	const FLinearColor AshFlake(0.09f, 0.085f, 0.08f);
	const FLinearColor Soot(0.07f, 0.06f, 0.055f);
	/** Salt plates: the element's colour at the crystals' glow (WeaponFXElements.cpp's set). */
	constexpr float SaltCrystalGlow = 0.8f;
	const FLinearColor SaltGlint(0.85f, 1.f, 0.97f);
	/** Wardbreak's body holds as salt this long (s) before it shatters. */
	constexpr float ShatterAt = 0.35f;

	FVector AroundOf(FRandomStream& Random)
	{
		const float Angle = Random.FRandRange(0.f, 2.f * UE_PI);
		return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	}
}

void FWeaponFX::SpawnElementDeath(ELooterElement Element, const FVector& Center, float Radius, float HalfHeight, const FVector& Front,
	float Amount)
{
	const FElementVfxTuning Tuning = FElementVfxTuning::Live();
	const float Share = Amount * Tuning.Death * DetailAt(Center);
	if (Element == ELooterElement::None || Share <= 0.f)
	{
		return;
	}
	const FVfxTuning Vfx = FVfxTuning::Live();
	const FVector Up = FVector::UpVector;
	const float Body = FMath::Clamp(Radius, 10.f, 200.f);
	const float Tall = FMath::Clamp(HalfHeight, 10.f, 300.f);
	// A spider (40 cm round) is the measure: pieces grow with the root of the body's size, as the auras' do.
	const float Scale = FMath::Clamp(Body / 40.f, 0.5f, 3.f);
	const float Reach = FMath::Sqrt(Scale);
	auto Count = [Share, Reach](int32 Full) { return VfxRules::Count(Full, Share * Reach); };
	const FVector Heart = Center + Front + Up * Tall * 0.2f;
	const FVector Foot = Center + Front * 0.5f - Up * Tall * 0.9f;
	const FLinearColor Color = ElementRules::FxColor(Element);
	auto Flash = [this](const FVector& Where, const FLinearColor& FlashColor, float Intensity, float From, float To, float Life, float Delay)
	{
		FParticle& Glow = AddParticle(EParticle::Flash, Where, FVector::ZeroVector, Life);
		Glow.Color = FlashColor;
		Glow.Intensity = Intensity;
		Glow.StartSize = From;
		Glow.EndSize = To;
		Glow.Gravity = 0.f;
		Glow.Age = -Delay;
	};
	// A shock ring that waits Delay s (SpawnShockRing's, with a delay: a particle younger than 0 is neither moved nor drawn).
	auto Ring = [this](const FVector& Where, const FVector& Axis, float From, float To, float Width, const FLinearColor& RingColor, float Intensity,
		float Life, bool bFacing, float Delay, const FVector& Velocity)
	{
		if (Intensity <= 0.f)
		{
			return;
		}
		FParticle& Shock = AddParticle(EParticle::Ring, Where, Velocity, Life);
		Shock.Color = RingColor;
		Shock.Intensity = Intensity;
		Shock.StartSize = From;
		Shock.EndSize = To;
		Shock.Width = FMath::Max(Width, 0.5f);
		Shock.Gravity = 0.f;
		Shock.bFacing = bFacing;
		Shock.Extent = Axis.IsNearlyZero() ? FVector::UpVector : Axis.GetSafeNormal();
		Shock.Age = -Delay;
	};
	// The splats are the level's (UVfxSubsystem's pool of decals), when the effects have a world with one.
	UVfxSubsystem* Splats = Owner.IsValid() ? UVfxSubsystem::Get(Owner.Get()) : nullptr;
	const FPlane Ground(Foot, Up);

	switch (Element)
	{
	case ELooterElement::Ember:
	{
		// The heat goes up in a burst: fire rolling up off the body into soot.
		for (int32 Index = 0, Total = Count(3); Index < Total; ++Index)
		{
			FVfxFlame Flame;
			Flame.Location = Center + Front * 0.5f + Up * Tall * 0.1f + AroundOf(Random) * Body * Random.FRandRange(0.f, 0.4f);
			Flame.Velocity = Up * Random.FRandRange(120.f, 220.f) * Reach + AroundOf(Random) * 50.f;
			Flame.StartSize = Body * 0.8f;
			Flame.EndSize = Body * Random.FRandRange(2.f, 2.6f);
			Flame.Fire = FMath::Lerp(Color, EmberHot, 0.3f);
			Flame.Smoke = Soot;
			Flame.Opacity = 0.9f;
			Flame.Life = Random.FRandRange(1.1f, 1.5f);
			Flame.Burn = 0.32f;
			Flame.Delay = Index * 0.05f;
			Flame.Gravity = -0.25f;
			Flame.Drag = 2.5f;
			SpawnFlame(Flame);
		}
		Ring(Foot + Up * 2.f, Up, Body * 0.4f, Body * 2.6f, 10.f * Reach, FMath::Lerp(Color, EmberHot, 0.3f), 2.2f * Vfx.Glow, 0.4f, false, 0.f, FVector::ZeroVector);
		// Ash thrown up off it, fluttering down (lit flakes, tumbling), a few still glowing at their edges.
		const uint8 Ash = BitSetFor(EBitShape::Plate, AshFlake, 0.f);
		for (int32 Index = 0, Total = Count(26); Index < Total; ++Index)
		{
			const FVector From = Center + AroundOf(Random) * Body * Random.FRandRange(0.f, 1.f) + Up * Tall * Random.FRandRange(0.3f, 1.3f);
			FParticle& Flake = AddParticle(EParticle::Bit, From, Up * Random.FRandRange(60.f, 180.f) * Reach + AroundOf(Random) * Random.FRandRange(20.f, 80.f),
				Random.FRandRange(1.8f, 2.8f));
			Flake.BitSet = Ash;
			Flake.StartSize = Random.FRandRange(0.8f, 1.8f) * Reach;
			Flake.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Flake.Spin = Random.GetUnitVector() * Random.FRandRange(4.f, 10.f);
			// Light and broad: it floats down slowly.
			Flake.Gravity = 0.12f;
			Flake.Drag = 2.2f;
			Flake.bFloor = true;
			Flake.Floor = Ground;
		}
		for (int32 Index = 0, Total = Count(9); Index < Total; ++Index)
		{
			const FVector From = Center + AroundOf(Random) * Body * Random.FRandRange(0.f, 1.f) + Up * Tall * Random.FRandRange(0.3f, 1.2f);
			FParticle& Ember = AddParticle(EParticle::Flash, From, Up * Random.FRandRange(40.f, 120.f) + AroundOf(Random) * 30.f, Random.FRandRange(1.f, 1.6f));
			Ember.Color = FMath::Lerp(Color, EmberHot, Random.FRandRange(0.f, 0.6f));
			Ember.Intensity = Random.FRandRange(10.f, 14.f) * Vfx.Glow;
			Ember.StartSize = Random.FRandRange(1.2f, 2.f) * Reach;
			Ember.EndSize = 0.3f;
			Ember.Gravity = 0.05f;
			Ember.Drag = 2.f;
		}
		if (Splats)
		{
			Splats->AddSplat(Foot, Up, EVfxSplat::Scorch, FLinearColor::Black, Body * 2.2f);
		}
		break;
	}
	case ELooterElement::Thunder:
	{
		// The shock pop: a white flash and starburst from its heart, a ring thrown out facing the eye...
		const FLinearColor Hot = FMath::Lerp(Color, FLinearColor::White, 0.6f);
		Flash(Heart, Hot, 30.f * Vfx.Glow, Body * 0.5f, Body * 2.5f, 0.07f, 0.f);
		SpawnStarburst(Heart, Body * 3.2f * Reach, Hot, 3.f * Vfx.Glow, 0.12f, 10);
		Ring(Heart, FVector::ZeroVector, Body * 0.3f, Body * 3.f, 8.f * Reach, Color, 3.f * Vfx.Glow * Share, 0.25f, true, 0.f, FVector::ZeroVector);
		Ring(Foot + Up * 2.f, Up, Body * 0.5f, Body * 3.5f, 6.f * Reach, Color, 2.5f * Vfx.Glow * Share, 0.35f, false, 0.f, FVector::ZeroVector);
		// ...a ring of crooked arcs round the body at its middle, each jumping to the next...
		const int32 Points = FMath::Clamp(Count(8), 5, 8);
		const float Turn = Random.FRandRange(0.f, 2.f * UE_PI);
		auto RingPoint = [&](int32 Index)
		{
			const float Angle = Turn + Index * (2.f * UE_PI / Points);
			return Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * Body * 1.4f * Random.FRandRange(0.85f, 1.15f)
				+ Up * Tall * Random.FRandRange(-0.15f, 0.15f);
		};
		const FVector First = RingPoint(0);
		FVector Last = First;
		for (int32 Index = 1; Index <= Points; ++Index)
		{
			const FVector Next = Index == Points ? First : RingPoint(Index);
			SpawnElementArc(Element, Last, Next);
			Last = Next;
		}
		// ...and bolts from its heart to the ground round it.
		for (int32 Index = 0, Total = Count(3); Index < Total; ++Index)
		{
			SpawnElementArc(Element, Heart, Foot + AroundOf(Random) * Body * Random.FRandRange(1.5f, 2.5f));
		}
		break;
	}
	case ELooterElement::Rot:
	{
		// It melts: drips running off the whole body to the ground...
		const uint8 Gobs = BitSetFor(EBitShape::Round, RotGob, RotGobGlow);
		for (int32 Index = 0, Total = Count(16); Index < Total; ++Index)
		{
			const FVector From = Center + AroundOf(Random) * Body * Random.FRandRange(0.3f, 1.f) + Up * Tall * Random.FRandRange(-0.2f, 0.8f);
			FParticle& Drip = AddParticle(EParticle::Bit, From, -Up * Random.FRandRange(20.f, 80.f) + AroundOf(Random) * Random.FRandRange(0.f, 30.f),
				Random.FRandRange(0.6f, 1.f));
			Drip.BitSet = Gobs;
			Drip.StartSize = Random.FRandRange(1.4f, 2.6f) * Reach;
			Drip.Gravity = 0.8f;
			Drip.Drag = 0.5f;
			Drip.bFloor = true;
			Drip.Floor = Ground;
			// Not all at once: it runs off over a moment.
			Drip.Age = -Random.FRandRange(0.f, 0.35f);
		}
		// ...then the puddle bursts: gobs thrown out low over the ground (they splat where they land)...
		for (int32 Index = 0, Total = Count(14); Index < Total; ++Index)
		{
			FParticle& Gob = AddParticle(EParticle::Bit, Foot + Up * 5.f, AroundOf(Random) * Random.FRandRange(220.f, 420.f) * Reach + Up * Random.FRandRange(60.f, 160.f),
				Random.FRandRange(0.7f, 1.f));
			Gob.BitSet = Gobs;
			Gob.StartSize = Random.FRandRange(1.6f, 3.f) * Reach;
			Gob.Drag = 0.4f;
			Gob.bFloor = true;
			Gob.Floor = Ground;
			Gob.Age = -0.2f;
		}
		Ring(Foot + Up * 2.f, Up, Body * 0.3f, Body * 2.4f, 12.f * Reach, Color, 1.2f * Vfx.Glow * Share, 0.5f, false, 0.2f, FVector::ZeroVector);
		// ...bubbles popping on the puddle a while, a cloud rolling out low.
		for (int32 Index = 0, Total = Count(8); Index < Total; ++Index)
		{
			FParticle& Bubble = AddParticle(EParticle::Flash, Foot + AroundOf(Random) * Body * Random.FRandRange(0.f, 1.2f) + Up * 3.f,
				Up * Random.FRandRange(10.f, 30.f), Random.FRandRange(0.4f, 0.8f));
			Bubble.Color = Color;
			Bubble.Intensity = Random.FRandRange(5.f, 8.f) * Vfx.Glow;
			Bubble.StartSize = Random.FRandRange(2.5f, 4.f) * Reach;
			Bubble.EndSize = 6.f * Reach;
			Bubble.Gravity = -0.05f;
			Bubble.Age = -Random.FRandRange(0.2f, 1.2f);
		}
		for (int32 Index = 0, Total = Count(4); Index < Total; ++Index)
		{
			FParticle& Cloud = AddParticle(EParticle::Smoke, Foot + Up * 10.f, AroundOf(Random) * Random.FRandRange(60.f, 140.f) * Reach,
				Random.FRandRange(1.6f, 2.2f));
			Cloud.Color = RotCloud;
			Cloud.Intensity = 0.4f * Vfx.Smoke;
			Cloud.StartSize = 20.f * Scale;
			Cloud.EndSize = Random.FRandRange(110.f, 150.f) * Scale;
			Cloud.Gravity = -0.01f;
			Cloud.Drag = 1.6f;
		}
		if (Splats)
		{
			Splats->AddSplat(Foot, Up, EVfxSplat::Gel, RotPuddle, Body * 3.f);
		}
		break;
	}
	case ELooterElement::GraveSalt:
	{
		// It crystallises: salt plates set still all over the body under a cold sheen, a breath of frost creeping off it...
		const uint8 Crystals = BitSetFor(EBitShape::Plate, Color, SaltCrystalGlow);
		for (int32 Index = 0, Total = Count(30); Index < Total; ++Index)
		{
			const FVector On = Center + Front * 0.3f + AroundOf(Random) * Body * Random.FRandRange(0.75f, 1.f) + Up * Tall * Random.FRandRange(-0.85f, 0.85f);
			FParticle& Plate = AddParticle(EParticle::Bit, On, FVector::ZeroVector, ShatterAt + 0.05f);
			Plate.BitSet = Crystals;
			Plate.StartSize = Random.FRandRange(2.5f, 4.5f) * Reach;
			Plate.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Plate.Gravity = 0.f;
		}
		Flash(Center + Front, FMath::Lerp(Color, FLinearColor::White, 0.6f), 8.f * Vfx.Glow, Body * 1.5f, Body * 2.6f, ShatterAt + 0.05f, 0.f);
		FParticle& Breath = AddParticle(EParticle::Smoke, Center, Up * 10.f, 0.6f);
		Breath.Color = FrostColor;
		Breath.Intensity = 0.3f * Vfx.Smoke;
		Breath.StartSize = Body * 0.8f;
		Breath.EndSize = Body * 2.f;
		Breath.Gravity = 0.02f;
		Breath.Drag = 2.f;
		// ...then it shatters: a crack of light, the salt flung out every way and bouncing on the ground, glints, a ring, a cold puff.
		Flash(Heart, FMath::Lerp(Color, FLinearColor::White, 0.7f), 26.f * Vfx.Glow, Body * 0.4f, Body * 2.4f, 0.08f, ShatterAt);
		for (int32 Index = 0, Total = Count(28); Index < Total; ++Index)
		{
			const FVector Way = (Random.GetUnitVector() + Up * 0.5f).GetSafeNormal();
			FParticle& Shard = AddParticle(EParticle::Bit, Center + Way * Body * 0.6f, Way * Random.FRandRange(300.f, 700.f) * Reach, Random.FRandRange(0.9f, 1.4f));
			Shard.BitSet = Crystals;
			Shard.StartSize = Random.FRandRange(1.5f, 3.5f) * Reach;
			Shard.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Shard.Spin = Random.GetUnitVector() * Random.FRandRange(8.f, 22.f);
			Shard.Drag = 0.3f;
			Shard.bFloor = true;
			Shard.Floor = Ground;
			Shard.Age = -ShatterAt;
		}
		for (int32 Index = 0, Total = Count(10); Index < Total; ++Index)
		{
			FParticle& Glint = AddParticle(EParticle::Flash, Center, Random.GetUnitVector() * Random.FRandRange(200.f, 500.f) * Reach, Random.FRandRange(0.2f, 0.35f));
			Glint.Color = FMath::Lerp(Color, SaltGlint, 0.6f);
			Glint.Intensity = 22.f * Vfx.Glow;
			Glint.StartSize = 3.f * Reach;
			Glint.EndSize = 0.5f;
			Glint.Gravity = 0.3f;
			Glint.Drag = 2.f;
			Glint.Age = -ShatterAt;
		}
		Ring(Heart, FVector::ZeroVector, Body * 0.4f, Body * 2.8f, 7.f * Reach, FMath::Lerp(Color, FLinearColor::White, 0.4f), 2.4f * Vfx.Glow * Share, 0.3f, true,
			ShatterAt, FVector::ZeroVector);
		for (int32 Index = 0, Total = Count(5); Index < Total; ++Index)
		{
			FParticle& Frost = AddParticle(EParticle::Smoke, Center, AroundOf(Random) * Random.FRandRange(120.f, 240.f) * Reach + Up * 20.f,
				Random.FRandRange(1.f, 1.4f));
			Frost.Color = FrostColor;
			Frost.Intensity = 0.35f * Vfx.Smoke;
			Frost.StartSize = 20.f * Scale;
			Frost.EndSize = Body * 3.f;
			Frost.Gravity = 0.03f;
			Frost.Drag = 2.5f;
			Frost.Age = -ShatterAt;
		}
		break;
	}
	case ELooterElement::SoulLight:
	{
		// A burst of light lifting: a gold starburst, a ring over the ground and a halo rising off the body...
		const FLinearColor Light = FMath::Lerp(Color, SoulWhite, 0.4f);
		SpawnStarburst(Heart, Body * 3.f * Reach, Light, 2.5f * Vfx.Glow, 0.2f, 12);
		Ring(Foot + Up * 2.f, Up, Body * 0.4f, Body * 3.f, 10.f * Reach, Color, 2.8f * Vfx.Glow * Share, 0.45f, false, 0.f, FVector::ZeroVector);
		Ring(Center + Up * Tall * 0.6f, Up, Body * 0.8f, Body * 1.6f, 5.f * Reach, Light, 2.4f * Vfx.Glow * Share, 0.8f, false, 0.05f, Up * 180.f * Reach);
		// ...a column of light going up out of it...
		for (int32 Index = 0, Total = Count(10); Index < Total; ++Index)
		{
			FParticle& Column = AddParticle(EParticle::Flash, Center + AroundOf(Random) * Body * Random.FRandRange(0.f, 0.5f) + Up * Tall * Random.FRandRange(-0.5f, 0.5f),
				Up * Random.FRandRange(280.f, 520.f) * Reach, Random.FRandRange(0.6f, 0.9f));
			Column.Color = FMath::Lerp(Color, SoulWhite, Random.FRandRange(0.3f, 0.7f));
			Column.Intensity = Random.FRandRange(12.f, 18.f) * Vfx.Glow;
			Column.StartSize = Body * 0.5f;
			Column.EndSize = Body * 0.15f;
			Column.Gravity = -0.3f;
			Column.Drag = 1.2f;
		}
		// ...and motes lifting off the whole body after it, a few at a time.
		for (int32 Index = 0, Total = Count(24); Index < Total; ++Index)
		{
			const FVector From = Center + AroundOf(Random) * Body * Random.FRandRange(0.5f, 1.f) + Up * Tall * Random.FRandRange(-0.8f, 0.8f);
			FParticle& Mote = AddParticle(EParticle::Flash, From, Up * Random.FRandRange(120.f, 300.f) * Reach + AroundOf(Random) * Random.FRandRange(10.f, 40.f),
				Random.FRandRange(1.2f, 2.f));
			Mote.Color = FMath::Lerp(Color, SoulWhite, Random.FRandRange(0.f, 0.6f));
			Mote.Intensity = Random.FRandRange(12.f, 20.f) * Vfx.Glow;
			Mote.StartSize = Random.FRandRange(3.f, 6.f) * Reach;
			Mote.EndSize = 1.f;
			Mote.Gravity = -0.35f;
			Mote.Drag = 1.f;
			Mote.Age = -Random.FRandRange(0.f, 0.4f);
		}
		break;
	}
	default:
		break;
	}
}
