#include "Weapons/WeaponFX.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"

// FWeaponFX's hit-feel looks (HITS lane D, 2026-10-28; Docs/Work/2026-10-28-Core/HITS.md): what a wound and a kill throw on
// top of the spills and death bursts that already exist. No blood: chitin chips, goo drips, ember streaks, soul tatters, gel
// and ember smoke. Spawning only (WeaponFXDraw.cpp moves and draws every particle); all of it on the existing sets and
// instanced quads, no lights, no new materials.
//
// Budget: each call counts its pieces by DetailAt (x0.35 at 60 m) and, for the kills' looks, KillCrowdDetail (a crowd killed
// at once throws less each), times Looter.Vfx.Hit.Scale or Looter.Vfx.Kill.Scale, then holds them to a small share of the
// pool (Looter.Vfx.MaxParticles) and a hard cap, so the biggest kill (an overkill's tatters, tear and gel pop together)
// stays under 40 pieces a look.

namespace
{
	/** One call may use at most this share of the particle pool, and never more than this many pieces. */
	constexpr float PoolShare = 0.04f;
	constexpr int32 PieceCap = 40;

	/** Chitin as the lit chunks show it: a warm brown. */
	const FLinearColor ChitinColor(0.32f, 0.2f, 0.1f);
	/** Embers' white-hot heart, and the soot an ember's smoke carries (unlit smoke, so it is the colour itself). */
	const FLinearColor EmberHeat(1.f, 0.9f, 0.7f);
	const FLinearColor EmberSoot(0.2f, 0.17f, 0.15f);
	const FLinearColor TatterPale(0.85f, 0.9f, 1.f);
	/** A gel pop's wet mist sits a little paler than the gel. */
	const FLinearColor GelMist(0.9f, 1.f, 0.9f);

	/** How many of Wanted pieces one call may throw, given the pool: held to PoolShare of it and PieceCap, at least one. */
	int32 Allowed(int32 Wanted, int32 Pool)
	{
		if (Wanted <= 0)
		{
			return 0;
		}
		const int32 Share = FMath::Max(1, FMath::RoundToInt32(static_cast<float>(Pool) * PoolShare));
		return FMath::Min3(Wanted, PieceCap, Share);
	}

	/** A random unit vector within a cone of Half angle (radians) about Axis. */
	FVector InCone(const FVector& Axis, float Half, FRandomStream& Random)
	{
		return Random.VRandCone(Axis, Half);
	}
}

void FWeaponFX::SpawnChitinChips(const FVector& Point, const FVector& Normal, float Size, int32 Count)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.HitScale <= 0.f || Count <= 0)
	{
		return;
	}
	const float Scale = FMath::Clamp(Size / 100.f, 0.3f, 2.5f);
	const float Reach = FMath::Sqrt(Scale);
	const FVector Out = Normal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	const int32 Plates = Allowed(VfxRules::Count(Count, DetailAt(Point) * Vfx.HitScale), MaxParticleCount);
	const uint8 Set = BitSetFor(EBitShape::Plate, ChitinColor, 0.f);
	for (int32 Index = 0; Index < Plates; ++Index)
	{
		// Plates snapped off the shell fly out of the wound and fall: small, dry, spinning.
		const FVector Way = (Out + Random.GetUnitVector() * 0.7f).GetSafeNormal();
		FParticle& Chip = AddBit(Set, Point + Out * 2.f, Way * Random.FRandRange(180.f, 420.f) * Reach, Random.FRandRange(2.5f, 5.f) * Scale,
			Random.FRandRange(0.6f, 0.95f));
		Chip.Drag = 0.4f;
		Chip.Spin = Random.GetUnitVector() * Random.FRandRange(8.f, 20.f);
	}
}

void FWeaponFX::SpawnGooDrip(const FVector& Point, const FLinearColor& Tint, float Size)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.HitScale <= 0.f || VfxRules::Count(1, DetailAt(Point) * Vfx.HitScale) <= 0)
	{
		return;
	}
	// One bit of goo letting go and falling, a touch of glow in it so it reads in shade; it shrinks away as it lands.
	const uint8 Set = BitSetFor(EBitShape::Round, Tint, 0.2f);
	FParticle& Drop = AddBit(Set, Point, FVector(Random.FRandRange(-15.f, 15.f), Random.FRandRange(-15.f, 15.f), Random.FRandRange(-60.f, -20.f)),
		FMath::Clamp(Size, 1.f, 8.f), Random.FRandRange(0.7f, 1.0f));
	Drop.Gravity = 1.1f;
}

