#include "Weapons/GunfeelKickTuning.h"
#include "AI_Looter_Shooter.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Input/Events.h"
#include "Misc/CoreDelegates.h"

// Controller rumble for the kick side of gunplay (GunfeelKickTuning.h): a shot's, and a hit taken's. It uses the engine's
// dynamic force feedback, so there are no assets, and only runs when the last thing the player pressed or moved was a
// gamepad's (a Slate input pre-processor watches), so nothing rumbles on a mouse and keys.

namespace
{
	namespace Def = GunfeelKick::Default;

	TAutoConsoleVariable<float> CVarRumbleScale(TEXT("Looter.Gunfeel.Rumble.Scale"), Def::RumbleScale,
		TEXT("Every rumble (shots and hits taken) times this, on top of the player's Controller rumble setting."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleRifle(TEXT("Looter.Gunfeel.Rumble.Rifle"), Def::RumbleRifle,
		TEXT("A rifle shot's rumble strength (0-1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleRifleSeconds(TEXT("Looter.Gunfeel.Rumble.RifleSeconds"), Def::RumbleRifleSeconds,
		TEXT("...and how long it lasts (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleShotgun(TEXT("Looter.Gunfeel.Rumble.Shotgun"), Def::RumbleShotgun,
		TEXT("A shotgun blast's rumble strength (0-1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleShotgunSeconds(TEXT("Looter.Gunfeel.Rumble.ShotgunSeconds"), Def::RumbleShotgunSeconds,
		TEXT("...and how long it lasts (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleRevolver(TEXT("Looter.Gunfeel.Rumble.Revolver"), Def::RumbleRevolver,
		TEXT("A revolver shot's rumble strength (0-1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleRevolverSeconds(TEXT("Looter.Gunfeel.Rumble.RevolverSeconds"), Def::RumbleRevolverSeconds,
		TEXT("...and how long it lasts (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleHurt(TEXT("Looter.Gunfeel.Rumble.Hurt"), Def::RumbleHurt,
		TEXT("The rumble strength (0-1) of a hit taken for a quarter of the health bar or more (smaller hits less)."), ECVF_Default);
	// The four frontier-tech kinds: the SMG a light buzz, the sniper a deep thump, the LMG a steady chug, the launcher the most.
	TAutoConsoleVariable<float> CVarRumbleSMG(TEXT("Looter.Gunfeel.Rumble.SMG"), Def::RumbleSMG,
		TEXT("An SMG shot's rumble strength (0-1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleSMGSeconds(TEXT("Looter.Gunfeel.Rumble.SMGSeconds"), Def::RumbleSMGSeconds,
		TEXT("...and how long it lasts (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleSniper(TEXT("Looter.Gunfeel.Rumble.Sniper"), Def::RumbleSniper,
		TEXT("A sniper shot's rumble strength (0-1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleSniperSeconds(TEXT("Looter.Gunfeel.Rumble.SniperSeconds"), Def::RumbleSniperSeconds,
		TEXT("...and how long it lasts (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleLMG(TEXT("Looter.Gunfeel.Rumble.LMG"), Def::RumbleLMG,
		TEXT("An LMG shot's rumble strength (0-1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleLMGSeconds(TEXT("Looter.Gunfeel.Rumble.LMGSeconds"), Def::RumbleLMGSeconds,
		TEXT("...and how long it lasts (s)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleLauncher(TEXT("Looter.Gunfeel.Rumble.Launcher"), Def::RumbleLauncher,
		TEXT("A rocket launch's rumble strength (0-1)."), ECVF_Default);
	TAutoConsoleVariable<float> CVarRumbleLauncherSeconds(TEXT("Looter.Gunfeel.Rumble.LauncherSeconds"), Def::RumbleLauncherSeconds,
		TEXT("...and how long it lasts (s)."), ECVF_Default);

	/** A hit for this share of the player's health or more is the full rumble; a scratch a quarter of it. */
	constexpr float FullHurtShare = 0.25f;
	constexpr float LeastHurtStrength = 0.25f;
	constexpr float HurtLeastSeconds = 0.1f;
	constexpr float HurtMostSeconds = 0.3f;
	/** A stick or trigger moved less than this isn't the player's hand (drift). */
	constexpr float AnalogDeadzone = 0.25f;

	bool bGamepadLast = false;

	/** Watches every key, stick and mouse event before Slate routes it, to know which device the player last used. */
	class FLastInputWatch : public IInputProcessor
	{
	public:
		virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
		{
			GunfeelKick::NoteInputDevice(InKeyEvent.GetKey().IsGamepadKey());
			return false;
		}

		virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override
		{
			if (InAnalogInputEvent.GetKey().IsGamepadKey() && FMath::Abs(InAnalogInputEvent.GetAnalogValue()) > AnalogDeadzone)
			{
				GunfeelKick::NoteInputDevice(true);
			}
			return false;
		}

		virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
		{
			if (!MouseEvent.GetCursorDelta().IsNearlyZero())
			{
				GunfeelKick::NoteInputDevice(false);
			}
			return false;
		}

		virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
		{
			GunfeelKick::NoteInputDevice(false);
			return false;
		}

		virtual const TCHAR* GetDebugName() const override { return TEXT("GunfeelKickInputWatch"); }
	};

	TSharedPtr<FLastInputWatch> InputWatch;

	void StopInputWatch()
	{
		if (InputWatch.IsValid() && FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().UnregisterInputPreProcessor(InputWatch);
		}
		InputWatch.Reset();
	}

	/** Rumbles Player's gamepad by Strength for Seconds; the shotgun and revolver shake both big motors, the rifle's tick the right one. */
	void Play(APlayerController* Player, const GunfeelKick::FRumble& Rumble, bool bBothSides)
	{
		if (!Player || !Player->IsLocalController() || !GunfeelKick::IsGamepadLastInput() || Rumble.Intensity <= 0.f || Rumble.Seconds <= 0.f)
		{
			return;
		}
		const ULocalPlayer* LocalPlayer = Player->GetLocalPlayer();
		const UControlSettingsSubsystem* Controls = LocalPlayer ? LocalPlayer->GetSubsystem<UControlSettingsSubsystem>() : nullptr;
		const float Setting = Controls ? Controls->GetControllerRumble() : UControlSettingsSubsystem::DefaultControllerRumble;
		const float Strength = GunfeelKick::RumbleStrength(Rumble.Intensity, Setting, CVarRumbleScale.GetValueOnGameThread());
		if (Strength <= 0.f)
		{
			return;
		}
		// Left large, left small, right large, right small. Handle 0 starts a fresh effect each time, so overlapping shots
		// of a burst keep rumbling their own length.
		Player->PlayDynamicForceFeedback(Strength, Rumble.Seconds, bBothSides, true, true, true);
	}
}

// ---------------------------------------------------------------------------
// Rules
// ---------------------------------------------------------------------------

float GunfeelKick::RumbleStrength(float Base, float Setting, float Scale)
{
	return FMath::Clamp(Base * FMath::Clamp(Setting, 0.f, 1.f) * FMath::Max(Scale, 0.f), 0.f, 1.f);
}

GunfeelKick::FRumble GunfeelKick::RumbleForShot(EWeaponKind Kind)
{
	FRumble Rumble;
	switch (Kind)
	{
	case EWeaponKind::Shotgun:
		Rumble.Intensity = CVarRumbleShotgun.GetValueOnGameThread();
		Rumble.Seconds = CVarRumbleShotgunSeconds.GetValueOnGameThread();
		break;
	case EWeaponKind::Revolver:
		Rumble.Intensity = CVarRumbleRevolver.GetValueOnGameThread();
		Rumble.Seconds = CVarRumbleRevolverSeconds.GetValueOnGameThread();
		break;
	case EWeaponKind::SMG:
		Rumble.Intensity = CVarRumbleSMG.GetValueOnGameThread();
		Rumble.Seconds = CVarRumbleSMGSeconds.GetValueOnGameThread();
		break;
	case EWeaponKind::Sniper:
		Rumble.Intensity = CVarRumbleSniper.GetValueOnGameThread();
		Rumble.Seconds = CVarRumbleSniperSeconds.GetValueOnGameThread();
		break;
	case EWeaponKind::LMG:
		Rumble.Intensity = CVarRumbleLMG.GetValueOnGameThread();
		Rumble.Seconds = CVarRumbleLMGSeconds.GetValueOnGameThread();
		break;
	case EWeaponKind::Launcher:
		Rumble.Intensity = CVarRumbleLauncher.GetValueOnGameThread();
		Rumble.Seconds = CVarRumbleLauncherSeconds.GetValueOnGameThread();
		break;
	case EWeaponKind::Rifle:
	case EWeaponKind::None:
	default:
		Rumble.Intensity = CVarRumbleRifle.GetValueOnGameThread();
		Rumble.Seconds = CVarRumbleRifleSeconds.GetValueOnGameThread();
		break;
	}
	return Rumble;
}

GunfeelKick::FRumble GunfeelKick::RumbleForHurt(float DamageShare, float HurtBase)
{
	const float Full = FMath::Clamp(DamageShare / FullHurtShare, 0.f, 1.f);
	FRumble Rumble;
	Rumble.Intensity = DamageShare > 0.f ? HurtBase * FMath::Max(Full, LeastHurtStrength) : 0.f;
	Rumble.Seconds = DamageShare > 0.f ? FMath::Lerp(HurtLeastSeconds, HurtMostSeconds, Full) : 0.f;
	return Rumble;
}

GunfeelKick::FRumble GunfeelKick::RumbleForHurt(float DamageShare)
{
	return RumbleForHurt(DamageShare, CVarRumbleHurt.GetValueOnGameThread());
}

// ---------------------------------------------------------------------------
// The gamepad
// ---------------------------------------------------------------------------

bool GunfeelKick::IsGamepadLastInput()
{
	return bGamepadLast;
}

void GunfeelKick::NoteInputDevice(bool bGamepad)
{
	bGamepadLast = bGamepad;
}

void GunfeelKick::EnsureInputWatch()
{
	// No Slate (a server, a commandlet, a test run without a window): nothing to watch, and nothing rumbles.
	if (InputWatch.IsValid() || !FSlateApplication::IsInitialized())
	{
		return;
	}
	InputWatch = MakeShared<FLastInputWatch>();
	FSlateApplication::Get().RegisterInputPreProcessor(InputWatch);
	FCoreDelegates::OnEnginePreExit.AddStatic(&StopInputWatch);
}

void GunfeelKick::RumbleShot(APlayerController* Player, EWeaponKind Kind)
{
	EnsureInputWatch();
	// The rifle's and the SMG's quick ticks only work the right-hand motors; the heavy guns shake the whole pad.
	Play(Player, RumbleForShot(Kind), Kind != EWeaponKind::Rifle && Kind != EWeaponKind::SMG && Kind != EWeaponKind::None);
}

void GunfeelKick::RumbleHurt(APlayerController* Player, float DamageShare)
{
	EnsureInputWatch();
	Play(Player, RumbleForHurt(DamageShare), true);
}

// ---------------------------------------------------------------------------
// Looter.ControllerRumble
// ---------------------------------------------------------------------------

namespace
{
	/**
	 * Looter.ControllerRumble [0-1]: the Controller rumble setting for this run (every local player's; the settings menu's
	 * slider saves it), or with no number, what it is.
	 */
	void ControllerRumbleCommand(const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			return;
		}
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* Player = It->Get();
			const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
			UControlSettingsSubsystem* Controls = LocalPlayer ? LocalPlayer->GetSubsystem<UControlSettingsSubsystem>() : nullptr;
			if (!Controls)
			{
				continue;
			}
			if (Args.Num() > 0)
			{
				Controls->SetControllerRumble(FCString::Atof(*Args[0]), /*bSave*/ false);
			}
			UE_LOG(LogLooter, Display, TEXT("Controller rumble: %d%% (last input %s)"), FMath::RoundToInt32(Controls->GetControllerRumble() * 100.f),
				GunfeelKick::IsGamepadLastInput() ? TEXT("a gamepad") : TEXT("keys and mouse"));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs ControllerRumbleConsoleCommand(
		TEXT("Looter.ControllerRumble"),
		TEXT("Looter.ControllerRumble [0-1]: scales every controller rumble (0 turns it off), as the settings' Controller rumble ")
		TEXT("slider does, for this run; with no number, prints it."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&ControllerRumbleCommand));
}
