#include "Combat/BulletSubsystem.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/GunfeelSoundTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "World/BreakableKinds.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"

// UBulletSubsystem: how a bullet's hit looks and sounds. The world by its surface (its puff, its chunks landing on the
// ground under the hit, its sound and mark, a glancing round's ricochet), creatures by what they spill, the plated
// targets' ring.

namespace
{
	/**
	 * Impacts of one sound closer together than this (seconds and cm) make one: a shotgun's nine pellets on a wall are one
	 * crack, and a long burst into the dirt doesn't stack a sound per round. A blast across two surfaces sounds both.
	 */
	constexpr double ImpactSoundGap = 0.05;
	constexpr float ImpactSoundMerge = 250.f;

	/**
	 * A ricochet: on a hit off stone, gravel or metal that met the surface at less than about 25 degrees (the cosine
	 * between the shot and the surface's normal under this), on about half of them. Its whine never twice within the gap;
	 * a blast's pellets show RicochetLooksPerVolley at most.
	 */
	constexpr float RicochetSquare = 0.42f;
	constexpr float RicochetChance = 0.5f;
	constexpr double RicochetGap = 0.35;
	constexpr int32 RicochetLooksPerVolley = 2;
	/** How far a ricochet's flight strays from the clean bounce, and how much it lifts off the surface. */
	constexpr float RicochetStray = 0.12f;
	constexpr float RicochetLift = 0.15f;

	/** How many volleys' mark counts are kept: pellets of one blast land within a frame or two of each other. */
	constexpr int32 VolleysKept = 4;

	/**
	 * The ground a world hit's chunks land on: the surface itself when it faces up this much (cosine), else the first
	 * floor straight below the hit (one trace, shared by a blast's pellets within DebrisFloorShare cm of each other),
	 * looked for no deeper than DebrisFloorDepth and only within DebrisFloorRange of the camera (farther, no one sees them
	 * land).
	 */
	constexpr float DebrisFloorUp = 0.6f;
	constexpr float DebrisFloorShare = 200.f;
	constexpr float DebrisFloorDepth = 500.f;
	constexpr float DebrisFloorRange = 4000.f;
}

EImpactSurface UBulletSubsystem::SurfaceOf(const AActor* Actor)
{
	if (Cast<ACreatureBase>(Actor))
	{
		return EImpactSurface::Flesh;
	}
	// A crate or a barrel has health but is wood: a shot splinters it, it doesn't ring like a plated target.
	if (Actor && Actor->ActorHasTag(FName(LooterBreakables::Tag)))
	{
		return EImpactSurface::World;
	}
	if (Actor && Actor->FindComponentByClass<UHealthComponent>())
	{
		return EImpactSurface::Target;
	}
	return EImpactSurface::World;
}

FName UBulletSubsystem::ImpactCueOf(const FHitResult& Hit, EImpactSurface Surface)
{
	if (Surface == EImpactSurface::Flesh)
	{
		return NAME_None;
	}
	if (Surface == EImpactSurface::Target)
	{
		// The training dummies are plated: they ring.
		return LooterSoundCue::ImpactMetal;
	}
	// What the world is made of there (ImpactSurface): tags, the trim sheet's strip, the terrain's map, the material's name.
	return ImpactSurface::PlayableCue(Hit.GetComponent(), ImpactSurface::Of(Hit));
}

bool UBulletSubsystem::RollRicochet(const FHitResult& Hit, EImpactMaterial Material, const FVector& ShotDirection)
{
	if (!ImpactSurface::Ricochets(Material))
	{
		return false;
	}
	const float Square = static_cast<float>(FVector::DotProduct(-ShotDirection.GetSafeNormal(), FVector(Hit.ImpactNormal)));
	return Square < RicochetSquare && FMath::FRand() < RicochetChance;
}

void UBulletSubsystem::ShowHit(const FBullet& Bullet, const FHitResult& Hit, EImpactSurface Surface, EImpactMaterial Material, bool bCritical,
	bool bRicochet)
{
	const FBulletShot& Shot = Bullet.Shot;
	Effects.Initialize(GetWorld());
	if (Surface == EImpactSurface::Flesh)
	{
		// Each creature spills its own: a spider's ichor, a slime's gel, an Unpaid's embers...
		const AActor* Creature = Hit.GetActor();
		Effects.SpawnCreatureHit(Hit.ImpactPoint, Hit.ImpactNormal, Shot.Direction, CreatureBlood::Of(Creature), CreatureBlood::TintOf(Creature),
			Bullet.MarkScale, bCritical);
		return;
	}
	if (Surface != EImpactSurface::World)
	{
		Effects.SpawnImpact(Hit.ImpactPoint, Hit.ImpactNormal, Shot.Direction, Surface, bCritical);
		return;
	}
	FPlane Floor;
	const bool bFloor = FindDebrisFloor(Bullet, Hit, Floor);
	Effects.SpawnSurfaceImpact(Hit.ImpactPoint, Hit.ImpactNormal, Shot.Direction, Material, Bullet.MarkScale, bCritical, bFloor ? &Floor : nullptr);

	if (bRicochet)
	{
		if (RicochetLookVolley != Bullet.Volley)
		{
			RicochetLookVolley = Bullet.Volley;
			RicochetLooks = 0;
		}
		if (RicochetLooks < RicochetLooksPerVolley)
		{
			++RicochetLooks;
			// Off at the bounce, a little up off the surface and a little astray.
			const FVector Normal = FVector(Hit.ImpactNormal).GetSafeNormal();
			const FVector Off = (Shot.Direction.MirrorByVector(Normal) + Normal * RicochetLift + FMath::VRand() * RicochetStray).GetSafeNormal();
			Effects.SpawnRicochet(Hit.ImpactPoint + Normal, Off, Bullet.MarkScale);
		}
	}
}