void FWeaponFX::SpawnEmberTear(const FVector& From, const FVector& Way, float Length, const FLinearColor& Tint, int32 Count)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.KillScale <= 0.f || Count <= 0)
	{
		return;
	}
	const float Amount = DetailAt(From) * KillCrowdDetail(From) * Vfx.KillScale;
	const FVector Shot = Way.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	// Streaks (and a few ash puffs in a cone behind them), each streak ending up Length-ish cm along the shot: it flies at the
	// speed that drag Drag brings to rest about that far on (distance = speed / drag).
	constexpr float Drag = 5.f;
	const int32 Puffs = FMath::Min(3, Allowed(VfxRules::Count(3, Amount), MaxParticleCount));
	const int32 Tears = Allowed(VfxRules::Count(Count, Amount), MaxParticleCount) - Puffs;
	const FLinearColor Hot = FMath::Lerp(Tint, EmberHeat, 0.4f);
	for (int32 Index = 0; Index < Tears; ++Index)
	{
		const FVector Direction = InCone(Shot, 0.3f, Random);
		const float Travel = FMath::Max(Length, 50.f) * Random.FRandRange(0.5f, 1.f);
		FParticle& Streak = AddParticle(EParticle::Streak, From, Direction * Travel * Drag, Random.FRandRange(0.35f, 0.55f));
		Streak.Color = FMath::Lerp(Tint, Hot, Random.FRandRange(0.f, 1.f));
		Streak.Intensity = Random.FRandRange(18.f, 28.f) * Vfx.Glow;
		Streak.StartSize = Random.FRandRange(1.4f, 2.4f);
		Streak.EndSize = Random.FRandRange(25.f, 45.f);
		Streak.Gravity = -0.05f;
		Streak.Drag = Drag;
	}
	for (int32 Index = 0; Index < Puffs; ++Index)
	{
		// The ash cone: dust thrown along the shot, spreading and sinking.
		FParticle& Ash = AddParticle(EParticle::Smoke, From + Shot * 20.f, (InCone(Shot, 0.4f, Random) * Random.FRandRange(160.f, 300.f)),
			Random.FRandRange(0.9f, 1.4f));
		Ash.Color = FLinearColor(0.47f, 0.45f, 0.43f);
		Ash.Intensity = 0.5f * Vfx.Smoke;
		Ash.StartSize = 25.f;
		Ash.EndSize = Random.FRandRange(90.f, 140.f);
		Ash.Gravity = 0.08f;
		Ash.Drag = 2.5f;
	}
}

void FWeaponFX::SpawnSoulTatters(const FVector& Center, const FVector& Way, const FLinearColor& Tint, float Size, int32 Count)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.KillScale <= 0.f || Count <= 0)
	{
		return;
	}
	const float Scale = FMath::Clamp(Size / 100.f, 0.3f, 3.f);
	const float Reach = FMath::Sqrt(Scale);
	const FVector Shot = Way.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const float Amount = DetailAt(Center) * KillCrowdDetail(Center) * Vfx.KillScale;
	// Two pieces a tatter (the wisp and its ember edge), so the call stays under PieceCap.
	const int32 Tatters = Allowed(VfxRules::Count(Count, Amount) * 2, MaxParticleCount) / 2;
	const FLinearColor Edge = FMath::Lerp(Tint, EmberHeat, 0.35f);
	for (int32 Index = 0; Index < Tatters; ++Index)
	{
		// Thrown along the shot, then it curls up: a sideways swirl that drag eats and a lift that grows (about 2 m in all).
		FVector Across = FVector::CrossProduct(Shot, FVector::UpVector).GetSafeNormal();
		if (Across.IsNearlyZero())
		{
			Across = FVector::RightVector;
		}
		const FVector Velocity = (InCone(Shot, 0.5f, Random) * Random.FRandRange(220.f, 520.f) + Across * Random.FRandRange(-90.f, 90.f)
			+ FVector(0.0, 0.0, Random.FRandRange(60.f, 140.f))) * Reach;
		const float Life = Random.FRandRange(1.1f, 1.7f);
		FParticle& Wisp = AddParticle(EParticle::Smoke, Center + Random.GetUnitVector() * 20.f * Scale, Velocity, Life);
		Wisp.Color = FMath::Lerp(TatterPale, Tint, 0.4f);
		Wisp.Intensity = 0.55f * Vfx.Smoke;
		Wisp.StartSize = Random.FRandRange(14.f, 24.f) * Scale;
		Wisp.EndSize = Random.FRandRange(40.f, 70.f) * Scale;
		Wisp.Gravity = -0.3f;
		Wisp.Drag = 1.1f;
		// Its ember edge: a small glow riding with it, burning out sooner.
		FParticle& Ember = AddParticle(EParticle::Flash, Wisp.Location, Velocity, Life * 0.7f);
		Ember.Color = Edge;
		Ember.Intensity = Random.FRandRange(10.f, 18.f) * Vfx.Glow;
		Ember.StartSize = Random.FRandRange(4.f, 7.f) * Reach;
		Ember.EndSize = 1.f;
		Ember.Gravity = -0.3f;
		Ember.Drag = 1.1f;
	}
}

