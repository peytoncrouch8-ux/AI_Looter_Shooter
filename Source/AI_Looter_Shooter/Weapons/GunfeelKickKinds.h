#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponRecoil.h"
#include "Weapons/WeaponTypes.h"

class AWeaponBase;
class APlayerController;
class UCameraComponent;

/**
 * Gun identity (GUNS.md lane A, 2026-10-28), the kick side: each kind's own numbers (FKindFeel, edited in Project Settings >
 * Game > Gun Feel, UGunFeelSettings), the rules that read them, and the chain that turns a gun's asset profile into the shot's
 * (ShotProfile). One kick layer since 2026-11-03 (RECOIL, Docs/Work/2026-11-03-Phase3): Looter.Gunfeel.Identity no longer
 * switches the kick, the sight or the rhythm, only the sound, heft, brass and reload beats. Included by GunfeelKickTuning.h.
 */
namespace GunfeelKick
{
	namespace Default
	{
		/**
		 * While firing (until RecoveryDelay after the last shot) the aim settles back at this share of its rate: so little that
		 * a held burst climbs and wants pulling down (0.1, was 0.35); the delay is past the automatics' 92 ms rounds (0.12, was
		 * 0.09, which let one frame of full recovery in between every rifle round).
		 */
		inline constexpr float RecoveryWhileFiring = 0.1f;
		inline constexpr float RecoveryDelay = 0.12f;
		/** Through the sight the gun's push back stops at this share of the eye-to-sight distance (FWeaponRecoilProfile::AimedBackMost). */
		inline constexpr float AimedBackGap = 0.6f;
		/** The automatics' stacking view lift (ViewKicks::ForLift) times this (0 none). */
		inline constexpr float LiftKick = 1.f;
	}

	/**
	 * One kind's identity numbers. Through the sight (AimAlpha 1) its view kick keeps AimedTurn of the hip's turn and
	 * AimedFieldOfView of its widening and push, and its gun model AimedBack/Flip/Roll of the hip's push back, flip (and twist)
	 * and roll. AimKick: the aim's kick times this on the asset's AimKick (and AimKickSide) at a base Common, so the aim's
	 * number is the kind's own (the rifle 2.3: 0.35 x 2.3 = 0.8 degrees a round; the others 1.4, as before). Recovery times the
	 * asset's AimRecovery. Cadence: each shot's interval is +- this share (deterministic, the mean kept). AimCurve: the ADS
	 * ease's exponent (UPlayerViewComponent::GetAimAlpha); AimSettle: the sight-arrival kick times this (ViewKicks::ForAimSettle;
	 * 0 none). SustainCap: an automatic's stacked view lifts (degrees; 0 none). Shake/ShakeSeconds: a camera shake each shot
	 * (UCameraShakeModifier strength; 0 none). The defaults here are no identity at all (the hip's shares, no extra).
	 */
	struct FKindFeel
	{
		float AimedTurn = 1.f;
		float AimedFieldOfView = 1.f;
		float AimedBack = 1.f;
		float AimedFlip = 1.f;
		float AimedRoll = 1.f;
		float AimKick = 1.f;
		float Recovery = 1.f;
		float Cadence = 0.f;
		float AimCurve = 2.f;
		float AimSettle = 0.f;
		float SustainCap = 0.f;
		float Shake = 0.f;
		float ShakeSeconds = 0.f;
	};

