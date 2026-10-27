#include "Weapons/GunKindsTuning.h"
#include "HAL/IConsoleManager.h"

// The Looter.Gunfeel.* variables for the four frontier-tech guns' own ways (GunKindsTuning.h), and the plain rules that
// read them. Each accessor reads its variable when asked, never caching it, so a change in play shows at once.

namespace
{
	namespace Def = GunKinds::Default;

	TAutoConsoleVariable<float> CVarSMGGlow(TEXT("Looter.Gunfeel.SMG.Glow"), Def::SMGGlow,
		TEXT("How bright the SMG's helix glows with its rounds."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSMGGlowLow(TEXT("Looter.Gunfeel.SMG.GlowLowShare"), Def::SMGGlowLowShare,
		TEXT("Under this share of the magazine the helix pulses (the last rounds)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSMGGlowPulse(TEXT("Looter.Gunfeel.SMG.GlowPulse"), Def::SMGGlowPulse,
		TEXT("...this many times a second."), ECVF_Default);

	TAutoConsoleVariable<float> CVarBoltDelay(TEXT("Looter.Gunfeel.Sniper.BoltDelay"), Def::SniperBoltDelay,
		TEXT("Seconds after a sniper shot before the bolt is worked (the kick first)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBoltShare(TEXT("Looter.Gunfeel.Sniper.BoltShare"), Def::SniperBoltShare,
		TEXT("The bolt's cycle takes this share of what's left of the time between shots..."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBoltMax(TEXT("Looter.Gunfeel.Sniper.BoltMaxSeconds"), Def::SniperBoltMaxSeconds,
		TEXT("...at most this long (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimedSpread(TEXT("Looter.Gunfeel.Sniper.AimedSpread"), Def::SniperAimedSpread,
		TEXT("The sniper's spread through the scope as a share of its hip spread (no halving at the hip)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSway(TEXT("Looter.Gunfeel.Sniper.Sway"), Def::SniperSway,
		TEXT("The scope's sway while aimed (degrees at its widest)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwayRate(TEXT("Looter.Gunfeel.Sniper.SwayRate"), Def::SniperSwayRate,
		TEXT("The sway's pace (Hz)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBreathSeconds(TEXT("Looter.Gunfeel.Sniper.BreathSeconds"), Def::SniperBreathSeconds,
		TEXT("How long a breath can be held through the scope (the sprint key while aimed)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBreathSteady(TEXT("Looter.Gunfeel.Sniper.BreathSteady"), Def::SniperBreathSteady,
		TEXT("The sway's share while the breath is held."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWindedSeconds(TEXT("Looter.Gunfeel.Sniper.WindedSeconds"), Def::SniperWindedSeconds,
		TEXT("A breath held to the end leaves the shooter winded this long..."), ECVF_Default);
	TAutoConsoleVariable<float> CVarWindedSway(TEXT("Looter.Gunfeel.Sniper.WindedSway"), Def::SniperWindedSway,
		TEXT("...the sway this many times wider meanwhile."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBreathRefill(TEXT("Looter.Gunfeel.Sniper.BreathRefill"), Def::SniperBreathRefill,
		TEXT("Seconds of held breath back for each second of breathing."), ECVF_Default);

	TAutoConsoleVariable<float> CVarSwayRifle(TEXT("Looter.Gunfeel.Sway.Rifle"), Def::SwayRifle,
		TEXT("The rifle's idle sway through its sights (degrees at its widest; 0 none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwayShotgun(TEXT("Looter.Gunfeel.Sway.Shotgun"), Def::SwayShotgun,
		TEXT("The shotgun's idle sway through its sights (degrees)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwayRevolver(TEXT("Looter.Gunfeel.Sway.Revolver"), Def::SwayRevolver,
		TEXT("The revolver's idle sway through its sights (degrees)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwaySMG(TEXT("Looter.Gunfeel.Sway.SMG"), Def::SwaySMG,
		TEXT("The SMG's idle sway through its sights (degrees)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwayLMG(TEXT("Looter.Gunfeel.Sway.LMG"), Def::SwayLMG,
		TEXT("The LMG's idle sway through its sights (degrees)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwayLauncher(TEXT("Looter.Gunfeel.Sway.Launcher"), Def::SwayLauncher,
		TEXT("The launcher's idle sway through its sight (degrees)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarAimSwayRate(TEXT("Looter.Gunfeel.Sway.Rate"), Def::SwayRate,
		TEXT("Their sways' pace (Hz); the sniper's scope keeps Sniper.SwayRate."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSwayCrouch(TEXT("Looter.Gunfeel.Sway.Crouch"), Def::SwayCrouch,
		TEXT("Crouched, every gun's sway (the scope's too) is this share of it."), ECVF_Default);

	TAutoConsoleVariable<float> CVarSpinUp(TEXT("Looter.Gunfeel.LMG.SpinUp"), Def::LMGSpinUp,
		TEXT("Seconds of firing for the LMG to spin up to its full rate."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSpinStart(TEXT("Looter.Gunfeel.LMG.SpinStart"), Def::LMGSpinStart,
		TEXT("The LMG's rate from cold, as a share of its full rate."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSpinDown(TEXT("Looter.Gunfeel.LMG.SpinDown"), Def::LMGSpinDown,
		TEXT("Seconds off the trigger for it to spin all the way down."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHeatPerShot(TEXT("Looter.Gunfeel.LMG.HeatPerShot"), Def::LMGHeatPerShot,
		TEXT("Each LMG shot heats its barrel by this (1 glowing its hottest)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHeatDelay(TEXT("Looter.Gunfeel.LMG.HeatDelay"), Def::LMGHeatDelay,
		TEXT("Seconds after the last shot the barrel holds its heat..."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHeatCool(TEXT("Looter.Gunfeel.LMG.HeatCool"), Def::LMGHeatCool,
		TEXT("...then it cools by this much a second."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHeatGlow(TEXT("Looter.Gunfeel.LMG.HeatGlow"), Def::LMGHeatGlow,
		TEXT("How bright the LMG's heat rings glow at their hottest."), ECVF_Default);

	TAutoConsoleVariable<float> CVarRocketSpeed(TEXT("Looter.Gunfeel.Launcher.RocketSpeed"), Def::RocketSpeed,
		TEXT("The rocket's speed (cm/s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRocketGravity(TEXT("Looter.Gunfeel.Launcher.RocketGravity"), Def::RocketGravity,
		TEXT("How fast the rocket's flight drops (cm/s per second): a slight arc over a long shot."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRocketLife(TEXT("Looter.Gunfeel.Launcher.RocketLife"), Def::RocketLife,
		TEXT("Seconds a rocket flies before it bursts on its own."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBlastRadius(TEXT("Looter.Gunfeel.Launcher.BlastRadius"), Def::BlastRadius,
		TEXT("How far a rocket's burst reaches (cm, to a body's nearest side)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBlastCore(TEXT("Looter.Gunfeel.Launcher.BlastCore"), Def::BlastCore,
		TEXT("Within this of its middle (cm) the burst does full damage..."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBlastEdge(TEXT("Looter.Gunfeel.Launcher.BlastEdgeShare"), Def::BlastEdgeShare,
		TEXT("...falling to this share of it at its edge."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBlastSelf(TEXT("Looter.Gunfeel.Launcher.SelfScale"), Def::BlastSelfScale,
		TEXT("The share of its burst a rocket's own shooter takes."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBlastSelfCap(TEXT("Looter.Gunfeel.Launcher.SelfCap"), Def::BlastSelfCap,
		TEXT("...at most this share of their own health from one rocket."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBlastKnock(TEXT("Looter.Gunfeel.Launcher.KnockShare"), Def::BlastKnockShare,
		TEXT("The living within this share of the burst's reach are thrown out of it."), ECVF_Default);

	TAutoConsoleVariable<float> CVarKillSniperBurst(TEXT("Looter.Gunfeel.Kill.SniperBurst"), Def::KillSniperBurst,
		TEXT("A sniper crit kill's death burst is the revolver's (Kill.RevolverBurst) times this."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKillLauncherKnock(TEXT("Looter.Gunfeel.Kill.LauncherKnock"), Def::KillLauncherKnock,
		TEXT("A rocket kill throws the corpse away from the burst this fast (cm/s)..."), ECVF_Default);
	TAutoConsoleVariable<float> CVarKillLauncherLift(TEXT("Looter.Gunfeel.Kill.LauncherLift"), Def::KillLauncherLift,
		TEXT("...with this much lift (cm)."), ECVF_Default);

	float Positive(const TAutoConsoleVariable<float>& Variable, float Least = 0.f)
	{
		return FMath::Max(Variable.GetValueOnGameThread(), Least);
	}
}

float GunKinds::SMGGlow() { return Positive(CVarSMGGlow); }
float GunKinds::SMGGlowLowShare() { return FMath::Clamp(CVarSMGGlowLow.GetValueOnGameThread(), 0.f, 1.f); }
float GunKinds::SMGGlowPulse() { return Positive(CVarSMGGlowPulse); }
float GunKinds::SniperBoltDelay() { return Positive(CVarBoltDelay); }
float GunKinds::SniperBoltShare() { return FMath::Clamp(CVarBoltShare.GetValueOnGameThread(), 0.1f, 1.f); }
float GunKinds::SniperBoltMaxSeconds() { return Positive(CVarBoltMax, 0.1f); }
float GunKinds::SniperAimedSpread() { return Positive(CVarAimedSpread); }
float GunKinds::SniperSway() { return Positive(CVarSway); }
float GunKinds::SniperSwayRate() { return Positive(CVarSwayRate, 0.01f); }

float GunKinds::AimSway(EWeaponKind Kind)
{
	switch (Kind)
	{
	case EWeaponKind::Sniper: return SniperSway();
	case EWeaponKind::Shotgun: return Positive(CVarSwayShotgun);
	case EWeaponKind::Revolver: return Positive(CVarSwayRevolver);
	case EWeaponKind::SMG: return Positive(CVarSwaySMG);
	case EWeaponKind::LMG: return Positive(CVarSwayLMG);
	case EWeaponKind::Launcher: return Positive(CVarSwayLauncher);
	case EWeaponKind::Rifle:
	case EWeaponKind::None:
	default: return Positive(CVarSwayRifle);
	}
}

float GunKinds::AimSwayRate(EWeaponKind Kind)
{
	return Kind == EWeaponKind::Sniper ? SniperSwayRate() : Positive(CVarAimSwayRate, 0.01f);
}

float GunKinds::SwayCrouch() { return Positive(CVarSwayCrouch); }

GunKinds::FBreathRules GunKinds::BreathRules()
{
	FBreathRules Rules;
	Rules.HoldSeconds = Positive(CVarBreathSeconds, 0.1f);
	Rules.Steady = Positive(CVarBreathSteady);
	Rules.WindedSeconds = Positive(CVarWindedSeconds);
	Rules.WindedSway = Positive(CVarWindedSway);
	Rules.Refill = Positive(CVarBreathRefill, 0.01f);
	return Rules;
}

float GunKinds::LMGSpinUp() { return Positive(CVarSpinUp, 0.01f); }
float GunKinds::LMGSpinStart() { return FMath::Clamp(CVarSpinStart.GetValueOnGameThread(), 0.1f, 1.f); }
float GunKinds::LMGSpinDown() { return Positive(CVarSpinDown, 0.01f); }
float GunKinds::LMGHeatPerShot() { return Positive(CVarHeatPerShot); }
float GunKinds::LMGHeatDelay() { return Positive(CVarHeatDelay); }
float GunKinds::LMGHeatCool() { return Positive(CVarHeatCool); }
float GunKinds::LMGHeatGlow() { return Positive(CVarHeatGlow); }
float GunKinds::RocketSpeed() { return Positive(CVarRocketSpeed, 100.f); }
float GunKinds::RocketGravity() { return Positive(CVarRocketGravity); }
float GunKinds::RocketLife() { return Positive(CVarRocketLife, 0.1f); }

GunKinds::FBlastRules GunKinds::BlastRules()
{
	FBlastRules Rules;
	Rules.Radius = Positive(CVarBlastRadius, 1.f);
	Rules.Core = FMath::Clamp(CVarBlastCore.GetValueOnGameThread(), 0.f, Rules.Radius);
	Rules.EdgeShare = FMath::Clamp(CVarBlastEdge.GetValueOnGameThread(), 0.f, 1.f);
	Rules.SelfScale = Positive(CVarBlastSelf);
	Rules.SelfCap = FMath::Clamp(CVarBlastSelfCap.GetValueOnGameThread(), 0.f, 1.f);
	Rules.KnockShare = FMath::Clamp(CVarBlastKnock.GetValueOnGameThread(), 0.f, 1.f);
	return Rules;
}

float GunKinds::KillSniperBurst() { return Positive(CVarKillSniperBurst, 0.1f); }
float GunKinds::KillLauncherKnock() { return Positive(CVarKillLauncherKnock); }
float GunKinds::KillLauncherLift() { return Positive(CVarKillLauncherLift); }

// ---------------------------------------------------------------------------
// Rules
// ---------------------------------------------------------------------------

float GunKinds::AmmoGlowFill(int32 Rounds, int32 Magazine)
{
	return Magazine > 0 ? FMath::Clamp(static_cast<float>(Rounds) / static_cast<float>(Magazine), 0.f, 1.f) : 0.f;
}

float GunKinds::AmmoGlowStrength(float Fill, float Base, float LowShare, float PulseHz, float Time)
{
	if (Fill <= 0.f)
	{
		return 0.f;
	}
	if (Fill > LowShare)
	{
		return Base;
	}
	// Nearly dry: a quick throb, the light coming and going at the feed end.
	return Base * (1.f + 0.5f * FMath::Sin(2.f * UE_PI * PulseHz * Time));
}

float GunKinds::SpinRateShare(float Spin, float Start)
{
	return FMath::Lerp(FMath::Clamp(Start, 0.01f, 1.f), 1.f, FMath::Clamp(Spin, 0.f, 1.f));
}

float GunKinds::SpinAfter(float Spin, float Seconds, bool bFiring, float UpSeconds, float DownSeconds)
{
	const float Step = FMath::Max(Seconds, 0.f) / FMath::Max(bFiring ? UpSeconds : DownSeconds, 0.01f);
	return FMath::Clamp(Spin + (bFiring ? Step : -Step), 0.f, 1.f);
}

float GunKinds::HeatAfterShot(float Heat, float PerShot)
{
	return FMath::Clamp(Heat + FMath::Max(PerShot, 0.f), 0.f, 1.f);
}

float GunKinds::HeatCooled(float Heat, float SinceShot, float Delay, float Cool)
{
	const float Cooling = FMath::Max(SinceShot - Delay, 0.f);
	return FMath::Clamp(Heat - Cooling * FMath::Max(Cool, 0.f), 0.f, 1.f);
}

GunKinds::FBoltTiming GunKinds::BoltTiming(float Interval, float Delay, float Share, float MaxSeconds)
{
	FBoltTiming Timing;
	// Never starting past the time between shots, and always home a little before the next can go.
	Timing.Start = FMath::Clamp(Delay, 0.f, Interval * 0.5f);
	const float Room = FMath::Max(Interval - Timing.Start, 0.f);
	Timing.Seconds = FMath::Min(Room * FMath::Clamp(Share, 0.f, 1.f), FMath::Max(MaxSeconds, 0.f));
	return Timing;
}

float GunKinds::BoltCycleProgress(float SinceShot, const FBoltTiming& Timing)
{
	if (Timing.Seconds <= 0.f || SinceShot < Timing.Start || SinceShot >= Timing.Start + Timing.Seconds)
	{
		return -1.f;
	}
	return (SinceShot - Timing.Start) / Timing.Seconds;
}

FRotator GunKinds::SwayAt(float Time, float Amplitude, float Rate)
{
	const float Phase = 2.f * UE_PI * Rate * Time;
	// A figure of eight: across once while it bobs up and down twice; a slower drift on top keeps it from looking machined.
	const float Yaw = Amplitude * (0.85f * FMath::Sin(Phase) + 0.15f * FMath::Sin(Phase * 0.37f + 1.3f));
	const float Pitch = Amplitude * 0.67f * (0.85f * FMath::Sin(2.f * Phase + 0.7f) + 0.15f * FMath::Sin(Phase * 0.53f));
	return FRotator(Pitch, Yaw, 0.f);
}

float GunKinds::SwayShare(float Breathing, bool bCrouched, float CrouchShare)
{
	return FMath::Max(Breathing, 0.f) * (bCrouched ? FMath::Max(CrouchShare, 0.f) : 1.f);
}

float GunKinds::FScopeBreath::Step(bool bWant, float Dt, const FBreathRules& Rules)
{
	const float Seconds = FMath::Max(Dt, 0.f);
	if (Winded > 0.f)
	{
		// Out of breath: the sway wide, easing back to normal as the winded spell ends; the breath refills meanwhile.
		Winded = FMath::Max(Winded - Seconds, 0.f);
		Held = FMath::Max(Held - Seconds * Rules.Refill, 0.f);
		bHolding = false;
		const float Spell = Rules.WindedSeconds > 0.f ? Winded / Rules.WindedSeconds : 0.f;
		return FMath::Lerp(1.f, Rules.WindedSway, Spell);
	}
	if (bWant && Held < Rules.HoldSeconds)
	{
		Held += Seconds;
		bHolding = true;
		if (Held >= Rules.HoldSeconds)
		{
			Held = Rules.HoldSeconds;
			Winded = Rules.WindedSeconds;
			bHolding = false;
			return Rules.WindedSway;
		}
		return Rules.Steady;
	}
	Held = FMath::Max(Held - Seconds * Rules.Refill, 0.f);
	bHolding = false;
	return 1.f;
}

float GunKinds::FScopeBreath::Left(const FBreathRules& Rules) const
{
	return Rules.HoldSeconds > 0.f ? FMath::Clamp(1.f - Held / Rules.HoldSeconds, 0.f, 1.f) : 0.f;
}

float GunKinds::AimSpreadScale(EWeaponKind Kind, float AimAlpha, float Aimed, float SniperAimed)
{
	const float Aim = FMath::Clamp(AimAlpha, 0.f, 1.f);
	return FMath::Lerp(1.f, Kind == EWeaponKind::Sniper ? SniperAimed : Aimed, Aim);
}

float GunKinds::BlastFalloff(float Distance, float Core, float Radius, float EdgeShare)
{
	if (Distance > Radius)
	{
		return 0.f;
	}
	if (Distance <= Core)
	{
		return 1.f;
	}
	const float Span = FMath::Max(Radius - Core, 1.f);
	return FMath::Lerp(1.f, FMath::Clamp(EdgeShare, 0.f, 1.f), (Distance - Core) / Span);
}
