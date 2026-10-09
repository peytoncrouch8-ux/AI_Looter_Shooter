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
		default:
			return NAME_None;
		}
	}
}