	/**
	 * Kind's shipped identity (a gun with no kind is the rifle): what UGunFeelSettings starts each kind at. In order: aimed turn,
	 * aimed widening, aimed back, flip, roll, aim kick, recovery, cadence, ADS curve, settle, sustain cap, shake, shake seconds.
	 */
	constexpr FKindFeel KindFeelDefault(EWeaponKind Kind)
	{
		switch (Kind)
		{
		// A hornet in the hand: light, quick to the eye and quick to settle (recovery 2, was 1.2, so a hosed magazine climbs
		// about 3.5 degrees, half the rifle's: the cone opens rather than the climb).
		case EWeaponKind::SMG: return { 0.6f, 0.6f, 1.f, 0.35f, 0.3f, 1.4f, 2.f, 0.05f, 1.6f, 0.5f, 0.5f, 0.f, 0.f };
		// A mill wheel: slow to wake, rolls the shoulders, a shake held small under the belt's rattle.
		case EWeaponKind::LMG: return { 0.55f, 0.6f, 1.f, 0.4f, 0.35f, 1.4f, 0.9f, 0.03f, 2.6f, 1.6f, 1.6f, 0.035f, 0.15f };
		// The gunslinger's iron: a big flip at the hip, about 5 degrees through the irons (flip 0.45, was 0.7: 11 degrees put
		// the sights off target before the next pull), back on target before it.
		case EWeaponKind::Revolver: return { 0.55f, 0.6f, 1.1f, 0.45f, 0.6f, 1.4f, 1.25f, 0.f, 1.6f, 0.6f, 0.f, 0.f, 0.f };
		// A porch cannon: one shove through the shoulders.
		case EWeaponKind::Shotgun: return { 0.6f, 0.6f, 1.5f, 0.5f, 0.45f, 1.4f, 0.8f, 0.f, 2.2f, 1.3f, 0.f, 0.1f, 0.18f };
		// One thunderclap: the scope kept readable (a smaller share of the turn and the widening).
		case EWeaponKind::Sniper: return { 0.4f, 0.4f, 1.3f, 0.45f, 0.4f, 1.4f, 1.f, 0.f, 2.6f, 1.2f, 0.f, 0.12f, 0.3f };
		// A keg on the shoulder: a whoomph that heaves, the tube sagging back.
		case EWeaponKind::Launcher: return { 0.6f, 0.6f, 1.4f, 0.55f, 0.5f, 1.4f, 0.8f, 0.f, 2.4f, 1.5f, 0.f, 0.3f, 0.45f };
		// The workhorse (2026-11-03): 0.8 degrees a round on the aim that barely settles while held, so 30 rounds climb about
		// 6.5 degrees and drift right, and quick to come back once let go (recovery 2.3: a 3-round tap settles in 0.3 s).
		case EWeaponKind::Rifle:
		case EWeaponKind::None:
		default: return { 0.5f, 0.6f, 1.2f, 0.45f, 0.4f, 2.3f, 2.3f, 0.025f, 2.f, 1.f, 1.f, 0.f, 0.f };
		}
	}

	/** The kinds that fire bursts: a pattern, a climb, a first shot's reward, a stacking lift and its cap. */
	constexpr bool IsAutomaticKind(EWeaponKind Kind)
	{
		return Kind == EWeaponKind::Rifle || Kind == EWeaponKind::SMG || Kind == EWeaponKind::LMG;
	}

	// --- The live identity (GunfeelKickKinds.cpp) ---

	/**
	 * Looter.Gunfeel.Identity 1|0 [1]: every kind its own sound, heft, brass and reload beats; 0 gives back the shared ones for an
	 * A/B. The kick has one layer since 2026-11-03 and never asks it.
	 */
	AI_LOOTER_SHOOTER_API bool IdentityOn();
	/** Kind's identity numbers now: its UGunFeelSettings entry (Project Settings > Game > Gun Feel), held to sane ranges. */
	AI_LOOTER_SHOOTER_API FKindFeel KindFeel(EWeaponKind Kind);
	/** The view kick of a shot from Kind times this: Kick.<Kind> (1; the table holds each kind's weight). */
	AI_LOOTER_SHOOTER_API float ViewKickScale(EWeaponKind Kind);
	/**
	 * A shot's interval times this: 1 + Amount x a number in -1..1 that is a hash of Shot (the gun's shot count), so the two
	 * reads a shot's interval gets (the trigger's ready check, the auto-fire timer) agree and the mean holds (DPS unchanged).
	 */
	AI_LOOTER_SHOOTER_API float CadenceJitter(int32 Shot, float Amount);
	/** The live one for a gun of Kind at its Shot: its cadence; an LMG's only once fully spun up (Spin 1). */
	AI_LOOTER_SHOOTER_API float CadenceScale(EWeaponKind Kind, int32 Shot, float Spin);
	/** The LMG's dig-in at Spin: Start up to spin From, easing to End at full spin. */
	AI_LOOTER_SHOOTER_API float DigIn(float Spin, float From, float Start, float End);
	/** The live one (LMG.DigInStart/End/From). */
	AI_LOOTER_SHOOTER_API float LMGDigIn(float Spin);
	/** Seconds a sniper's aim kick holds before it recovers: Share of the bolt's work (BoltSeconds, until it is home), at most MaxSeconds. */
	AI_LOOTER_SHOOTER_API float RecoilHold(float BoltSeconds, float MaxSeconds, float Share);
	/** The live one (Recoil.SniperHold, Sniper.BoltMaxSeconds). */
	AI_LOOTER_SHOOTER_API float SniperRecoilHold(float BoltSeconds);

