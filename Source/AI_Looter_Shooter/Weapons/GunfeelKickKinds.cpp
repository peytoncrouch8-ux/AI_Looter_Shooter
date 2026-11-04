#include "Weapons/GunfeelKickKinds.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Weapons/GunFeelSettings.h"
#include "Weapons/GunKindsTuning.h"
#include "Weapons/WeaponTypes.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

// Gun identity (GUNS.md lane A, 2026-10-28), the kick side: each kind's own numbers (FKindFeel, from Project Settings > Game >
// Gun Feel since 2026-11-03; they were one Looter.Gunfeel.* variable per number per kind) and the plain rules that read them:
// the cadence's jitter, the LMG's dig-in, the sniper's held recovery, the shot shakes, the bolt-home beat's rumble. One kick
// layer since 2026-11-03: the master switch Looter.Gunfeel.Identity still lives here, but only the sound, heft, brass and
// reload beats ask it.

namespace
{
	namespace IdDef = GunfeelKick::Default;

	TAutoConsoleVariable<int32> CVarIdentity(TEXT("Looter.Gunfeel.Identity"), 1,
		TEXT("1: every gun kind its own sound, heft, brass and reload beats. 0: the shared ones, for an A/B (the kick has one layer)."),
		ECVF_Default);
	TAutoConsoleVariable<float> CVarDigInStart(TEXT("Looter.Gunfeel.LMG.DigInStart"), IdDef::LMGDigInStart,
		TEXT("An LMG still spinning up kicks this much harder (aim, view, model)..."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDigInEnd(TEXT("Looter.Gunfeel.LMG.DigInEnd"), IdDef::LMGDigInEnd,
		TEXT("...easing to this at full spin, so a held belt settles."), ECVF_Default);
	TAutoConsoleVariable<float> CVarDigInFrom(TEXT("Looter.Gunfeel.LMG.DigInFrom"), IdDef::LMGDigInFrom,
		TEXT("The spin (0-1) the LMG's dig-in starts easing at."), ECVF_Default);
	TAutoConsoleVariable<float> CVarShakeLMGMax(TEXT("Looter.Gunfeel.Shake.LMGMax"), IdDef::ShakeLMGMax,
		TEXT("The LMG's shot shakes never stack the camera shake past this strength."), ECVF_Default);
	TAutoConsoleVariable<float> CVarSniperHold(TEXT("Looter.Gunfeel.Recoil.SniperHold"), IdDef::SniperHold,
		TEXT("The sniper's aim kick holds this share of its bolt's work before it recovers (0 recovers at once)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarHeave(TEXT("Looter.Gunfeel.Kick.Heave"), IdDef::HeaveKick,
		TEXT("The launcher's heave (the tube settling a moment after the shove) times this (0 none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarBoltHome(TEXT("Looter.Gunfeel.Kick.BoltHome"), IdDef::BoltHomeKick,
		TEXT("The nudge (and rumble) the sniper's bolt gives as it closes times this (0 none)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarLift(TEXT("Looter.Gunfeel.Kick.Lift"), IdDef::LiftKick,
		TEXT("The automatics' slow view lift that stacks over a held burst (under each kind's sustain cap) times this (0 none)."),
		ECVF_Default);
}

// ---------------------------------------------------------------------------
// The switch and the kinds' numbers
// ---------------------------------------------------------------------------

bool GunfeelKick::IdentityOn()
{
	return CVarIdentity.GetValueOnGameThread() != 0;
}

GunfeelKick::FKindFeel GunfeelKick::KindFeel(EWeaponKind Kind)
{
	// Held to sane ranges whatever was typed into the settings.
	FKindFeel Feel = UGunFeelSettings::Get().For(Kind).ToFeel();
	Feel.AimedTurn = FMath::Max(Feel.AimedTurn, 0.f);
	Feel.AimedFieldOfView = FMath::Max(Feel.AimedFieldOfView, 0.f);
	Feel.AimedBack = FMath::Max(Feel.AimedBack, 0.f);
	Feel.AimedFlip = FMath::Max(Feel.AimedFlip, 0.f);
	Feel.AimedRoll = FMath::Max(Feel.AimedRoll, 0.f);
	Feel.AimKick = FMath::Max(Feel.AimKick, 0.f);
	Feel.Recovery = FMath::Max(Feel.Recovery, 0.1f);
	// More than a fifth either way would be heard as a stutter, not a rhythm.
	Feel.Cadence = FMath::Clamp(Feel.Cadence, 0.f, 0.2f);
	// Under 1 the ease would rush the middle and crawl at both ends.
	Feel.AimCurve = FMath::Clamp(Feel.AimCurve, 1.f, 6.f);
	Feel.AimSettle = FMath::Max(Feel.AimSettle, 0.f);
	Feel.SustainCap = FMath::Max(Feel.SustainCap, 0.f);
	Feel.Shake = FMath::Clamp(Feel.Shake, 0.f, 1.f);
	Feel.ShakeSeconds = FMath::Max(Feel.ShakeSeconds, 0.f);
	return Feel;
}

float GunfeelKick::ViewKickScale(EWeaponKind Kind)
{
	return KickScale(Kind);
}

// ---------------------------------------------------------------------------
// Cadence
// ---------------------------------------------------------------------------

float GunfeelKick::CadenceJitter(int32 Shot, float Amount)
{
	// A hash of the shot's number (a murmur finish): the same shot always gets the same share, spread evenly over -1..1, so a
	// run of shots keeps the gun's mean rate.
	uint32 Hash = static_cast<uint32>(Shot) * 0x9E3779B9u + 0x7F4A7C15u;
	Hash ^= Hash >> 16;
	Hash *= 0x85EBCA6Bu;
	Hash ^= Hash >> 13;
	Hash *= 0xC2B2AE35u;
	Hash ^= Hash >> 16;
	const float Unit = static_cast<float>(Hash & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu) * 2.f - 1.f;
	return 1.f + FMath::Clamp(Amount, 0.f, 0.5f) * Unit;
}

float GunfeelKick::CadenceScale(EWeaponKind Kind, int32 Shot, float Spin)
{
	if (Kind == EWeaponKind::None)
	{
		return 1.f;
	}
	// The LMG's rate is still climbing as it spins up (and its spin-up sound is cut to that): its rhythm only once at full.
	if (Kind == EWeaponKind::LMG && Spin < 0.999f)
	{
		return 1.f;
	}
	return CadenceJitter(Shot, KindFeel(Kind).Cadence);
}

// ---------------------------------------------------------------------------
// The LMG's dig-in, the sniper's hold
// ---------------------------------------------------------------------------

float GunfeelKick::DigIn(float Spin, float From, float Start, float End)
{
	const float Begin = FMath::Clamp(From, 0.f, 0.99f);
	const float Into = FMath::Clamp((Spin - Begin) / (1.f - Begin), 0.f, 1.f);
	return FMath::Max(FMath::Lerp(Start, End, Into), 0.f);
}

float GunfeelKick::LMGDigIn(float Spin)
{
	return DigIn(Spin, CVarDigInFrom.GetValueOnGameThread(), CVarDigInStart.GetValueOnGameThread(), CVarDigInEnd.GetValueOnGameThread());
}

float GunfeelKick::RecoilHold(float BoltSeconds, float MaxSeconds, float Share)
{
	return FMath::Min(FMath::Max(BoltSeconds, 0.f) * FMath::Max(Share, 0.f), FMath::Max(MaxSeconds, 0.f));
}

float GunfeelKick::SniperRecoilHold(float BoltSeconds)
{
	return RecoilHold(BoltSeconds, GunKinds::SniperBoltMaxSeconds(), CVarSniperHold.GetValueOnGameThread());
}

// ---------------------------------------------------------------------------
// Shakes and beats
// ---------------------------------------------------------------------------

GunfeelKick::FShotShake GunfeelKick::ShotShake(EWeaponKind Kind, const FKindFeel& Feel, float AimAlpha, float Amount, float LMGMax)
{
	FShotShake Shot;
	if (Feel.Shake <= 0.f || Feel.ShakeSeconds <= 0.f)
	{
		return Shot;
	}
	// Through the sight it shakes by the same share the view kick turns, so the reticle stays readable.
	float Strength = Feel.Shake * FMath::Lerp(1.f, Feel.AimedTurn, FMath::Clamp(AimAlpha, 0.f, 1.f));
	if (Kind == EWeaponKind::LMG)
	{
		// The belt's rattle stays a rattle: only what room is left under the cap.
		Strength = FMath::Min(Strength, FMath::Max(LMGMax - FMath::Max(Amount, 0.f), 0.f));
	}
	if (Strength > 0.f)
	{
		Shot.Strength = Strength;
		Shot.Seconds = Feel.ShakeSeconds;
	}
	return Shot;
}

GunfeelKick::FShotShake GunfeelKick::ShotShake(EWeaponKind Kind, float AimAlpha, float Amount)
{
	return ShotShake(Kind, KindFeel(Kind), AimAlpha, Amount, CVarShakeLMGMax.GetValueOnGameThread());
}

float GunfeelKick::HeaveScale()
{
	return FMath::Max(CVarHeave.GetValueOnGameThread(), 0.f);
}

float GunfeelKick::BoltHomeScale()
{
	return FMath::Max(CVarBoltHome.GetValueOnGameThread(), 0.f);
}

float GunfeelKick::LiftScale()
{
	return FMath::Max(CVarLift.GetValueOnGameThread(), 0.f);
}

void GunfeelKick::RumbleBeat(APlayerController* Player, float Intensity, float Seconds)
{
	EnsureInputWatch();
	if (!Player || !Player->IsLocalController() || !IsGamepadLastInput() || Intensity <= 0.f || Seconds <= 0.f)
	{
		return;
	}
	const ULocalPlayer* LocalPlayer = Player->GetLocalPlayer();
	const UControlSettingsSubsystem* Controls = LocalPlayer ? LocalPlayer->GetSubsystem<UControlSettingsSubsystem>() : nullptr;
	const float Setting = Controls ? Controls->GetControllerRumble() : UControlSettingsSubsystem::DefaultControllerRumble;
	// The shots' global scale (GunfeelKickRumble.cpp owns the variable).
	const IConsoleVariable* Scale = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Gunfeel.Rumble.Scale"));
	const float Strength = RumbleStrength(Intensity, Setting, Scale ? Scale->GetFloat() : 1.f);
	if (Strength <= 0.f)
	{
		return;
	}
	// The small motors only: a beat is a tick in the hands, not a shot's thump.
	Player->PlayDynamicForceFeedback(Strength, Seconds, false, true, false, true);
}
