#include "Weapons/WeaponFX.h"
#include "Elements/ElementRules.h"
#include "Weapons/ElementVfxTuning.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"

// FWeaponFX's looks of an element gun's own shots ("Elements matter" lane F, 2026-10-29; the user: "particles ... for each
// element when it fires, when it hurts an enemy, and more"): the element's sparks round the muzzle flash, the trail behind
// its tracer (the held gun's idle pieces and a mark's flare: WeaponFXElementMarks.cpp). Spawning only (WeaponFXDraw.cpp moves and draws
// every particle); strengths Looter.Vfx.Element.* (FElementVfxTuning). All of it from the pool's glows, smoke and lit bits: no
// lights, no new materials. A hit on a creature is WeaponFXElements.cpp, the deaths WeaponFXElementDeaths.cpp.

namespace
{
	/** The same hearts as WeaponFXElements.cpp's (a burn's hotter core, a smite's white), so the looks agree. */
	const FLinearColor EmberHot(1.f, 0.62f, 0.16f);
	const FLinearColor SoulWhite(1.f, 0.97f, 0.9f);
	/** Unlit smoke (as the light shows it): frost's cold breath, a rot's sick air. */
	const FLinearColor FrostColor(0.72f, 0.82f, 0.88f);
	const FLinearColor RotCloud(0.22f, 0.34f, 0.1f);
	/** A rot's wet gobs: WeaponFXElements.cpp's colour and glow, so they share its bit set. */
	const FLinearColor RotGob(0.3f, 0.52f, 0.12f);
	constexpr float RotGobGlow = 0.2f;
	/** Salt crystals: the element's colour and glow WeaponFXElements.cpp's crystals use (one bit set). */
	constexpr float SaltCrystalGlow = 0.8f;
	/** A salt glint's white-teal twinkle. */
	const FLinearColor SaltGlint(0.85f, 1.f, 0.97f);
	/**
	 * The trails' threads (the tracer's smoke thread's soft line, tinted): heat shimmer a faint warm haze, frost a pale cold
	 * line, Hallow's ribbon pale gold, Rot's a sick green. Unlit and faint: bright unlit lines read as solid strings in shade.
	 */
	const FLinearColor HeatShimmer(0.86f, 0.62f, 0.46f);
	const FLinearColor FrostLine(0.8f, 0.9f, 0.95f);
	const FLinearColor RibbonGold(0.95f, 0.86f, 0.62f);
	const FLinearColor RotThread(0.42f, 0.5f, 0.24f);

	/** In the player's own view the gun is drawn about half a metre off: its muzzle's pieces are this share of their size from behind. */
	constexpr float FirstPersonPieceSize = 0.3f;
	/** ...and they fly this share as fast, and live this share as long (none hangs in front of the eye). */
	constexpr float FirstPersonPieceSpeed = 0.45f;
	constexpr float FirstPersonPieceLife = 0.6f;
}