bool UBulletSubsystem::FindDebrisFloor(const FBullet& Bullet, const FHitResult& Hit, FPlane& OutFloor)
{
	const FVector Normal = FVector(Hit.ImpactNormal).GetSafeNormal();
	if (Normal.Z >= DebrisFloorUp)
	{
		OutFloor = FPlane(Hit.ImpactPoint, Normal);
		return true;
	}
	UWorld* World = GetWorld();
	const FVector Camera = GetCameraLocation();
	if (!World || (!Camera.IsZero() && FVector::DistSquared(Camera, Hit.ImpactPoint) > FMath::Square(DebrisFloorRange)))
	{
		return false;
	}
	if (Bullet.Volley == FloorVolley && FVector::DistSquared2D(Hit.ImpactPoint, FloorAt) < FMath::Square(DebrisFloorShare))
	{
		OutFloor = LastFloor;
		return bLastFloor;
	}
	// A wall's (or an overhang's) chunks fall to whatever floor is below the hit.
	FloorVolley = Bullet.Volley;
	FloorAt = Hit.ImpactPoint;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(DebrisFloor), /*bTraceComplex*/ false);
	if (const AActor* Shooter = Bullet.Shot.Shooter.Get())
	{
		Params.AddIgnoredActor(Shooter);
	}
	if (const AActor* Weapon = Bullet.Shot.Weapon.Get())
	{
		Params.AddIgnoredActor(Weapon);
	}
	const FVector From = FVector(Hit.ImpactPoint) + Normal * 4.f;
	FHitResult Ground;
	bLastFloor = World->LineTraceSingleByChannel(Ground, From, From - FVector(0.0, 0.0, DebrisFloorDepth), Bullet.Shot.Channel, Params)
		&& Ground.ImpactNormal.Z >= 0.5f;
	if (bLastFloor)
	{
		LastFloor = FPlane(Ground.ImpactPoint, FVector(Ground.ImpactNormal).GetSafeNormal());
	}
	OutFloor = LastFloor;
	return bLastFloor;
}

void UBulletSubsystem::PlayImpactSound(const FBullet& Bullet, const FHitResult& Hit, EImpactSurface Surface, EImpactMaterial Material,
	bool bRicochet)
{
	const UWorld* World = GetWorld();
	if (!World || Surface == EImpactSurface::Flesh)
	{
		return;
	}
	const double Now = World->GetTimeSeconds();
	// A round that glanced off stone or metal whines away: the gunfight's ricochets.
	if (bRicochet && Now - LastRicochetTime >= RicochetGap)
	{
		LastRicochetTime = Now;
		LooterSound::PlayAt(this, LooterSoundCue::Impact::Ricochet, Hit.ImpactPoint);
	}
	const FName Cue = Surface == EImpactSurface::Target ? FName(LooterSoundCue::ImpactMetal) : ImpactSurface::PlayableCue(this, Material);
	if (Cue == LastImpactCue && Now - LastImpactSoundTime < ImpactSoundGap
		&& FVector::DistSquared(Hit.ImpactPoint, LastImpactSoundAt) < FMath::Square(ImpactSoundMerge))
	{
		return;
	}
	LastImpactCue = Cue;
	LastImpactSoundTime = Now;
	LastImpactSoundAt = Hit.ImpactPoint;

	// The surface cue by the gun's weight (a revolver's slug and a sniper's round land harder than an SMG's), and a shotgun
	// blast's one cue (its pellets merge above) louder, with one pepper of pellets on top. A big gun adds its heavy layer
	// under the surface (sub-thump and debris). All of them go through the merge above, so a blast is one of each.
	const AWeaponBase* Gun = Bullet.Shot.Weapon.Get();
	const UWeaponDefinition* Definition = Gun ? Gun->GetInstance().Definition : nullptr;
	const bool bBlast = Definition && Definition->Kind == EWeaponKind::Shotgun;
	LooterSound::PlayAt(this, Cue, Hit.ImpactPoint, GunfeelSound::ImpactVolume(Bullet.MarkScale, bBlast));
	const float Heavy = GunfeelSound::ImpactHeavyVolume();
	if (Heavy > 0.f && GunfeelSound::IsHeavy(Bullet.MarkScale, GunfeelSound::ImpactHeavyFrom()))
	{
		LooterSound::PlayAt(this, LooterSoundCue::Gunfeel::ImpactHeavy, Hit.ImpactPoint, Heavy);
	}
	const float Scatter = GunfeelSound::ImpactScatterVolume();
	if (bBlast && Scatter > 0.f)
	{
		LooterSound::PlayAt(this, LooterSoundCue::Gunfeel::ImpactScatter, Hit.ImpactPoint, Scatter);
	}
}

void UBulletSubsystem::LeaveMark(const FBullet& Bullet, const FHitResult& Hit, EImpactMaterial Material)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// One mark a pellet, but at most MarksPerVolley from one blast.
	int32 Entry = VolleyMarks.IndexOfByPredicate([&Bullet](const FVolleyMarks& Each) { return Each.Volley == Bullet.Volley; });
	if (Entry == INDEX_NONE)
	{
		if (VolleyMarks.Num() >= VolleysKept)
		{
			VolleyMarks.RemoveAt(0, 1, EAllowShrinking::No);
		}
		Entry = VolleyMarks.Add({ Bullet.Volley, 0 });
	}
	if (VolleyMarks[Entry].Count >= FBulletMarks::MarksPerVolley)
	{
		return;
	}
	if (Marks.Add(*World, Hit, Bullet.Shot.Direction, Material, Bullet.MarkScale, FMath::FRand()))
	{
		++VolleyMarks[Entry].Count;
	}
}
