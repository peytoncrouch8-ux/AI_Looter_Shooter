#include "Creatures/CreatureKillLook.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/GraveSaltBurst.h"
#include "Combat/HitFeelTuning.h"
#include "Combat/HitReaction.h"
#include "Audio/LooterSound.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureHitCues.h"
#include "Creatures/CreatureHitReactionComponent.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Creatures/CreatureTierLook.h"
#include "Elements/ElementDeathLook.h"
#include "Elements/ElementRules.h"
#include "Weapons/CreatureBlood.h"
#include "Weapons/VfxSubsystem.h"
#include "Weapons/WeaponFX.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

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

	/**
	 * How far downrange of the body a blast or overkill throws a second splat (cm, HITS): a spider's goo 40, an Unpaid's ash 80, a
	 * slime's gel 60 to 90; 0: none for that blood in this style (a spider's blast kill is only its thrown slide).
	 */
	float DownrangeFor(ECreatureBlood Blood, CreatureKillLook::EStyle Style)
	{
		switch (Blood)
		{
		case ECreatureBlood::Gel: return FMath::FRandRange(60.f, 90.f);
		case ECreatureBlood::Embers:
		case ECreatureBlood::MinerEmbers: return 80.f;
		case ECreatureBlood::Ichor:
		case ECreatureBlood::DarkIchor: return Style == CreatureKillLook::EStyle::Overkill ? 40.f : 0.f;
		default: return 0.f;
		}
	}

	/** When the last Big-hit splat was laid (the world's seconds), so no more than one comes every BigSplatGap in the world. */
	double LastBigSplatAt = -1.0e9;
	constexpr double BigSplatGap = 0.5;

	FVector BodyMiddle(const ACreatureBase& Creature)
	{
		const USkeletalMeshComponent* Body = Creature.GetMesh();
		return Body ? FVector(Body->Bounds.Origin) : Creature.GetActorLocation();
	}

	/** An element status's kill: the element's own death over the body, with its sound (the gun's gore layers are already laid). */
	void PlayElementDeath(const ACreatureBase& Creature, FWeaponFX& Effects, ELooterElement Element)
	{
		float Radius = 40.f;
		float HalfHeight = 60.f;
		Creature.GetSimpleCollisionCylinder(Radius, HalfHeight);
		const APlayerController* Player = Creature.GetWorld() ? Creature.GetWorld()->GetFirstPlayerController() : nullptr;
		const FVector Camera = Player && Player->PlayerCameraManager ? FVector(Player->PlayerCameraManager->GetCameraLocation()) : FVector::ZeroVector;
		const FVector Middle = BodyMiddle(Creature);
		ElementDeathLook::Play(Effects, Element, Middle, FMath::Max(Radius, 10.f), FMath::Max(HalfHeight, 10.f), Camera);
		const FName Cue = ElementRules::Cue(Element, EElementCue::Kill);
		if (!Cue.IsNone())
		{
			LooterSound::PlayAt(&Creature, Cue, Middle);
		}
	}
}

