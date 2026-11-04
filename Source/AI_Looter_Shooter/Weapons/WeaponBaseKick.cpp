#include "Weapons/WeaponBase.h"
#include "Player/PlayerViewComponent.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/GunKindsTuning.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"

// AWeaponBase's kick: what a shot does to the gun in the hand (its recoil profile, by kind) and to its cone (an automatic's
// bloom, and the reward of a first shot after a rest). The numbers are GunfeelKickTuning's Looter.Gunfeel.* variables and
// each kind's identity (Project Settings > Game > Gun Feel), read per shot, so a change in play shows on the next one. The
// kick's size is GunfeelKick::KickShare (the Recoil stat, the "more kick" for all, the accuracy's calm, over a base Common's);
// the view's kick and the pattern read the same (PlayerViewKicks/PlayerViewAim), so each is applied once and alike.
// Gun identity (2026-10-28) adds each kind's sight shares, aim kick and recovery, the LMG's dig-in and the sniper's held
// recovery; the LMG's climb cap (2026-10-29) keeps a held belt from walking the view up.

namespace
{
	namespace KickDef = GunfeelKick::Default;

	/** Recoil.<Kind>'s shipped default: the gun model's own weight by kind. */
	float ShippedModelWeight(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun: return KickDef::RecoilShotgun;
		case EWeaponKind::Revolver: return KickDef::RecoilRevolver;
		case EWeaponKind::SMG: return KickDef::RecoilSMG;
		case EWeaponKind::Sniper: return KickDef::RecoilSniper;
		case EWeaponKind::LMG: return KickDef::RecoilLMG;
		case EWeaponKind::Launcher: return KickDef::RecoilLauncher;
		default: return KickDef::RecoilRifle;
		}
	}

	/** A burst's climb per shot as shipped (GunfeelKick::ClimbPerShot's defaults). */
	float ShippedClimb(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Rifle: return KickDef::RifleClimb;
		case EWeaponKind::SMG: return KickDef::SMGClimb;
		case EWeaponKind::LMG: return KickDef::LMGClimb;
		default: return 0.f;
		}
	}
}

GunfeelKick::FProfileNumbers GunfeelKick::ShippedProfileNumbers(EWeaponKind Kind)
{
	FProfileNumbers Numbers;
	Numbers.Feel = KindFeelDefault(Kind);
	Numbers.ModelWeight = ShippedModelWeight(Kind);
	// The pattern (shipped at full strength) takes over most of an automatic's sideways kick.
	Numbers.SideRandom = Kind == EWeaponKind::SMG ? KickDef::SideRandomSMG : (IsAutomaticKind(Kind) ? KickDef::SideRandom : 1.f);
	Numbers.Settle = Kind == EWeaponKind::Shotgun ? KickDef::ShotgunSettle : 1.f;
	Numbers.ClimbPerShot = ShippedClimb(Kind);
	Numbers.ClimbMax = IsAutomaticKind(Kind) ? KickDef::RifleClimbMax : 1.f;
	Numbers.ClimbCap = Kind == EWeaponKind::LMG ? GunKinds::Default::LMGClimbCap : 0.f;
	Numbers.ClimbCapFloor = Kind == EWeaponKind::LMG ? GunKinds::Default::LMGClimbCapFloor : 0.f;
	Numbers.WhileFiring = KickDef::RecoveryWhileFiring;
	Numbers.RecoveryDelay = KickDef::RecoveryDelay;
	return Numbers;
}

GunfeelKick::FProfileNumbers GunfeelKick::LiveProfileNumbers(EWeaponKind Kind)
{
	FProfileNumbers Numbers;
	Numbers.Feel = KindFeel(Kind);
	Numbers.ModelWeight = RecoilScale(Kind);
	Numbers.SideRandom = SideRandomScale(Kind);
	Numbers.Settle = Kind == EWeaponKind::Shotgun ? ShotgunSettle() : 1.f;
	Numbers.ClimbPerShot = ClimbPerShot(Kind);
	Numbers.ClimbMax = IsAutomaticKind(Kind) ? RifleClimbMax() : 1.f;
	Numbers.ClimbCap = Kind == EWeaponKind::LMG ? GunKinds::LMGClimbCap() : 0.f;
	Numbers.ClimbCapFloor = Kind == EWeaponKind::LMG ? GunKinds::LMGClimbCapFloor() : 0.f;
	Numbers.WhileFiring = WhileFiringShare();
	Numbers.RecoveryDelay = RecoveryDelaySeconds();
	return Numbers;
}

