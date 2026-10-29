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

	TAutoConsoleVariable<float> CVarRecoilMore(TEXT("Looter.Gunfeel.Recoil.More"), Def::RecoilMore,
		TEXT("Every gun's kick (aim, model, view, pattern) times this, before the accuracy's calm."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAccuracyCalm(TEXT("Looter.Gunfeel.Recoil.AccuracyCalm"), Def::AccuracyCalm,
		TEXT("A gun shooting R times tighter than its kind's base kicks R^-this as hard (0: accuracy leaves the kick alone)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAccuracyCalmLeast(TEXT("Looter.Gunfeel.Recoil.AccuracyCalmLeast"), Def::AccuracyCalmLeast,
		TEXT("...but never under this share of the kick (the most accurate guns)..."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAccuracyCalmMost(TEXT("Looter.Gunfeel.Recoil.AccuracyCalmMost"), Def::AccuracyCalmMost,
		TEXT("...nor over this many times it (the sloppiest rolls)."), ECVF_Default);

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

	// The four frontier-tech kinds (Phase 12.3), beside the three above.
	TAutoConsoleVariable<float> CVarKickSMG(TEXT("Looter.Gunfeel.Kick.SMG"), Def::KickSMG,
		TEXT("The SMG's view kick times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKickSniper(TEXT("Looter.Gunfeel.Kick.Sniper"), Def::KickSniper,
		TEXT("The sniper rifle's view kick times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKickLMG(TEXT("Looter.Gunfeel.Kick.LMG"), Def::KickLMG,
		TEXT("The LMG's view kick times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKickLauncher(TEXT("Looter.Gunfeel.Kick.Launcher"), Def::KickLauncher,
		TEXT("The launcher's view kick times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRecoilSMG(TEXT("Looter.Gunfeel.Recoil.SMG"), Def::RecoilSMG,
		TEXT("The SMG's own kick times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRecoilSniper(TEXT("Looter.Gunfeel.Recoil.Sniper"), Def::RecoilSniper,
		TEXT("The sniper rifle's own kick times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRecoilLMG(TEXT("Looter.Gunfeel.Recoil.LMG"), Def::RecoilLMG,
		TEXT("The LMG's own kick times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRecoilLauncher(TEXT("Looter.Gunfeel.Recoil.Launcher"), Def::RecoilLauncher,
		TEXT("The launcher's own kick times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimTimeSMG(TEXT("Looter.Gunfeel.Aim.Time.SMG"), Def::AimTimeSMG,
		TEXT("Time to bring the SMG's sights up times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimTimeSniper(TEXT("Looter.Gunfeel.Aim.Time.Sniper"), Def::AimTimeSniper,
		TEXT("Time to bring the sniper rifle's scope up times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimTimeLMG(TEXT("Looter.Gunfeel.Aim.Time.LMG"), Def::AimTimeLMG,
		TEXT("Time to bring the LMG's sights up times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimTimeLauncher(TEXT("Looter.Gunfeel.Aim.Time.Launcher"), Def::AimTimeLauncher,
		TEXT("Time to bring the launcher's sight up times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimMoveSMG(TEXT("Looter.Gunfeel.Aim.Move.SMG"), Def::AimMoveSMG,
		TEXT("Walk speed while aiming the SMG times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimMoveSniper(TEXT("Looter.Gunfeel.Aim.Move.Sniper"), Def::AimMoveSniper,
		TEXT("Walk speed while aiming the sniper rifle times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimMoveLMG(TEXT("Looter.Gunfeel.Aim.Move.LMG"), Def::AimMoveLMG,
		TEXT("Walk speed while aiming the LMG times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimMoveLauncher(TEXT("Looter.Gunfeel.Aim.Move.Launcher"), Def::AimMoveLauncher,
		TEXT("Walk speed while aiming the launcher times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwapSMG(TEXT("Looter.Gunfeel.Swap.SMG"), Def::SwapSMG,
		TEXT("The SMG's holster and draw times times this (the quickest)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwapSniper(TEXT("Looter.Gunfeel.Swap.Sniper"), Def::SwapSniper,
		TEXT("The sniper rifle's holster and draw times times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwapLMG(TEXT("Looter.Gunfeel.Swap.LMG"), Def::SwapLMG,
		TEXT("The LMG's holster and draw times times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwapLauncher(TEXT("Looter.Gunfeel.Swap.Launcher"), Def::SwapLauncher,
		TEXT("The launcher's holster and draw times times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarCritSniper(TEXT("Looter.Gunfeel.Crit.Sniper"), Def::CritSniper,
		TEXT("What a sniper rifle's critical hit multiplies the damage by (a cursed iron's own wins when larger)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSMGClimb(TEXT("Looter.Gunfeel.Recoil.SMGClimb"), Def::SMGClimb,
		TEXT("The SMG's aim kick grows by this share for each shot of a burst (to RifleClimbMax)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLMGClimb(TEXT("Looter.Gunfeel.Recoil.LMGClimb"), Def::LMGClimb,
		TEXT("The LMG's aim kick grows by this share for each shot of a burst (to RifleClimbMax)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSMGBloom(TEXT("Looter.Gunfeel.Spread.SMGBloom"), Def::SMGBloom,
		TEXT("Each SMG shot widens its spread by this share of the gun's own (a quarter as much through the sight)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLMGBloom(TEXT("Looter.Gunfeel.Spread.LMGBloom"), Def::LMGBloom,
		TEXT("Each LMG shot widens its spread by this share of the gun's own (a quarter as much through the sight)."), ECVF_Default);

	/** One number's variable for every kind (a gun with no kind of its own is the rifle). */
	struct FByKind
	{
		const TAutoConsoleVariable<float>& Rifle;
		const TAutoConsoleVariable<float>& Shotgun;
		const TAutoConsoleVariable<float>& Revolver;
		const TAutoConsoleVariable<float>& SMG;
		const TAutoConsoleVariable<float>& Sniper;
		const TAutoConsoleVariable<float>& LMG;
		const TAutoConsoleVariable<float>& Launcher;

		float Get(EWeaponKind Kind) const
		{
			switch (Kind)
			{
			case EWeaponKind::Shotgun: return Shotgun.GetValueOnGameThread();
			case EWeaponKind::Revolver: return Revolver.GetValueOnGameThread();
			case EWeaponKind::SMG: return SMG.GetValueOnGameThread();
			case EWeaponKind::Sniper: return Sniper.GetValueOnGameThread();
			case EWeaponKind::LMG: return LMG.GetValueOnGameThread();
			case EWeaponKind::Launcher: return Launcher.GetValueOnGameThread();
			case EWeaponKind::Rifle:
			case EWeaponKind::None:
			default: return Rifle.GetValueOnGameThread();
			}
		}
	};

	const FByKind Kicks{ CVarKickRifle, CVarKickShotgun, CVarKickRevolver, CVarKickSMG, CVarKickSniper, CVarKickLMG, CVarKickLauncher };
	const FByKind Recoils{ CVarRecoilRifle, CVarRecoilShotgun, CVarRecoilRevolver, CVarRecoilSMG, CVarRecoilSniper, CVarRecoilLMG,
		CVarRecoilLauncher };
	const FByKind AimTimes{ CVarAimTimeRifle, CVarAimTimeShotgun, CVarAimTimeRevolver, CVarAimTimeSMG, CVarAimTimeSniper, CVarAimTimeLMG,
		CVarAimTimeLauncher };
	const FByKind AimMoves{ CVarAimMoveRifle, CVarAimMoveShotgun, CVarAimMoveRevolver, CVarAimMoveSMG, CVarAimMoveSniper, CVarAimMoveLMG,
		CVarAimMoveLauncher };
	const FByKind Swaps{ CVarSwapRifle, CVarSwapShotgun, CVarSwapRevolver, CVarSwapSMG, CVarSwapSniper, CVarSwapLMG, CVarSwapLauncher };
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
	return FMath::Max(Kicks.Get(Kind), 0.f);
}

float GunfeelKick::RecoilScale(EWeaponKind Kind)
{
	return FMath::Max(Recoils.Get(Kind), 0.f);
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
	return FMath::Max(AimTimes.Get(Kind), 0.1f);
}

float GunfeelKick::AimMoveScale(EWeaponKind Kind)
{
	return FMath::Max(AimMoves.Get(Kind), 0.1f);
}

float GunfeelKick::SwapScale(EWeaponKind Kind)
{
	return FMath::Max(Swaps.Get(Kind), 0.1f);
}

float GunfeelKick::RifleClimb()
{
	return FMath::Max(CVarRifleClimb.GetValueOnGameThread(), 0.f);
}

float GunfeelKick::RifleClimbMax()
{
	return FMath::Max(CVarRifleClimbMax.GetValueOnGameThread(), 1.f);
}

float GunfeelKick::ClimbPerShot(EWeaponKind Kind)
{
	switch (Kind)
	{
	case EWeaponKind::Rifle: return RifleClimb();
	case EWeaponKind::SMG: return FMath::Max(CVarSMGClimb.GetValueOnGameThread(), 0.f);
	case EWeaponKind::LMG: return FMath::Max(CVarLMGClimb.GetValueOnGameThread(), 0.f);
	default: return 0.f;
	}
}

// ---------------------------------------------------------------------------
// Recoil by accuracy
// ---------------------------------------------------------------------------

GunfeelKick::FAccuracyKick GunfeelKick::AccuracyKickNumbers()
{
	FAccuracyKick Numbers;
	Numbers.More = FMath::Max(CVarRecoilMore.GetValueOnGameThread(), 0.f);
	Numbers.Exponent = FMath::Max(CVarAccuracyCalm.GetValueOnGameThread(), 0.f);
	Numbers.Least = FMath::Max(CVarAccuracyCalmLeast.GetValueOnGameThread(), 0.f);
	Numbers.Most = FMath::Max(CVarAccuracyCalmMost.GetValueOnGameThread(), Numbers.Least);
	return Numbers;
}

float GunfeelKick::AccuracyCalm(float BaseSpread, float Spread, const FAccuracyKick& Numbers)
{
	if (BaseSpread <= 0.f || Spread <= 0.f)
	{
		return 1.f;
	}
	// Measured against the gun's own kind, so a shotgun's wide cone isn't a "bad" stat; the gun's rarity, parts, maker and
	// curse all move it. Held both ways: a pinpoint gun still kicks, a sloppy one never throws the view off the screen.
	const float Ratio = BaseSpread / Spread;
	const float Least = FMath::Max(Numbers.Least, 0.f);
	return FMath::Clamp(FMath::Pow(Ratio, -FMath::Max(Numbers.Exponent, 0.f)), Least, FMath::Max(Numbers.Most, Least));
}

float GunfeelKick::ShotRecoil(float RecoilStat, float BaseSpread, float Spread, const FAccuracyKick& Numbers)
{
	return RecoilStat * FMath::Max(Numbers.More, 0.f) * AccuracyCalm(BaseSpread, Spread, Numbers);
}

float GunfeelKick::ShotRecoil(const UWeaponDefinition* Definition, const FWeaponStats& Stats)
{
	return ShotRecoil(Stats.Recoil, Definition ? Definition->BaseStats.Spread : 0.f, Stats.Spread, AccuracyKickNumbers());
}

float GunfeelKick::ShotRecoil(const AWeaponBase* Weapon)
{
	return Weapon ? ShotRecoil(Weapon->GetInstance().Definition.Get(), Weapon->GetStats()) : 0.f;
}

float GunfeelKick::FeltRecoil(const UWeaponDefinition* Definition, const FWeaponStats& Stats)
{
	return Stats.Recoil * AccuracyCalm(Definition ? Definition->BaseStats.Spread : 0.f, Stats.Spread, AccuracyKickNumbers());
}

// ---------------------------------------------------------------------------
// Rules
// ---------------------------------------------------------------------------

float GunfeelKick::CritMultiplier(EWeaponKind Kind, float CurseMultiplier, float RevolverCrit, float SniperCrit)
{
	float OfKind = LooterCombat::CriticalHitMultiplier;
	switch (Kind)
	{
	case EWeaponKind::Revolver: OfKind = RevolverCrit; break;
	case EWeaponKind::Sniper: OfKind = SniperCrit; break;
	default: break;
	}
	return FMath::Max(CurseMultiplier, OfKind);
}

float GunfeelKick::CritMultiplier(EWeaponKind Kind, float CurseMultiplier, float RevolverCrit)
{
	return CritMultiplier(Kind, CurseMultiplier, RevolverCrit, Default::CritSniper);
}

float GunfeelKick::CritMultiplier(EWeaponKind Kind, float CurseMultiplier)
{
	return CritMultiplier(Kind, CurseMultiplier, CVarCritRevolver.GetValueOnGameThread(), CVarCritSniper.GetValueOnGameThread());
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
	// The automatic guns open up as they're held down; a single heavy shot (shotgun, six-gun, bolt, rocket) never does.
	if (Kind != EWeaponKind::Rifle && Kind != EWeaponKind::SMG && Kind != EWeaponKind::LMG)
	{
		return Bloom;
	}
	const float Share = FMath::Lerp(1.f, 0.25f, FMath::Clamp(AimAlpha, 0.f, 1.f));
	return FMath::Min(Bloom + FMath::Max(PerShot, 0.f) * Share, FMath::Max(MaxMultiplier - 1.f, 0.f));
}

float GunfeelKick::BloomAfterShot(EWeaponKind Kind, float Bloom, float AimAlpha)
{
	// The SMG's cone opens wider since gun identity (hosed, it spreads rather than climbs); Identity 0 gives today's back.
	const float SMGPerShot = IdentityOn() ? CVarSMGBloom.GetValueOnGameThread() : Default::Today::SMGBloom;
	const float PerShot = Kind == EWeaponKind::SMG ? SMGPerShot
		: (Kind == EWeaponKind::LMG ? CVarLMGBloom.GetValueOnGameThread() : CVarRifleBloom.GetValueOnGameThread());
	return BloomAfterShot(Kind, Bloom, AimAlpha, PerShot, CVarBloomMax.GetValueOnGameThread());
}

float GunfeelKick::BloomRecovered(float Bloom, float Seconds, float Recovery)
{
	return Bloom * FMath::Exp(-FMath::Max(Recovery, 0.f) * FMath::Max(Seconds, 0.f));
}

float GunfeelKick::BloomRecovered(float Bloom, float Seconds)
{
	return BloomRecovered(Bloom, Seconds, CVarBloomRecovery.GetValueOnGameThread());
}
