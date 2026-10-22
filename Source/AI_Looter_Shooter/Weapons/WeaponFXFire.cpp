#include "Weapons/WeaponFX.h"
#include "Weapons/VfxRules.h"
#include "Weapons/VfxTuning.h"
#include "Templates/UnrealTemplate.h"

// FWeaponFX's fire (VFX-2, 2026-10-22): flames rolling into soot on M_FX_Fire (alpha-blended, so fire keeps its colour in
// full sun; the additive fireballs on M_FX_Shape washed out to a pale puff in daylight), and the blast built on them
// (SpawnExplosion, moved here from WeaponFXKills.cpp). WeaponFXDraw.cpp moves every particle; CollectFlames draws the
// flames, far to near, since alpha-blended quads must be drawn back to front.

namespace
{
	/** A blast's soot (unlit: times the light it's in) and a soul blast's pale breath; the dust its skirt kicks up. */
	const FLinearColor BlastSoot(0.075f, 0.067f, 0.06f);
	const FLinearColor BlastPale(0.62f, 0.62f, 0.66f);
	const FLinearColor BlastDust(0.27f, 0.22f, 0.165f);
	const FLinearColor BlastWhiteHot(1.f, 0.95f, 0.85f);

	/** A blast's layers, in its reach's shares (the rocket's 350 cm is the design's middle). */
	/** The core: the hot ball the blast opens with, filling its heart for a blink. */
	constexpr float CoreStart = 0.65f;
	constexpr float CoreEnd = 1.f;
	constexpr float CoreLife = 0.75f;
	constexpr float CoreBurn = 0.4f;
	/** The fire puffs rolling out and up, burning for a share of their lives, then soot. */
	constexpr float PuffStartMin = 0.48f;
	constexpr float PuffStartMax = 0.62f;
	constexpr float PuffEndMin = 0.7f;
	constexpr float PuffEndMax = 0.95f;
	constexpr float PuffLifeMin = 1.5f;
	constexpr float PuffLifeMax = 2.1f;
	constexpr float PuffBurnMin = 0.24f;
	constexpr float PuffBurnMax = 0.34f;
	/** The column: soot rising off the spot once the fire's out. */
	constexpr float ColumnDelayMin = 0.15f;
	constexpr float ColumnDelayMax = 0.55f;
	constexpr float ColumnLifeMin = 2.4f;
	constexpr float ColumnLifeMax = 3.2f;
	/** The reach (cm) a blast's fire is sized by at most. */
	constexpr float FlameReachMost = 380.f;
	/** A blast nearer the eye than this many times its fire's reach has its fire drawn smaller, down to this share. */
	constexpr float BlastCloseShare = 2.5f;
	constexpr float BlastCloseLeast = 0.4f;

	/** A flame's size over its distance from the eye where it starts fading, and where it's gone (1: about half a 90-degree view). */
	constexpr float FlameCoverFrom = 0.8f;
	constexpr float FlameCoverTo = 1.6f;

	/** A round quad facing the camera (the engine plane is 100 x 100 in XY, facing +Z). */
	FTransform FlameTransform(const FVector& Location, float Size, float Roll, const FVector& Camera)
	{
		FVector Facing = Camera - Location;
		if (!Facing.Normalize())
		{
			Facing = FVector::UpVector;
		}
		const FQuat Rotation = FRotationMatrix::MakeFromZ(Facing).ToQuat() * FQuat(FVector::UpVector, Roll);
		return FTransform(Rotation, Location, FVector(Size / 100.f, Size / 100.f, 1.f));
	}

	/** A random way across a surface whose up is Up. */
	FVector FlatWay(const FVector& Up, FRandomStream& Random)
	{
		const FVector Any = Random.GetUnitVector();
		const FVector Flat = (Any - Up * FVector::DotProduct(Any, Up)).GetSafeNormal();
		return Flat.IsNearlyZero() ? FVector::ForwardVector : Flat;
	}
}

void FWeaponFX::SpawnFlame(const FVfxFlame& Flame)
{
	if (Flame.Opacity <= 0.f || Flame.Life <= 0.f)
	{
		return;
	}
	const float Delay = FMath::Max(Flame.Delay, 0.f);
	FParticle& Puff = AddParticle(EParticle::Flame, Flame.Location, Flame.Velocity, Flame.Life);
	// A late one waits unseen and still (WeaponFXDraw.cpp: a particle younger than 0 is neither moved nor drawn).
	Puff.Age = -Delay;
	Puff.Color = Flame.Fire;
	Puff.Shade = Flame.Smoke;
	Puff.Intensity = FMath::Min(Flame.Opacity, 1.f);
	Puff.StartSize = FMath::Max(Flame.StartSize, 1.f);
	Puff.EndSize = FMath::Max(Flame.EndSize, 1.f);
	Puff.Burn = FMath::Clamp(Flame.Burn, 0.f, 1.f);
	Puff.Gravity = Flame.Gravity;
	Puff.Drag = FMath::Max(Flame.Drag, 0.f);
}

