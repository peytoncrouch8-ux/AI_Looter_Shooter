#include "Weapons/WeaponFX.h"
#include "Elements/ElementRules.h"
#include "Weapons/ElementVfxTuning.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"

// FWeaponFX's quieter element looks ("Elements matter" lane F, 2026-10-29): the held gun's idle pieces (UWeaponModelComponent
// spawns them, first person only; WeaponModelElementIdle.cpp) and the brief flare on a fresh element mark (UBulletSubsystem's
// LeaveMark; the decal itself is FBulletMarks'). Spawning only; the muzzle's sparks and the tracer's trail are
// WeaponFXElementShots.cpp. Colours are WeaponFXElements.cpp's, so the looks agree (and the gobs share one bit set).

namespace
{
	const FLinearColor EmberHot(1.f, 0.62f, 0.16f);
	const FLinearColor SoulWhite(1.f, 0.97f, 0.9f);
	/** Unlit smoke (as the light shows it): frost's cold breath, a rot's sick air, a smoulder's soot. */
	const FLinearColor FrostColor(0.72f, 0.82f, 0.88f);
	const FLinearColor RotCloud(0.22f, 0.34f, 0.1f);
	const FLinearColor SmoulderSmoke(0.16f, 0.14f, 0.13f);
	const FLinearColor RotGob(0.3f, 0.52f, 0.12f);
	constexpr float RotGobGlow = 0.2f;
	/** A salt glint's white-teal twinkle. */
	const FLinearColor SaltGlint(0.85f, 1.f, 0.97f);
	/** An idle smoulder's curl off a Cinder gun's vents: paler than soot, it is seen against the gun. */
	const FLinearColor IdleSmoke(0.36f, 0.34f, 0.32f);
}

