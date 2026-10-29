#include "Weapons/ReloadBeats.h"
#include "AI_Looter_Shooter.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Player/CameraShakeModifier.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Weapons/GunfeelKickTuning.h"
#include "Weapons/ReloadTuning.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"

// The felt beats of a reload (ReloadBeats.h). The table is GUNS.md's: degrees of view pitch (a dip is negative), roll,
// field of view share, and gamepad rumble strength / seconds, per kind and reload step. The springs are quick for the
// light guns and slower for the heavy ones, in the range of the shotgun's rack beat (ViewKicks::ForRack: 12 Hz, 0.6).

namespace
{
	TAutoConsoleVariable<float> CVarBeats(TEXT("Looter.Gunfeel.Reload.Beats"), 1.f,
		TEXT("The felt beats of a reload (a magazine seating, a bolt slammed, a cylinder snapped home): kick and rumble times this (0 = none)."),
		ECVF_Default);

	/** A beat from its numbers; Hz and Damping are the spring of the kind's weight. */
	FReloadBeat Beat(float Pitch, float Roll, float Fov, float Hz, float Damping, float Rumble, float RumbleSeconds)
	{
		FReloadBeat Result;
		Result.Kick.Pitch = Pitch;
		Result.Kick.Roll = Roll;
		Result.Kick.FieldOfView = Fov;
		Result.Kick.Frequency = Hz;
		Result.Kick.Damping = Damping;
		Result.Rumble = Rumble;
		Result.RumbleSeconds = RumbleSeconds;
		return Result;
	}

	/** The rumble: only a gamepad in hand (GunfeelKick::IsGamepadLastInput), by the player's rumble setting and Looter.Gunfeel.Rumble.Scale. */
	void Rumble(APlayerController* Player, const FReloadBeat& Felt, float BeatScale)
	{
		if (Felt.Rumble <= 0.f || Felt.RumbleSeconds <= 0.f || !GunfeelKick::IsGamepadLastInput())
		{
			return;
		}
		const ULocalPlayer* LocalPlayer = Player->GetLocalPlayer();
		const UControlSettingsSubsystem* Controls = LocalPlayer ? LocalPlayer->GetSubsystem<UControlSettingsSubsystem>() : nullptr;
		const float Setting = Controls ? Controls->GetControllerRumble() : UControlSettingsSubsystem::DefaultControllerRumble;
		const IConsoleVariable* Global = IConsoleManager::Get().FindConsoleVariable(TEXT("Looter.Gunfeel.Rumble.Scale"));
		const float Strength = GunfeelKick::RumbleStrength(Felt.Rumble * BeatScale, Setting, Global ? Global->GetFloat() : 1.f);
		if (Strength > 0.f)
		{
			// The heavy beats (a belt box, a rocket) shake both big motors too; the light ones only the small ones.
			Player->PlayDynamicForceFeedback(Strength, Felt.RumbleSeconds, Felt.Rumble >= 0.4f, true, Felt.Rumble >= 0.4f, true);
		}
	}
}