namespace CreatureKillLook
{
	EVfxSplat SplatFor(ECreatureBlood Blood)
	{
		switch (Blood)
		{
		case ECreatureBlood::DarkIchor: return EVfxSplat::DarkIchor;
		// A placeholder's clay lies as a gel splat does, in its clay's colour.
		case ECreatureBlood::Gel:
		case ECreatureBlood::Clay: return EVfxSplat::Gel;
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

	EStyle StyleOf(const FCreatureHit& Hit)
	{
		if (!FHitFeelTuning::Live().KillStyles)
		{
			return EStyle::Body;
		}
		if (Hit.Weight == EHitWeight::Overkill)
		{
			return EStyle::Overkill;
		}
		// A shotgun's or a rocket's kill is a blast whatever its weight; so is any Big hit.
		if (Hit.Kind == EWeaponKind::Shotgun || Hit.Kind == EWeaponKind::Launcher || Hit.Weight == EHitWeight::Big)
		{
			return EStyle::Blast;
		}
		return Hit.bCritical ? EStyle::Crit : EStyle::Body;
	}

	bool LayBigHitSplat(ACreatureBase& Creature, const FCreatureHit& Hit)
	{
		UWorld* World = Creature.GetWorld();
		if (!World || Hit.Weight != EHitWeight::Big || Hit.bKilled || Hit.bBoss || GraveSaltBurst::IsBoss(&Creature)
			|| !FHitFeelTuning::Live().KillStyles)
		{
			return false;
		}
		const double Now = World->GetTimeSeconds();
		// (A clock that went backwards, a new level, starts over.)
		if (Now >= LastBigSplatAt && Now - LastBigSplatAt < BigSplatGap)
		{
			return false;
		}
		UVfxSubsystem* Vfx = UVfxSubsystem::Get(&Creature);
		UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(&Creature);
		const FVector Middle = BodyMiddle(Creature);
		if (!Vfx || (Reacts && !Reacts->IsNearAViewer(Middle, SeenWithin)))
		{
			return false;
		}
		// A little beyond the body along the shot, where the spill that went through it would land.
		const FVector Shot = Hit.Direction.GetSafeNormal2D();
		FVector Ground;
		FVector Normal;
		if (Shot.IsNearlyZero()
			|| !KillGroundUnder(Creature, Middle + Shot * (FMath::FRandRange(30.f, 45.f) + 40.f * Creature.GetSizeScale()), Ground, Normal))
		{
			return false;
		}
		LastBigSplatAt = Now;
		const ECreatureBlood Blood = CreatureBlood::Of(&Creature);
		return Vfx->AddSplat(Ground, Normal, SplatFor(Blood), CreatureBlood::TintOf(&Creature), 45.f * Creature.GetSizeScale());
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
		if (Hit.Element != ELooterElement::None)
		{
			// Killed by an element's status: the element's death on top (embers, light, crumbling salt), not the gun's.
			PlayElementDeath(Creature, Effects, Hit.Element);
		}
		// The kill's style (HITS): a slime's gel flung out along the shot (a core crit's pop, a blast's splatter x1.5, an overkill's
		// burst x1.6), and a second splat downrange of a blast's or an overkill's. The spiders' and the Unpaid's own style looks are
		// their kinds' (SpiderCreatureHurt.cpp, UnpaidCreatureHurt.cpp). Not on a status's kill: the element's death is the whole.
		const EStyle Style = Hit.Element == ELooterElement::None ? StyleOf(Hit) : EStyle::Body;
		if (Style == EStyle::Overkill || (Style == EStyle::Blast && Kill.Blood == ECreatureBlood::Gel))
		{
			// The body giving way all at once: its own thud under the kind's sounds.
			LooterSound::PlayAt(&Creature, FName(CreatureHitCue::HitOverkill), Kill.Center);
		}
		if (Style != EStyle::Body && Kill.Blood == ECreatureBlood::Gel)
		{
			const float Burst = Style == EStyle::Overkill ? 1.6f : (Style == EStyle::Blast ? 1.5f : 1.f);
			Effects.SpawnGelPop(Kill.Center, Kill.Direction, Kill.Spill, Kill.Size * Burst);
		}
		if (bSplat && Kill.bGround)
		{
			if (UVfxSubsystem* Vfx = UVfxSubsystem::Get(&Creature))
			{
				Vfx->AddSplat(Kill.Ground, Kill.GroundNormal, SplatFor(Kill.Blood), Kill.Spill, Kill.Size);
				const FVector Shot = Kill.Direction.GetSafeNormal2D();
				const float Downrange = Style == EStyle::Blast || Style == EStyle::Overkill ? DownrangeFor(Kill.Blood, Style) : 0.f;
				FVector Beyond;
				FVector BeyondNormal;
				if (Downrange > 0.f && !Shot.IsNearlyZero()
					&& KillGroundUnder(Creature, Kill.Ground + Shot * Downrange * Creature.GetSizeScale(), Beyond, BeyondNormal))
				{
					Vfx->AddSplat(Beyond, BeyondNormal, SplatFor(Kill.Blood), Kill.Spill, Kill.Size * 0.6f);
				}
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
