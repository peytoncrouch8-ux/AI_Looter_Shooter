#include "Weapons/WeaponRocketBlast.h"
#include "Combat/BulletSubsystem.h"
#include "Weapons/ImpactSurface.h"
#include "Weapons/WeaponFX.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"

// The launcher's rocket as it's seen: its motor's flame and smoke in flight, the back-blast out of the tube, and the burst,
// all drawn in code through the pooled FWeaponFX (no particle assets, no lights), in the frontier-tech guns' ember orange.

namespace
{
	/** The burst's fire: white-hot at the heart, ember orange round it (the guns' ember glow), and its smoke. */
	const FLinearColor FireTint(1.f, 0.42f, 0.1f);
	const FLinearColor FlameColor(1.f, 0.6f, 0.22f);
	/** Smoke is unlit (the color it shows in afternoon light): the burst's dark and sooty, the motor's and the back-blast's paler. */
	const FLinearColor BurstSmoke(0.24f, 0.22f, 0.2f);
	const FLinearColor TrailSmoke(0.62f, 0.6f, 0.57f);

	constexpr float BurstFlashScale = 4.5f;
	constexpr float FireballSize = 260.f;
	constexpr int32 RingPuffs = 12;
	constexpr int32 HazePuffs = 5;
	constexpr int32 GritPieces = 16;
	/** A burst on the ground throws up its dirt; one on a wall or in the air doesn't. */
	constexpr float GroundUp = 0.6f;

	FWeaponFX* EffectsOf(UWorld& World)
	{
		UBulletSubsystem* Bullets = World.GetSubsystem<UBulletSubsystem>();
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(&World);
		return &Effects;
	}

	FVector AcrossFrom(const FVector& Up, FRandomStream& Random)
	{
		const FVector Any = Random.GetUnitVector();
		const FVector Flat = (Any - Up * FVector::DotProduct(Any, Up)).GetSafeNormal();
		return Flat.IsNearlyZero() ? FVector::ForwardVector : Flat;
	}
}

void WeaponRocketBlast::SpawnEffects(UWorld& World, const FVector& Center, const FVector& Up, const FHitResult* Hit)
{
	FWeaponFX* Effects = EffectsOf(World);
	if (!Effects)
	{
		return;
	}
	FRandomStream Random(static_cast<int32>(GetTypeHash(Center)));

	// The crack of light, then the fireball: a white-hot heart in an orange halo, sparks flung and embers rising off it
	// (the death burst's soul-light shapes, in fire).
	Effects->SpawnFlash(Center, Up, BurstFlashScale);
	Effects->SpawnDeathBurst(EDeathBurst::SoulLight, Center, Up, FireballSize, FireTint);

	// What it met answers in its own stuff: dirt, stone grit, splinters, sparks off metal, a splash.
	if (Hit && Hit->bBlockingHit && Hit->GetActor())
	{
		Effects->SpawnSurfaceImpact(Hit->ImpactPoint, Up, -Up, ImpactSurface::Of(*Hit), 3.f, false);
	}
	if (FVector::DotProduct(Up, FVector::UpVector) > GroundUp)
	{
		Effects->SpawnDirt(Center, Up, 1.f);
	}

	// A ring of dark smoke rolling out along the surface...
	for (int32 Index = 0; Index < RingPuffs; ++Index)
	{
		const FVector Out = AcrossFrom(Up, Random);
		const FVector Velocity = Out * Random.FRandRange(320.f, 560.f) + Up * Random.FRandRange(30.f, 110.f);
		Effects->SpawnDustPuff(Center + Out * 25.f, Velocity, BurstSmoke, 0.65f, 40.f, Random.FRandRange(150.f, 230.f),
			Random.FRandRange(1.2f, 1.9f));
	}
	// ...a column of it rising off the spot...
	for (int32 Index = 0; Index < HazePuffs; ++Index)
	{
		const FVector Drift = AcrossFrom(Up, Random) * Random.FRandRange(10.f, 40.f) + Up * Random.FRandRange(60.f, 140.f);
		Effects->SpawnDustPuff(Center + Up * Random.FRandRange(20.f, 80.f), Drift, BurstSmoke, 0.45f, 90.f, Random.FRandRange(240.f, 320.f),
			Random.FRandRange(2.4f, 3.4f));
	}
	// ...and grit and casing shards flung every way, mostly up.
	for (int32 Index = 0; Index < GritPieces; ++Index)
	{
		const FVector Out = (Up * Random.FRandRange(0.4f, 1.2f) + Random.GetUnitVector() * 0.9f).GetSafeNormal();
		Effects->SpawnGrit(Center, Out * Random.FRandRange(400.f, 900.f), Random.FRandRange(1.4f, 3.4f), Random.FRandRange(0.7f, 1.3f));
	}
}

void WeaponRocketBlast::SpawnBackblast(UWorld& World, const FVector& Location, const FVector& Direction)
{
	FWeaponFX* Effects = EffectsOf(World);
	if (!Effects)
	{
		return;
	}
	const FVector Back = Direction.IsNearlyZero() ? -FVector::ForwardVector : Direction.GetSafeNormal();
	FRandomStream Random(static_cast<int32>(GetTypeHash(Location)));
	// The motor's first gout of flame out of the exhaust, then a cone of pale smoke thrown back and spreading.
	Effects->SpawnFlash(Location, Back, 1.2f);
	for (int32 Index = 0; Index < 7; ++Index)
	{
		const FVector Way = (Back + Random.GetUnitVector() * 0.35f).GetSafeNormal();
		Effects->SpawnDustPuff(Location + Way * 10.f, Way * Random.FRandRange(350.f, 700.f), TrailSmoke, 0.5f, 16.f,
			Random.FRandRange(90.f, 150.f), Random.FRandRange(0.8f, 1.3f));
	}
}

void WeaponRocketBlast::DrawFlame(UWorld& World, const FVector& Exhaust, const FVector& Direction)
{
	FWeaponFX* Effects = EffectsOf(World);
	if (!Effects)
	{
		return;
	}
	// A hot round glow on the motor (what reads of a rocket flying away from the eye) and a short tongue of flame behind it.
	const FVector Way = Direction.GetSafeNormal();
	Effects->AddGlow(Exhaust, 26.f, FlameColor, 9.f);
	Effects->AddStreak(Exhaust - Way * 42.f, Exhaust, 7.f, FlameColor, 7.f);
}

void WeaponRocketBlast::PuffSmoke(UWorld& World, const FVector& Exhaust, const FVector& Direction, FRandomStream& Random)
{
	FWeaponFX* Effects = EffectsOf(World);
	if (!Effects)
	{
		return;
	}
	// Left hanging along the flight, drifting a little and spreading as it thins: the trail that points back at the shooter.
	const FVector Drift = -Direction.GetSafeNormal() * Random.FRandRange(20.f, 60.f) + Random.GetUnitVector() * 15.f + FVector(0.f, 0.f, 12.f);
	Effects->SpawnDustPuff(Exhaust, Drift, TrailSmoke, 0.45f, 12.f, Random.FRandRange(55.f, 80.f), Random.FRandRange(1.3f, 1.9f));
}