void FWeaponFX::SpawnElementIdle(ELooterElement Element, const FVector& Location, const FVector& Velocity, float Scale)
{
	if (Element == ELooterElement::None)
	{
		return;
	}
	// The gun is half a metre from the eye: these are millimetres. Each piece is carried at its holder's speed (Velocity), so
	// walking forward doesn't sweep it into the eye; lightly dragged, short-lived.
	const FVfxTuning Vfx = FVfxTuning::Live();
	const float Size = FMath::Clamp(Scale, 0.3f, 3.f);
	const FVector Up = FVector::UpVector;
	const FLinearColor Color = ElementRules::FxColor(Element);
	switch (Element)
	{
	case ELooterElement::Ember:
	{
		// Mostly an ember lifting off the vents, now and then a curl of smoulder.
		if (Random.FRand() < 0.75f)
		{
			FParticle& Ember = AddParticle(EParticle::Flash, Location, Velocity + Up * Random.FRandRange(8.f, 20.f) + Random.GetUnitVector() * 4.f,
				Random.FRandRange(0.4f, 0.7f));
			Ember.Color = FMath::Lerp(Color, EmberHot, Random.FRandRange(0.2f, 0.8f));
			Ember.Intensity = Random.FRandRange(10.f, 16.f) * Vfx.Glow;
			Ember.StartSize = Random.FRandRange(0.5f, 0.8f) * Size;
			Ember.EndSize = 0.15f * Size;
			Ember.Gravity = -0.05f;
			Ember.Drag = 0.5f;
		}
		else
		{
			FParticle& Curl = AddParticle(EParticle::Smoke, Location, Velocity + Up * 8.f, 0.8f);
			Curl.Color = IdleSmoke;
			Curl.Intensity = 0.1f * Vfx.Smoke;
			Curl.StartSize = 1.f * Size;
			Curl.EndSize = 4.f * Size;
			Curl.Gravity = -0.03f;
			Curl.Drag = 0.5f;
		}
		break;
	}
	case ELooterElement::Thunder:
	{
		// A crackle: two beads jumping a hair apart, sometimes a tiny spark spat off.
		const FLinearColor Hot = FMath::Lerp(Color, FLinearColor::White, 0.5f);
		FVector At = Location;
		for (int32 Bead = 0; Bead < 2; ++Bead)
		{
			FParticle& Spark = AddParticle(EParticle::Flash, At, Velocity, Random.FRandRange(0.04f, 0.07f));
			Spark.Color = Hot;
			Spark.Intensity = 22.f * Vfx.Glow;
			Spark.StartSize = 0.45f * Size;
			Spark.EndSize = 0.15f * Size;
			Spark.Gravity = 0.f;
			At += Random.GetUnitVector() * 0.6f * Size;
		}
		if (Random.FRand() < 0.4f)
		{
			FParticle& Spat = AddParticle(EParticle::Spark, Location, Velocity + Random.GetUnitVector() * 120.f, 0.05f);
			Spat.Color = Hot;
			Spat.Intensity = 18.f * Vfx.Glow;
			Spat.StartSize = 0.12f * Size;
			Spat.Gravity = 0.f;
			Spat.Drag = 3.f;
		}
		break;
	}
	case ELooterElement::Rot:
	{
		// A drip falling off it, and now and then a faint green breath.
		FParticle& Drip = AddParticle(EParticle::Bit, Location, Velocity - Up * 5.f, 0.5f);
		Drip.BitSet = BitSetFor(EBitShape::Round, RotGob, RotGobGlow);
		Drip.StartSize = Random.FRandRange(0.25f, 0.4f) * Size;
		Drip.Drag = 0.3f;
		if (Random.FRand() < 0.3f)
		{
			FParticle& Breath = AddParticle(EParticle::Smoke, Location, Velocity + Up * 4.f, 0.7f);
			Breath.Color = RotCloud;
			Breath.Intensity = 0.1f * Vfx.Smoke;
			Breath.StartSize = 1.f * Size;
			Breath.EndSize = 3.5f * Size;
			Breath.Gravity = 0.f;
			Breath.Drag = 0.5f;
		}
		break;
	}
	case ELooterElement::GraveSalt:
	{
		// A breath of frost sinking off it, or a glint of salt.
		if (Random.FRand() < 0.6f)
		{
			FParticle& Frost = AddParticle(EParticle::Smoke, Location, Velocity - Up * 3.f, 0.7f);
			Frost.Color = FrostColor;
			Frost.Intensity = 0.1f * Vfx.Smoke;
			Frost.StartSize = 0.8f * Size;
			Frost.EndSize = 3.5f * Size;
			Frost.Gravity = 0.02f;
			Frost.Drag = 0.5f;
		}
		else
		{
			FParticle& Glint = AddParticle(EParticle::Flash, Location, Velocity, 0.12f);
			Glint.Color = FMath::Lerp(Color, SaltGlint, 0.6f);
			Glint.Intensity = 18.f * Vfx.Glow;
			Glint.StartSize = 0.5f * Size;
			Glint.EndSize = 0.1f * Size;
			Glint.Gravity = 0.f;
		}
		break;
	}
	case ELooterElement::SoulLight:
	{
		// A mote of light drifting up off it.
		FParticle& Mote = AddParticle(EParticle::Flash, Location, Velocity + Up * Random.FRandRange(6.f, 14.f) + Random.GetUnitVector() * 3.f,
			Random.FRandRange(0.6f, 0.9f));
		Mote.Color = FMath::Lerp(Color, SoulWhite, Random.FRandRange(0.f, 0.5f));
		Mote.Intensity = Random.FRandRange(10.f, 14.f) * Vfx.Glow;
		Mote.StartSize = Random.FRandRange(0.5f, 0.8f) * Size;
		Mote.EndSize = 0.15f * Size;
		Mote.Gravity = -0.04f;
		Mote.Drag = 0.6f;
		break;
	}
	default:
		break;
	}
}

