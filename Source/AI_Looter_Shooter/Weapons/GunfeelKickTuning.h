#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

class AWeaponBase;
class APlayerController;
class UWeaponDefinition;

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
		// The view kick by kind (gun feel pass, 2026-10-22): spread wider than before so the kinds read apart. Hip degrees of
		// a shot (ViewKicks::ForShot pitch x this): rifle 0.42, SMG 0.16, LMG 0.5, revolver 1.44, shotgun 2.19, sniper 2.6,
		// launcher 2.8; the sight halves them.
		inline constexpr float KickRifle = 1.3f;
		inline constexpr float KickShotgun = 1.9f;
		inline constexpr float KickRevolver = 1.6f;
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
		inline constexpr float KickSMG = 0.8f;
		inline constexpr float KickSniper = 2.f;
		inline constexpr float KickLMG = 1.2f;
		inline constexpr float KickLauncher = 2.f;
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

		// Gun feel pass (GF-KICK, 2026-10-22). The sight: the gun model kicks less up and twisting, more back (the view kick
		// already halves; the model's 11 degree revolver flip put the sights off target before the next pull).
		inline constexpr float AimedFlip = 0.45f;
		inline constexpr float AimedRoll = 0.4f;
		inline constexpr float AimedBack = 1.2f;
		/** The first shot of a burst (rifle, SMG) kicks the view this much harder: the pull is felt. */
		inline constexpr float KickFirstShot = 1.3f;
		/** The first shot after a rest (the rifle, SMG, LMG) has this share of the spread: tap fire is rewarded. */
		inline constexpr float SpreadFirstShot = 0.6f;
		inline constexpr float SpreadFirstShotRest = 0.35f;
		inline constexpr float SpreadFirstShotEase = 0.25f;
		/** The recoil pattern's strength (0 off, 1 as shipped), and the share of the old random sideways kick that stays with it. */
		inline constexpr float RecoilPattern = 1.f;
		inline constexpr float SideRandom = 0.4f;
		/** An automatic's summed shot kicks (degrees of view pitch) are held under this; a stacked kick keeps at least the floor's share. */
		inline constexpr float SustainCap = 1.2f;
		inline constexpr float SustainFloor = 0.25f;
		/** The rifle drifts right by RifleDrift degrees a shot from shot RifleDriftFrom on; the SMG shimmies +- SMGShimmy; the LMG sways. */
		// 0.1, was 0.04: at 0.04 twenty shots drifted 0.11 deg, too little to see (play-check 2026-10-23).
		inline constexpr float RifleDrift = 0.1f;
		inline constexpr float RifleDriftFrom = 6.f;
		inline constexpr float SMGShimmy = 0.03f;
		inline constexpr float LMGSway = 0.15f;
		inline constexpr float LMGSwayShots = 7.f;
		/** The shotgun's pump rack nudges the view this much (1 = ViewKicks::ForRack as it is). */
		inline constexpr float RackKick = 1.f;

		// Playtest round 2 (the user, 2026-10-27, note 28): "more kick on every gun; a high accuracy stat calms it a lot".
		/** Every gun's kick (aim, model, view, pattern) times this, before the accuracy's calm: the "more kick" for all. */
		inline constexpr float RecoilMore = 1.4f;
		/**
		 * The accuracy's calm: a gun shooting Ratio times tighter than its kind's base (the definition's spread over its own)
		 * kicks Ratio^-Exponent as hard, held between Least and Most. 1.5 so the stat clearly matters: a +30% accuracy gun
		 * kicks 0.67x, a 1.5x tight one 0.54x, twice as tight 0.35x; a sloppy roll (0.8) 1.4x.
		 */
		inline constexpr float AccuracyCalm = 1.5f;
		/** The calm's floor: a pinpoint gun still kicks this share (never a laser). */
		inline constexpr float AccuracyCalmLeast = 0.3f;
		/** The wild end's ceiling: the worst roll (about 0.64 of base accuracy) kicks at most this much (x RecoilMore = 1.96x the old kick). */
		inline constexpr float AccuracyCalmMost = 1.4f;
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

	// --- Recoil by accuracy (playtest round 2, note 28) ---

	/** The accuracy-recoil numbers (defaults are the shipped ones; the live ones are Looter.Gunfeel.Recoil.More, .AccuracyCalm*). */
	struct FAccuracyKick
	{
		float More = Default::RecoilMore;
		float Exponent = Default::AccuracyCalm;
		float Least = Default::AccuracyCalmLeast;
		float Most = Default::AccuracyCalmMost;
	};
	AI_LOOTER_SHOOTER_API FAccuracyKick AccuracyKickNumbers();
	/**
	 * The accuracy's calm on the kick: (BaseSpread / Spread)^-Exponent, held to Least..Most; 1 when either spread is unknown
	 * (0 or less). BaseSpread is the kind's own (the definition's BaseStats.Spread), Spread the gun's rolled one (rarity, parts,
	 * maker, curse): the tighter it shoots against its kind, the calmer it kicks, so a shotgun isn't judged by a rifle's cone.
	 */
	AI_LOOTER_SHOOTER_API float AccuracyCalm(float BaseSpread, float Spread, const FAccuracyKick& Numbers);
	/** The recoil a shot kicks with: the Recoil stat x More x AccuracyCalm. Every kick a shot makes reads this, not Stats.Recoil. */
	AI_LOOTER_SHOOTER_API float ShotRecoil(float RecoilStat, float BaseSpread, float Spread, const FAccuracyKick& Numbers);
	/** The live one for a gun's stats (Definition may be null: no calm, More only). */
	AI_LOOTER_SHOOTER_API float ShotRecoil(const UWeaponDefinition* Definition, const FWeaponStats& Stats);
	AI_LOOTER_SHOOTER_API float ShotRecoil(const AWeaponBase* Weapon);
	/**
	 * The recoil a card should show: the Recoil stat x the live AccuracyCalm, without More (so a plain gun still reads 100%
	 * and the number moves with accuracy). For the loadout's/bench's Recoil row.
	 */
	AI_LOOTER_SHOOTER_API float FeltRecoil(const UWeaponDefinition* Definition, const FWeaponStats& Stats);

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

	// --- Gun feel pass: pattern, first shot, sustain, the sight (GF-KICK; GunfeelKickPattern.cpp) ---

	/** The numbers of the recoil pattern (defaults are the shipped ones; tests pass their own). */
	struct FPatternNumbers
	{
		float RifleDrift = Default::RifleDrift;
		float RifleDriftFrom = Default::RifleDriftFrom;
		float SMGShimmy = Default::SMGShimmy;
		float LMGSway = Default::LMGSway;
		float LMGSwayShots = Default::LMGSwayShots;
	};
	/**
	 * The sideways aim kick (degrees, + right) a shot adds for its place in the burst (0 the first), so a burst has a shape to
	 * learn: the rifle straight up for RifleDriftFrom shots, then right by RifleDrift a shot; the SMG a shimmy, +- SMGShimmy
	 * alternating; the LMG a slow sway, LMGSway x sin over LMGSwayShots shots (its 11 shots a second make about 1.5 Hz). The
	 * other kinds none. The live one is also times Recoil.Pattern (0 turns it off).
	 */
	AI_LOOTER_SHOOTER_API float PatternSide(EWeaponKind Kind, int32 ShotInBurst, const FPatternNumbers& Numbers);
	AI_LOOTER_SHOOTER_API float PatternSide(EWeaponKind Kind, int32 ShotInBurst);
	/** What stays of the old random sideways aim kick: Recoil.SideRandom for the kinds with a pattern (rifle, SMG, LMG), whole for the rest and when Recoil.Pattern is 0. */
	AI_LOOTER_SHOOTER_API float SideRandomScale(EWeaponKind Kind);
	/** The view kick of a burst's first shot (ShotInBurst 0) times FirstShot for the rifle and the SMG; 1 otherwise. */
	AI_LOOTER_SHOOTER_API float FirstShotKick(EWeaponKind Kind, int32 ShotInBurst, float FirstShot);
	AI_LOOTER_SHOOTER_API float FirstShotKick(EWeaponKind Kind, int32 ShotInBurst);
	/** The cap on the summed shot kicks of an automatic (rifle, SMG, LMG): Kick.SustainCap degrees; 0 (none) for the single shots. */
	AI_LOOTER_SHOOTER_API float SustainCapFor(EWeaponKind Kind);
	AI_LOOTER_SHOOTER_API float SustainFloor();
	/**
	 * The gun model's kick through the sight: BackFlipRoll = (back, flip, roll) multipliers at AimAlpha (1 at the hip, easing
	 * to AimedBack/AimedFlip/AimedRoll at 1). The flip's share also scales the sideways twist. AimedModelKick(1, ...) fills the
	 * profile's full-sight numbers (AWeaponBase::GetRecoilProfile); FWeaponRecoil::AddShot blends them by the shot's AimAlpha.
	 */
	AI_LOOTER_SHOOTER_API void AimedModelKick(float AimAlpha, FVector& BackFlipRoll);
	AI_LOOTER_SHOOTER_API void AimedModelKick(float AimAlpha, float Back, float Flip, float Roll, FVector& BackFlipRoll);
	/**
	 * The bloom the next shot's spread uses: Bloom (the raw one) until the gun has rested Rest seconds, then easing over Ease
	 * seconds to FirstShot - 1 (negative: a tighter cone, 0.6 = 40% less) for the automatics (rifle, SMG, LMG). The first shot
	 * of a burst lands where it was aimed; holding the trigger blooms as before.
	 */
	AI_LOOTER_SHOOTER_API float FirstShotBloom(EWeaponKind Kind, float Bloom, float SecondsSinceShot, float Rest, float Ease, float FirstShot);
	AI_LOOTER_SHOOTER_API float FirstShotBloom(EWeaponKind Kind, float Bloom, float SecondsSinceShot);
	/** The shotgun's pump, racked between shots, nudges the local player's view (ViewKicks::ForRack x Kick.Rack). GF-RELOAD's pump calls it once per rack. */
	AI_LOOTER_SHOOTER_API void RackBeat(const AWeaponBase& Weapon);
	AI_LOOTER_SHOOTER_API float RackKickScale();

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
