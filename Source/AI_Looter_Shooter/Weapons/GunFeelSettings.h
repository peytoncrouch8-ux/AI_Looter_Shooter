#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Weapons/GunfeelKickKinds.h"
#include "Weapons/WeaponTypes.h"
#include "GunFeelSettings.generated.h"

/** One gun kind's identity numbers, as a designer edits them (GunfeelKick::FKindFeel says what each does). */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FGunKindFeelSettings
{
	GENERATED_BODY()

	FGunKindFeelSettings() : FGunKindFeelSettings(GunfeelKick::FKindFeel()) {}
	explicit FGunKindFeelSettings(const GunfeelKick::FKindFeel& Feel);

	/** These numbers as the rules read them. */
	GunfeelKick::FKindFeel ToFeel() const;

	/** Through the sight the shot's view kick keeps this share of its hip turn. */
	UPROPERTY(Config, EditAnywhere, Category = "Sight", meta = (ClampMin = "0", ClampMax = "1.5"))
	float AimedTurn = 1.f;

	/** ...and this share of its widening and push. */
	UPROPERTY(Config, EditAnywhere, Category = "Sight", meta = (ClampMin = "0", ClampMax = "1.5"))
	float AimedFieldOfView = 1.f;

	/** Through the sight the gun model's push back is this share of the hip's. */
	UPROPERTY(Config, EditAnywhere, Category = "Sight", meta = (ClampMin = "0", ClampMax = "3"))
	float AimedBack = 1.f;

	/** ...its flip and twist this share. */
	UPROPERTY(Config, EditAnywhere, Category = "Sight", meta = (ClampMin = "0", ClampMax = "1.5"))
	float AimedFlip = 1.f;

	/** ...its roll this share. */
	UPROPERTY(Config, EditAnywhere, Category = "Sight", meta = (ClampMin = "0", ClampMax = "1.5"))
	float AimedRoll = 1.f;

	/** A base Common's aim kick (what the player pulls down against) is the asset's AimKick times this. */
	UPROPERTY(Config, EditAnywhere, Category = "Aim", meta = (ClampMin = "0", ClampMax = "5"))
	float AimKick = 1.f;

	/** The aim settles back at the asset's AimRecovery times this. */
	UPROPERTY(Config, EditAnywhere, Category = "Aim", meta = (ClampMin = "0.1", ClampMax = "5"))
	float Recovery = 1.f;

	/** An automatic's shot interval is this share either way, the mean kept (its rhythm; 0 none). */
	UPROPERTY(Config, EditAnywhere, Category = "Rhythm", meta = (ClampMin = "0", ClampMax = "0.2"))
	float Cadence = 0.f;

	/** The ease of the sights coming up (its exponent: higher eases in and out harder). */
	UPROPERTY(Config, EditAnywhere, Category = "Sight", meta = (ClampMin = "1", ClampMax = "6"))
	float AimCurve = 2.f;

	/** The view's settle as the sights arrive times this (0 none). */
	UPROPERTY(Config, EditAnywhere, Category = "Sight", meta = (ClampMin = "0", ClampMax = "4"))
	float AimSettle = 0.f;

	/** An automatic's stacked view lift is held under this many degrees (0 none; single shots have none). */
	UPROPERTY(Config, EditAnywhere, Category = "View", meta = (ClampMin = "0", ClampMax = "3"))
	float SustainCap = 0.f;

	/** Each shot shakes the camera this hard (0 none)... */
	UPROPERTY(Config, EditAnywhere, Category = "View", meta = (ClampMin = "0", ClampMax = "1"))
	float Shake = 0.f;

	/** ...dying away over this many seconds. */
	UPROPERTY(Config, EditAnywhere, Category = "View", meta = (ClampMin = "0", ClampMax = "2"))
	float ShakeSeconds = 0.f;
};

/**
 * Each gun kind's identity (gun identity, 2026-10-28), tunable without code: Project Settings > Game > Gun Feel, saved to
 * Config/DefaultGame.ini under [/Script/AI_Looter_Shooter.GunFeelSettings] when a designer changes one. Each kind starts at
 * GunfeelKick::KindFeelDefault (the one place the shipped numbers live). Read each shot and frame (GunfeelKick::KindFeel), so
 * a change in the editor shows in play at once. Replaced the per-kind Looter.Gunfeel.* variables (2026-11-03, RECOIL).
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Gun Feel"))
class AI_LOOTER_SHOOTER_API UGunFeelSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGunFeelSettings();

	UPROPERTY(Config, EditAnywhere, Category = "Kinds")
	FGunKindFeelSettings Rifle;

	UPROPERTY(Config, EditAnywhere, Category = "Kinds")
	FGunKindFeelSettings Shotgun;

	UPROPERTY(Config, EditAnywhere, Category = "Kinds")
	FGunKindFeelSettings Revolver;

	UPROPERTY(Config, EditAnywhere, Category = "Kinds")
	FGunKindFeelSettings SMG;

	UPROPERTY(Config, EditAnywhere, Category = "Kinds")
	FGunKindFeelSettings Sniper;

	UPROPERTY(Config, EditAnywhere, Category = "Kinds")
	FGunKindFeelSettings LMG;

	UPROPERTY(Config, EditAnywhere, Category = "Kinds")
	FGunKindFeelSettings Launcher;

	/** The project's settings. */
	static const UGunFeelSettings& Get();

	/** Kind's entry (a gun with no kind is the rifle). */
	const FGunKindFeelSettings& For(EWeaponKind Kind) const;
	FGunKindFeelSettings& For(EWeaponKind Kind);
};
