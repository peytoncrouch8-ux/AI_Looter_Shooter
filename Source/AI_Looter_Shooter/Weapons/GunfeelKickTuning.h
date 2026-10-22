#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

class AWeaponBase;
class APlayerController;

/**
 * Gunfeel step 2 and 3, the kick side (Docs/Plan.md 12.1, Saved/Gunfeel/CONTRACT.md): how hard each gun kicks the view and
 * the gun, the controller's rumble, the magazine's last-rounds click, and each gun's job in the hand (the rifle's climb and
 * hip bloom, the shotgun's sharper falloff, the revolver's bigger crit) plus the handling per kind (aim, aimed walk, swap).
 *
 * Every number is a Looter.Gunfeel.* console variable read through an accessor here, each time it is asked, so the player
 * tunes it live in play. The rules are plain functions over numbers (Default holds the shipped ones) so tests drive them
 * without a world or a variable the player may have set.
 */
namespace GunfeelKick
{
	/** The numbers a variable starts at: what the rules use when a test asks for "the defaults". */
	namespace Default
	{
		inline constexpr float KickRifle = 1.6f;
		inline constexpr float KickShotgun = 1.5f;
		inline constexpr float KickRevolver = 1.4f;
		inline constexpr float RecoilRifle = 1.4f;
		inline constexpr float RecoilShotgun = 1.5f;
		inline constexpr float RecoilRevolver = 1.4f;
		inline constexpr float ShotgunSettle = 0.8f;
		inline constexpr float Muzzle1P = 1.4f;

		inline constexpr float RumbleScale = 1.f;
		inline constexpr float RumbleRifle = 0.35f;
		inline constexpr float RumbleRifleSeconds = 0.06f;
		inline constexpr float RumbleShotgun = 0.9f;
		inline constexpr float RumbleShotgunSeconds = 0.18f;
		inline constexpr float RumbleRevolver = 0.7f;
		inline constexpr float RumbleRevolverSeconds = 0.12f;
		inline constexpr float RumbleHurt = 0.6f;

		inline constexpr float LowAmmoShare = 0.25f;
		inline constexpr float LowAmmoPitchTop = 1.35f;

		inline constexpr float RifleClimb = 0.06f;
		inline constexpr float RifleClimbMax = 1.8f;
		inline constexpr float RifleBloom = 0.15f;
		inline constexpr float BloomMax = 2.f;
		inline constexpr float BloomRecovery = 3.f;

		inline constexpr float ShotgunFalloffStart = 0.6f;
		inline constexpr float ShotgunFalloffEnd = 1.6f;
		inline constexpr float ShotgunFalloffFloor = 0.25f;

		inline constexpr float CritRevolver = 1.75f;

		inline constexpr float AimTimeRifle = 1.f;
		inline constexpr float AimTimeShotgun = 1.1f;
		inline constexpr float AimTimeRevolver = 0.8f;
		inline constexpr float AimMoveRifle = 1.f;
		inline constexpr float AimMoveShotgun = 0.9f;
		inline constexpr float AimMoveRevolver = 1.1f;
		inline constexpr float SwapRifle = 1.f;
		inline constexpr float SwapShotgun = 1.1f;
		inline constexpr float SwapRevolver = 0.8f;

