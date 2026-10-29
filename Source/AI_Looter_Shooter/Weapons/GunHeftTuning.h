#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

/**
 * Gun identity, heft (GUNS.md lane B, 2026-10-28): how heavy the gun in the player's hands feels, one weight W per kind.
 * A heavy gun lags behind the mouse further and returns slower, bobs and breathes more, lags more into a jump and sinks
 * more at a landing, and the sight arrives with a small forward push; a light one does the opposite. Only the view model
 * (PlayerLocomotionViewModel.cpp) reads it: no shot, spread or aim point changes, and the aimed sight still sits on the
 * line (the view model settles the motion by AimSteadiness and corrects what is left with its Lock).
 *
 * The weights are Looter.Gunfeel.Heft.<Kind>; Looter.Gunfeel.Heft.Scale scales every weight's distance from 1 (0: no
 * heft), and Looter.Gunfeel.Identity 0 (GunfeelKick::IdentityOn) turns the lot back into today's feel exactly. The rules
 * are plain functions over numbers so tests drive them without a world or a variable the player may have set.
 */
namespace GunHeft
{
	namespace Default
	{
		// Light to heavy: revolver < SMG < rifle < shotgun < sniper < launcher < LMG.
		inline constexpr float WeightRevolver = 0.6f;
		inline constexpr float WeightSMG = 0.7f;
		inline constexpr float WeightRifle = 1.f;
		inline constexpr float WeightShotgun = 1.25f;
		inline constexpr float WeightSniper = 1.3f;
		inline constexpr float WeightLauncher = 1.5f;
		inline constexpr float WeightLMG = 1.6f;
		inline constexpr float Scale = 1.f;

		// Walking speed on foot by kind (Main's call, 2026-10-28): the walk's speed times this.
		inline constexpr float MoveRevolver = 1.05f;
		inline constexpr float MoveSMG = 1.05f;
		inline constexpr float MoveSniper = 0.97f;
		inline constexpr float MoveLMG = 0.95f;
		inline constexpr float MoveLauncher = 0.95f;

		// The rules' own constants.
		/** Look lag: today's mouse-turn gain, its clamps (degrees) and its return speed, at weight 1. */
		inline constexpr float LagGain = 0.012f;
		inline constexpr float LagClampYaw = 4.f;
		inline constexpr float LagClampPitch = 3.f;
		inline constexpr float LagInterp = 9.f;
		/** A weight W returns at LagInterp / W^this. */
		inline constexpr float LagInterpExponent = 0.8f;
		/** Bob and the idle breath grow by this share of (W - 1). */
		inline constexpr float BobShare = 0.35f;
		inline constexpr float BreathShare = 0.4f;
		/** The jump's lag into the kick spring, today's number at weight 1. */
		inline constexpr float JumpLag = 18.f;
		/** The sight-arrival push (cm, times W) and how long it takes to ease out. */
		inline constexpr float ArrivalPush = 0.3f;
		inline constexpr float ArrivalSeconds = 0.15f;
		/** The view model's weight follows the gun in hand at this rate (1/s), so a swap never pops the bob or lag. */
		inline constexpr float WeightFollowRate = 8.f;
	}

	/** Everything the view model reads for one weight: today's numbers times the weight's rule (weight 1, push 0 = today's feel). */
	struct FGunHeft
	{
		/** The weight itself (1 = today). */
		float Weight = 1.f;
		/** Look lag: the mouse-turn gain and both clamps are times the weight. */
		float LagGain = Default::LagGain;
		float LagClampYaw = Default::LagClampYaw;
		float LagClampPitch = Default::LagClampPitch;
		/** How fast the lag returns (1/s); a heavy gun's is lower. */
		float LagInterp = Default::LagInterp;
		/** The walking bob's depth, side and roll are times this. */
		float Bob = 1.f;
		/** The idle breath's drift and tilt are times this. */
		float Breath = 1.f;
		/** The jump's lag into the kick spring (positive; the spring is pushed down by it), and the landing's kick times this. */
		float JumpLag = Default::JumpLag;
		float LandKick = 1.f;
		/** The forward push (cm) as the sight arrives; 0 with heft off. */
		float ArrivalPush = 0.f;
	};

	/** The numbers for a weight; Share is Heft.Scale with the identity switch on (0 = off: push 0, the weight counts as 1). */
	AI_LOOTER_SHOOTER_API FGunHeft FromWeight(float Weight, float Share = 1.f);
	/** The forward push (cm) this long after the sight arrived: a quick rise, then an ease out; 0 once ArrivalSeconds has gone. */
	AI_LOOTER_SHOOTER_API float ArrivalAt(const FGunHeft& Heft, float SecondsSinceArrival);

	// --- The live numbers (the console variables' current values) ---

	/** Heft.Scale with Looter.Gunfeel.Identity on, else 0. 0 turns every rule back into today's feel. */
	AI_LOOTER_SHOOTER_API float Share();
	/** The kind's weight as set, before the scale (None: 1). */
	AI_LOOTER_SHOOTER_API float RawWeight(EWeaponKind Kind);
	/** The weight the view model works to for this kind now: 1 + (raw - 1) x Share. */
	AI_LOOTER_SHOOTER_API float Weight(EWeaponKind Kind);
	/** The numbers for the kind now (FromWeight of its weight). */
	AI_LOOTER_SHOOTER_API FGunHeft For(EWeaponKind Kind);
	/** The walking speed on foot times this for the kind (1 for the kinds with none, and with the identity switch off). */
	AI_LOOTER_SHOOTER_API float MoveScale(EWeaponKind Kind);
}
