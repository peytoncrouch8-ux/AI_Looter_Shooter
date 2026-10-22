#include "Creatures/CreatureKillLook.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/GraveSaltBurst.h"
#include "Combat/HitReaction.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Creatures/CreatureTierLook.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/VfxSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"

// CreatureKillLook: a kill's own layers and its ground splat (the VFX pass), for each kind's death to call.

namespace
{
	/** The ground under a body is looked for this far over its middle and under it (cm). */
	constexpr float KillGroundAbove = 60.f;
	constexpr float KillGroundBelow = 400.f;

	/** The world-static ground under Point (terrain and what's built on it, never a volume or the body itself). */
	bool KillGroundUnder(const ACreatureBase& Creature, const FVector& Point, FVector& OutGround, FVector& OutNormal)
	{
		const UWorld* World = Creature.GetWorld();
		if (!World)
		{
			return false;
		}
		const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, FName(TEXT("KillGround")), &Creature, /*bTraceComplex*/ false);
		FHitResult Hit;
		if (!World->LineTraceSingleByObjectType(Hit, Point + FVector(0.0, 0.0, KillGroundAbove), Point - FVector(0.0, 0.0, KillGroundBelow),
				FCollisionObjectQueryParams(ECC_WorldStatic), Params)
			|| Hit.bStartPenetrating)
		{
			return false;
		}
		OutGround = Hit.ImpactPoint;
		OutNormal = FVector(Hit.ImpactNormal).GetSafeNormal();
		return OutNormal.Z > 0.3f;
	}

	FVector BodyMiddle(const ACreatureBase& Creature)
	{
		const USkeletalMeshComponent* Body = Creature.GetMesh();
		return Body ? FVector(Body->Bounds.Origin) : Creature.GetActorLocation();
	}
}

namespace CreatureKillLook
{
	EVfxSplat SplatFor(ECreatureBlood Blood)
	{
		switch (Blood)
		{
		case ECreatureBlood::DarkIchor: return EVfxSplat::DarkIchor;
		case ECreatureBlood::Gel: return EVfxSplat::Gel;
		case ECreatureBlood::Embers:
		case ECreatureBlood::MinerEmbers: return EVfxSplat::Ash;
		case ECreatureBlood::Ichor:
		case ECreatureBlood::None:
		default: return EVfxSplat::Ichor;
		}
	}

	FVfxKill MakeKill(const ACreatureBase& Creature, const FCreatureHit& Hit)
	{
		FVfxKill Kill;
		Kill.Blood = CreatureBlood::Of(&Creature);
		if (Kill.Blood == ECreatureBlood::None)
		{
			Kill.Blood = ECreatureBlood::Ichor;
		}
		Kill.Tint = UCreatureHitReactionComponent::DeathBurstTint(Creature);
		Kill.Spill = CreatureBlood::TintOf(&Creature);
		Kill.Center = BodyMiddle(Creature);
		Kill.bGround = KillGroundUnder(Creature, Kill.Center, Kill.Ground, Kill.GroundNormal);
		Kill.Direction = Hit.Direction;
		Kill.HitAt = Hit.Location;
		Kill.Size = 100.f * Creature.GetSizeScale();
		Kill.Rank = Creature.GetRank();
		Kill.TierColor = CreatureTierLook::GlowColor(Kill.Rank);
		Kill.Gun = Hit.Kind;
		Kill.bCritical = Hit.bCritical;
		Kill.Share = Hit.Share;
		if (UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(&Creature))
		{
			Kill.DustLight = Reacts->GetDustLight();
		}
		return Kill;
	}

	bool Play(ACreatureBase& Creature, const FCreatureHit& Hit, bool bSplat)
	{
		UWorld* World = Creature.GetWorld();
		// A boss's death is its own show (UBossComponent's slow-motion, loot shower): nothing over it.
		if (!World || GraveSaltBurst::IsBoss(&Creature))
		{
			return false;
		}
		UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(&Creature);
		const FVector Middle = BodyMiddle(Creature);
		if (Reacts && !Reacts->IsNearAViewer(Middle, SeenWithin))
		{
			return false;
		}
		UBulletSubsystem* Bullets = World->GetSubsystem<UBulletSubsystem>();
		if (!Bullets)
		{
			return false;
		}
		const FVfxKill Kill = MakeKill(Creature, Hit);
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		Effects.SpawnKillLayers(Kill);
		if (bSplat && Kill.bGround)
		{
			if (UVfxSubsystem* Vfx = UVfxSubsystem::Get(&Creature))
			{
				Vfx->AddSplat(Kill.Ground, Kill.GroundNormal, SplatFor(Kill.Blood), Kill.Spill, Kill.Size);
			}
		}
		return true;
	}

	bool LaySplat(ACreatureBase& Creature)
	{
		if (GraveSaltBurst::IsBoss(&Creature))
		{
			return false;
		}
		UVfxSubsystem* Vfx = UVfxSubsystem::Get(&Creature);
		UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(&Creature);
		const FVector Middle = BodyMiddle(Creature);
		FVector Ground;
		FVector Normal;
		if (!Vfx || (Reacts && !Reacts->IsNearAViewer(Middle, SeenWithin)) || !KillGroundUnder(Creature, Middle, Ground, Normal))
		{
			return false;
		}
		const ECreatureBlood Blood = CreatureBlood::Of(&Creature);
		return Vfx->AddSplat(Ground, Normal, SplatFor(Blood), CreatureBlood::TintOf(&Creature), 100.f * Creature.GetSizeScale());
	}
}