		// The four frontier-tech kinds (Phase 12.3): the SMG a light, quick rattle; the sniper one heavy shove; the LMG a
		// steady weight; the launcher the biggest shove of all.
		inline constexpr float KickSMG = 1.f;
		inline constexpr float KickSniper = 1.6f;
		inline constexpr float KickLMG = 1.3f;
		inline constexpr float KickLauncher = 1.8f;
		inline constexpr float RecoilSMG = 1.f;
		inline constexpr float RecoilSniper = 1.5f;
		inline constexpr float RecoilLMG = 1.3f;
		inline constexpr float RecoilLauncher = 1.6f;
		inline constexpr float RumbleSMG = 0.25f;
		inline constexpr float RumbleSMGSeconds = 0.04f;
		inline constexpr float RumbleSniper = 0.85f;
		inline constexpr float RumbleSniperSeconds = 0.16f;
		inline constexpr float RumbleLMG = 0.45f;
		inline constexpr float RumbleLMGSeconds = 0.07f;
		inline constexpr float RumbleLauncher = 1.f;
		inline constexpr float RumbleLauncherSeconds = 0.3f;
		inline constexpr float AimTimeSMG = 0.75f;
		inline constexpr float AimTimeSniper = 1.35f;
		inline constexpr float AimTimeLMG = 1.4f;
		inline constexpr float AimTimeLauncher = 1.2f;
		inline constexpr float AimMoveSMG = 1.15f;
		inline constexpr float AimMoveSniper = 0.85f;
		inline constexpr float AimMoveLMG = 0.8f;
		inline constexpr float AimMoveLauncher = 0.85f;
		inline constexpr float SwapSMG = 0.7f;
		inline constexpr float SwapSniper = 1.2f;
		inline constexpr float SwapLMG = 1.35f;
		inline constexpr float SwapLauncher = 1.3f;
		/** The sniper's job is crits (Main's unattended call, 2026-10-21, as the revolver's role allowed above 1.5x). */
		inline constexpr float CritSniper = 2.f;
		/** A burst's climb and hip bloom per shot for the two automatic newcomers (the rifle's are above). */
		inline constexpr float SMGClimb = 0.03f;
		inline constexpr float LMGClimb = 0.04f;
		inline constexpr float SMGBloom = 0.09f;
		inline constexpr float LMGBloom = 0.12f;
	}

	/** The gun's kind by its definition (Rifle without one; the kinds with no numbers of their own take the rifle's). */
	AI_LOOTER_SHOOTER_API EWeaponKind KindOf(const AWeaponBase* Weapon);

	// --- The live numbers (the console variables' current values) ---

	/** The view kick of a shot from Kind times this (ViewKicks::ForShot's turn and widening). */
	AI_LOOTER_SHOOTER_API float KickScale(EWeaponKind Kind);
	/** The gun's own kick (back, flip, twist, roll) of a shot from Kind times this. */
	AI_LOOTER_SHOOTER_API float RecoilScale(EWeaponKind Kind);
	/** The shotgun's springs run at this much of their speed, so it settles heavier. */
	AI_LOOTER_SHOOTER_API float ShotgunSettle();
	/** The built-in muzzle flash in first person is this much bigger. */
	AI_LOOTER_SHOOTER_API float Muzzle1PScale();
	/** Time to bring the sights up, times this (more is slower). */
	AI_LOOTER_SHOOTER_API float AimTimeScale(EWeaponKind Kind);
	/** Walk speed while aimed, times this (the walk while aiming is its own share of the normal one). */
	AI_LOOTER_SHOOTER_API float AimMoveScale(EWeaponKind Kind);
	/** A swap's holster and draw times, times this (more is slower). */
	AI_LOOTER_SHOOTER_API float SwapScale(EWeaponKind Kind);
	/** The rifle's aim kick grows by this share per shot of a burst, up to RifleClimbMax times the first shot's (FWeaponRecoilProfile::ClimbPerShot, ClimbMax). */
	AI_LOOTER_SHOOTER_API float RifleClimb();
	AI_LOOTER_SHOOTER_API float RifleClimbMax();
	/**
	 * A burst's climb per shot for Kind: the rifle's RifleClimb, the SMG's and the LMG's own (Recoil.SMGClimb, LMGClimb),
	 * none for the kinds that don't fire bursts (0).
	 */
	AI_LOOTER_SHOOTER_API float ClimbPerShot(EWeaponKind Kind);

	// --- Rules over numbers ---

	/**
	 * The multiplier a critical hit from Kind deals: the revolver's own (Crit.Revolver), the sniper's (Crit.Sniper), the
	 * game's 1.5x for the others, but a cursed iron's own (CurseMultiplier, WeaponCurses::CritMultiplier) wins when larger.
	 */
	AI_LOOTER_SHOOTER_API float CritMultiplier(EWeaponKind Kind, float CurseMultiplier);
	AI_LOOTER_SHOOTER_API float CritMultiplier(EWeaponKind Kind, float CurseMultiplier, float RevolverCrit);
	AI_LOOTER_SHOOTER_API float CritMultiplier(EWeaponKind Kind, float CurseMultiplier, float RevolverCrit, float SniperCrit);

