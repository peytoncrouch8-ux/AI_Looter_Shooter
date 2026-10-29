#include "Weapons/WeaponBase.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"

// AWeaponBase's kick: what a shot does to the gun in the hand (its recoil profile, by kind) and to its cone (an automatic's
// bloom, and the reward of a first shot after a rest). The numbers are GunfeelKickTuning's Looter.Gunfeel.* variables, read
// per shot, so a change in play shows on the next one. The kick's size is GunfeelKick::ShotRecoil (the Recoil stat, the
// "more kick" for all, the accuracy's calm); the view's kick and the pattern read the same (PlayerViewKicks/PlayerViewAim).
// Gun identity (2026-10-28) adds each kind's sight shares and recovery, the LMG's dig-in and the sniper's held recovery.

FWeaponRecoilProfile AWeaponBase::GetRecoilProfile() const
{
	FWeaponRecoilProfile Profile = Instance.Definition ? Instance.Definition->Recoil : FWeaponRecoilProfile();
	const EWeaponKind Kind = GunfeelKick::KindOf(this);
	// The gun's Recoil stat (its parts) scales every kick, on the gun and on the aim alike; the springs stay the same. Since
	// playtest round 2 every gun kicks harder (Recoil.More) and its accuracy calms it (the tighter it shoots against its
	// kind, the steadier: the user's call, 2026-10-27), so the accuracy stat is felt on every shot.
	// An LMG still spinning up kicks harder, a spun one steadier (its dig-in; 1 for the rest).
	const float Kick = GunfeelKick::ShotRecoil(Instance.Definition.Get(), Instance.Stats) * GetKickDigIn();
	// The kind's own weight in the hand on top: its model kicks back, flips, twists and rolls that much more. The aim's
	// kick (what the player pulls down against) is left alone.
	const float Weight = Kick * GunfeelKick::RecoilScale(Kind);
	Profile.KickBack *= Weight;
	Profile.MuzzleFlip *= Weight;
	Profile.MuzzleTwist *= Weight;
	Profile.Roll *= Weight;
	Profile.AimKick *= Kick;
	// An automatic's sideways kick is mostly its pattern now (UPlayerViewComponent::HandleWeaponFired adds that), so only a
	// share of the old random stays; the other kinds keep all of it.
	Profile.AimKickSide *= Kick * GunfeelKick::SideRandomScale(Kind);
	// Through the sight the model kicks by its kind's shares (full-sight numbers; AddShot blends by how far the sights are
	// up): the six-gun's flip stays its character through the irons, the shotgun shoves back. And the aim comes back at its
	// kind's pace on the asset's own (the six-gun back on target before the next pull, the heavy tubes slower).
	const GunfeelKick::FKindFeel Feel = GunfeelKick::KindFeel(Kind);
	Profile.AimedBack = Feel.AimedBack;
	Profile.AimedFlip = Feel.AimedFlip;
	Profile.AimedRoll = Feel.AimedRoll;
	Profile.AimRecovery *= Feel.Recovery;
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

float AWeaponBase::GetKickDigIn() const
{
	if (GunfeelKick::KindOf(this) != EWeaponKind::LMG)
	{
		return 1.f;
	}
	return GunfeelKick::LMGDigIn(GetSpin());
}

float AWeaponBase::GetRecoilHoldSeconds() const
{
	if (GunfeelKick::KindOf(this) != EWeaponKind::Sniper || !bBoltCycling)
	{
		return 0.f;
	}
	// The bolt's timing is in the world's seconds; the holder's recoil runs on their own clock (faster under Sundown).
	return GunfeelKick::SniperRecoilHold(BoltTiming.Start + BoltTiming.Seconds) * GetOwnerTimeScale();
}

float AWeaponBase::GetSpreadBloom() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}
	const float SinceShot = static_cast<float>(World->GetTimeSeconds() - BloomTime);
	const float Raw = SpreadBloom > 0.f ? GunfeelKick::BloomRecovered(SpreadBloom, SinceShot) : 0.f;
	// A player's automatic that has rested aims tighter for its first shot (negative bloom: the cone eases in); an NPC's gun
	// has no bloom to speak of and no reward.
	const AActor* Holder = GetOwner();
	if (!Holder || !Holder->FindComponentByClass<UPlayerViewComponent>())
	{
		return Raw;
	}
	return GunfeelKick::FirstShotBloom(GunfeelKick::KindOf(this), Raw, SinceShot);
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
	// Build on the real bloom, not the first-shot reward GetSpreadBloom may show (that is negative at rest).
	const float SinceShot = static_cast<float>(World->GetTimeSeconds() - BloomTime);
	const float Raw = SpreadBloom > 0.f ? GunfeelKick::BloomRecovered(SpreadBloom, SinceShot) : 0.f;
	SpreadBloom = GunfeelKick::BloomAfterShot(GunfeelKick::KindOf(this), Raw, View->GetAimAlpha());
	BloomTime = World->GetTimeSeconds();
}
