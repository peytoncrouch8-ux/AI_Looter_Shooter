#include "Weapons/WeaponFX.h"
#include "Elements/ElementRules.h"

// FWeaponFX's looks for the elements (a round's element hit, a status on a body, a smite's burst and leap), the part
// effects (an Explosive round's pop) and the loot's drop moments (the landing ring, a Legendary's motes). Spawning only;
// WeaponFXDraw.cpp moves and draws every particle. Phase 12.3 (LOOT-CODE).

namespace
{
	/** A burn's hotter heart: its embers are mixed between the element's red-orange and this. */
	const FLinearColor EmberHot(1.f, 0.62f, 0.16f);
	/** A smite's white heart. */
	const FLinearColor SoulWhite(1.f, 0.97f, 0.9f);
	/** Frost's breath: unlit smoke, a cold pale grey-blue. */
	const FLinearColor FrostColor(0.72f, 0.82f, 0.88f);
	/** Dust and smoke thrown by an Explosive round (unlit, as afternoon light shows it). */
	const FLinearColor PopSmokeColor(0.42f, 0.38f, 0.33f);
	const FLinearColor PopFlashColor(1.f, 0.78f, 0.45f);

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
	Flash.Color = Element == ELooterElement::SoulLight ? FMath::Lerp(Color, SoulWhite, 0.4f) : Color;
	Flash.Intensity = 14.f * Bright;
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
		default:
			return;
		}
	}
}

