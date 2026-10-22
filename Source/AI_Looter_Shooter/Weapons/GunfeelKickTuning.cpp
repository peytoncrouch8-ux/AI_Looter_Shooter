#include "Weapons/GunfeelKickTuning.h"
#include "Combat/CombatRules.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponTypes.h"
#include "HAL/IConsoleManager.h"

// The Looter.Gunfeel.* variables for the kick side of gunplay (GunfeelKickTuning.h), and the plain rules that read them.
// Each accessor reads its variable when asked, never caching it, so a change in play shows on the next shot. The rumble is
// in GunfeelKickRumble.cpp.

namespace
{
	namespace Def = GunfeelKick::Default;

	TAutoConsoleVariable<float> CVarKickRifle(TEXT("Looter.Gunfeel.Kick.Rifle"), Def::KickRifle,
		TEXT("The rifle's view kick (turn and widening of the view per shot) times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKickShotgun(TEXT("Looter.Gunfeel.Kick.Shotgun"), Def::KickShotgun,
		TEXT("The shotgun's view kick times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKickRevolver(TEXT("Looter.Gunfeel.Kick.Revolver"), Def::KickRevolver,
		TEXT("The revolver's view kick times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRecoilRifle(TEXT("Looter.Gunfeel.Recoil.Rifle"), Def::RecoilRifle,
		TEXT("The rifle's own kick (back, flip, twist, roll) times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRecoilShotgun(TEXT("Looter.Gunfeel.Recoil.Shotgun"), Def::RecoilShotgun,
		TEXT("The shotgun's own kick times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRecoilRevolver(TEXT("Looter.Gunfeel.Recoil.Revolver"), Def::RecoilRevolver,
		TEXT("The revolver's own kick times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarShotgunSettle(TEXT("Looter.Gunfeel.Recoil.ShotgunSettle"), Def::ShotgunSettle,
		TEXT("How fast the shotgun's kick springs run (its snappiness times this; under 1 settles heavier)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarMuzzle1P(TEXT("Looter.Gunfeel.Muzzle.Scale1P"), Def::Muzzle1P,
		TEXT("The built-in muzzle flash is this much bigger in first person (third person unchanged)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarLowAmmoShare(TEXT("Looter.Gunfeel.LowAmmo.Share"), Def::LowAmmoShare,
		TEXT("The last share of the magazine clicks as it empties (magazines under 4 rounds never)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLowAmmoPitchTop(TEXT("Looter.Gunfeel.LowAmmo.PitchTop"), Def::LowAmmoPitchTop,
		TEXT("The click's pitch at the last round (1 at the first click of the last share)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarRifleClimb(TEXT("Looter.Gunfeel.Recoil.RifleClimb"), Def::RifleClimb,
		TEXT("The rifle's aim kick grows by this share for each shot of a burst."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRifleClimbMax(TEXT("Looter.Gunfeel.Recoil.RifleClimbMax"), Def::RifleClimbMax,
		TEXT("...up to this many times the first shot's aim kick."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRifleBloom(TEXT("Looter.Gunfeel.Spread.RifleBloom"), Def::RifleBloom,
		TEXT("Each rifle shot widens its spread by this share of the gun's own (a quarter as much through the sight)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBloomMax(TEXT("Looter.Gunfeel.Spread.BloomMax"), Def::BloomMax,
		TEXT("...to at most this many times the spread."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBloomRecovery(TEXT("Looter.Gunfeel.Spread.BloomRecovery"), Def::BloomRecovery,
		TEXT("How fast the bloom closes once firing stops (an exponential rate per second)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarFalloffStart(TEXT("Looter.Gunfeel.Shotgun.FalloffStart"), Def::ShotgunFalloffStart,
		TEXT("A shotgun pellet does full damage out to this share of the gun's range."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFalloffEnd(TEXT("Looter.Gunfeel.Shotgun.FalloffEnd"), Def::ShotgunFalloffEnd,
		TEXT("...falls off to the floor at this many times its range."), ECVF_Default);
	TAutoConsoleVariable<float> CVarFalloffFloor(TEXT("Looter.Gunfeel.Shotgun.FalloffFloor"), Def::ShotgunFalloffFloor,
		TEXT("...and does this share of its damage from there on."), ECVF_Default);

	TAutoConsoleVariable<float> CVarCritRevolver(TEXT("Looter.Gunfeel.Crit.Revolver"), Def::CritRevolver,
		TEXT("What a revolver's critical hit multiplies the damage by (a cursed iron's own wins when larger)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarAimTimeRifle(TEXT("Looter.Gunfeel.Aim.Time.Rifle"), Def::AimTimeRifle,
		TEXT("Time to bring the rifle's sights up times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimTimeShotgun(TEXT("Looter.Gunfeel.Aim.Time.Shotgun"), Def::AimTimeShotgun,
		TEXT("Time to bring the shotgun's sights up times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimTimeRevolver(TEXT("Looter.Gunfeel.Aim.Time.Revolver"), Def::AimTimeRevolver,
		TEXT("Time to bring the revolver's sights up times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimMoveRifle(TEXT("Looter.Gunfeel.Aim.Move.Rifle"), Def::AimMoveRifle,
		TEXT("Walk speed while aiming the rifle times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimMoveShotgun(TEXT("Looter.Gunfeel.Aim.Move.Shotgun"), Def::AimMoveShotgun,
		TEXT("Walk speed while aiming the shotgun times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimMoveRevolver(TEXT("Looter.Gunfeel.Aim.Move.Revolver"), Def::AimMoveRevolver,
		TEXT("Walk speed while aiming the revolver times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwapRifle(TEXT("Looter.Gunfeel.Swap.Rifle"), Def::SwapRifle,
		TEXT("The rifle's holster and draw times times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwapShotgun(TEXT("Looter.Gunfeel.Swap.Shotgun"), Def::SwapShotgun,
		TEXT("The shotgun's holster and draw times times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwapRevolver(TEXT("Looter.Gunfeel.Swap.Revolver"), Def::SwapRevolver,
		TEXT("The revolver's holster and draw times times this."), ECVF_Default);

	/** Picks the variable for Kind (a gun with no kind of its own is the rifle). */
	float ByKind(EWeaponKind Kind, const TAutoConsoleVariable<float>& Rifle, const TAutoConsoleVariable<float>& Shotgun,
		const TAutoConsoleVariable<float>& Revolver)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun: return Shotgun.GetValueOnGameThread();
		case EWeaponKind::Revolver: return Revolver.GetValueOnGameThread();
		default: return Rifle.GetValueOnGameThread();
		}
	}
}

EWeaponKind GunfeelKick::KindOf(const AWeaponBase* Weapon)
{
	const UWeaponDefinition* Definition = Weapon ? Weapon->GetInstance().Definition.Get() : nullptr;
	return Definition ? Definition->Kind : EWeaponKind::Rifle;
}

// ---------------------------------------------------------------------------
// The live numbers
// ---------------------------------------------------------------------------

float GunfeelKick::KickScale(EWeaponKind Kind)
{
	return FMath::Max(ByKind(Kind, CVarKickRifle, CVarKickShotgun, CVarKickRevolver), 0.f);
}

float GunfeelKick::RecoilScale(EWeaponKind Kind)
{
	return FMath::Max(ByKind(Kind, CVarRecoilRifle, CVarRecoilShotgun, CVarRecoilRevolver), 0.f);
}

float GunfeelKick::ShotgunSettle()
{
	return FMath::Max(CVarShotgunSettle.GetValueOnGameThread(), 0.1f);
}

float GunfeelKick::Muzzle1PScale()
{
	return FMath::Max(CVarMuzzle1P.GetValueOnGameThread(), 0.1f);
}

float GunfeelKick::AimTimeScale(EWeaponKind Kind)
{
	return FMath::Max(ByKind(Kind, CVarAimTimeRifle, CVarAimTimeShotgun, CVarAimTimeRevolver), 0.1f);
}

float GunfeelKick::AimMoveScale(EWeaponKind Kind)
{
	return FMath::Max(ByKind(Kind, CVarAimMoveRifle, CVarAimMoveShotgun, CVarAimMoveRevolver), 0.1f);
}

float GunfeelKick::SwapScale(EWeaponKind Kind)
{
	return FMath::Max(ByKind(Kind, CVarSwapRifle, CVarSwapShotgun, CVarSwapRevolver), 0.1f);
}

float GunfeelKick::RifleClimb()
{
	return FMath::Max(CVarRifleClimb.GetValueOnGameThread(), 0.f);
}

float GunfeelKick::RifleClimbMax()
{
	return FMath::Max(CVarRifleClimbMax.GetValueOnGameThread(), 1.f);
}

// ---------------------------------------------------------------------------
// Rules
// ---------------------------------------------------------------------------

float GunfeelKick::CritMultiplier(EWeaponKind Kind, float CurseMultiplier, float RevolverCrit)
{
	const float OfKind = Kind == EWeaponKind::Revolver ? RevolverCrit : LooterCombat::CriticalHitMultiplier;
	return FMath::Max(CurseMultiplier, OfKind);
}

float GunfeelKick::CritMultiplier(EWeaponKind Kind, float CurseMultiplier)
{
	return CritMultiplier(Kind, CurseMultiplier, CVarCritRevolver.GetValueOnGameThread());
}

float GunfeelKick::ShotgunFalloff(float Distance, float Range, float Start, float End, float Floor)
{
	const float From = Range * Start;
	const float To = FMath::Max(Range * End, From + 1.f);
	return FMath::Lerp(1.f, Floor, FMath::Clamp((Distance - From) / (To - From), 0.f, 1.f));
}

float GunfeelKick::DamageShareAtDistance(EWeaponKind Kind, const FWeaponStats& Stats, float Distance)
{
	if (Kind != EWeaponKind::Shotgun)
	{
		return Stats.DamageAtDistance(Distance);
	}
	return ShotgunFalloff(Distance, Stats.Range, CVarFalloffStart.GetValueOnGameThread(), CVarFalloffEnd.GetValueOnGameThread(),
		CVarFalloffFloor.GetValueOnGameThread());
}

float GunfeelKick::LowAmmoPitch(int32 RoundsLeft, int32 Magazine, float Share, float Top)
{
	// Tiny magazines (a six-gun's few rounds are fine, but under 4 it would click through the whole thing) never click.
	if (Magazine < 4 || RoundsLeft <= 0 || RoundsLeft > Magazine)
	{
		return 0.f;
	}
	const int32 LowRounds = FMath::Max(FMath::FloorToInt(static_cast<float>(Magazine) * FMath::Clamp(Share, 0.f, 1.f) + 0.0001f), 1);
	if (RoundsLeft > LowRounds)
	{
		return 0.f;
	}
	// The first click of the run is at pitch 1, the last round's at Top; a run of one is the last round's.
	const float Run = LowRounds > 1 ? static_cast<float>(LowRounds - RoundsLeft) / static_cast<float>(LowRounds - 1) : 1.f;
	return FMath::Lerp(1.f, Top, Run);
}

float GunfeelKick::LowAmmoPitch(int32 RoundsLeft, int32 Magazine)
{
	return LowAmmoPitch(RoundsLeft, Magazine, CVarLowAmmoShare.GetValueOnGameThread(), CVarLowAmmoPitchTop.GetValueOnGameThread());
}

float GunfeelKick::BloomAfterShot(EWeaponKind Kind, float Bloom, float AimAlpha, float PerShot, float MaxMultiplier)
{
	if (Kind != EWeaponKind::Rifle)
	{
		return Bloom;
	}
	const float Share = FMath::Lerp(1.f, 0.25f, FMath::Clamp(AimAlpha, 0.f, 1.f));
	return FMath::Min(Bloom + FMath::Max(PerShot, 0.f) * Share, FMath::Max(MaxMultiplier - 1.f, 0.f));
}

float GunfeelKick::BloomAfterShot(EWeaponKind Kind, float Bloom, float AimAlpha)
{
	return BloomAfterShot(Kind, Bloom, AimAlpha, CVarRifleBloom.GetValueOnGameThread(), CVarBloomMax.GetValueOnGameThread());
}

float GunfeelKick::BloomRecovered(float Bloom, float Seconds, float Recovery)
{
	return Bloom * FMath::Exp(-FMath::Max(Recovery, 0.f) * FMath::Max(Seconds, 0.f));
}

float GunfeelKick::BloomRecovered(float Bloom, float Seconds)
{
	return BloomRecovered(Bloom, Seconds, CVarBloomRecovery.GetValueOnGameThread());
}
