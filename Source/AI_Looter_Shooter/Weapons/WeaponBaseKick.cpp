#include "Weapons/WeaponBase.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"

// AWeaponBase's kick: what a shot does to the gun in the hand (its recoil profile, by kind) and to its cone (a rifle's bloom).
// The numbers are GunfeelKickTuning's Looter.Gunfeel.* variables, read per shot, so a change in play shows on the next one.

FWeaponRecoilProfile AWeaponBase::GetRecoilProfile() const
{
	FWeaponRecoilProfile Profile = Instance.Definition ? Instance.Definition->Recoil : FWeaponRecoilProfile();
	const EWeaponKind Kind = GunfeelKick::KindOf(this);
	// The gun's Recoil stat (its parts) scales every kick, on the gun and on the aim alike; the springs stay the same.
	const float Kick = Instance.Stats.Recoil;
	// The kind's own weight in the hand on top: its model kicks back, flips, twists and rolls that much more. The aim's
	// kick (what the player pulls down against) is left alone.
	const float Weight = Kick * GunfeelKick::RecoilScale(Kind);
	Profile.KickBack *= Weight;
	Profile.MuzzleFlip *= Weight;
	Profile.MuzzleTwist *= Weight;
	Profile.Roll *= Weight;
	Profile.AimKick *= Kick;
	Profile.AimKickSide *= Kick;
	if (Kind == EWeaponKind::Shotgun)
	{
		// Heavy: it settles slower, so the shove is held a moment.
		Profile.Snappiness *= GunfeelKick::ShotgunSettle();
	}
	else if (Kind == EWeaponKind::Rifle || Kind == EWeaponKind::SMG || Kind == EWeaponKind::LMG)
	{
		// A burst climbs: the rifle's, the SMG's a little, the LMG's under its weight.
		Profile.ClimbPerShot = GunfeelKick::ClimbPerShot(Kind);
		Profile.ClimbMax = GunfeelKick::RifleClimbMax();
	}
	return Profile;
}

float AWeaponBase::GetSpreadBloom() const
{
	const UWorld* World = GetWorld();
	if (SpreadBloom <= 0.f || !World)
	{
		return 0.f;
	}
	return GunfeelKick::BloomRecovered(SpreadBloom, static_cast<float>(World->GetTimeSeconds() - BloomTime));
}

void AWeaponBase::AddShotBloom()
{
	// Only a player's gun blooms (the crosshair opens with it).
	const AActor* Holder = GetOwner();
	const UPlayerViewComponent* View = Holder ? Holder->FindComponentByClass<UPlayerViewComponent>() : nullptr;
	const UWorld* World = GetWorld();
	if (!View || !World)
	{
		return;
	}
	SpreadBloom = GunfeelKick::BloomAfterShot(GunfeelKick::KindOf(this), GetSpreadBloom(), View->GetAimAlpha());
	BloomTime = World->GetTimeSeconds();
}
