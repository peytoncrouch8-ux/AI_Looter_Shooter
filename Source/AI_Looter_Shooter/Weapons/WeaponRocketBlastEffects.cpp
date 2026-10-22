#include "Weapons/WeaponRocketBlast.h"
#include "Combat/BulletSubsystem.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Weapons/GunKindsTuning.h"
#include "Weapons/VfxSubsystem.h"
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
	const FLinearColor TrailSmoke(0.62f, 0.6f, 0.57f);

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
}

void WeaponRocketBlast::SpawnEffects(UWorld& World, const FVector& Center, const FVector& Up, const FHitResult* Hit)
{
	FWeaponFX* Effects = EffectsOf(World);
	if (!Effects)
	{
		return;
	}
	// The VFX pass's blast (FWeaponFX::SpawnExplosion): a white crack, fireballs blooming white-hot to ember orange to a dull
	// red, a ring racing over the ground, a faint shock in the air, a soot skirt and a rising column, embers arcing, the
	// ground thrown up in its own look and debris raining back; then a scorch left on the ground (UVfxSubsystem).
	const EImpactMaterial Ground = Hit && Hit->bBlockingHit && Hit->GetActor() ? ImpactSurface::Of(*Hit) : EImpactMaterial::Unknown;
	FVfxExplosion Blast;
	Blast.Center = Center;
	Blast.Up = Up;
	Blast.Radius = GunKinds::BlastRules().Radius;
	Blast.Fire = FireTint;
	Blast.Ground = Ground;
	Blast.bOnGround = FVector::DotProduct(Up, FVector::UpVector) > GroundUp && Ground != EImpactMaterial::Unknown;
	if (UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(&World))
	{
		Blast.DustLight = Reacts->GetDustLight();
	}
	Effects->SpawnExplosion(Blast);
	// A wall or a body takes the hit's own look too (the ground's is the blast's).
	if (!Blast.bOnGround && Ground != EImpactMaterial::Unknown)
	{
		Effects->SpawnSurfaceImpact(Hit->ImpactPoint, Up, -Up, Ground, 1.6f, false);
	}
	if (Blast.bOnGround)
	{
		if (UVfxSubsystem* Vfx = UVfxSubsystem::Get(&World))
		{
			Vfx->AddSplat(Center, Up, EVfxSplat::Scorch, FLinearColor::Black, Blast.Radius * 0.6f);
		}
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
	Effects->AddGlow(Exhaust, 34.f, FlameColor, 14.f);
	Effects->AddStreak(Exhaust - Way * 52.f, Exhaust, 9.f, FlameColor, 9.f);
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
	// (The VFX pass) a lick of the motor's fire left behind it, cooling in a blink: a flame you see from behind the tube.
	Effects->SpawnFireball(Exhaust, -Direction.GetSafeNormal() * 300.f, 10.f, 30.f, FlameColor, 2.5f, 0.12f);
}