void FWeaponFX::SpawnElementMuzzle(const FVfxMuzzle& Muzzle)
{
	const FElementVfxTuning Tuning = FElementVfxTuning::Live();
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Muzzle.Element == ELooterElement::None || Tuning.Muzzle <= 0.f || Vfx.MuzzleScale <= 0.f)
	{
		return;
	}
	// A fast gun's every round would be a cloud: the element's sparks go with the powder's, on the same rounds of a burst.
	const FVfxMuzzleLook Look = VfxRules::MuzzleLook(Muzzle.Kind);
	if (Look.Every > 1 && Muzzle.InBurst % Look.Every != 0)
	{
		return;
	}
	const FVector Forward = Muzzle.Forward.IsNearlyZero() ? FVector::ForwardVector : Muzzle.Forward.GetSafeNormal();
	const FVector Up = FVector::UpVector;
	const bool bFirstPerson = Muzzle.bFirstPerson;
	// In the player's own view they fly off away from the crosshair, as the powder glints do (nothing crosses the sights);
	// from behind out with the shot.
	const FVector Away = bFirstPerson && !Muzzle.Away.IsNearlyZero() ? Muzzle.Away.GetSafeNormal() : FVector::ZeroVector;
	auto Way = [this, &Forward, &Away, bFirstPerson](float Spread)
	{
		return bFirstPerson ? (Away + Forward * 0.35f + Random.GetUnitVector() * Spread).GetSafeNormal()
			: (Forward + Random.GetUnitVector() * Spread).GetSafeNormal();
	};
	const float GunScale = FMath::Sqrt(FMath::Max(Muzzle.Scale, 0.2f) * Vfx.MuzzleScale);
	const float Size = (bFirstPerson ? FirstPersonPieceSize : 1.f) * GunScale;
	const float Speed = bFirstPerson ? FirstPersonPieceSpeed : 1.f;
	const float Life = bFirstPerson ? FirstPersonPieceLife : 1.f;
	const float Amount = Tuning.Muzzle * (bFirstPerson ? Vfx.MuzzleSparks1P : 1.f) * DetailAt(Muzzle.Location);
	auto Count = [Amount](int32 Full) { return VfxRules::Count(Full, Amount); };
	const FVector From = Muzzle.Location;
	const FLinearColor Color = ElementRules::FxColor(Muzzle.Element);

	switch (Muzzle.Element)
	{
	case ELooterElement::Ember:
	{
		// Embers spat out with the shot, rising as they cool; from behind a hot breath at the muzzle too.
		for (int32 Index = 0, Total = Count(4); Index < Total; ++Index)
		{
			FParticle& Ember = AddParticle(EParticle::Flash, From, Way(0.5f) * Random.FRandRange(250.f, 650.f) * Speed, Random.FRandRange(0.3f, 0.55f) * Life);
			Ember.Color = FMath::Lerp(Color, EmberHot, Random.FRandRange(0.2f, 0.8f));
			Ember.Intensity = Random.FRandRange(14.f, 20.f) * Vfx.Glow;
			Ember.StartSize = Random.FRandRange(2.f, 3.2f) * Size;
			Ember.EndSize = 0.4f * Size;
			Ember.Gravity = -0.2f;
			Ember.Drag = 2.2f;
		}
		if (!bFirstPerson)
		{
			FParticle& Breath = AddParticle(EParticle::Flash, From + Forward * 6.f * GunScale, Forward * 120.f, 0.08f);
			Breath.Color = FMath::Lerp(Color, EmberHot, 0.4f);
			Breath.Intensity = 8.f * Vfx.Glow;
			Breath.StartSize = 8.f * GunScale;
			Breath.EndSize = 18.f * GunScale;
			Breath.Gravity = 0.f;
		}
		break;
	}
	case ELooterElement::Thunder:
	{
		// Jagged arcs jumping off the muzzle: two or so crooked forks of beads, gone in a blink, and a spark or two.
		const FLinearColor Hot = FMath::Lerp(Color, FLinearColor::White, 0.5f);
		const float Step = bFirstPerson ? 1.2f : 4.5f;
		for (int32 Fork = 0, Forks = Count(2); Fork < Forks; ++Fork)
		{
			FVector At = From;
			const FVector Out = Way(0.7f);
			for (int32 Bead = 0; Bead < 3; ++Bead)
			{
				At += (Out + Random.GetUnitVector() * 0.8f).GetSafeNormal() * Random.FRandRange(0.7f, 1.3f) * Step * GunScale;
				FParticle& Spark = AddParticle(EParticle::Flash, At, FVector::ZeroVector, Random.FRandRange(0.04f, 0.07f));
				Spark.Color = Hot;
				Spark.Intensity = 24.f * Vfx.Glow;
				Spark.StartSize = 2.2f * Size;
				Spark.EndSize = 0.6f * Size;
				Spark.Gravity = 0.f;
			}
		}
		for (int32 Index = 0, Total = Count(2); Index < Total; ++Index)
		{
			FParticle& Spark = AddParticle(EParticle::Spark, From, Way(0.6f) * Random.FRandRange(600.f, 1100.f) * Speed, Random.FRandRange(0.05f, 0.1f) * Life);
			Spark.Color = Hot;
			Spark.Intensity = 22.f * Vfx.Glow;
			Spark.StartSize = 0.8f * Size;
			Spark.Gravity = 0.2f;
			Spark.Drag = 4.f;
		}
		break;
	}
	case ELooterElement::Rot:
	{
		// Green gobs spat out and falling, and from behind a breath of sick air hanging where it fired.
		const uint8 Gobs = BitSetFor(EBitShape::Round, RotGob, RotGobGlow);
		for (int32 Index = 0, Total = Count(3); Index < Total; ++Index)
		{
			FParticle& Gob = AddParticle(EParticle::Bit, From, (Way(0.45f) * Random.FRandRange(150.f, 350.f) - Up * 40.f) * Speed,
				Random.FRandRange(0.4f, 0.6f) * Life);
			Gob.BitSet = Gobs;
			Gob.StartSize = Random.FRandRange(0.9f, 1.6f) * Size;
			Gob.Drag = 0.6f;
		}
		if (!bFirstPerson)
		{
			FParticle& Puff = AddParticle(EParticle::Smoke, From + Forward * 10.f * GunScale, Forward * Random.FRandRange(60.f, 120.f) + Up * 15.f, 0.6f);
			Puff.Color = RotCloud * Muzzle.DustLight;
			Puff.Intensity = 0.2f * Vfx.Smoke;
			Puff.StartSize = 6.f * GunScale;
			Puff.EndSize = 24.f * GunScale;
			Puff.Gravity = -0.02f;
			Puff.Drag = 2.5f;
		}
		break;
	}
	case ELooterElement::GraveSalt:
	{
		// Salt glints twinkling off the muzzle, a couple of crystals tossed, a breath of frost from behind.
		for (int32 Index = 0, Total = Count(4); Index < Total; ++Index)
		{
			FParticle& Glint = AddParticle(EParticle::Flash, From, Way(0.6f) * Random.FRandRange(100.f, 300.f) * Speed, Random.FRandRange(0.08f, 0.16f));
			Glint.Color = FMath::Lerp(Color, SaltGlint, 0.6f);
			Glint.Intensity = 20.f * Vfx.Glow;
			Glint.StartSize = 2.2f * Size;
			Glint.EndSize = 0.3f * Size;
			Glint.Gravity = 0.2f;
			Glint.Drag = 3.f;
		}
		const uint8 Crystals = BitSetFor(EBitShape::Plate, Color, SaltCrystalGlow);
		for (int32 Index = 0, Total = Count(2); Index < Total; ++Index)
		{
			FParticle& Crystal = AddParticle(EParticle::Bit, From, Way(0.5f) * Random.FRandRange(200.f, 450.f) * Speed, Random.FRandRange(0.35f, 0.5f) * Life);
			Crystal.BitSet = Crystals;
			Crystal.StartSize = Random.FRandRange(0.6f, 1.1f) * Size;
			Crystal.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Crystal.Spin = Random.GetUnitVector() * Random.FRandRange(8.f, 20.f);
			Crystal.Drag = 0.4f;
		}
		if (!bFirstPerson)
		{
			FParticle& Frost = AddParticle(EParticle::Smoke, From + Forward * 8.f * GunScale, Forward * Random.FRandRange(50.f, 110.f), 0.7f);
			Frost.Color = FrostColor * Muzzle.DustLight;
			Frost.Intensity = 0.22f * Vfx.Smoke;
			Frost.StartSize = 6.f * GunScale;
			Frost.EndSize = 22.f * GunScale;
			// Cold air sinks.
			Frost.Gravity = 0.02f;
			Frost.Drag = 2.5f;
		}
		break;
	}
	case ELooterElement::SoulLight:
	{
		// Gold motes drifting up off the muzzle; from behind a couple of thin rays shot out with it.
		for (int32 Index = 0, Total = Count(4); Index < Total; ++Index)
		{
			FParticle& Mote = AddParticle(EParticle::Flash, From, (Way(0.6f) * Random.FRandRange(60.f, 200.f) + Up * Random.FRandRange(40.f, 90.f)) * Speed,
				Random.FRandRange(0.35f, 0.6f) * Life);
			Mote.Color = FMath::Lerp(Color, SoulWhite, Random.FRandRange(0.f, 0.5f));
			Mote.Intensity = Random.FRandRange(12.f, 18.f) * Vfx.Glow;
			Mote.StartSize = Random.FRandRange(2.4f, 3.4f) * Size;
			Mote.EndSize = 0.6f * Size;
			Mote.Gravity = -0.15f;
			Mote.Drag = 2.5f;
		}
		for (int32 Index = 0, Total = bFirstPerson ? 0 : Count(2); Index < Total; ++Index)
		{
			FParticle& Ray = AddParticle(EParticle::Spark, From, Way(0.25f) * Random.FRandRange(800.f, 1100.f), 0.08f);
			Ray.Color = FMath::Lerp(Color, SoulWhite, 0.4f);
			Ray.Intensity = 20.f * Vfx.Glow;
			Ray.StartSize = 1.2f * GunScale;
			Ray.Gravity = 0.f;
			Ray.Drag = 3.f;
		}
		break;
	}
	default:
		break;
	}
}

