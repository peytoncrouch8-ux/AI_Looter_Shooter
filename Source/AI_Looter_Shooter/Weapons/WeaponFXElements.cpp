#include "Weapons/WeaponFX.h"
#include "Elements/ElementRules.h"

// FWeaponFX's looks for the elements on a body: a round's element hit and a status's pieces while it lasts. Spawning only;
// WeaponFXDraw.cpp moves and draws every particle. The bursts, the leaps, the part effects and the loot's drop moments are
// WeaponFXElementBursts.cpp. Phase 12.3 (LOOT-CODE); Thunder and Rot 2026-10-29.

namespace
{
	/** A burn's hotter heart: its embers are mixed between the element's red-orange and this. */
	const FLinearColor EmberHot(1.f, 0.62f, 0.16f);
	/** A smite's white heart. */
	const FLinearColor SoulWhite(1.f, 0.97f, 0.9f);
	/** Frost's breath: unlit smoke, a cold pale grey-blue. */
	const FLinearColor FrostColor(0.72f, 0.82f, 0.88f);
	/** A rot's wet gobs (lit drops; the same colour in WeaponFXElementBursts.cpp, so they share one bit set). */
	const FLinearColor RotGob(0.3f, 0.52f, 0.12f);
	/** A rot's sick air: unlit smoke, a dim green (bright green smoke would glow in shade). */
	const FLinearColor RotCloud(0.22f, 0.34f, 0.1f);

	FVector Across(FRandomStream& Random)
	{
		const float Angle = Random.FRandRange(0.f, 2.f * UE_PI);
		return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	}
}

