#pragma once

#include "CoreMinimal.h"
#include "Audio/LooterSoundCues.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponTypes.h"

/**
 * Which sound cue a gun plays, by its kind (its shot, its click on an empty chamber, its draw) and by its reload's steps.
 * One place, so a new kind of gun is heard everywhere at once; kinds without sounds of their own share the rifle's and the
 * game's general ones.
 */
namespace WeaponSounds
{
	inline FName Fire(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun:
			return LooterSoundCue::ShotgunFire;
		case EWeaponKind::Revolver:
			return LooterSoundCue::Revolver::Fire;
		case EWeaponKind::SMG:
			return LooterSoundCue::Gunshot::SMGFire;
		case EWeaponKind::Sniper:
			return LooterSoundCue::Gunshot::SniperFire;
		case EWeaponKind::LMG:
			return LooterSoundCue::Gunshot::LMGFire;
		case EWeaponKind::Launcher:
			return LooterSoundCue::Gunshot::LauncherFire;
		default:
			return LooterSoundCue::RifleFire;
		}
	}

	inline FName DryFire(EWeaponKind Kind)
	{
		return Kind == EWeaponKind::Revolver ? FName(LooterSoundCue::Revolver::DryFire) : FName(LooterSoundCue::DryFire);
	}

	/** A six-gun comes out of its holster; long guns swing off the back. */
	inline FName Equip(EWeaponKind Kind)
	{
		return Kind == EWeaponKind::Revolver ? FName(LooterSoundCue::Revolver::Equip) : FName(LooterSoundCue::Equip);
	}

	/** Put away as a swap begins (the weapon manager): a six-gun back into its holster; long guns swung onto the back. */
	inline FName Holster(EWeaponKind Kind)
	{
		return Kind == EWeaponKind::Revolver ? FName(LooterSoundCue::Revolver::Holster) : FName(LooterSoundCue::Holster);
	}

	/** The sound of a reload's step. */
	inline FName ReloadStep(EReloadStep Step)
	{
		switch (Step)
		{
		case EReloadStep::MagOut:
			return LooterSoundCue::RifleMagOut;
		case EReloadStep::MagIn:
			return LooterSoundCue::RifleMagIn;
		case EReloadStep::Bolt:
			return LooterSoundCue::RifleBolt;
		case EReloadStep::ShellIn:
			return LooterSoundCue::ShotgunShellIn;
		case EReloadStep::Pump:
			return LooterSoundCue::ShotgunPump;
		case EReloadStep::CylinderOut:
			return LooterSoundCue::Revolver::CylinderOut;
		case EReloadStep::Eject:
			return LooterSoundCue::Revolver::Eject;
		case EReloadStep::RoundsIn:
			return LooterSoundCue::Revolver::RoundsIn;
		case EReloadStep::CylinderIn:
			return LooterSoundCue::Revolver::CylinderIn;
		// The frontier-tech kinds' own steps (Saved/Loot/CONTRACT.md: Weapon.<Kind>.<ReloadStep>; a cue with no sounds yet
		// plays nothing). Their magazine steps and the SMG's knob share the rifle's.
		case EReloadStep::BoltOpen:
			return LooterSoundCue::Sniper::BoltOpen;
		case EReloadStep::BoltClose:
			return LooterSoundCue::Sniper::BoltClose;
		case EReloadStep::BoxOut:
			return LooterSoundCue::LMG::BoxOut;
		case EReloadStep::BoxIn:
			return LooterSoundCue::LMG::BoxIn;
		case EReloadStep::BeltLay:
			return LooterSoundCue::LMG::BeltLay;
		case EReloadStep::RocketOut:
			return LooterSoundCue::Launcher::RocketOut;
		case EReloadStep::RocketIn:
			return LooterSoundCue::Launcher::RocketIn;
		case EReloadStep::HelixOut:
			return LooterSoundCue::SMG::HelixOut;
		case EReloadStep::HelixIn:
			return LooterSoundCue::SMG::HelixIn;
		default:
			return NAME_None;
		}
	}
}
