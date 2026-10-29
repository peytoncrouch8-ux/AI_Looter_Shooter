#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/WeaponTypes.h"
#include "HAL/IConsoleManager.h"

// The gun feel pass's kick rules (GF-KICK, 2026-10-22): the recoil pattern, the first shot's reward, the sustained-kick cap
// and the gun model's kick through the sight; since gun identity (2026-10-28) the rifle's S, the SMG's own side random and
// each automatic's own cap (Looter.Gunfeel.Identity 0 gives today's back). Variables are Looter.Gunfeel.*, read each time
// they are asked (live in play); the rules are plain functions over numbers so the tests drive them with the shipped
// defaults (GunfeelKickTuning.h).

namespace
{
	namespace KpDef = GunfeelKick::Default;

	TAutoConsoleVariable<float> CVarKpPattern(TEXT("Looter.Gunfeel.Recoil.Pattern"), KpDef::RecoilPattern,
		TEXT("The recoil pattern's strength (0 off): the rifle's drift, the SMG's shimmy, the LMG's sway."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpSideRandom(TEXT("Looter.Gunfeel.Recoil.SideRandom"), KpDef::SideRandom,
		TEXT("The share of the random sideways aim kick the automatics keep (the pattern replaces the rest)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpRifleDrift(TEXT("Looter.Gunfeel.Recoil.RifleDrift"), KpDef::RifleDrift,
		TEXT("Degrees a rifle shot drifts the aim right, from shot RifleDriftFrom of a burst."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpRifleDriftFrom(TEXT("Looter.Gunfeel.Recoil.RifleDriftFrom"), KpDef::RifleDriftFrom,
		TEXT("The rifle holds straight up for this many shots of a burst before it drifts."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpRifleDriftBack(TEXT("Looter.Gunfeel.Recoil.RifleDriftBack"), KpDef::RifleDriftBack,
		TEXT("Degrees a rifle shot drifts the aim from shot RifleDriftBackFrom on (negative: back left, the S; 0 keeps drifting right)."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarKpRifleDriftBackFrom(TEXT("Looter.Gunfeel.Recoil.RifleDriftBackFrom"), KpDef::RifleDriftBackFrom,
		TEXT("The shot of a burst the rifle's drift turns back from."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpSMGShimmy(TEXT("Looter.Gunfeel.Recoil.SMGShimmy"), KpDef::SMGShimmy,
		TEXT("Degrees each SMG shot nudges the aim, left and right alternating."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpSideRandomSMG(TEXT("Looter.Gunfeel.Recoil.SideRandom.SMG"), KpDef::SideRandomSMG,
		TEXT("The share of the random sideways aim kick the SMG keeps (gun identity; the rifle and LMG keep Recoil.SideRandom)."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarKpLMGSway(TEXT("Looter.Gunfeel.Recoil.LMGSway"), KpDef::LMGSway,
		TEXT("Degrees of the LMG's slow sideways sway per shot (a sine over LMGSwayShots shots)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpLMGSwayShots(TEXT("Looter.Gunfeel.Recoil.LMGSwayShots"), KpDef::LMGSwayShots,
		TEXT("Shots in one sway of the LMG (11 a second makes about 1.5 Hz)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarKpFirstShotKick(TEXT("Looter.Gunfeel.Kick.FirstShot"), KpDef::KickFirstShot,
		TEXT("The view kick of a burst's first shot (rifle, SMG) times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpSustainCap(TEXT("Looter.Gunfeel.Kick.SustainCap"), KpDef::SustainCap,
		TEXT("An automatic's summed shot kicks (view pitch, degrees) are held under this; 0 turns the cap off."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpSustainFloor(TEXT("Looter.Gunfeel.Kick.SustainFloor"), KpDef::SustainFloor,
		TEXT("A shot kick stacked over the cap still keeps this share of its size."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpRack(TEXT("Looter.Gunfeel.Kick.Rack"), KpDef::RackKick,
		TEXT("The nudge the shotgun's pump rack gives the view times this (0 none)."), ECVF_Default);

	TAutoConsoleVariable<float> CVarKpAimedFlip(TEXT("Looter.Gunfeel.Recoil.AimedFlip"), KpDef::AimedFlip,
		TEXT("Through the sight the gun model's muzzle flip and twist are this share of the hip's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpAimedRoll(TEXT("Looter.Gunfeel.Recoil.AimedRoll"), KpDef::AimedRoll,
		TEXT("Through the sight the gun model's roll is this share of the hip's."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpAimedBack(TEXT("Looter.Gunfeel.Recoil.AimedBack"), KpDef::AimedBack,
		TEXT("Through the sight the gun model's push back along the sight line is this share of the hip's."), ECVF_Default);

	TAutoConsoleVariable<float> CVarKpSpreadFirst(TEXT("Looter.Gunfeel.Spread.FirstShot"), KpDef::SpreadFirstShot,
		TEXT("The first shot after a rest (rifle, SMG, LMG) has this share of the spread (1 = no reward)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpSpreadRest(TEXT("Looter.Gunfeel.Spread.FirstShotRest"), KpDef::SpreadFirstShotRest,
		TEXT("Seconds without a shot before the next counts as a first shot."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKpSpreadEase(TEXT("Looter.Gunfeel.Spread.FirstShotEase"), KpDef::SpreadFirstShotEase,
		TEXT("Seconds the cone takes to tighten once the rest is over (the crosshair eases in)."), ECVF_Default);

	/** The kinds that fire bursts and so have a pattern, a first shot's reward and a sustained cap. */
	bool IsAutomatic(EWeaponKind Kind)
	{
		return Kind == EWeaponKind::Rifle || Kind == EWeaponKind::SMG || Kind == EWeaponKind::LMG;
	}
}

float GunfeelKick::PatternSide(EWeaponKind Kind, int32 ShotInBurst, const FPatternNumbers& Numbers)
{
	const int32 Shot = FMath::Max(ShotInBurst, 0);
	switch (Kind)
	{
	case EWeaponKind::Rifle:
		// Straight up for the first shots (the climb is learnable), then a steady drift right to pull against, then (gun
		// identity) back left: an S the hand learns.
		if (Numbers.RifleDriftBack != 0.f && static_cast<float>(Shot) >= Numbers.RifleDriftBackFrom)
		{
			return Numbers.RifleDriftBack;
		}
		return static_cast<float>(Shot) >= Numbers.RifleDriftFrom ? Numbers.RifleDrift : 0.f;
	case EWeaponKind::SMG:
		// A shimmy: one way, the other, so the light gun hums rather than walks.
		return (Shot % 2 == 0 ? 1.f : -1.f) * Numbers.SMGShimmy;
	case EWeaponKind::LMG:
	{
		// A slow sway left and right across the burst: the weight of the belt swinging.
		const float Period = FMath::Max(Numbers.LMGSwayShots, 1.f);
		return Numbers.LMGSway * FMath::Sin(2.f * UE_PI * static_cast<float>(Shot) / Period);
	}
	default:
		return 0.f;
	}
}

float GunfeelKick::PatternSide(EWeaponKind Kind, int32 ShotInBurst)
{
	FPatternNumbers Numbers;
	Numbers.RifleDrift = CVarKpRifleDrift.GetValueOnGameThread();
	Numbers.RifleDriftFrom = CVarKpRifleDriftFrom.GetValueOnGameThread();
	Numbers.LMGSway = CVarKpLMGSway.GetValueOnGameThread();
	Numbers.LMGSwayShots = CVarKpLMGSwayShots.GetValueOnGameThread();
	// Identity 0: today's shimmy and a drift that never turns back.
	const bool bIdentity = IdentityOn();
	Numbers.SMGShimmy = bIdentity ? CVarKpSMGShimmy.GetValueOnGameThread() : KpDef::Today::SMGShimmy;
	Numbers.RifleDriftBack = bIdentity ? CVarKpRifleDriftBack.GetValueOnGameThread() : 0.f;
	Numbers.RifleDriftBackFrom = CVarKpRifleDriftBackFrom.GetValueOnGameThread();
	return PatternSide(Kind, ShotInBurst, Numbers) * FMath::Max(CVarKpPattern.GetValueOnGameThread(), 0.f);
}

float GunfeelKick::PatternLean(EWeaponKind Kind, int32 ShotInBurst, float LMGSwayShots)
{
	if (Kind != EWeaponKind::LMG)
	{
		return 0.f;
	}
	// The sway's own sine, so the shoulders roll the way the belt swings.
	const float Period = FMath::Max(LMGSwayShots, 1.f);
	return FMath::Sin(2.f * UE_PI * static_cast<float>(FMath::Max(ShotInBurst, 0)) / Period);
}

float GunfeelKick::PatternLean(EWeaponKind Kind, int32 ShotInBurst)
{
	return PatternLean(Kind, ShotInBurst, CVarKpLMGSwayShots.GetValueOnGameThread());
}

float GunfeelKick::SideRandomScale(EWeaponKind Kind)
{
	if (!IsAutomatic(Kind))
	{
		return 1.f;
	}
	// Pattern 0 gives the old random back whole; 1 keeps SideRandom of it (the SMG its own since gun identity: the hornet
	// buzzes about, it doesn't walk).
	const float Strength = FMath::Clamp(CVarKpPattern.GetValueOnGameThread(), 0.f, 1.f);
	const float Keep = Kind == EWeaponKind::SMG && IdentityOn() ? CVarKpSideRandomSMG.GetValueOnGameThread() : CVarKpSideRandom.GetValueOnGameThread();
	return FMath::Lerp(1.f, FMath::Max(Keep, 0.f), Strength);
}

float GunfeelKick::FirstShotKick(EWeaponKind Kind, int32 ShotInBurst, float FirstShot)
{
	const bool bFirst = ShotInBurst <= 0 && (Kind == EWeaponKind::Rifle || Kind == EWeaponKind::SMG);
	return bFirst ? FMath::Max(FirstShot, 0.f) : 1.f;
}

float GunfeelKick::FirstShotKick(EWeaponKind Kind, int32 ShotInBurst)
{
	return FirstShotKick(Kind, ShotInBurst, CVarKpFirstShotKick.GetValueOnGameThread());
}

float GunfeelKick::SustainCapFor(EWeaponKind Kind)
{
	if (!IsAutomatic(Kind))
	{
		return 0.f;
	}
	// Each automatic its own since gun identity: the SMG's hum held low, the LMG's chug allowed to dig in.
	return FMath::Max(IdentityOn() ? KindFeel(Kind).SustainCap : CVarKpSustainCap.GetValueOnGameThread(), 0.f);
}

float GunfeelKick::SustainFloor()
{
	return FMath::Clamp(CVarKpSustainFloor.GetValueOnGameThread(), 0.f, 1.f);
}

float GunfeelKick::RackKickScale()
{
	return FMath::Max(CVarKpRack.GetValueOnGameThread(), 0.f);
}

void GunfeelKick::AimedModelKick(float AimAlpha, float Back, float Flip, float Roll, FVector& BackFlipRoll)
{
	const float Alpha = FMath::Clamp(AimAlpha, 0.f, 1.f);
	BackFlipRoll = FVector(FMath::Lerp(1.f, FMath::Max(Back, 0.f), Alpha), FMath::Lerp(1.f, FMath::Max(Flip, 0.f), Alpha),
		FMath::Lerp(1.f, FMath::Max(Roll, 0.f), Alpha));
}

void GunfeelKick::AimedModelKick(float AimAlpha, FVector& BackFlipRoll)
{
	AimedModelKick(AimAlpha, CVarKpAimedBack.GetValueOnGameThread(), CVarKpAimedFlip.GetValueOnGameThread(),
		CVarKpAimedRoll.GetValueOnGameThread(), BackFlipRoll);
}

float GunfeelKick::FirstShotBloom(EWeaponKind Kind, float Bloom, float SecondsSinceShot, float Rest, float Ease, float FirstShot)
{
	if (!IsAutomatic(Kind) || FirstShot >= 1.f)
	{
		return Bloom;
	}
	// Rested: the cone eases from wherever the bloom has closed to the first shot's tighter one (a smooth step, so the
	// crosshair breathes in instead of jumping).
	const float Into = (SecondsSinceShot - FMath::Max(Rest, 0.f)) / FMath::Max(Ease, 0.01f);
	const float Blend = FMath::SmoothStep(0.f, 1.f, Into);
	return FMath::Lerp(Bloom, FMath::Max(FirstShot, 0.f) - 1.f, Blend);
}

float GunfeelKick::FirstShotBloom(EWeaponKind Kind, float Bloom, float SecondsSinceShot)
{
	return FirstShotBloom(Kind, Bloom, SecondsSinceShot, CVarKpSpreadRest.GetValueOnGameThread(), CVarKpSpreadEase.GetValueOnGameThread(),
		CVarKpSpreadFirst.GetValueOnGameThread());
}
