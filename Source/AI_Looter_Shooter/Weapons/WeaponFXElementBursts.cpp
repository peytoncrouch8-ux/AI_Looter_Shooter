#include "Weapons/WeaponFX.h"
#include "Elements/ElementRules.h"

// FWeaponFX's element bursts and leaps (a smite's light, a jolt's flash, a status's kill; a smite's or a jolt's arc to the
// next), the part effects (an Explosive round's pop) and the loot's drop moments (the landing ring, a Legendary's motes).
// Spawning only; WeaponFXDraw.cpp moves and draws every particle. A hit's and a status's looks are WeaponFXElements.cpp.

namespace
{
	/** A burn's hotter heart: its embers are mixed between the element's red-orange and this. */
	const FLinearColor EmberHot(1.f, 0.62f, 0.16f);
	/** A smite's white heart. */
	const FLinearColor SoulWhite(1.f, 0.97f, 0.9f);
	/** Frost's breath: unlit smoke, a cold pale grey-blue. */
	const FLinearColor FrostColor(0.72f, 0.82f, 0.88f);
	/** A rot's wet gobs and sick air (the same colours as WeaponFXElements.cpp's, so the gobs share one bit set). */
	const FLinearColor RotGob(0.3f, 0.52f, 0.12f);
	const FLinearColor RotCloud(0.22f, 0.34f, 0.1f);
	/** Dust and smoke thrown by an Explosive round (unlit, as afternoon light shows it). */
	const FLinearColor PopSmokeColor(0.42f, 0.38f, 0.33f);
	const FLinearColor PopFlashColor(1.f, 0.78f, 0.45f);

	FVector Across(FRandomStream& Random)
	{
		const float Angle = Random.FRandRange(0.f, 2.f * UE_PI);
		return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
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
	case ELooterElement::Thunder:
	{
		// The jolt bursts: a hard white-blue flash gone in a blink, sparks flung out every way, crooked forks jumping off it.
		const FLinearColor Hot = FMath::Lerp(Color, FLinearColor::White, 0.7f);
		Glow(Hot, 34.f, 15.f * Scale, 110.f * Scale, 0.08f);
		Glow(Color, 14.f, 60.f * Scale, (bKill ? 260.f : 180.f) * Scale, 0.2f);
		for (int32 Index = 0, Total = bKill ? 22 : 14; Index < Total; ++Index)
		{
			const FVector Direction = (Random.GetUnitVector() + Up * 0.3f).GetSafeNormal();
			FParticle& Spark = AddParticle(EParticle::Spark, Center, Direction * Random.FRandRange(700.f, 1400.f) * Reach, Random.FRandRange(0.15f, 0.3f));
			Spark.Color = FMath::Lerp(Color, FLinearColor::White, Random.FRandRange(0.2f, 0.7f));
			Spark.Intensity = 24.f;
			Spark.StartSize = 1.6f;
			Spark.Gravity = 0.4f;
			Spark.Drag = 2.5f;
		}
		for (int32 Fork = 0, Forks = bKill ? 6 : 4; Fork < Forks; ++Fork)
		{
			FVector At = Center;
			const FVector Way = Random.GetUnitVector();
			for (int32 Step = 0; Step < 5; ++Step)
			{
				At += (Way + Random.GetUnitVector() * 0.8f).GetSafeNormal() * Random.FRandRange(8.f, 14.f) * Reach;
				FParticle& Bead = AddParticle(EParticle::Flash, At, FVector::ZeroVector, Random.FRandRange(0.07f, 0.12f));
				Bead.Color = Hot;
				Bead.Intensity = 24.f;
				Bead.StartSize = 4.f * Reach;
				Bead.EndSize = 1.f;
				Bead.Gravity = 0.f;
			}
		}
		// A kill's sparks then rain down over the body, slow and cooling, the charge running out of it.
		for (int32 Index = 0, Total = bKill ? 14 : 0; Index < Total; ++Index)
		{
			const FVector Above = Center + Across(Random) * Random.FRandRange(0.f, 40.f) * Scale + Up * Random.FRandRange(20.f, 50.f) * Scale;
			FParticle& Drop = AddParticle(EParticle::Spark, Above, Across(Random) * Random.FRandRange(30.f, 90.f) - Up * Random.FRandRange(80.f, 200.f),
				Random.FRandRange(0.5f, 0.9f));
			Drop.Color = FMath::Lerp(Color, FLinearColor::White, Random.FRandRange(0.3f, 0.6f));
			Drop.Intensity = Random.FRandRange(14.f, 20.f);
			Drop.StartSize = 1.1f;
			Drop.Gravity = 1.f;
			Drop.Drag = 0.6f;
		}
		break;
	}
	case ELooterElement::Rot:
	{
		// It bursts wet: a dim sick flash, green gobs flung out and falling, a cloud rolling out low and spreading.
		Glow(Color, 8.f, 25.f * Scale, 120.f * Scale, 0.16f);
		const uint8 Gobs = BitSetFor(EBitShape::Round, RotGob, 0.2f);
		for (int32 Index = 0, Total = bKill ? 24 : 14; Index < Total; ++Index)
		{
			const FVector Direction = (Random.GetUnitVector() + Up * 0.6f).GetSafeNormal();
			FParticle& Gob = AddParticle(EParticle::Bit, Center, Direction * Random.FRandRange(200.f, 480.f) * Reach, Random.FRandRange(0.8f, 1.3f));
			Gob.BitSet = Gobs;
			Gob.StartSize = Random.FRandRange(2.f, 4.f) * Reach;
			Gob.Drag = 0.4f;
		}
		for (int32 Index = 0, Total = bKill ? 6 : 4; Index < Total; ++Index)
		{
			FParticle& Cloud = AddParticle(EParticle::Smoke, Center, Across(Random) * Random.FRandRange(60.f, 150.f) * Reach + Up * 20.f,
				Random.FRandRange(1.4f, 2.f));
			Cloud.Color = RotCloud;
			Cloud.Intensity = 0.45f;
			Cloud.StartSize = 20.f * Scale;
			Cloud.EndSize = Random.FRandRange(120.f, 170.f) * Scale;
			Cloud.Gravity = -0.01f;
			Cloud.Drag = 1.8f;
		}
		break;
	}
	default:
		break;
	}
}