void FWeaponFX::SpawnGelPop(const FVector& Center, const FVector& Way, const FLinearColor& Tint, float Size)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.KillScale <= 0.f)
	{
		return;
	}
	const float Scale = FMath::Clamp(Size / 100.f, 0.3f, 4.f);
	const float Reach = FMath::Sqrt(Scale);
	const FVector Shot = Way.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	const float Amount = DetailAt(Center) * KillCrowdDetail(Center) * Vfx.KillScale;
	// A fan of gel along the shot (blobs, ten for a slime at size 100), a low wet mist, a soft flash, and the core flung out the back.
	const int32 Blobs = Allowed(VfxRules::Count(FMath::Clamp(FMath::RoundToInt32(Size * 0.1f), 6, 24), Amount), MaxParticleCount);
	const uint8 Gel = BitSetFor(EBitShape::Round, Tint, 0.45f);
	for (int32 Index = 0; Index < Blobs; ++Index)
	{
		const FVector Direction = (InCone(Shot, 0.85f, Random) + FVector::UpVector * Random.FRandRange(0.1f, 0.5f)).GetSafeNormal();
		FParticle& Blob = AddBit(Gel, Center, Direction * Random.FRandRange(240.f, 560.f) * Reach, Random.FRandRange(3.f, 7.f) * Scale,
			Random.FRandRange(0.7f, 1.1f));
		Blob.Drag = 0.4f;
	}
	const int32 Mist = FMath::Min(2, Allowed(VfxRules::Count(2, Amount), MaxParticleCount));
	for (int32 Index = 0; Index < Mist; ++Index)
	{
		FParticle& Spray = AddParticle(EParticle::Smoke, Center, (Shot * Random.FRandRange(120.f, 220.f) + FVector(0.0, 0.0, 20.0)) * Reach,
			Random.FRandRange(0.5f, 0.8f));
		Spray.Color = FMath::Lerp(Tint, GelMist, 0.3f);
		Spray.Intensity = 0.4f * Vfx.Smoke;
		Spray.StartSize = 18.f * Scale;
		Spray.EndSize = Random.FRandRange(70.f, 100.f) * Scale;
		Spray.Gravity = 0.1f;
		Spray.Drag = 4.f;
	}
	if (Amount > 0.f)
	{
		FParticle& Flash = AddParticle(EParticle::Flash, Center, FVector::ZeroVector, 0.07f);
		Flash.Color = FMath::Lerp(Tint, GelMist, 0.5f);
		Flash.Intensity = 5.f * Vfx.Glow;
		Flash.StartSize = WoundFlashSize(Center, 40.f * Scale) * 0.4f;
		Flash.EndSize = WoundFlashSize(Center, 40.f * Scale);
		// The core: a dark kernel that carries about 150 cm along the shot (distance = speed / drag).
		constexpr float CoreDrag = 3.f;
		const uint8 Core = BitSetFor(EBitShape::Round, Tint * 0.2f, 0.f);
		FParticle& Kernel = AddBit(Core, Center, Shot * 150.f * CoreDrag * Reach, 9.f * Scale, 1.2f);
		Kernel.Drag = CoreDrag;
		Kernel.Gravity = 0.8f;
	}
}

void FWeaponFX::SpawnEmberPuff(const FVector& Point, const FLinearColor& Tint, float Size)
{
	const FVfxTuning Vfx = FVfxTuning::Live();
	if (Vfx.HitScale <= 0.f || VfxRules::Count(1, DetailAt(Point) * Vfx.HitScale) <= 0)
	{
		return;
	}
	const float Scale = FMath::Clamp(Size / 30.f, 0.3f, 3.f);
	// A small dark puff rising off the coal, and a spark or two floating up out of it.
	FParticle& Plume = AddParticle(EParticle::Smoke, Point, FVector(Random.FRandRange(-12.f, 12.f), Random.FRandRange(-12.f, 12.f), Random.FRandRange(45.f, 80.f)),
		Random.FRandRange(0.8f, 1.2f));
	Plume.Color = FMath::Lerp(EmberSoot, Tint, 0.12f);
	Plume.Intensity = 0.4f * Vfx.Smoke;
	Plume.StartSize = 8.f * Scale;
	Plume.EndSize = Random.FRandRange(26.f, 40.f) * Scale;
	Plume.Gravity = -0.05f;
	Plume.Drag = 2.2f;
	const int32 Sparks = FMath::Min(2, Allowed(2, MaxParticleCount));
	for (int32 Index = 0; Index < Sparks; ++Index)
	{
		FParticle& Ember = AddParticle(EParticle::Spark, Point, (FVector::UpVector * 90.f + Random.GetUnitVector() * 40.f), Random.FRandRange(0.4f, 0.7f));
		Ember.Color = FMath::Lerp(Tint, EmberHeat, Random.FRandRange(0.f, 0.5f));
		Ember.Intensity = Random.FRandRange(14.f, 22.f) * Vfx.Glow;
		Ember.StartSize = Random.FRandRange(0.7f, 1.1f);
		Ember.Gravity = -0.15f;
		Ember.Drag = 2.f;
	}
}