	/**
	 * The share of its damage a pellet from Kind still does after flying Distance: the shotgun's sharper falloff (full to
	 * Start times its range, down to Floor at End times it, the floor beyond); today's gentler FWeaponStats::DamageAtDistance
	 * for the others (full to the range, down to half at twice it).
	 */
	AI_LOOTER_SHOOTER_API float DamageShareAtDistance(EWeaponKind Kind, const FWeaponStats& Stats, float Distance);
	AI_LOOTER_SHOOTER_API float ShotgunFalloff(float Distance, float Range, float Start, float End, float Floor);

	/**
	 * The click in the magazine's last rounds: the pitch (1 up to Top at the last round) for a shot that left RoundsLeft of
	 * Magazine, or 0 for no click (above the last Share of the magazine, an empty magazine, a magazine under 4 rounds). The
	 * last Share of it, at least the last round.
	 */
	AI_LOOTER_SHOOTER_API float LowAmmoPitch(int32 RoundsLeft, int32 Magazine, float Share, float Top);
	AI_LOOTER_SHOOTER_API float LowAmmoPitch(int32 RoundsLeft, int32 Magazine);

	/**
	 * The hip bloom after a shot: Bloom (the extra share of spread now) plus PerShot (the kind's own), a quarter of that
	 * through the sight (AimAlpha 1), held to MaxMultiplier - 1. The automatic guns bloom (the rifle, the SMG and the LMG:
	 * Spread.RifleBloom, SMGBloom, LMGBloom in the live one); Bloom comes back unchanged for the others.
	 */
	AI_LOOTER_SHOOTER_API float BloomAfterShot(EWeaponKind Kind, float Bloom, float AimAlpha, float PerShot, float MaxMultiplier);
	AI_LOOTER_SHOOTER_API float BloomAfterShot(EWeaponKind Kind, float Bloom, float AimAlpha);
	/** Bloom after Seconds of not firing: it shrinks at Recovery a second (an exponential rate, as the aim's recovery is). */
	AI_LOOTER_SHOOTER_API float BloomRecovered(float Bloom, float Seconds, float Recovery);
	AI_LOOTER_SHOOTER_API float BloomRecovered(float Bloom, float Seconds);

	/** Rumble from numbers: the base intensity times the player's setting (0-1) times the global scale, held to 0-1. */
	AI_LOOTER_SHOOTER_API float RumbleStrength(float Base, float Setting, float Scale);

	/** A shot's rumble: how strong (0-1, before the setting) and for how long (s). */
	struct FRumble
	{
		float Intensity = 0.f;
		float Seconds = 0.f;
	};
	AI_LOOTER_SHOOTER_API FRumble RumbleForShot(EWeaponKind Kind);
	/** A hit taken for DamageShare of the player's health: the bigger the share, the stronger and longer (a quarter of the bar is the most). */
	AI_LOOTER_SHOOTER_API FRumble RumbleForHurt(float DamageShare, float HurtBase);
	AI_LOOTER_SHOOTER_API FRumble RumbleForHurt(float DamageShare);

	// --- The gamepad ---

	/** The last thing the player pressed or moved was a gamepad's (nothing rumbles on a mouse and keys). */
	AI_LOOTER_SHOOTER_API bool IsGamepadLastInput();
	/** Notes which device the last input came from (the watch calls it; a test sets it). */
	AI_LOOTER_SHOOTER_API void NoteInputDevice(bool bGamepad);
	/** Starts watching Slate's input for the last device used (once; harmless without Slate). */
	AI_LOOTER_SHOOTER_API void EnsureInputWatch();

	/**
	 * Rumbles Player's gamepad: only a local controller whose last input was a gamepad's, scaled by their Controller rumble
	 * setting and Rumble.Scale. A shot is the kind's; a hit taken by its share of health.
	 */
	AI_LOOTER_SHOOTER_API void RumbleShot(APlayerController* Player, EWeaponKind Kind);
	AI_LOOTER_SHOOTER_API void RumbleHurt(APlayerController* Player, float DamageShare);
}