void FWeaponFX::SpawnElementMark(ELooterElement Element, const FVector& Location, const FVector& Normal, float Scale)
{
	const FElementVfxTuning Tuning = FElementVfxTuning::Live();
	if (Element == ELooterElement::None || Tuning.MarkFlare <= 0.f)
	{
		return;
	}
	const FVfxTuning Vfx = FVfxTuning::Live();
	const FVector Out = Normal.IsNearlyZero() ? FVector::UpVector : Normal.GetSafeNormal();
	const FVector Point = Location + Out * 1.5f;
	FVector Flat, Flat2;
	Out.FindBestAxisVectors(Flat, Flat2);
	auto OnSurface = [this, &Flat, &Flat2](float Reach)
	{
		const float Angle = Random.FRandRange(0.f, 2.f * UE_PI);
		return (Flat * FMath::Cos(Angle) + Flat2 * FMath::Sin(Angle)) * Random.FRandRange(0.f, Reach);
	};
	const float Size = FMath::Clamp(Scale, 0.5f, 2.f);
	const float Amount = Tuning.MarkFlare * FMath::Sqrt(Size) * DetailAt(Point);
	auto Count = [Amount](int32 Full) { return VfxRules::Count(Full, Amount); };
	const FVector Up = FVector::UpVector;
	const FLinearColor Color = ElementRules::FxColor(Element);
	auto Glow = [this, &Point](const FLinearColor& GlowColor, float Intensity, float From, float To, float Life)
	{
		FParticle& Flash = AddParticle(EParticle::Flash, Point, FVector::ZeroVector, Life);
		Flash.Color = GlowColor;
		Flash.Intensity = Intensity;
		Flash.StartSize = From;
		Flash.EndSize = To;
		Flash.Gravity = 0.f;
	};

	switch (Element)
	{
	case ELooterElement::Ember:
	{
		// The scorch smoulders: its edge lit a moment (the decal can't glow; this does), embers lifting, a curl of soot.
		Glow(FMath::Lerp(Color, EmberHot, 0.3f), 6.f * Vfx.Glow, 6.f * Size, 9.f * Size, 0.7f);
		for (int32 Index = 0, Total = Count(4); Index < Total; ++Index)
		{
			FParticle& Ember = AddParticle(EParticle::Flash, Point + OnSurface(4.f * Size), Out * Random.FRandRange(20.f, 60.f) + Up * Random.FRandRange(30.f, 80.f),
				Random.FRandRange(0.5f, 0.9f));
			Ember.Color = FMath::Lerp(Color, EmberHot, Random.FRandRange(0.f, 0.7f));
			Ember.Intensity = Random.FRandRange(12.f, 18.f) * Vfx.Glow;
			Ember.StartSize = Random.FRandRange(1.5f, 2.5f);
			Ember.EndSize = 0.4f;
			Ember.Gravity = -0.15f;
			Ember.Drag = 1.8f;
		}
		FParticle& Soot = AddParticle(EParticle::Smoke, Point, Out * 20.f + Up * 25.f, 1.f);
		Soot.Color = SmoulderSmoke;
		Soot.Intensity = 0.25f * Vfx.Smoke;
		Soot.StartSize = 4.f * Size;
		Soot.EndSize = 22.f * Size;
		Soot.Gravity = -0.05f;
		Soot.Drag = 2.f;
		break;
	}
	case ELooterElement::Thunder:
	{
		// The burn ring's edge flashes, sparks skitter off across the surface, a crackle jumps over it.
		const FLinearColor Hot = FMath::Lerp(Color, FLinearColor::White, 0.5f);
		Glow(Hot, 10.f * Vfx.Glow, 5.f * Size, 12.f * Size, 0.12f);
		for (int32 Index = 0, Total = Count(4); Index < Total; ++Index)
		{
			FParticle& Spark = AddParticle(EParticle::Spark, Point, (OnSurface(1.f).GetSafeNormal() + Out * 0.3f) * Random.FRandRange(300.f, 700.f),
				Random.FRandRange(0.12f, 0.25f));
			Spark.Color = Hot;
			Spark.Intensity = 20.f * Vfx.Glow;
			Spark.StartSize = 0.8f;
			Spark.Gravity = 0.6f;
			Spark.Drag = 3.f;
		}
		FVector At = Point;
		for (int32 Bead = 0, Beads = Count(1) > 0 ? 3 : 0; Bead < Beads; ++Bead)
		{
			At += OnSurface(3.f * Size) + Out * 0.5f;
			FParticle& Crackle = AddParticle(EParticle::Flash, At, FVector::ZeroVector, Random.FRandRange(0.05f, 0.08f));
			Crackle.Color = Hot;
			Crackle.Intensity = 22.f * Vfx.Glow;
			Crackle.StartSize = 2.f;
			Crackle.EndSize = 0.6f;
			Crackle.Gravity = 0.f;
		}
		break;
	}
	case ELooterElement::Rot:
	{
		// The acid works: drips running off it, bubbles popping, a breath of its fume.
		const uint8 Gobs = BitSetFor(EBitShape::Round, RotGob, RotGobGlow);
		for (int32 Index = 0, Total = Count(2); Index < Total; ++Index)
		{
			FParticle& Drip = AddParticle(EParticle::Bit, Point + OnSurface(2.f * Size), Out * 30.f - Up * 40.f, 0.6f);
			Drip.BitSet = Gobs;
			Drip.StartSize = Random.FRandRange(0.7f, 1.1f);
			Drip.Drag = 0.6f;
		}
		for (int32 Index = 0, Total = Count(2); Index < Total; ++Index)
		{
			FParticle& Bubble = AddParticle(EParticle::Flash, Point + OnSurface(3.f * Size), Out * 15.f, Random.FRandRange(0.4f, 0.6f));
			Bubble.Color = Color;
			Bubble.Intensity = 5.f * Vfx.Glow;
			Bubble.StartSize = 1.2f;
			Bubble.EndSize = 2.5f;
			Bubble.Gravity = 0.f;
			// Staggered: they pop over a moment, not all at once.
			Bubble.Age = -Random.FRandRange(0.f, 0.4f);
		}
		FParticle& Fume = AddParticle(EParticle::Smoke, Point, Out * 15.f + Up * 15.f, 0.9f);
		Fume.Color = RotCloud;
		Fume.Intensity = 0.2f * Vfx.Smoke;
		Fume.StartSize = 4.f * Size;
		Fume.EndSize = 18.f * Size;
		Fume.Gravity = -0.03f;
		Fume.Drag = 2.f;
		break;
	}
	case ELooterElement::GraveSalt:
	{
		// The frost glints as it sets, one glint after another, and a cold breath rolls off it.
		for (int32 Index = 0, Total = Count(4); Index < Total; ++Index)
		{
			FParticle& Glint = AddParticle(EParticle::Flash, Point + OnSurface(4.f * Size), FVector::ZeroVector, Random.FRandRange(0.1f, 0.25f));
			Glint.Color = FMath::Lerp(Color, SaltGlint, 0.6f);
			Glint.Intensity = Random.FRandRange(18.f, 24.f) * Vfx.Glow;
			Glint.StartSize = 2.f;
			Glint.EndSize = 0.3f;
			Glint.Gravity = 0.f;
			Glint.Age = -Random.FRandRange(0.f, 0.3f);
		}
		FParticle& Frost = AddParticle(EParticle::Smoke, Point, Out * 15.f, 0.8f);
		Frost.Color = FrostColor;
		Frost.Intensity = 0.25f * Vfx.Smoke;
		Frost.StartSize = 4.f * Size;
		Frost.EndSize = 18.f * Size;
		Frost.Gravity = 0.02f;
		Frost.Drag = 2.f;
		break;
	}
	case ELooterElement::SoulLight:
	{
		// The gilt shines a moment and motes lift off it.
		Glow(FMath::Lerp(Color, SoulWhite, 0.3f), 8.f * Vfx.Glow, 5.f * Size, 10.f * Size, 0.35f);
		for (int32 Index = 0, Total = Count(4); Index < Total; ++Index)
		{
			FParticle& Mote = AddParticle(EParticle::Flash, Point + OnSurface(4.f * Size), Out * 20.f + Up * Random.FRandRange(40.f, 90.f),
				Random.FRandRange(0.6f, 1.f));
			Mote.Color = FMath::Lerp(Color, SoulWhite, Random.FRandRange(0.f, 0.5f));
			Mote.Intensity = Random.FRandRange(10.f, 16.f) * Vfx.Glow;
			Mote.StartSize = Random.FRandRange(2.f, 3.f);
			Mote.EndSize = 0.6f;
			Mote.Gravity = -0.15f;
			Mote.Drag = 1.5f;
		}
		break;
	}
	default:
		break;
	}
}