void FWeaponFX::SpawnElementHit(ELooterElement Element, const FVector& Location, const FVector& Normal, const FVector& ShotDirection,
	float Scale, bool bSet, bool bWeak)
{
	if (Element == ELooterElement::None)
	{
		return;
	}
	const FVector Out = Normal.IsNearlyZero() ? -ShotDirection.GetSafeNormal() : Normal.GetSafeNormal();
	const FVector Point = Location + Out * 3.f;
	const FVector Up = FVector::UpVector;
	const FLinearColor Color = ElementRules::FxColor(Element);
	// A hit that set its status reads fuller; one on a creature weak to it fuller and brighter again.
	const float Amount = FMath::Clamp(Scale, 0.3f, 2.f) * (bSet ? 1.f : 0.45f) * (bWeak ? 1.4f : 1.f);
	const float Bright = bWeak ? 1.35f : 1.f;
	auto Count = [Amount](float Full) { return FMath::Max(1, FMath::RoundToInt32(Full * Amount)); };

	FParticle& Flash = AddParticle(EParticle::Flash, Point, FVector::ZeroVector, bSet ? 0.1f : 0.07f);
	// Soul-light and Thunder flash with a white heart; a rot doesn't flare, it festers (a dimmer flash).
	Flash.Color = Element == ELooterElement::SoulLight ? FMath::Lerp(Color, SoulWhite, 0.4f)
		: Element == ELooterElement::Thunder ? FMath::Lerp(Color, FLinearColor::White, 0.55f) : Color;
	Flash.Intensity = 14.f * Bright * (Element == ELooterElement::Rot ? 0.6f : 1.f);
	Flash.StartSize = 6.f * Scale;
	Flash.EndSize = (bSet ? 34.f : 22.f) * Scale;
	Flash.Gravity = 0.f;

	switch (Element)
	{
	case ELooterElement::Ember:
	{
		// A spit of embers off the hit, rising as they cool...
		for (int32 Index = 0, Total = Count(10.f); Index < Total; ++Index)
		{
			const FVector Velocity = Out * Random.FRandRange(60.f, 180.f) + Random.GetUnitVector() * 80.f + Up * Random.FRandRange(60.f, 160.f);
			FParticle& Ember = AddParticle(EParticle::Flash, Point, Velocity, Random.FRandRange(0.5f, 0.9f));
			Ember.Color = FMath::Lerp(Color, EmberHot, Random.FRandRange(0.f, 0.7f));
			Ember.Intensity = Random.FRandRange(12.f, 20.f) * Bright;
			Ember.StartSize = Random.FRandRange(2.5f, 4.5f);
			Ember.EndSize = 0.8f;
			Ember.Gravity = -0.25f;
			Ember.Drag = 1.8f;
		}
		// ...and a lick of flame climbing off it when it catches.
		for (int32 Index = 0, Total = bSet ? Count(3.f) : 0; Index < Total; ++Index)
		{
			FParticle& Lick = AddParticle(EParticle::Flash, Point + Random.GetUnitVector() * 4.f, Up * Random.FRandRange(150.f, 260.f) + Out * 30.f,
				Random.FRandRange(0.22f, 0.32f));
			Lick.Color = FMath::Lerp(Color, EmberHot, 0.35f);
			Lick.Intensity = 16.f * Bright;
			Lick.StartSize = Random.FRandRange(8.f, 14.f) * Scale;
			Lick.EndSize = 3.f;
			Lick.Gravity = -0.5f;
			Lick.Drag = 2.5f;
		}
		break;
	}
	case ELooterElement::SoulLight:
	{
		// Short rays thrown out of the hit, and motes of light lifting off.
		for (int32 Index = 0, Total = Count(6.f); Index < Total; ++Index)
		{
			const FVector Direction = (Out * 0.7f + Random.GetUnitVector()).GetSafeNormal();
			FParticle& Ray = AddParticle(EParticle::Spark, Point, Direction * Random.FRandRange(500.f, 900.f), Random.FRandRange(0.12f, 0.2f));
			Ray.Color = FMath::Lerp(Color, SoulWhite, 0.3f);
			Ray.Intensity = 20.f * Bright;
			Ray.StartSize = 1.6f;
			Ray.Gravity = 0.f;
			Ray.Drag = 4.f;
		}
		for (int32 Index = 0, Total = Count(5.f); Index < Total; ++Index)
		{
			FParticle& Mote = AddParticle(EParticle::Flash, Point + Random.GetUnitVector() * 6.f, Up * Random.FRandRange(40.f, 110.f) + Random.GetUnitVector() * 30.f,
				Random.FRandRange(0.6f, 1.f));
			Mote.Color = Color;
			Mote.Intensity = Random.FRandRange(12.f, 18.f) * Bright;
			Mote.StartSize = Random.FRandRange(3.f, 5.f);
			Mote.EndSize = 1.f;
			Mote.Gravity = -0.15f;
			Mote.Drag = 2.f;
		}
		break;
	}
	case ELooterElement::GraveSalt:
	{
		// Pale crystals knocked off the hit, falling and glinting, and a breath of frost.
		const uint8 Crystals = BitSetFor(EBitShape::Plate, Color, 0.8f);
		for (int32 Index = 0, Total = Count(10.f); Index < Total; ++Index)
		{
			const FVector Velocity = Out * Random.FRandRange(200.f, 450.f) + Random.GetUnitVector() * 150.f + Up * 80.f;
			FParticle& Crystal = AddParticle(EParticle::Bit, Point, Velocity, Random.FRandRange(0.6f, 1.f));
			Crystal.BitSet = Crystals;
			Crystal.StartSize = Random.FRandRange(1.2f, 2.6f);
			Crystal.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Crystal.Spin = Random.GetUnitVector() * Random.FRandRange(8.f, 20.f);
			Crystal.Drag = 0.3f;
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			FParticle& Glint = AddParticle(EParticle::Flash, Point + Random.GetUnitVector() * 10.f * Scale, FVector::ZeroVector, 0.1f);
			Glint.Color = FMath::Lerp(Color, FLinearColor::White, 0.5f);
			Glint.Intensity = 18.f * Bright;
			Glint.StartSize = 3.f;
			Glint.EndSize = 1.f;
			Glint.Gravity = 0.f;
		}
		FParticle& Frost = AddParticle(EParticle::Smoke, Point, Out * 40.f + Up * 20.f, Random.FRandRange(0.5f, 0.7f));
		Frost.Color = FrostColor;
		Frost.Intensity = bSet ? 0.4f : 0.25f;
		Frost.StartSize = 10.f * Scale;
		Frost.EndSize = (bSet ? 55.f : 35.f) * Scale;
		Frost.Gravity = -0.02f;
		Frost.Drag = 3.f;
		break;
	}
	case ELooterElement::Thunder:
	{
		// The jolt strikes: blue-white sparks spat off the hit, and short crooked forks of light jumping out of it (lightning
		// never runs straight), gone in a blink.
		for (int32 Index = 0, Total = Count(9.f); Index < Total; ++Index)
		{
			const FVector Direction = (Out * 0.8f + Random.GetUnitVector()).GetSafeNormal();
			FParticle& Spark = AddParticle(EParticle::Spark, Point, Direction * Random.FRandRange(600.f, 1100.f), Random.FRandRange(0.1f, 0.2f));
			Spark.Color = FMath::Lerp(Color, FLinearColor::White, Random.FRandRange(0.2f, 0.7f));
			Spark.Intensity = 22.f * Bright;
			Spark.StartSize = 1.4f;
			Spark.Gravity = 0.3f;
			Spark.Drag = 4.f;
		}
		const FLinearColor Hot = FMath::Lerp(Color, FLinearColor::White, 0.6f);
		for (int32 Fork = 0, Forks = Count(3.f); Fork < Forks; ++Fork)
		{
			// Each fork steps out from the hit, turning hard at every step.
			FVector At = Point;
			const FVector Way = (Out + Random.GetUnitVector() * 0.9f).GetSafeNormal();
			for (int32 Step = 0; Step < 4; ++Step)
			{
				At += (Way + Random.GetUnitVector() * 0.8f).GetSafeNormal() * Random.FRandRange(5.f, 9.f) * Scale;
				FParticle& Bead = AddParticle(EParticle::Flash, At, FVector::ZeroVector, Random.FRandRange(0.06f, 0.1f));
				Bead.Color = Hot;
				Bead.Intensity = 22.f * Bright;
				Bead.StartSize = 3.5f;
				Bead.EndSize = 1.f;
				Bead.Gravity = 0.f;
			}
		}
		break;
	}
	case ELooterElement::Rot:
	{
		// Wet green gobs thrown off the hit and falling, and a sick puff hanging where it struck.
		const uint8 Gobs = BitSetFor(EBitShape::Round, RotGob, 0.2f);
		for (int32 Index = 0, Total = Count(8.f); Index < Total; ++Index)
		{
			const FVector Velocity = Out * Random.FRandRange(120.f, 320.f) + Random.GetUnitVector() * 90.f + Up * Random.FRandRange(40.f, 120.f);
			FParticle& Gob = AddParticle(EParticle::Bit, Point, Velocity, Random.FRandRange(0.6f, 1.f));
			Gob.BitSet = Gobs;
			Gob.StartSize = Random.FRandRange(1.4f, 2.8f);
			Gob.Drag = 0.6f;
		}
		FParticle& Puff = AddParticle(EParticle::Smoke, Point, Out * 30.f + Up * 15.f, Random.FRandRange(0.8f, 1.1f));
		Puff.Color = RotCloud;
		Puff.Intensity = bSet ? 0.45f : 0.3f;
		Puff.StartSize = 10.f * Scale;
		Puff.EndSize = (bSet ? 50.f : 32.f) * Scale;
		Puff.Gravity = -0.02f;
		Puff.Drag = 3.f;
		break;
	}
	default:
		break;
	}
}

