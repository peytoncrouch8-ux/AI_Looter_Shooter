#include "Weapons/GunfeelKickTuning.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Weapons/GunKindsTuning.h"
#include "Weapons/WeaponTypes.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

// Gun identity (GUNS.md lane A, 2026-10-28): the master switch Looter.Gunfeel.Identity, each kind's own numbers (FKindFeel,
// one Looter.Gunfeel.* variable per number per kind, defaulting to KindFeelDefault), and the plain rules that read them: the
// cadence's jitter, the LMG's dig-in, the sniper's held recovery, the shot shakes, the bolt-home beat's rumble. Identity 0
// hands every rule today's numbers back, so the two are compared in one console line.

namespace
{
	namespace IdDef = GunfeelKick::Default;
	using GunfeelKick::FKindFeel;

	TAutoConsoleVariable<int32> CVarIdentity(TEXT("Looter.Gunfeel.Identity"), 1,
		TEXT("1: every gun kind its own feel (kick, sight, sound, heft, brass, reload beats). 0: today's shared feel, for an A/B."),
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

	/** The seven kinds, in EWeaponKind's order after None, as the variables name them. */
	constexpr EWeaponKind Kinds[] = { EWeaponKind::Rifle, EWeaponKind::Shotgun, EWeaponKind::Revolver, EWeaponKind::SMG,
		EWeaponKind::Sniper, EWeaponKind::LMG, EWeaponKind::Launcher };
	constexpr int32 KindCount = static_cast<int32>(UE_ARRAY_COUNT(Kinds));

	const TCHAR* KindName(EWeaponKind Kind)
	{
		switch (Kind)
		{
		case EWeaponKind::Shotgun: return TEXT("Shotgun");
		case EWeaponKind::Revolver: return TEXT("Revolver");
		case EWeaponKind::SMG: return TEXT("SMG");
		case EWeaponKind::Sniper: return TEXT("Sniper");
		case EWeaponKind::LMG: return TEXT("LMG");
		case EWeaponKind::Launcher: return TEXT("Launcher");
		default: return TEXT("Rifle");
		}
	}

	/** Where Kind sits in Kinds (a gun with no kind is the rifle). */
	int32 KindIndex(EWeaponKind Kind)
	{
		for (int32 Index = 0; Index < KindCount; ++Index)
		{
			if (Kinds[Index] == Kind)
			{
				return Index;
			}
		}
		return 0;
	}

	/** Which kinds a number has a variable for: all, the automatics (cadence, sustain cap), or the heavy kinds that shake. */
	enum class EKinds : uint8
	{
		All,
		Automatics,
		Shakers
	};

	bool Covers(EKinds Which, EWeaponKind Kind)
	{
		switch (Which)
		{
		case EKinds::Automatics: return Kind == EWeaponKind::Rifle || Kind == EWeaponKind::SMG || Kind == EWeaponKind::LMG;
		case EKinds::Shakers: return Kind == EWeaponKind::LMG || Kind == EWeaponKind::Shotgun || Kind == EWeaponKind::Sniper
			|| Kind == EWeaponKind::Launcher;
		default: return true;
		}
	}

	/**
	 * One number of FKindFeel as a variable per kind, Looter.Gunfeel.<Prefix><Kind><Suffix>, starting at KindFeelDefault's
	 * value; a kind it has no variable for keeps its default. Registered as the module loads, as TAutoConsoleVariable is.
	 */
	class FKindVariable
	{
	public:
		FKindVariable(const TCHAR* Prefix, const TCHAR* Suffix, float FKindFeel::*InField, const TCHAR* Help, EKinds Which)
			: Field(InField)
		{
			for (int32 Index = 0; Index < KindCount; ++Index)
			{
				const EWeaponKind Kind = Kinds[Index];
				if (Covers(Which, Kind))
				{
					const FString Name = FString::Printf(TEXT("Looter.Gunfeel.%s%s%s"), Prefix, KindName(Kind), Suffix);
					Variables[Index] = IConsoleManager::Get().RegisterConsoleVariable(*Name, GunfeelKick::KindFeelDefault(Kind).*Field, Help,
						ECVF_Default);
				}
			}
		}

		~FKindVariable()
		{
			for (IConsoleVariable* Variable : Variables)
			{
				if (Variable)
				{
					IConsoleManager::Get().UnregisterConsoleObject(Variable);
				}
			}
		}

		FKindVariable(const FKindVariable&) = delete;
		FKindVariable& operator=(const FKindVariable&) = delete;

		float Get(EWeaponKind Kind) const
		{
			const IConsoleVariable* Variable = Variables[KindIndex(Kind)];
			return Variable ? Variable->GetFloat() : GunfeelKick::KindFeelDefault(Kind).*Field;
		}

	private:
		float FKindFeel::*Field;
		IConsoleVariable* Variables[KindCount] = {};
	};

	const FKindVariable AimedTurn(TEXT("Kick.AimedTurn."), TEXT(""), &FKindFeel::AimedTurn,
		TEXT("Through the sight a shot's view kick keeps this share of its hip turn (gun identity, one per kind)."), EKinds::All);
	const FKindVariable AimedFov(TEXT("Kick.AimedFov."), TEXT(""), &FKindFeel::AimedFieldOfView,
		TEXT("Through the sight a shot's view kick keeps this share of its widening and push (one per kind)."), EKinds::All);
	const FKindVariable AimedBack(TEXT("Recoil.AimedBack."), TEXT(""), &FKindFeel::AimedBack,
		TEXT("Through the sight the gun model's push back is this share of the hip's (one per kind)."), EKinds::All);
	const FKindVariable AimedFlip(TEXT("Recoil.AimedFlip."), TEXT(""), &FKindFeel::AimedFlip,
		TEXT("Through the sight the gun model's flip and twist are this share of the hip's (one per kind)."), EKinds::All);
	const FKindVariable AimedRoll(TEXT("Recoil.AimedRoll."), TEXT(""), &FKindFeel::AimedRoll,
		TEXT("Through the sight the gun model's roll is this share of the hip's (one per kind)."), EKinds::All);
	const FKindVariable Recovery(TEXT("Recoil.Recovery."), TEXT(""), &FKindFeel::Recovery,
		TEXT("The aim's recovery after a shot times this, on the gun's own AimRecovery (one per kind)."), EKinds::All);
	const FKindVariable Cadence(TEXT("Cadence."), TEXT(""), &FKindFeel::Cadence,
		TEXT("Each shot's interval is +- this share, the mean kept (an automatic's rhythm; the LMG's once spun up)."), EKinds::Automatics);
	const FKindVariable AimCurve(TEXT("Aim.Curve."), TEXT(""), &FKindFeel::AimCurve,
		TEXT("The ease of the sights coming up (its exponent; 2 the old one for all; higher eases in and out harder)."), EKinds::All);
	const FKindVariable AimSettle(TEXT("Aim.Settle."), TEXT(""), &FKindFeel::AimSettle,
		TEXT("The view's settle as the sights arrive times this (0 none)."), EKinds::All);
	const FKindVariable SustainCap(TEXT("Kick.SustainCap."), TEXT(""), &FKindFeel::SustainCap,
		TEXT("This automatic's summed shot kicks (view pitch, degrees) are held under this; 0 turns its cap off."), EKinds::Automatics);
	const FKindVariable Shake(TEXT("Shake."), TEXT(""), &FKindFeel::Shake,
		TEXT("A shot shakes the camera this hard (0-1; 0 none; the Camera shake setting scales it)..."), EKinds::Shakers);
	const FKindVariable ShakeSeconds(TEXT("Shake."), TEXT("Seconds"), &FKindFeel::ShakeSeconds,
		TEXT("...dying away over this many seconds."), EKinds::Shakers);

	float TodayKick(EWeaponKind Kind)
	{
		namespace Today = IdDef::Today;
		switch (Kind)
		{
		case EWeaponKind::Shotgun: return Today::KickShotgun;
		case EWeaponKind::Revolver: return Today::KickRevolver;
		case EWeaponKind::SMG: return Today::KickSMG;
		case EWeaponKind::Sniper: return Today::KickSniper;
		case EWeaponKind::LMG: return Today::KickLMG;
		case EWeaponKind::Launcher: return Today::KickLauncher;
		default: return Today::KickRifle;
		}
	}
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
	FKindFeel Feel;
	if (!IdentityOn())
	{
		// Today's shared numbers: the struct's own (half the turn, curve 2, no cadence, settle or shake), the sight's model
		// shares and the automatics' cap as their old variables have them.
		FVector Shares(1.f, 1.f, 1.f);
		AimedModelKick(1.f, Shares);
		Feel.AimedBack = static_cast<float>(Shares.X);
		Feel.AimedFlip = static_cast<float>(Shares.Y);
		Feel.AimedRoll = static_cast<float>(Shares.Z);
		Feel.SustainCap = SustainCapFor(Kind);
		return Feel;
	}
	Feel.AimedTurn = FMath::Max(AimedTurn.Get(Kind), 0.f);
	Feel.AimedFieldOfView = FMath::Max(AimedFov.Get(Kind), 0.f);
	Feel.AimedBack = FMath::Max(AimedBack.Get(Kind), 0.f);
	Feel.AimedFlip = FMath::Max(AimedFlip.Get(Kind), 0.f);
	Feel.AimedRoll = FMath::Max(AimedRoll.Get(Kind), 0.f);
	Feel.Recovery = FMath::Max(Recovery.Get(Kind), 0.1f);
	// More than a fifth either way would be heard as a stutter, not a rhythm.
	Feel.Cadence = FMath::Clamp(Cadence.Get(Kind), 0.f, 0.2f);
	// Under 1 the ease would rush the middle and crawl at both ends.
	Feel.AimCurve = FMath::Clamp(AimCurve.Get(Kind), 1.f, 6.f);
	Feel.AimSettle = FMath::Max(AimSettle.Get(Kind), 0.f);
	Feel.SustainCap = FMath::Max(SustainCap.Get(Kind), 0.f);
	Feel.Shake = FMath::Clamp(Shake.Get(Kind), 0.f, 1.f);
	Feel.ShakeSeconds = FMath::Max(ShakeSeconds.Get(Kind), 0.f);
	return Feel;
}

float GunfeelKick::ViewKickScale(EWeaponKind Kind)
{
	return KickScale(Kind) * (IdentityOn() ? 1.f : TodayKick(Kind));
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
	if (Kind == EWeaponKind::None || !IdentityOn())
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
	if (!IdentityOn())
	{
		return 1.f;
	}
	return DigIn(Spin, CVarDigInFrom.GetValueOnGameThread(), CVarDigInStart.GetValueOnGameThread(), CVarDigInEnd.GetValueOnGameThread());
}

float GunfeelKick::RecoilHold(float BoltSeconds, float MaxSeconds, float Share)
{
	return FMath::Min(FMath::Max(BoltSeconds, 0.f) * FMath::Max(Share, 0.f), FMath::Max(MaxSeconds, 0.f));
}

float GunfeelKick::SniperRecoilHold(float BoltSeconds)
{
	return IdentityOn() ? RecoilHold(BoltSeconds, GunKinds::SniperBoltMaxSeconds(), CVarSniperHold.GetValueOnGameThread()) : 0.f;
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
	if (!IdentityOn())
	{
		return FShotShake();
	}
	return ShotShake(Kind, KindFeel(Kind), AimAlpha, Amount, CVarShakeLMGMax.GetValueOnGameThread());
}

float GunfeelKick::HeaveScale()
{
	return IdentityOn() ? FMath::Max(CVarHeave.GetValueOnGameThread(), 0.f) : 0.f;
}

float GunfeelKick::BoltHomeScale()
{
	return IdentityOn() ? FMath::Max(CVarBoltHome.GetValueOnGameThread(), 0.f) : 0.f;
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
