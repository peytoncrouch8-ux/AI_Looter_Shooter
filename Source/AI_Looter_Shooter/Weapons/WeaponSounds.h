#pragma once

#include "CoreMinimal.h"
#include "Audio/LooterSoundCues.h"
#include "Weapons/ReloadMotion.h"
#include "Weapons/WeaponBrass.h"
#include "Weapons/WeaponTypes.h"

/**
 * Which sound cue a gun plays, by its kind (its shot, its click on an empty chamber, its draw) and by its reload's steps.
 * One place, so a new kind of gun is heard everywhere at once; kinds without sounds of their own share the rifle's and the
 * game's general ones. The gun feel pass (2026-10-22) gave the sniper its own box, the SMG its own charge, every reload its
 * cloth and the long guns their equips by weight (LooterSoundCue::Gunfeel).
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

	/** The heavy long guns, heard by their weight as they're taken up and put away. */
	inline bool IsHeavy(EWeaponKind Kind)
	{
		return Kind == EWeaponKind::Sniper || Kind == EWeaponKind::LMG || Kind == EWeaponKind::Launcher;
	}

	/** A six-gun comes out of its holster; long guns swing off the back, by their weight (the SMG light, the heavy ones heavy). */
	inline FName Equip(EWeaponKind Kind)
	{
		if (Kind == EWeaponKind::Revolver)
		{
			return LooterSoundCue::Revolver::Equip;
		}
		if (Kind == EWeaponKind::SMG)
		{
			return LooterSoundCue::Gunfeel::EquipLight;
		}
		return IsHeavy(Kind) ? FName(LooterSoundCue::Gunfeel::EquipHeavy) : FName(LooterSoundCue::Equip);
	}

	/** Put away as a swap begins (the weapon manager): a six-gun back into its holster; long guns swung onto the back. */
	inline FName Holster(EWeaponKind Kind)
	{
		if (Kind == EWeaponKind::Revolver)
		{
			return LooterSoundCue::Revolver::Holster;
		}
		return IsHeavy(Kind) ? FName(LooterSoundCue::Gunfeel::HolsterHeavy) : FName(LooterSoundCue::Holster);
	}

	/** The cue of the sights rising on a gun of Kind (gun identity): its own foley. */
	inline FName AimInOf(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun:
			return LooterSoundCue::Gunshot::ShotgunAimIn;
		case EWeaponKind::Revolver:
			return LooterSoundCue::Gunshot::RevolverAimIn;
		case EWeaponKind::SMG:
			return LooterSoundCue::Gunshot::SMGAimIn;
		case EWeaponKind::Sniper:
			return LooterSoundCue::Gunshot::SniperAimIn;
		case EWeaponKind::LMG:
			return LooterSoundCue::Gunshot::LMGAimIn;
		case EWeaponKind::Launcher:
			return LooterSoundCue::Gunshot::LauncherAimIn;
		default:
			return LooterSoundCue::Gunshot::RifleAimIn;
		}
	}

	/**
	 * The sights rising: Kind's own cue (Weapon.<Kind>.AimIn), or the general Weapon.AimIn when the sound bank has no
	 * cue of the kind's yet (bKindCueAvailable false; GunshotSound::AimInCue asks the bank).
	 */
	inline FName AimIn(EWeaponKind Kind, bool bKindCueAvailable = true)
	{
		return bKindCueAvailable ? AimInOf(Kind) : FName(LooterSoundCue::AimIn);
	}

	/** The sights lowering (Weapon.AimOut): one for every kind. */
	inline FName AimOut()
	{
		return LooterSoundCue::AimOut;
	}

	/**
	 * The pitch a spent case of Kind lands with (FWeaponBrass::PlayLanding): the six-gun's fat case a little lower than a
	 * rifle's, the SMG's small one bright, the sniper's magnum low and heavy, the LMG's belt link in between. A hull
	 * keeps the rifle's (it drops lower on soft ground: FWeaponBrass).
	 */
	inline float CasingPitch(ECasingKind Kind)
	{
		switch (Kind)
		{
		case ECasingKind::Pistol:
			return 0.92f;
		case ECasingKind::Small:
			return 1.25f;
		case ECasingKind::Magnum:
			return 0.8f;
		case ECasingKind::Belt:
			return 0.95f;
		case ECasingKind::Rifle:
		case ECasingKind::Hull:
		default:
			return 1.f;
		}
	}

	/** Glove and sling as any reload starts, under its steps. */
	inline FName ReloadStart()
	{
		return LooterSoundCue::Gunfeel::ReloadCloth;
	}

	/** The sound of a reload's step, any gun's (the rifle's for the shared magazine and handle steps). */
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
		// plays nothing).
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

	/**
	 * The sound of a reload's step on a gun of Kind: the sniper's heavier box, the SMG's helix charged where the rifle's
	 * handle is (EReloadStep::Bolt); every other step as ReloadStep(Step).
	 */
	inline FName ReloadStep(EWeaponKind Kind, EReloadStep Step)
	{
		if (Kind == EWeaponKind::Sniper && Step == EReloadStep::MagOut)
		{
			return LooterSoundCue::Gunfeel::SniperMagOut;
		}
		if (Kind == EWeaponKind::Sniper && Step == EReloadStep::MagIn)
		{
			return LooterSoundCue::Gunfeel::SniperMagIn;
		}
		if (Kind == EWeaponKind::SMG && Step == EReloadStep::Bolt)
		{
			return LooterSoundCue::Gunfeel::SMGCharge;
		}
		return ReloadStep(Step);
	}
}