void FWeaponFX::SpawnElementBurst(ELooterElement Element, const FVector& Center, float Size, bool bKill)
{
	const FLinearColor Color = ElementRules::FxColor(Element);
	const FVector Up = FVector::UpVector;
	const float Scale = FMath::Clamp(Size / 100.f, 0.4f, 4.f);
	const float Reach = FMath::Sqrt(Scale);
	auto Glow = [this, &Center](const FLinearColor& GlowColor, float Intensity, float From, float To, float Life)
	{
		FParticle& Flash = AddParticle(EParticle::Flash, Center, FVector::ZeroVector, Life);
		Flash.Color = GlowColor;
		Flash.Intensity = Intensity;
		Flash.StartSize = From;
		Flash.EndSize = To;
		Flash.Gravity = 0.f;
	};
	switch (Element)
	{
	case ELooterElement::SoulLight:
	{
		// The gathered light bursts: a white heart in a gold halo, rays flung out (mostly up), motes lifting away.
		Glow(FMath::Lerp(Color, SoulWhite, 0.55f), 30.f, 20.f * Scale, 120.f * Scale, 0.14f);
		Glow(Color, 14.f, 70.f * Scale, (bKill ? 300.f : 210.f) * Scale, 0.35f);
		for (int32 Index = 0, Total = bKill ? 16 : 10; Index < Total; ++Index)
		{
			const FVector Direction = (Random.GetUnitVector() + Up * 0.6f).GetSafeNormal();
			FParticle& Ray = AddParticle(EParticle::Spark, Center, Direction * Random.FRandRange(900.f, 1500.f) * Reach, Random.FRandRange(0.15f, 0.22f));
			Ray.Color = FMath::Lerp(Color, SoulWhite, 0.4f);
			Ray.Intensity = 24.f;
			Ray.StartSize = 2.2f;
			Ray.Gravity = 0.f;
			Ray.Drag = 3.f;
		}
		for (int32 Index = 0, Total = bKill ? 18 : 10; Index < Total; ++Index)
		{
			FParticle& Mote = AddParticle(EParticle::Flash, Center + Random.GetUnitVector() * 25.f * Scale,
				Across(Random) * Random.FRandRange(40.f, 120.f) * Reach + Up * Random.FRandRange(120.f, 240.f) * Reach, Random.FRandRange(0.8f, 1.3f));
			Mote.Color = FMath::Lerp(Color, SoulWhite, Random.FRandRange(0.f, 0.5f));
			Mote.Intensity = Random.FRandRange(14.f, 24.f);
			Mote.StartSize = Random.FRandRange(6.f, 11.f) * Scale;
			Mote.EndSize = 1.5f * Scale;
			Mote.Gravity = -0.2f;
			Mote.Drag = 1.5f;
		}
		break;
	}
	case ELooterElement::Ember:
	{
		// It goes up all at once: a flash, a fountain of embers and tongues of flame, a curl of dark smoke.
		Glow(FMath::Lerp(Color, EmberHot, 0.4f), 20.f, 30.f * Scale, 150.f * Scale, 0.18f);
		for (int32 Index = 0; Index < 22; ++Index)
		{
			FParticle& Ember = AddParticle(EParticle::Flash, Center + Random.GetUnitVector() * 30.f * Scale,
				Across(Random) * Random.FRandRange(60.f, 200.f) * Reach + Up * Random.FRandRange(150.f, 320.f) * Reach, Random.FRandRange(0.7f, 1.3f));
			Ember.Color = FMath::Lerp(Color, EmberHot, Random.FRandRange(0.f, 0.8f));
			Ember.Intensity = Random.FRandRange(14.f, 22.f);
			Ember.StartSize = Random.FRandRange(3.f, 6.f) * Reach;
			Ember.EndSize = 0.8f;
			Ember.Gravity = -0.25f;
			Ember.Drag = 1.6f;
		}
		for (int32 Index = 0; Index < 6; ++Index)
		{
			FParticle& Lick = AddParticle(EParticle::Flash, Center + Across(Random) * 20.f * Scale, Up * Random.FRandRange(220.f, 340.f) * Reach,
				Random.FRandRange(0.28f, 0.4f));
			Lick.Color = FMath::Lerp(Color, EmberHot, 0.3f);
			Lick.Intensity = 16.f;
			Lick.StartSize = Random.FRandRange(14.f, 24.f) * Scale;
			Lick.EndSize = 4.f;
			Lick.Gravity = -0.4f;
			Lick.Drag = 2.f;
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			FParticle& Curl = AddParticle(EParticle::Smoke, Center + Up * 20.f * Scale, Up * Random.FRandRange(60.f, 120.f) + Random.GetUnitVector() * 30.f,
				Random.FRandRange(1.2f, 1.7f));
			Curl.Color = FLinearColor(0.12f, 0.1f, 0.09f);
			Curl.Intensity = 0.4f;
			Curl.StartSize = 25.f * Scale;
			Curl.EndSize = Random.FRandRange(110.f, 160.f) * Scale;
			Curl.Gravity = -0.06f;
			Curl.Drag = 2.f;
		}
		break;
	}
	case ELooterElement::GraveSalt:
	{
		// It crumbles to salt: crystals flung out every way and falling, glints, a cold puff of frost.
		Glow(FMath::Lerp(Color, FLinearColor::White, 0.4f), 12.f, 20.f * Scale, 110.f * Scale, 0.12f);
		const uint8 Crystals = BitSetFor(EBitShape::Plate, Color, 0.8f);
		for (int32 Index = 0; Index < 24; ++Index)
		{
			const FVector Direction = (Random.GetUnitVector() + Up * 0.7f).GetSafeNormal();
			FParticle& Crystal = AddParticle(EParticle::Bit, Center, Direction * Random.FRandRange(220.f, 520.f) * Reach, Random.FRandRange(0.8f, 1.3f));
			Crystal.BitSet = Crystals;
			Crystal.StartSize = Random.FRandRange(1.5f, 3.5f) * Reach;
			Crystal.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Crystal.Spin = Random.GetUnitVector() * Random.FRandRange(6.f, 18.f);
			Crystal.Drag = 0.3f;
		}
		for (int32 Index = 0; Index < 4; ++Index)
		{
			FParticle& Frost = AddParticle(EParticle::Smoke, Center, Across(Random) * Random.FRandRange(80.f, 160.f) + Up * 30.f, Random.FRandRange(0.9f, 1.3f));
			Frost.Color = FrostColor;
			Frost.Intensity = 0.4f;
			Frost.StartSize = 20.f * Scale;
			Frost.EndSize = Random.FRandRange(90.f, 140.f) * Scale;
			Frost.Gravity = 0.03f;
			Frost.Drag = 2.5f;
		}
		break;
	}
	default:
		break;
	}
}

void FWeaponFX::SpawnElementArc(ELooterElement Element, const FVector& From, const FVector& To)
{
	const FLinearColor Color = FMath::Lerp(ElementRules::FxColor(Element), SoulWhite, 0.35f);
	const FVector Along = To - From;
	const float Length = Along.Size();
	if (Length < 1.f)
	{
		return;
	}
	// Beads of light along the way, a little off the line, all fading together: a leap seen for a moment.
	const int32 Beads = FMath::Clamp(FMath::CeilToInt32(Length / 30.f), 2, 24);
	for (int32 Index = 0; Index <= Beads; ++Index)
	{
		const FVector At = From + Along * (static_cast<float>(Index) / Beads) + Random.GetUnitVector() * 4.f;
		FParticle& Bead = AddParticle(EParticle::Flash, At, Random.GetUnitVector() * 10.f, Random.FRandRange(0.22f, 0.3f));
		Bead.Color = Color;
		Bead.Intensity = 18.f;
		Bead.StartSize = 5.f;
		Bead.EndSize = 2.f;
		Bead.Gravity = 0.f;
	}
	FParticle& Landing = AddParticle(EParticle::Flash, To, FVector::ZeroVector, 0.16f);
	Landing.Color = Color;
	Landing.Intensity = 22.f;
	Landing.StartSize = 12.f;
	Landing.EndSize = 60.f;
	Landing.Gravity = 0.f;
}

