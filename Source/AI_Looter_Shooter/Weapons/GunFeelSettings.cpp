#include "Weapons/GunFeelSettings.h"

FGunKindFeelSettings::FGunKindFeelSettings(const GunfeelKick::FKindFeel& Feel)
	: AimedTurn(Feel.AimedTurn)
	, AimedFieldOfView(Feel.AimedFieldOfView)
	, AimedBack(Feel.AimedBack)
	, AimedFlip(Feel.AimedFlip)
	, AimedRoll(Feel.AimedRoll)
	, AimKick(Feel.AimKick)
	, Recovery(Feel.Recovery)
	, Cadence(Feel.Cadence)
	, AimCurve(Feel.AimCurve)
	, AimSettle(Feel.AimSettle)
	, SustainCap(Feel.SustainCap)
	, Shake(Feel.Shake)
	, ShakeSeconds(Feel.ShakeSeconds)
{
}

GunfeelKick::FKindFeel FGunKindFeelSettings::ToFeel() const
{
	GunfeelKick::FKindFeel Feel;
	Feel.AimedTurn = AimedTurn;
	Feel.AimedFieldOfView = AimedFieldOfView;
	Feel.AimedBack = AimedBack;
	Feel.AimedFlip = AimedFlip;
	Feel.AimedRoll = AimedRoll;
	Feel.AimKick = AimKick;
	Feel.Recovery = Recovery;
	Feel.Cadence = Cadence;
	Feel.AimCurve = AimCurve;
	Feel.AimSettle = AimSettle;
	Feel.SustainCap = SustainCap;
	Feel.Shake = Shake;
	Feel.ShakeSeconds = ShakeSeconds;
	return Feel;
}

UGunFeelSettings::UGunFeelSettings()
	: Rifle(GunfeelKick::KindFeelDefault(EWeaponKind::Rifle))
	, Shotgun(GunfeelKick::KindFeelDefault(EWeaponKind::Shotgun))
	, Revolver(GunfeelKick::KindFeelDefault(EWeaponKind::Revolver))
	, SMG(GunfeelKick::KindFeelDefault(EWeaponKind::SMG))
	, Sniper(GunfeelKick::KindFeelDefault(EWeaponKind::Sniper))
	, LMG(GunfeelKick::KindFeelDefault(EWeaponKind::LMG))
	, Launcher(GunfeelKick::KindFeelDefault(EWeaponKind::Launcher))
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("Gun Feel");
}

const UGunFeelSettings& UGunFeelSettings::Get()
{
	return *GetDefault<UGunFeelSettings>();
}

const FGunKindFeelSettings& UGunFeelSettings::For(EWeaponKind Kind) const
{
	switch (Kind)
	{
	case EWeaponKind::Shotgun: return Shotgun;
	case EWeaponKind::Revolver: return Revolver;
	case EWeaponKind::SMG: return SMG;
	case EWeaponKind::Sniper: return Sniper;
	case EWeaponKind::LMG: return LMG;
	case EWeaponKind::Launcher: return Launcher;
	case EWeaponKind::Rifle:
	case EWeaponKind::None:
	default: return Rifle;
	}
}

FGunKindFeelSettings& UGunFeelSettings::For(EWeaponKind Kind)
{
	return const_cast<FGunKindFeelSettings&>(static_cast<const UGunFeelSettings*>(this)->For(Kind));
}
