// The world's side of the Unpaid's hits (HITS, 2026-10-28; the numbers are FHurtChannels', UnpaidCreatureHurt.cpp): the arm
// and chain a shot's bone is of, its hat knocked off (UnpaidHat.cpp), the sparks its coal spits on a crit, the ember smoke of
// a wounded coal, and each kill style's effects and sounds (FWeaponFX's hit-feel block, CreatureHitCue). All on the Unpaid's
// public face, so AUnpaidCreature's header stays as it is. Its kill's shared layers (the overkill's sound, splats) are
// CreatureKillLook's.

#include "Creatures/UnpaidCreature.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/BulletSubsystem.h"
#include "Combat/HitFeelTuning.h"
#include "Combat/HitReaction.h"
#include "Creatures/CreatureHitCues.h"
#include "Creatures/CreatureKillLook.h"
#include "Creatures/CreatureReactComponent.h"
#include "Creatures/CreatureReactSubsystem.h"
#include "Creatures/UnpaidHat.h"
#include "Weapons/WeaponFX.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Math/RandomStream.h"

namespace
{
	using UnpaidBody::EKillStyle;

	/** A coal crit spits this many sparks this far (cm at size 1) out of its back. */
	constexpr int32 CoalSparks = 6;
	constexpr float CoalSparkReach = 120.f;
	/** A wounded coal's smoke: only this near a player (cm), this many puffs at most in any PuffWindow seconds world-wide (~6 smoking at once), this big (cm). */
	constexpr float WoundSeenWithin = 2500.f;
	constexpr double PuffWindow = 0.4;
	constexpr int32 MaxPuffsAtOnce = 6;
	constexpr float PuffSize = 18.f;
	/** Torn: embers torn along the shot this far (cm at size 1), this many; a cone of this much ash after them. */
	constexpr float TearLength = 300.f;
	constexpr int32 TearCount[2] = { 10, 14 };
	constexpr int32 AshCone = 8;
	/** Scattered: torn into this many soul tatters, its body this big (cm at size 1). */
	constexpr int32 TatterCount[2] = { 10, 16 };
	constexpr float TatterBody = 90.f;

	FWeaponFX* HurtEffects(const UObject* Context)
	{
		UWorld* World = Context ? Context->GetWorld() : nullptr;
		UBulletSubsystem* Bullets = World ? World->GetSubsystem<UBulletSubsystem>() : nullptr;
		if (!Bullets)
		{
			return nullptr;
		}
		FWeaponFX& Effects = Bullets->GetEffects();
		Effects.Initialize(World);
		return &Effects;
	}

	bool NearAPlayer(const AUnpaidCreature& Unpaid, const FVector& Where)
	{
		UCreatureReactSubsystem* Reacts = UCreatureReactSubsystem::Get(&Unpaid);
		return Reacts && Reacts->IsNearAViewer(Where, WoundSeenWithin);
	}

	/** The shot's way (world, unit): the hit's, else its push turned into the world. */
	FVector ShotWay(const AUnpaidCreature& Unpaid, const FVector& Away)
	{
		const FVector Direction = Unpaid.GetLastHit().Direction;
		if (!Direction.IsNearlyZero())
		{
			return Direction.GetSafeNormal();
		}
		return FRotator(0.0, Unpaid.GetActorRotation().Yaw, 0.0).RotateVector(Away).GetSafeNormal();
	}

	void KnockHat(AUnpaidCreature& Unpaid, UnpaidBody::FHurtChannels& Hurt, const FVector& Way)
	{
		UStaticMeshComponent* HatMesh = Unpaid.GetHat();
		UCreatureReactComponent* Reacts = Unpaid.GetReact();
		if (HatMesh && Reacts && UnpaidHat::KnockOff(*HatMesh, Hurt.Hat, Way, Reacts->GetRandom()))
		{
			UnpaidHurtPose::PlayCue(Unpaid, CreatureHitCue::UnpaidHatOff);
		}
	}
}

int32 UnpaidHurtPose::ArmOf(const FUnpaidRigBones& Rig, FName Bone)
{
	if (Bone.IsNone())
	{
		return -1;
	}
	const FUnpaidArmBones* Sides[2] = { &Rig.LeftArm, &Rig.RightArm };
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FUnpaidArmBones& Arm = *Sides[Side];
		if (Bone == Arm.UpperArm || Bone == Arm.LowerArm || Bone == Arm.Hand || Arm.Fingers.Contains(Bone))
		{
			return Side;
		}
	}
	return -1;
}

int32 UnpaidHurtPose::ChainOf(const FUnpaidRigBones& Rig, FName Bone)
{
	if (Bone.IsNone())
	{
		return -1;
	}
	return Rig.Shroud.Contains(Bone) ? 0 : Rig.LeftStrip.Contains(Bone) ? 1 : Rig.RightStrip.Contains(Bone) ? 2 : -1;
}