void FWeaponFX::SpawnElementArc(ELooterElement Element, const FVector& From, const FVector& To)
{
	const FVector Along = To - From;
	const float Length = Along.Size();
	if (Length < 1.f)
	{
		return;
	}
	if (Element == ELooterElement::Thunder)
	{
		// Lightning: a crooked line of kinks thrown well off the straight way, beads close along each leg so it reads as a
		// line, not dots, hot blue-white and gone in a blink, sparks spat off its kinks.
		const FLinearColor Hot = FMath::Lerp(ElementRules::FxColor(Element), FLinearColor::White, 0.55f);
		const FVector Way = Along / Length;
		const FVector Side = FVector::CrossProduct(Way, FMath::Abs(Way.Z) > 0.9f ? FVector::ForwardVector : FVector::UpVector).GetSafeNormal();
		const FVector Side2 = FVector::CrossProduct(Way, Side);
		const int32 Kinks = FMath::Clamp(FMath::CeilToInt32(Length / 45.f), 3, 10);
		const float Wander = FMath::Min(Length * 0.12f, 28.f);
		FVector Last = From;
		int32 Budget = 40;
		for (int32 Kink = 1; Kink <= Kinks; ++Kink)
		{
			const FVector Next = Kink == Kinks ? To
				: From + Along * (static_cast<float>(Kink) / Kinks) + (Side * Random.FRandRange(-1.f, 1.f) + Side2 * Random.FRandRange(-1.f, 1.f)) * Wander;
			const FVector Leg = Next - Last;
			const int32 Steps = FMath::Clamp(FMath::CeilToInt32(Leg.Size() / 12.f), 1, 6);
			for (int32 Step = 0; Step < Steps && Budget > 0; ++Step, --Budget)
			{
				FParticle& Bead = AddParticle(EParticle::Flash, Last + Leg * (static_cast<float>(Step) / Steps), FVector::ZeroVector, Random.FRandRange(0.12f, 0.18f));
				Bead.Color = Hot;
				Bead.Intensity = 26.f;
				Bead.StartSize = 4.5f;
				Bead.EndSize = 1.5f;
				Bead.Gravity = 0.f;
			}
			if (Kink < Kinks)
			{
				FParticle& Spark = AddParticle(EParticle::Spark, Next, (Side * Random.FRandRange(-1.f, 1.f) + Side2 * Random.FRandRange(-1.f, 1.f)
					+ Random.GetUnitVector() * 0.5f).GetSafeNormal() * Random.FRandRange(300.f, 600.f), Random.FRandRange(0.08f, 0.14f));
				Spark.Color = Hot;
				Spark.Intensity = 22.f;
				Spark.StartSize = 1.2f;
				Spark.Gravity = 0.3f;
				Spark.Drag = 4.f;
			}
			Last = Next;
		}
		FParticle& Strike = AddParticle(EParticle::Flash, To, FVector::ZeroVector, 0.1f);
		Strike.Color = Hot;
		Strike.Intensity = 28.f;
		Strike.StartSize = 10.f;
		Strike.EndSize = 50.f;
		Strike.Gravity = 0.f;
		return;
	}
	const FLinearColor Color = FMath::Lerp(ElementRules::FxColor(Element), SoulWhite, 0.35f);
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