void FWeaponFX::SpawnSplashPop(const FVector& Location, const FVector& Normal, float Radius)
{
	const FVector Out = Normal.IsNearlyZero() ? FVector::UpVector : Normal.GetSafeNormal();
	const FVector Point = Location + Out * 4.f;
	const float Scale = FMath::Clamp(Radius / 150.f, 0.3f, 3.f);
	FParticle& Core = AddParticle(EParticle::Flash, Point, FVector::ZeroVector, 0.09f);
	Core.Color = PopFlashColor;
	Core.Intensity = 28.f;
	Core.StartSize = 10.f * Scale;
	Core.EndSize = Radius * 0.9f;
	Core.Gravity = 0.f;
	FParticle& Fire = AddParticle(EParticle::Flash, Point + Out * 10.f, FVector::ZeroVector, 0.2f);
	Fire.Color = FLinearColor(1.f, 0.45f, 0.12f);
	Fire.Intensity = 10.f;
	Fire.StartSize = Radius * 0.5f;
	Fire.EndSize = Radius * 1.3f;
	Fire.Gravity = 0.f;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		const FVector Direction = (Out + Random.GetUnitVector() * 0.9f).GetSafeNormal();
		FParticle& Spark = AddParticle(EParticle::Spark, Point, Direction * Random.FRandRange(600.f, 1200.f) * Scale, Random.FRandRange(0.12f, 0.28f));
		Spark.Color = FLinearColor(1.f, 0.55f, 0.18f);
		Spark.Intensity = 20.f;
		Spark.StartSize = 1.3f;
		Spark.Drag = 1.8f;
	}
	// A ring of smoke rolling out along the surface from the burst.
	const FVector Side = FVector::CrossProduct(Out, FMath::Abs(Out.Z) > 0.9f ? FVector::ForwardVector : FVector::UpVector).GetSafeNormal();
	const FVector Side2 = FVector::CrossProduct(Out, Side);
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const float Angle = (Index + Random.FRand() * 0.5f) * (2.f * UE_PI / 6.f);
		const FVector Way = Side * FMath::Cos(Angle) + Side2 * FMath::Sin(Angle);
		FParticle& Puff = AddParticle(EParticle::Smoke, Point, Way * Radius * 2.2f + Out * 40.f, Random.FRandRange(0.6f, 0.9f));
		Puff.Color = PopSmokeColor;
		Puff.Intensity = 0.45f;
		Puff.StartSize = 20.f * Scale;
		Puff.EndSize = Radius * 0.8f;
		Puff.Gravity = -0.03f;
		Puff.Drag = 3.5f;
	}
}

void FWeaponFX::AddGroundRing(const FVector& Center, float Radius, float Width, const FLinearColor& Color, float Intensity, int32 Segments)
{
	if (Radius <= 1.f || Intensity <= 0.f)
	{
		return;
	}
	const int32 Pieces = FMath::Clamp(Segments, 8, 96);
	FVector Last = Center + FVector(Radius, 0.f, 0.f);
	for (int32 Index = 1; Index <= Pieces; ++Index)
	{
		const float Angle = Index * (2.f * UE_PI / Pieces);
		const FVector Next = Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		AddStreak(Last, Next, Width, Color, Intensity);
		Last = Next;
	}
}

void FWeaponFX::SpawnLootMotes(const FVector& Center, float Radius, const FLinearColor& Color, int32 Count)
{
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector At = Center + Across(Random) * Random.FRandRange(0.2f, 1.f) * Radius;
		FParticle& Mote = AddParticle(EParticle::Flash, At, FVector(0.f, 0.f, Random.FRandRange(60.f, 140.f)) + Random.GetUnitVector() * 15.f,
			Random.FRandRange(1.2f, 2.f));
		Mote.Color = Color;
		Mote.Intensity = Random.FRandRange(10.f, 16.f);
		Mote.StartSize = Random.FRandRange(4.f, 7.f);
		Mote.EndSize = 1.f;
		Mote.Gravity = -0.1f;
		Mote.Drag = 1.2f;
	}
}