int32 FWeaponFX::NumFlames() const
{
	int32 Count = 0;
	for (const FParticle& Particle : Particles)
	{
		Count += Particle.Type == EParticle::Flame ? 1 : 0;
	}
	return Count;
}

void FWeaponFX::CollectFlames(const FVector& Camera, TArray<FTransform>& FireTransforms, TArray<float>& FireData, TArray<FTransform>& SmokeTransforms,
	TArray<float>& SmokeData, bool bPuffData)
{
	struct FDrawn
	{
		double Distance;
		int32 Index;
	};
	TArray<FDrawn, TInlineAllocator<64>> Drawn;
	for (int32 Index = 0; Index < Particles.Num(); ++Index)
	{
		const FParticle& Particle = Particles[Index];
		if (Particle.Type == EParticle::Flame && Particle.Age >= 0.f)
		{
			Drawn.Add({ FVector::DistSquared(Particle.Location, Camera), Index });
		}
	}
	// Alpha-blended: far ones first, so a near puff covers a far one (instances draw in their order).
	Drawn.Sort([](const FDrawn& A, const FDrawn& B) { return A.Distance > B.Distance; });
	const bool bFireQuads = FireQuads.IsValid();
	for (const FDrawn& Each : Drawn)
	{
		const FParticle& Particle = Particles[Each.Index];
		const float T = FMath::Clamp(Particle.Age / Particle.Life, 0.f, 1.f);
		// Blooms fast, then spreads slowly as it rises.
		const float Size = FMath::Lerp(Particle.StartSize, Particle.EndSize, 1.f - FMath::Pow(1.f - T, 2.2f));
		// Burns for its share of its life, cooling faster toward the end of it.
		const float Heat = Particle.Burn > 0.f ? FMath::Pow(FMath::Clamp(1.f - T / Particle.Burn, 0.f, 1.f), 0.7f) : 0.f;
		// In at once, out over its last half; thinned as the eye gets into it (a puff filling the view is all overdraw).
		const float Distance = FMath::Sqrt(static_cast<float>(Each.Distance));
		const float Near = FMath::Clamp((Distance - Size * 0.3f) / (Size * 0.5f + 40.f), 0.f, 1.f);
		// A puff filling most of the view reads as a haze and costs the most (translucent overdraw over the whole screen):
		// faded out, then not drawn, as it grows past about half the view's width (a Slag Bomb at the feet).
		const float Cover = Size / FMath::Max(Distance, 1.f);
		const float Fill = 1.f - FMath::Clamp((Cover - FlameCoverFrom) / (FlameCoverTo - FlameCoverFrom), 0.f, 1.f);
		const float Opacity = Particle.Intensity * FMath::Clamp(T * 14.f + 0.15f, 0.f, 1.f) * (1.f - FMath::SmoothStep(0.6f, 1.f, T)) * Near * Fill;
		if (Opacity <= 0.002f)
		{
			continue;
		}
		const FTransform Transform = FlameTransform(Particle.Location, Size, Particle.Roll, Camera);
		if (bFireQuads)
		{
			FireTransforms.Add(Transform);
			FireData.Append({ Particle.Color.R, Particle.Color.G, Particle.Color.B, Particle.Shade.R, Particle.Shade.G, Particle.Shade.B, Opacity, Heat,
				Particle.Roll / UE_TWO_PI, T });
		}
		else
		{
			// Without M_FX_Fire: a smoke puff tinted by its heat (the code runs before the material script has).
			SmokeTransforms.Add(Transform);
			const FLinearColor Color = FMath::Lerp(Particle.Shade, Particle.Color * 1.4f, Heat);
			SmokeData.Append({ Color.R, Color.G, Color.B, Opacity });
			if (bPuffData)
			{
				SmokeData.Append({ Particle.Roll / UE_TWO_PI, T });
			}
		}
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
	// The fire's reach: puffs, smoke and dust sized by the blast's reach up to FlameReachMost (a bigger blast reads by its
	// ring, embers and debris; fire past that size was all overdraw: Slag Bomb's at the feet). A blast at the eye has them
	// drawn smaller still: it fills the view, without screens of overdraw on top of each other.
	const float Fr = FMath::Min(Radius, FlameReachMost);
	const float Eye = bHasCamera ? static_cast<float>(FVector::Dist(Center, LastCamera)) : 1.0e6f;
	const float Fs = Fr * FMath::Clamp(Eye / (Fr * BlastCloseShare), BlastCloseLeast, 1.f);
	TGuardValue<TOptional<FPlane>> FloorDuringSpawn(SpawnFloor, Blast.bOnGround ? TOptional<FPlane>(FPlane(Center, Up)) : TOptional<FPlane>());

	// The crack: a white starburst over the core, gone in a blink (small: the core's fire is what reads in daylight).
	SpawnStarburst(Center + Up * Radius * 0.15f, Radius * (Blast.bSoul ? 0.95f : 0.7f), BlastWhiteHot, (Blast.bSoul ? 18.f : 9.f) * Vfx.Glow, 0.08f, 14);

	if (Blast.bSoul)
	{
		// A soul blast is light, not fire: its glowing balls cooling in its colour (additive) and a pale breath rising.
		for (int32 Index = 0; Index < Look.Fireballs; ++Index)
		{
			const FVector Out = (Up * Random.FRandRange(0.3f, 1.f) + Random.GetUnitVector() * 0.8f).GetSafeNormal();
			SpawnFireball(Center + Out * Radius * Random.FRandRange(0.05f, 0.25f), Out * Radius * Random.FRandRange(0.6f, 1.1f) + Up * Radius * 0.4f,
				Radius * Random.FRandRange(0.18f, 0.28f), Radius * Look.FireballSize * Random.FRandRange(0.8f, 1.2f), Fire, 2.f * Vfx.Glow,
				Look.FireballLife * Random.FRandRange(0.8f, 1.2f));
		}
		if (Blast.bOnGround)
		{
			SpawnShockRing(Center + Up * 5.f, Up, Radius * 0.15f, Radius * Look.RingReach, Radius * 0.07f, FMath::Lerp(Fire, BlastWhiteHot, 0.3f),
				18.f * Vfx.Glow, Look.RingLife);
		}
		const FLinearColor Pale = BlastPale * Blast.DustLight;
		for (int32 Index = 0; Index < N(Look.SmokeColumn / 2); ++Index)
		{
			const FVector Drift = FlatWay(Up, Random) * Random.FRandRange(10.f, 40.f) + Up * Radius * Random.FRandRange(0.2f, 0.45f);
			FParticle& Puff = AddParticle(EParticle::Smoke, Center + Up * Radius * Random.FRandRange(0.05f, 0.25f), Drift, Random.FRandRange(2.4f, 3.4f));
			Puff.Color = Pale;
			Puff.Intensity = 0.45f * Vfx.Smoke;
			Puff.StartSize = Radius * 0.25f;
			Puff.EndSize = Radius * Random.FRandRange(0.7f, 0.95f);
			Puff.Gravity = -0.05f;
			Puff.Drag = 1.6f;
		}
	}
	else
	{
		const FLinearColor Soot = BlastSoot * Blast.DustLight;
		// The core: a hot ball filling the heart at once, white-yellow cooling to its fire's colour, then a dark knot of smoke.
		FVfxFlame Core;
		Core.Location = Center + Up * Fr * 0.2f;
		Core.Velocity = Up * Fr * 0.35f;
		Core.StartSize = Fs * CoreStart;
		Core.EndSize = Fs * CoreEnd;
		Core.Fire = Fire;
		Core.Smoke = Soot;
		Core.Opacity = FMath::Min(Vfx.Smoke * 1.2f, 1.f);
		Core.Life = CoreLife;
		Core.Burn = CoreBurn;
		Core.Gravity = -0.1f;
		Core.Drag = 4.f;
		SpawnFlame(Core);
		// The fire: puffs rolling out and up from the heart, each burning out from its edge into soot that keeps rising.
		const int32 Puffs = FMath::Max(N(Look.Fireballs), 3);
		for (int32 Index = 0; Index < Puffs; ++Index)
		{
			const float Angle = UE_TWO_PI * (static_cast<float>(Index) + Random.FRandRange(-0.3f, 0.3f)) / Puffs;
			FVector SideA, SideB;
			Up.FindBestAxisVectors(SideA, SideB);
			const FVector Across = SideA * FMath::Cos(Angle) + SideB * FMath::Sin(Angle);
			const FVector Out = (Up * Random.FRandRange(0.5f, 1.1f) + Across * Random.FRandRange(0.6f, 1.f)).GetSafeNormal();
			FVfxFlame Puff;
			Puff.Location = Center + Up * Fr * 0.12f + Out * Fr * Random.FRandRange(0.05f, 0.15f);
			Puff.Velocity = Out * Fr * Random.FRandRange(0.55f, 0.95f) + Up * Fr * 0.35f;
			Puff.StartSize = Fs * Random.FRandRange(PuffStartMin, PuffStartMax);
			Puff.EndSize = Fs * Random.FRandRange(PuffEndMin, PuffEndMax);
			Puff.Fire = Fire;
			Puff.Smoke = Soot * Random.FRandRange(0.8f, 1.2f);
			Puff.Opacity = FMath::Min(0.95f * Vfx.Smoke, 1.f);
			Puff.Life = Random.FRandRange(PuffLifeMin, PuffLifeMax);
			Puff.Burn = Random.FRandRange(PuffBurnMin, PuffBurnMax);
			Puff.Delay = Random.FRandRange(0.f, 0.06f);
			// Hot air: thrown out hard, stopped by the air, then rising.
			Puff.Gravity = -0.22f;
			Puff.Drag = 3.f;
			SpawnFlame(Puff);
		}
		// The column: soot rising off the spot after the fire's out, wide and slow.
		for (int32 Index = 0; Index < N(FMath::Max(Look.SmokeColumn * 3 / 5, 2)); ++Index)
		{
			FVfxFlame Puff;
			Puff.Location = Center + Up * Fr * Random.FRandRange(0.25f, 0.5f) + FlatWay(Up, Random) * Fr * Random.FRandRange(0.f, 0.2f);
			Puff.Velocity = Up * Fr * Random.FRandRange(0.15f, 0.3f) + FlatWay(Up, Random) * Random.FRandRange(10.f, 40.f);
			Puff.StartSize = Fs * 0.42f;
			Puff.EndSize = Fs * Random.FRandRange(0.95f, 1.2f);
			Puff.Smoke = Soot * Random.FRandRange(0.9f, 1.3f);
			Puff.Opacity = FMath::Min(0.9f * Vfx.Smoke, 1.f);
			Puff.Life = Random.FRandRange(ColumnLifeMin, ColumnLifeMax);
			Puff.Burn = 0.f;
			Puff.Delay = Random.FRandRange(ColumnDelayMin, ColumnDelayMax);
			Puff.Gravity = -0.05f;
			Puff.Drag = 1.2f;
			SpawnFlame(Puff);
		}
		// A faint ring of light over the ground (the fire and the dust are the blast now; the ring only hints at its reach).
		if (Blast.bOnGround)
		{
			SpawnShockRing(Center + Up * 5.f, Up, Radius * 0.2f, Radius * Look.RingReach, Radius * 0.05f, Fire, 3.5f * Vfx.Glow, Look.RingLife);
		}
	}
	// A faint shock in the air.
	SpawnShockRing(Center + Up * Radius * 0.2f, FVector::ZeroVector, Radius * 0.1f, Radius * 1.2f, Radius * 0.03f, BlastWhiteHot, 2.5f * Vfx.Glow, 0.16f,
		true);

	// The dust the blast throws out over the ground: a skirt rolling out (the ground's colour; a soul blast's pale).
	const FLinearColor Skirt = (Blast.bSoul ? BlastPale : BlastDust) * Blast.DustLight;
	for (int32 Index = 0; Index < N(Look.SmokeRing * 2 / 3); ++Index)
	{
		const FVector Out = FlatWay(Up, Random);
		FParticle& Puff = AddParticle(EParticle::Smoke, Center + Out * Radius * 0.1f + Up * Radius * 0.05f,
			Out * Radius * Random.FRandRange(1.f, 1.7f) + Up * Radius * Random.FRandRange(0.08f, 0.2f), Random.FRandRange(1.2f, 1.9f));
		Puff.Color = Skirt;
		Puff.Intensity = 0.6f * Vfx.Smoke;
		Puff.StartSize = Radius * 0.14f;
		Puff.EndSize = Fs * Random.FRandRange(0.45f, 0.65f);
		Puff.Gravity = -0.02f;
		Puff.Drag = 2.6f;
	}

	// Embers arcing out and down (a soul blast's motes rise instead).
	for (int32 Index = 0; Index < N(Look.Embers); ++Index)
	{
		const FVector Out = (Up * Random.FRandRange(0.3f, 1.2f) + Random.GetUnitVector() * 0.9f).GetSafeNormal();
		FParticle& Ember = AddParticle(EParticle::Spark, Center + Up * 10.f, Out * Radius * Random.FRandRange(1.5f, 3.f), Random.FRandRange(0.6f, 1.3f));
		Ember.Color = FMath::Lerp(Fire, BlastWhiteHot, Random.FRandRange(0.f, 0.5f));
		Ember.Intensity = Random.FRandRange(18.f, 28.f) * Vfx.Glow;
		Ember.StartSize = Random.FRandRange(1.2f, 2.2f);
		Ember.Gravity = Blast.bSoul ? -0.2f : 0.9f;
		Ember.Drag = 1.2f;
	}

	// The ground thrown up in its own look, and its debris raining back.
	if (Blast.bOnGround && Blast.Ground != EImpactMaterial::Unknown)
	{
		const FPlane Floor(Center, Up);
		// A pellet's worth (under 0.8: half its pieces): the blast's own dust and debris are the most of it.
		SpawnSurfaceImpact(Center, Up, -Up, Blast.Ground, 0.7f, false, &Floor);
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