void FWeaponFX::SpawnElementTrail(ELooterElement Element, const FVector& Tail, const FVector& Head, float Amount)
{
	const FVector Along = Head - Tail;
	const float Length = static_cast<float>(Along.Size());
	if (Element == ELooterElement::None || Amount <= 0.f || Length < 1.f)
	{
		return;
	}
	const FVfxTuning Vfx = FVfxTuning::Live();
	const FVector Way = Along / Length;
	FVector Side, Side2;
	Way.FindBestAxisVectors(Side, Side2);
	const FVector Up = FVector::UpVector;
	const FLinearColor Color = ElementRules::FxColor(Element);
	// The pieces are thinned with distance (a share of a chance each); the soft line, one a piece, only fades in.
	const float Share = FMath::Min(Amount, 1.5f) * DetailAt((Tail + Head) * 0.5f);
	auto Chance = [this, Share](float Full) { return Random.FRand() < Full * Share; };
	auto At = [this, &Tail, &Along]() { return Tail + Along * Random.FRand(); };
	// The tracer's smoke thread, in the element's tint (AddThread's look: still, widening as it thins, drifting up a little).
	auto Line = [this, &Tail, &Head, Amount](const FLinearColor& LineColor, float Width, float Opacity, float Life)
	{
		FParticle& Thread = AddParticle(EParticle::Thread, (Tail + Head) * 0.5, FVector(0.0, 0.0, 6.0), Life * Random.FRandRange(0.85f, 1.15f));
		Thread.Extent = (Head - Tail) * 0.5;
		Thread.Color = LineColor;
		Thread.Intensity = Opacity * FMath::Min(Amount, 1.f);
		Thread.StartSize = Width;
		Thread.EndSize = Width * 3.f;
		Thread.Gravity = 0.f;
		Thread.Drag = 0.5f;
	};

	switch (Element)
	{
	case ELooterElement::Ember:
	{
		// Heat shimmer along the way, embers shed off it rising as they cool.
		Line(HeatShimmer, 3.f, 0.07f * Vfx.Smoke, 0.35f);
		for (int32 Index = 0; Index < 2; ++Index)
		{
			if (!Chance(Index == 0 ? 0.85f : 0.35f))
			{
				continue;
			}
			FParticle& Ember = AddParticle(EParticle::Flash, At(), Up * Random.FRandRange(30.f, 90.f) + Random.GetUnitVector() * 30.f,
				Random.FRandRange(0.35f, 0.65f));
			Ember.Color = FMath::Lerp(Color, EmberHot, Random.FRandRange(0.2f, 0.8f));
			Ember.Intensity = Random.FRandRange(12.f, 18.f) * Vfx.Glow;
			Ember.StartSize = Random.FRandRange(1.6f, 2.6f);
			Ember.EndSize = 0.4f;
			Ember.Gravity = -0.12f;
			Ember.Drag = 1.6f;
		}
		break;
	}
	case ELooterElement::Thunder:
	{
		// A forked spark line: the piece crooked into three legs, each a spark streaking along it (so it crawls on a little
		// and is gone), and now and then a fork spat off a kink.
		const FLinearColor Hot = FMath::Lerp(Color, FLinearColor::White, 0.45f);
		const float Wander = FMath::Min(Length * 0.06f, 7.f);
		FVector Last = Tail;
		for (int32 Leg = 1; Leg <= 3; ++Leg)
		{
			const FVector Next = Leg == 3 ? Head
				: Tail + Along * (Leg / 3.f) + (Side * Random.FRandRange(-1.f, 1.f) + Side2 * Random.FRandRange(-1.f, 1.f)) * Wander;
			const FVector Step = Next - Last;
			const float StepLength = static_cast<float>(Step.Size());
			if (StepLength > 1.f && Chance(0.9f))
			{
				// A spark's streak is its speed x 0.025 s long (WeaponFXDraw.cpp): this speed draws it the leg's length.
				FParticle& Spark = AddParticle(EParticle::Spark, Next, Step / StepLength * FMath::Min(StepLength / 0.025f, 1600.f), Random.FRandRange(0.06f, 0.1f));
				Spark.Color = Hot;
				Spark.Intensity = 22.f * Vfx.Glow;
				Spark.StartSize = 1.f;
				Spark.Gravity = 0.f;
				Spark.Drag = 25.f;
			}
			if (Leg < 3 && Chance(0.15f))
			{
				FParticle& Fork = AddParticle(EParticle::Spark, Next, (Side * Random.FRandRange(-1.f, 1.f) + Side2 * Random.FRandRange(-1.f, 1.f)).GetSafeNormal()
					* Random.FRandRange(300.f, 600.f), Random.FRandRange(0.06f, 0.1f));
				Fork.Color = Hot;
				Fork.Intensity = 20.f * Vfx.Glow;
				Fork.StartSize = 0.8f;
				Fork.Gravity = 0.3f;
				Fork.Drag = 5.f;
			}
			Last = Next;
		}
		break;
	}
	case ELooterElement::Rot:
	{
		// A dripping spatter: green gobs dropping off the way, a faint sick line, now and then a breath of its air.
		Line(RotThread, 2.5f, 0.06f * Vfx.Smoke, 0.4f);
		if (Chance(0.7f))
		{
			FParticle& Drip = AddParticle(EParticle::Bit, At(), Way * Random.FRandRange(0.f, 120.f) - Up * Random.FRandRange(20.f, 60.f), Random.FRandRange(0.6f, 0.9f));
			Drip.BitSet = BitSetFor(EBitShape::Round, RotGob, RotGobGlow);
			Drip.StartSize = Random.FRandRange(0.9f, 1.5f);
			Drip.Gravity = 0.9f;
			Drip.Drag = 0.8f;
		}
		if (Chance(0.3f))
		{
			FParticle& Air = AddParticle(EParticle::Smoke, At(), Up * 10.f + Random.GetUnitVector() * 10.f, 0.6f);
			Air.Color = RotCloud;
			Air.Intensity = 0.14f * Vfx.Smoke;
			Air.StartSize = 5.f;
			Air.EndSize = 20.f;
			Air.Gravity = -0.02f;
			Air.Drag = 2.f;
		}
		break;
	}
	case ELooterElement::GraveSalt:
	{
		// A frost line hanging where it flew, salt glinting in it, a crystal now and then falling out of it.
		Line(FrostLine, 2.5f, 0.14f * Vfx.Smoke, 0.55f);
		if (Chance(0.55f))
		{
			FParticle& Glint = AddParticle(EParticle::Flash, At(), Random.GetUnitVector() * 15.f, Random.FRandRange(0.1f, 0.2f));
			Glint.Color = FMath::Lerp(Color, SaltGlint, 0.6f);
			Glint.Intensity = Random.FRandRange(16.f, 22.f) * Vfx.Glow;
			Glint.StartSize = 2.5f;
			Glint.EndSize = 0.5f;
			Glint.Gravity = 0.05f;
		}
		if (Chance(0.25f))
		{
			FParticle& Crystal = AddParticle(EParticle::Bit, At(), -Up * 20.f + Random.GetUnitVector() * 40.f, Random.FRandRange(0.5f, 0.8f));
			Crystal.BitSet = BitSetFor(EBitShape::Plate, Color, SaltCrystalGlow);
			Crystal.StartSize = Random.FRandRange(0.6f, 1.f);
			Crystal.Rotation = FQuat(Random.GetUnitVector(), Random.FRandRange(0.f, 2.f * UE_PI));
			Crystal.Spin = Random.GetUnitVector() * Random.FRandRange(6.f, 16.f);
			Crystal.Drag = 0.5f;
		}
		break;
	}
	case ELooterElement::SoulLight:
	{
		// A ribbon of light: a pale gold line, motes along it set to either side as a slow twist (its phase by where along the
		// flight the piece lies), drifting up.
		Line(RibbonGold, 3.f, 0.07f * Vfx.Smoke, 0.45f);
		const float Phase = FMath::Frac(static_cast<float>(FVector::DotProduct(Tail, Way)) / 240.f) * 2.f * UE_PI;
		for (int32 Index = 0; Index < 2; ++Index)
		{
			if (!Chance(Index == 0 ? 1.f : 0.6f))
			{
				continue;
			}
			const float T = Random.FRand();
			const FVector Twist = Side * (FMath::Sin(Phase + T * UE_PI) * 3.f) + Side2 * (FMath::Cos(Phase + T * UE_PI) * 3.f);
			FParticle& Mote = AddParticle(EParticle::Flash, Tail + Along * T + Twist, Up * Random.FRandRange(20.f, 60.f) + Random.GetUnitVector() * 15.f,
				Random.FRandRange(0.35f, 0.55f));
			Mote.Color = FMath::Lerp(Color, SoulWhite, Random.FRandRange(0.f, 0.5f));
			Mote.Intensity = Random.FRandRange(10.f, 15.f) * Vfx.Glow;
			Mote.StartSize = Random.FRandRange(2.2f, 3.f);
			Mote.EndSize = 0.6f;
			Mote.Gravity = -0.1f;
			Mote.Drag = 2.f;
		}
		break;
	}
	default:
		break;
	}
}