void FWeaponFX::SpawnElementAura(ELooterElement Element, const FVector& Center, float Radius, float HalfHeight, int32 Count, float Strength)
{
	const FLinearColor Color = ElementRules::FxColor(Element);
	const FVector Up = FVector::UpVector;
	const float BodyScale = FMath::Clamp(Radius / 40.f, 0.5f, 3.f);
	const float Bright = FMath::Clamp(Strength, 0.4f, 2.f);
	// Somewhere on the body: inside its footprint, from its lower third to near its top.
	auto OnBody = [this, &Center, Radius, HalfHeight]()
	{
		const FVector Flat = Across(Random) * Random.FRandRange(0.2f, 0.85f) * Radius;
		return Center + Flat + FVector(0.f, 0.f, Random.FRandRange(-0.55f, 0.8f) * HalfHeight);
	};
	for (int32 Index = 0; Index < Count; ++Index)
	{
		switch (Element)
		{
		case ELooterElement::Ember:
		{
			if (Random.FRand() < 0.55f)
			{
				FParticle& Ember = AddParticle(EParticle::Flash, OnBody(), Up * Random.FRandRange(80.f, 160.f) + Random.GetUnitVector() * 30.f,
					Random.FRandRange(0.6f, 1.1f));
				Ember.Color = FMath::Lerp(Color, EmberHot, Random.FRandRange(0.f, 0.8f));
				Ember.Intensity = Random.FRandRange(12.f, 20.f) * Bright;
				Ember.StartSize = Random.FRandRange(2.f, 4.f) * FMath::Sqrt(BodyScale);
				Ember.EndSize = 0.6f;
				Ember.Gravity = -0.3f;
				Ember.Drag = 1.5f;
			}
			else
			{
				// A flame lick: a soft tongue climbing quickly off the body and shrinking away.
				FParticle& Lick = AddParticle(EParticle::Flash, OnBody(), Up * Random.FRandRange(170.f, 240.f), Random.FRandRange(0.22f, 0.3f));
				Lick.Color = FMath::Lerp(Color, EmberHot, Random.FRandRange(0.2f, 0.5f));
				Lick.Intensity = 14.f * Bright;
				Lick.StartSize = Random.FRandRange(10.f, 18.f) * BodyScale;
				Lick.EndSize = 2.f;
				Lick.Gravity = -0.4f;
				Lick.Drag = 2.f;
			}
			break;
		}
		case ELooterElement::SoulLight:
		{
			// Motes drawn in to its heart from round it, the light gathering for the burst.
			const FVector Heart = Center + FVector(0.f, 0.f, HalfHeight * 0.35f);
			const FVector From = Heart + Random.GetUnitVector() * Radius * Random.FRandRange(1.3f, 1.9f);
			const float Life = Random.FRandRange(0.35f, 0.5f);
			FParticle& Mote = AddParticle(EParticle::Flash, From, (Heart - From) / Life, Life);
			Mote.Color = FMath::Lerp(Color, SoulWhite, Random.FRandRange(0.f, 0.4f));
			Mote.Intensity = Random.FRandRange(12.f, 18.f) * Bright;
			Mote.StartSize = Random.FRandRange(3.f, 5.f);
			Mote.EndSize = 1.5f;
			Mote.Gravity = 0.f;
			Mote.Drag = 0.f;
			break;
		}
		case ELooterElement::GraveSalt:
		{
			if (Random.FRand() < 0.75f)
			{
				// A crystal of salt dropping off it, glinting as it turns.
				const uint8 Crystals = BitSetFor(EBitShape::Plate, Color, 0.8f);
				FParticle& Crystal = AddParticle(EParticle::Bit, OnBody(), Across(Random) * Random.FRandRange(20.f, 60.f) - Up * 20.f,
					Random.FRandRange(0.8f, 1.2f));
				Crystal.BitSet = Crystals;
				Crystal.StartSize = Random.FRandRange(1.6f, 3.f);
				Crystal.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
				Crystal.Spin = Random.GetUnitVector() * Random.FRandRange(4.f, 12.f);
				Crystal.Drag = 0.5f;
			}
			else
			{
				FParticle& Frost = AddParticle(EParticle::Smoke, OnBody(), Across(Random) * 25.f, Random.FRandRange(0.7f, 1.1f));
				Frost.Color = FrostColor;
				Frost.Intensity = 0.35f * Bright;
				Frost.StartSize = 8.f * BodyScale;
				Frost.EndSize = 30.f * BodyScale;
				Frost.Gravity = 0.05f;
				Frost.Drag = 2.5f;
			}
			break;
		}
		case ELooterElement::Thunder:
		{
			if (Random.FRand() < 0.65f)
			{
				// An arc crawling over its skin: three crooked beads, there and gone (their short lives are the flicker).
				FVector At = OnBody();
				const FVector Way = (Across(Random) + Up * Random.FRandRange(-0.6f, 0.6f)).GetSafeNormal();
				const float Step = FMath::Clamp(Radius * 0.22f, 2.f, 14.f);
				const FLinearColor Hot = FMath::Lerp(Color, FLinearColor::White, Random.FRandRange(0.3f, 0.7f));
				for (int32 Piece = 0; Piece < 3; ++Piece)
				{
					At += (Way + Random.GetUnitVector() * 0.9f).GetSafeNormal() * Step;
					FParticle& Bead = AddParticle(EParticle::Flash, At, FVector::ZeroVector, Random.FRandRange(0.05f, 0.09f));
					Bead.Color = Hot;
					Bead.Intensity = 20.f * Bright;
					Bead.StartSize = 3.f * FMath::Sqrt(BodyScale);
					Bead.EndSize = 1.f;
					Bead.Gravity = 0.f;
				}
			}
			else
			{
				// A spark spat off it, outward from the body.
				const FVector Start = OnBody();
				const FVector Direction = ((Start - Center).GetSafeNormal2D() + Random.GetUnitVector() * 0.7f).GetSafeNormal();
				FParticle& Spark = AddParticle(EParticle::Spark, Start, Direction * Random.FRandRange(300.f, 650.f), Random.FRandRange(0.1f, 0.18f));
				Spark.Color = FMath::Lerp(Color, FLinearColor::White, 0.5f);
				Spark.Intensity = 22.f * Bright;
				Spark.StartSize = 1.3f;
				Spark.Gravity = 0.5f;
				Spark.Drag = 3.f;
			}
			break;
		}
		case ELooterElement::Rot:
		{
			const float Roll = Random.FRand();
			if (Roll < 0.55f)
			{
				// A drip of rot falling off it.
				const uint8 Gobs = BitSetFor(EBitShape::Round, RotGob, 0.2f);
				FParticle& Drip = AddParticle(EParticle::Bit, OnBody(), Across(Random) * Random.FRandRange(5.f, 20.f) - Up * Random.FRandRange(10.f, 40.f),
					Random.FRandRange(0.7f, 1.1f));
				Drip.BitSet = Gobs;
				Drip.StartSize = Random.FRandRange(1.f, 2.2f) * FMath::Sqrt(BodyScale);
				Drip.Drag = 0.8f;
			}
			else if (Roll < 0.85f)
			{
				// A slow bubble swelling on its skin and bursting (it fades as it grows).
				FParticle& Bubble = AddParticle(EParticle::Flash, OnBody(), Up * Random.FRandRange(6.f, 20.f), Random.FRandRange(0.6f, 1.f));
				Bubble.Color = Color;
				Bubble.Intensity = Random.FRandRange(5.f, 9.f) * Bright;
				Bubble.StartSize = 1.5f;
				Bubble.EndSize = Random.FRandRange(3.f, 5.f) * FMath::Sqrt(BodyScale);
				Bubble.Gravity = 0.f;
				Bubble.Drag = 1.f;
			}
			else
			{
				// A breath of sick air lifting off it.
				FParticle& Wisp = AddParticle(EParticle::Smoke, OnBody(), Across(Random) * 15.f + Up * 15.f, Random.FRandRange(0.9f, 1.4f));
				Wisp.Color = RotCloud;
				Wisp.Intensity = 0.28f * Bright;
				Wisp.StartSize = 6.f * BodyScale;
				Wisp.EndSize = 24.f * BodyScale;
				Wisp.Gravity = -0.02f;
				Wisp.Drag = 2.f;
			}
			break;
		}
		default:
			return;
		}
	}
}