void UnpaidHurtPose::OnHit(AUnpaidCreature& Unpaid, UnpaidBody::FHurtChannels& Hurt, float Strength, const FVector& Away)
{
	const FCreatureHit& Hit = Unpaid.GetLastHit();
	if (!Hit.bFresh)
	{
		// A boss's own jolt played through OnHurt: its flinch as it asks, nothing else.
		Hurt.ClearShares();
		return;
	}
	const FVector Way = ShotWay(Unpaid, Away);
	const FVector ToShooter = FRotator(0.0, Unpaid.GetActorRotation().Yaw, 0.0).UnrotateVector(-Way);
	const UStaticMeshComponent* HatMesh = Unpaid.GetHat();
	const bool bHatOn = HatMesh && HatMesh->GetStaticMesh() && HatMesh->IsVisible() && !Hurt.Hat.IsOff();
	Hurt.OnHit(Hit, Strength, ArmOf(Unpaid.Rig, Hit.Bone), ChainOf(Unpaid.Rig, Hit.Bone), Away, ToShooter, bHatOn, Unpaid.IsBossCreature());
	if (Hurt.bHatOff)
	{
		KnockHat(Unpaid, Hurt, Way);
	}
	if (Hit.Weight == EHitWeight::Big)
	{
		// A Big hit that didn't kill leaves a splat of ash on the ground behind it (CreatureKillLook's pool and rules).
		CreatureKillLook::LayBigHitSplat(Unpaid, Hit);
	}
	if (Hurt.bCoalSparks)
	{
		// Sparks spat out of its back along the shot, as if the round went through the coal.
		if (FWeaponFX* Effects = HurtEffects(&Unpaid))
		{
			const float Size = Unpaid.GetSizeScale();
			Effects->SpawnEmberTear(Unpaid.GetCoalLocation() + Way * (12.f * Size), Way, CoalSparkReach * Size, Unpaid.GetCoalColor(), CoalSparks);
		}
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s hit: zone %d, weight %d, side %d, strength %.2f, twist %.1f cm off its middle."), *Unpaid.GetName(),
		static_cast<int32>(Hit.Zone), static_cast<int32>(Hit.Weight), static_cast<int32>(Hit.Side), Strength, Hit.LateralOffset);
}

void UnpaidHurtPose::OnKilled(AUnpaidCreature& Unpaid, UnpaidBody::FHurtChannels& Hurt, const FVector& Away)
{
	const FCreatureHit& Hit = Unpaid.GetLastHit();
	const bool bBoss = Unpaid.IsBossCreature();
	const EKillStyle Style = Hurt.OnKilled(Hit, Away, bBoss);
	const FVector Way = ShotWay(Unpaid, Away);
	// Killed by a shot to the head, or overkilled: its hat goes flying.
	if (!bBoss && FHitFeelTuning::Live().Hat && (Hit.Zone == ECreatureHitZone::Head || Style == EKillStyle::Scattered))
	{
		KnockHat(Unpaid, Hurt, Way);
	}
	if (Style == EKillStyle::Body)
	{
		return;
	}
	UE_LOG(LogLooter, Verbose, TEXT("%s killed: style %d."), *Unpaid.GetName(), static_cast<int32>(Style));
	FWeaponFX* Effects = HurtEffects(&Unpaid);
	UCreatureReactComponent* Reacts = Unpaid.GetReact();
	const float Size = Unpaid.GetSizeScale();
	const FVector Coal = Unpaid.GetCoalLocation();
	const FLinearColor Tint = Unpaid.GetCoalColor();
	switch (Style)
	{
	case EKillStyle::Snuffed:
		PlayCue(Unpaid, CreatureHitCue::UnpaidSnuff);
		break;
	case EKillStyle::Torn:
		PlayCue(Unpaid, CreatureHitCue::UnpaidTear);
		if (Effects && Reacts)
		{
			FRandomStream& Dice = Reacts->GetRandom();
			Effects->SpawnEmberTear(Coal, Way, TearLength * Size, Tint, Dice.RandRange(TearCount[0], TearCount[1]));
			// A cone of ash thrown on along the shot.
			for (int32 Mote = 0; Mote < AshCone; ++Mote)
			{
				const FVector Drift = Way * Dice.FRandRange(80.f, 160.f)
					+ FVector(Dice.FRandRange(-40.f, 40.f), Dice.FRandRange(-40.f, 40.f), Dice.FRandRange(0.f, 30.f));
				Reacts->ShedAsh(Coal + FVector(0.0, 0.0, Dice.FRandRange(-30.f, 20.f) * Size), Drift, Size, 1);
			}
		}
		break;
	case EKillStyle::Scattered:
		// (The overkill's own sound, Creature.Hit.Overkill, is CreatureKillLook::Play's on any overkill.)
		PlayCue(Unpaid, CreatureHitCue::UnpaidTear);
		if (Effects && Reacts)
		{
			FRandomStream& Dice = Reacts->GetRandom();
			Effects->SpawnSoulTatters(Coal, Way, Tint, TatterBody * Size, Dice.RandRange(TatterCount[0], TatterCount[1]));
		}
		break;
	default:
		break;
	}
}

void UnpaidHurtPose::PuffWound(const AUnpaidCreature& Unpaid)
{
	const FVector Coal = Unpaid.GetCoalLocation();
	const UWorld* World = Unpaid.GetWorld();
	if (!World || !NearAPlayer(Unpaid, Coal))
	{
		return;
	}
	// A few at a time world-wide: a hurt pack doesn't fill the air with smoke.
	static double WindowStart = -1.0;
	static int32 InWindow = 0;
	const double Now = World->GetTimeSeconds();
	if (Now < WindowStart || Now - WindowStart >= PuffWindow)
	{
		WindowStart = Now;
		InWindow = 0;
	}
	if (InWindow >= MaxPuffsAtOnce)
	{
		return;
	}
	if (FWeaponFX* Effects = HurtEffects(&Unpaid))
	{
		++InWindow;
		Effects->SpawnEmberPuff(Coal + FVector(0.0, 0.0, 4.0), Unpaid.GetCoalColor(), PuffSize * Unpaid.GetSizeScale());
	}
}

void UnpaidHurtPose::PlayCue(const AUnpaidCreature& Unpaid, const TCHAR* Cue, float Volume, bool bNearOnly)
{
	const FVector Where = Unpaid.GetCoalLocation();
	if (!bNearOnly || NearAPlayer(Unpaid, Where))
	{
		LooterSound::PlayAt(&Unpaid, FName(Cue), Where, Volume);
	}
}