FReloadBeat ReloadBeats::For(EWeaponKind Kind, EReloadStep Step)
{
	switch (Kind)
	{
	case EWeaponKind::SMG:
		// A light helix clicked home; the charge a tick harder.
		if (Step == EReloadStep::HelixIn)
		{
			return Beat(-0.08f, 0.1f, 0.f, 18.f, 0.6f, 0.15f, 0.04f);
		}
		if (Step == EReloadStep::Bolt)
		{
			return Beat(0.1f, 0.f, 0.f, 18.f, 0.6f, 0.2f, 0.04f);
		}
		break;
	case EWeaponKind::Rifle:
		// The magazine slapped home; the handle's slap from empty.
		if (Step == EReloadStep::MagIn)
		{
			return Beat(-0.12f, 0.f, -0.003f, 14.f, 0.6f, 0.2f, 0.05f);
		}
		if (Step == EReloadStep::Bolt)
		{
			return Beat(0.18f, 0.2f, 0.f, 14.f, 0.6f, 0.3f, 0.06f);
		}
		break;
	case EWeaponKind::LMG:
		// The belt box: off with a lurch, on with a thud, the belt laid in the tray.
		if (Step == EReloadStep::BoxOut)
		{
			return Beat(-0.1f, 0.f, 0.f, 9.f, 0.55f, 0.f, 0.f);
		}
		if (Step == EReloadStep::BoxIn)
		{
			return Beat(-0.25f, 0.f, -0.005f, 9.f, 0.55f, 0.45f, 0.08f);
		}
		if (Step == EReloadStep::BeltLay)
		{
			return Beat(-0.06f, 0.f, 0.f, 9.f, 0.55f, 0.f, 0.f);
		}
		break;
	case EWeaponKind::Revolver:
		// The wrist: the cylinder swung out rolls it, the rod punches, the speedloader drops, the cylinder snaps home.
		if (Step == EReloadStep::CylinderOut)
		{
			return Beat(0.f, -0.5f, 0.f, 12.f, 0.6f, 0.f, 0.f);
		}
		if (Step == EReloadStep::Eject)
		{
			return Beat(-0.15f, 0.f, 0.f, 12.f, 0.6f, 0.f, 0.f);
		}
		if (Step == EReloadStep::RoundsIn)
		{
			return Beat(-0.05f, 0.f, 0.f, 12.f, 0.6f, 0.f, 0.f);
		}
		if (Step == EReloadStep::CylinderIn)
		{
			return Beat(-0.1f, 0.6f, 0.f, 12.f, 0.6f, 0.3f, 0.05f);
		}
		break;
	case EWeaponKind::Shotgun:
		// Each shell pushed home; the pump racked at the end.
		if (Step == EReloadStep::ShellIn)
		{
			return Beat(-0.05f, 0.f, 0.f, 11.f, 0.6f, 0.1f, 0.03f);
		}
		if (Step == EReloadStep::Pump)
		{
			return Beat(0.15f, 0.f, 0.f, 11.f, 0.6f, 0.25f, 0.05f);
		}
		break;
	case EWeaponKind::Sniper:
		// The bolt lifted, the heavy magazine in, the bolt run home and locked.
		if (Step == EReloadStep::BoltOpen)
		{
			return Beat(0.06f, 0.f, 0.f, 10.f, 0.6f, 0.f, 0.f);
		}
		if (Step == EReloadStep::MagIn)
		{
			return Beat(-0.12f, 0.f, 0.f, 10.f, 0.6f, 0.f, 0.f);
		}
		if (Step == EReloadStep::BoltClose)
		{
			return Beat(-0.2f, 0.15f, 0.f, 10.f, 0.6f, 0.35f, 0.06f);
		}
		break;
	case EWeaponKind::Launcher:
		// The rocket seated in the tube: the whole gun sags.
		if (Step == EReloadStep::RocketIn)
		{
			return Beat(-0.3f, 0.f, -0.006f, 7.f, 0.5f, 0.5f, 0.1f);
		}
		break;
	default:
		break;
	}
	return FReloadBeat();
}

float ReloadBeats::Scale()
{
	return FMath::Max(CVarBeats.GetValueOnGameThread(), 0.f);
}

bool ReloadBeats::Plays(bool bIdentity, float BeatScale, float AimAlpha)
{
	return bIdentity && BeatScale > 0.f && AimAlpha < AimedCutoff;
}

void ReloadBeats::Play(const AWeaponBase& Weapon, EReloadStep Step)
{
	const UWeaponDefinition* Definition = Weapon.GetInstance().Definition;
	// Only the player's own gun is felt (an enemy's reload is seen and heard).
	const ACharacter* Holder = Cast<ACharacter>(Weapon.GetOwner());
	APlayerController* Player = Holder ? Cast<APlayerController>(Holder->GetController()) : nullptr;
	if (!Definition || !Player || !Player->IsLocalController())
	{
		return;
	}
	const UPlayerViewComponent* View = Holder->FindComponentByClass<UPlayerViewComponent>();
	const float BeatScale = Scale();
	if (!Plays(GunfeelKick::IdentityOn(), BeatScale, View ? View->GetAimAlpha() : 0.f))
	{
		return;
	}
	const FReloadBeat Found = For(Definition->Kind, Step);
	if (Found.IsNone())
	{
		return;
	}
	if (ReloadTuning::Debug())
	{
		UE_LOG(LogLooter, Display, TEXT("Reload beat: %s step %d pitch %.2f roll %.2f fov %.3f rumble %.2f/%.2f s (x%.2f)"), *Weapon.GetName(),
			static_cast<int32>(Step), Found.Kick.Pitch, Found.Kick.Roll, Found.Kick.FieldOfView, Found.Rumble, Found.RumbleSeconds, BeatScale);
	}
	UCameraShakeModifier::Kick(Player, Found.Kick * BeatScale);
	Rumble(Player, Found, BeatScale);
}