	/** A camera shake to add with a shot: its strength (UCameraShakeModifier::AddShake) and seconds; no strength, none. */
	struct FShotShake
	{
		float Strength = 0.f;
		float Seconds = 0.f;
	};
	/**
	 * The shake a shot from Kind adds: Feel's Shake for ShakeSeconds, through the sight (AimAlpha 1) Feel.AimedTurn of it; the
	 * LMG's only up to what keeps the shakes already going (Amount, UCameraShakeModifier::GetAmount) under LMGMax.
	 */
	AI_LOOTER_SHOOTER_API FShotShake ShotShake(EWeaponKind Kind, const FKindFeel& Feel, float AimAlpha, float Amount, float LMGMax);
	/** The live one (the kind's shake, Shake.LMGMax). */
	AI_LOOTER_SHOOTER_API FShotShake ShotShake(EWeaponKind Kind, float AimAlpha, float Amount);
	/** The launcher's heave, the sniper's bolt-home beat and the automatics' lift times these (Kick.Heave, Kick.BoltHome, Kick.Lift). */
	AI_LOOTER_SHOOTER_API float HeaveScale();
	AI_LOOTER_SHOOTER_API float BoltHomeScale();
	AI_LOOTER_SHOOTER_API float LiftScale();
	/**
	 * The sniper's bolt home after its cycle (AWeaponBase::UpdateBoltCycle): a small nudge of the local player's view
	 * (ViewKicks::ForBoltHome x Kick.BoltHome) and a tick of rumble on a gamepad. PlayerViewKicks.cpp.
	 */
	AI_LOOTER_SHOOTER_API void BoltHomeBeat(const AWeaponBase& Weapon);
	/**
	 * A beat's rumble (a bolt home, a magazine seated): Intensity (0-1) for Seconds on Player's gamepad, only a local player's
	 * whose last input was a gamepad's, x the Controller rumble setting and Rumble.Scale, both sides' small motors.
	 */
	AI_LOOTER_SHOOTER_API void RumbleBeat(APlayerController* Player, float Intensity, float Seconds);

	// --- A shot's recoil profile (WeaponBaseKick.cpp; AWeaponBase::GetRecoilProfile is its live use) ---

	/** Everything that turns a gun's asset profile into its shot's, for one kind (the shipped or the live numbers). */
	struct FProfileNumbers
	{
		FKindFeel Feel;
		/** The gun model's kick (back, flip, twist, roll) times this: Recoil.<Kind>. */
		float ModelWeight = 1.f;
		/** The random sideways aim kick's share that stays beside the pattern (SideRandomScale). */
		float SideRandom = 1.f;
		/** The model's springs' speed times this (the shotgun's Recoil.ShotgunSettle; 1 the rest). */
		float Settle = 1.f;
		/** A burst's climb per shot and its most (FWeaponRecoilProfile::ClimbPerShot, ClimbMax; none for single shots). */
		float ClimbPerShot = 0.f;
		float ClimbMax = 1.f;
		/** The LMG's climb cap and the share past it (GunKinds LMG.ClimbCap / .ClimbCapFloor; none for the rest). */
		float ClimbCap = 0.f;
		float ClimbCapFloor = 0.f;
		float WhileFiring = Default::RecoveryWhileFiring;
		float RecoveryDelay = Default::RecoveryDelay;
	};
	/** Kind's numbers as they ship (tests drive the chain with these). */
	AI_LOOTER_SHOOTER_API FProfileNumbers ShippedProfileNumbers(EWeaponKind Kind);
	/** Kind's numbers now (the Looter.Gunfeel.* variables and Project Settings > Game > Gun Feel). */
	AI_LOOTER_SHOOTER_API FProfileNumbers LiveProfileNumbers(EWeaponKind Kind);
	/**
	 * The shot's profile from the asset's: Kick (the shot's KickShare, x an LMG's dig-in) scales the model's kick (x ModelWeight)
	 * and the aim's (x Feel.AimKick, the side's x SideRandom too) alike, so Recoil.More and the gun's stats reach every layer
	 * once; the kind's sight shares, recovery, climb, cap and the recovery while firing are set; AimedBackMost is left as the
	 * asset has it (the view measures it per shot).
	 */
	AI_LOOTER_SHOOTER_API FWeaponRecoilProfile ShotProfile(const FWeaponRecoilProfile& Asset, float Kick, const FProfileNumbers& Numbers);

	/**
	 * How far the gun's sight sits ahead of Camera along its view (cm), for the aimed push-back's limit and the sight settle's
	 * pivot; 0 without a camera. PlayerViewKicks.cpp.
	 */
	AI_LOOTER_SHOOTER_API float SightGap(const UCameraComponent* Camera, const AWeaponBase& Weapon);
	/** The live limit on the aimed push back for a sight Gap cm ahead of the eye: Gap x Recoil.AimedBackGap (0 none). */
	AI_LOOTER_SHOOTER_API float AimedBackMost(float Gap);
}