FWeaponRecoilProfile GunfeelKick::ShotProfile(const FWeaponRecoilProfile& Asset, float Kick, const FProfileNumbers& Numbers)
{
	FWeaponRecoilProfile Profile = Asset;
	const float Share = FMath::Max(Kick, 0.f);
	// The gun's kick (its stats, the "more kick" for all, the accuracy's calm) scales the gun model and the aim alike; the
	// springs stay the same. The kind's own weight in the hand on top: its model kicks back, flips, twists and rolls that much.
	const float Weight = Share * FMath::Max(Numbers.ModelWeight, 0.f);
	Profile.KickBack *= Weight;
	Profile.MuzzleFlip *= Weight;
	Profile.MuzzleTwist *= Weight;
	Profile.Roll *= Weight;
	// The aim's kick (what the player pulls down against) is the kind's own number on the asset's (the rifle's 0.8 degrees).
	const float Aim = Share * FMath::Max(Numbers.Feel.AimKick, 0.f);
	Profile.AimKick *= Aim;
	// An automatic's sideways kick is mostly its pattern (UPlayerViewComponent::HandleWeaponFired adds that), so only a share
	// of the random stays; the other kinds keep all of it.
	Profile.AimKickSide *= Aim * FMath::Max(Numbers.SideRandom, 0.f);
	// Through the sight the model kicks by its kind's shares (full-sight numbers; AddShot blends by how far the sights are
	// up): the six-gun's flip about 5 degrees through the irons, the shotgun shoving back. And the aim comes back at its
	// kind's pace on the asset's own (the six-gun back on target before the next pull, the heavy tubes slower).
	Profile.AimedBack = Numbers.Feel.AimedBack;
	Profile.AimedFlip = Numbers.Feel.AimedFlip;
	Profile.AimedRoll = Numbers.Feel.AimedRoll;
	Profile.AimRecovery *= FMath::Max(Numbers.Feel.Recovery, 0.1f);
	// Heavy (the shotgun's): it settles slower, so the shove is held a moment.
	Profile.Snappiness *= FMath::Max(Numbers.Settle, 0.1f);
	// A burst climbs: the rifle's, the SMG's a little, the LMG's under its weight; a single shot's profile keeps the asset's.
	if (Numbers.ClimbPerShot > 0.f)
	{
		Profile.ClimbPerShot = Numbers.ClimbPerShot;
		Profile.ClimbMax = Numbers.ClimbMax;
	}
	// A held belt lifts the aim a few degrees and no further: past its cap a shot only nods, and the sway carries the burst
	// sideways (a belt that walks into the sky was never the feel we wanted).
	if (Numbers.ClimbCap > 0.f)
	{
		Profile.ClimbCap = Numbers.ClimbCap;
		Profile.ClimbCapFloor = Numbers.ClimbCapFloor;
	}
	// Held, the aim barely settles, so a burst climbs and wants pulling down; let go, it comes back.
	Profile.RecoveryWhileFiring = Numbers.WhileFiring;
	Profile.RecoveryDelay = Numbers.RecoveryDelay;
	return Profile;
}

FWeaponRecoilProfile AWeaponBase::GetRecoilProfile() const
{
	const FWeaponRecoilProfile Asset = Instance.Definition ? Instance.Definition->Recoil : FWeaponRecoilProfile();
	const EWeaponKind Kind = GunfeelKick::KindOf(this);
	// The gun's Recoil stat (its parts) and its accuracy against its kind (the user's call, 2026-10-27: the tighter it shoots,
	// the steadier) set the shot's share of a base Common's kick; an LMG still spinning up kicks harder, a spun one steadier
	// (its dig-in; 1 for the rest).
	const float Kick = GunfeelKick::KickShare(GunfeelKick::ShotRecoil(Instance.Definition.Get(), Instance.Stats)) * GetKickDigIn();
	return GunfeelKick::ShotProfile(Asset, Kick, GunfeelKick::LiveProfileNumbers(Kind));
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
