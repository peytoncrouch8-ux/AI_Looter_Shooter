#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

/**
 * What makes each of the four frontier-tech guns (Phase 12.3, Saved/Loot/CONTRACT.md "GUNS-CODE") its own in the hand,
 * beside the kick and handling numbers every kind has (GunfeelKickTuning):
 *  - the Rustler SMG: its helix magazine glows with the rounds left, hot and pulsing once it's nearly dry;
 *  - the Longhorn sniper: the bolt cycles between shots (lifted, drawn back, the case out, run home), the scope sways
 *    while aimed until the player holds their breath (the sprint key while aimed) for a few steady seconds, a tight shot
 *    only through the scope (no spread mercy at the hip), and its crits hit hardest (GunfeelKick::CritMultiplier);
 *  - the Thresher LMG: it spins up as it fires (its rate climbs from a share to full) and its barrel glows with the heat
 *    of sustained fire;
 *  - the Powderkeg launcher: a rocket that flies and bursts (AWeaponRocket, WeaponRocketBlast), hurting its shooter a little.
 * Every other gun sways through its sights too, smaller than the scope (Looter.Gunfeel.Sway.*; a crouch steadies any gun).
 *
 * Every number is a Looter.Gunfeel.* console variable read through an accessor here, each time it's asked, so the player
 * tunes it live in play; the rules are plain functions over numbers (Default holds the shipped ones) for the tests.
 */
namespace GunKinds
{
	namespace Default
	{
		// The SMG's helix.
		inline constexpr float SMGGlow = 4.f;
		inline constexpr float SMGGlowLowShare = 0.25f;
		inline constexpr float SMGGlowPulse = 5.f;

		// The sniper.
		inline constexpr float SniperBoltDelay = 0.12f;
		inline constexpr float SniperBoltShare = 0.7f;
		inline constexpr float SniperBoltMaxSeconds = 0.85f;
		inline constexpr float SniperAimedSpread = 0.04f;
		inline constexpr float SniperSway = 0.3f;
		inline constexpr float SniperSwayRate = 0.35f;
		inline constexpr float SniperBreathSeconds = 3.f;
		inline constexpr float SniperBreathSteady = 0.1f;
		inline constexpr float SniperWindedSeconds = 2.5f;
		inline constexpr float SniperWindedSway = 1.8f;
		inline constexpr float SniperBreathRefill = 1.5f;

		// Every other gun's idle sway through its sights (playtest round 2, the user 2026-10-27, note 29: "an idle sway just
		// like the sniper does when you zoom in"), smaller than the scope's 0.3 degrees: light guns least, the heavy ones
		// more, the six-gun a little loose (held out at arm's length). Degrees at the widest, as SniperSway.
		inline constexpr float SwayRifle = 0.2f;
		inline constexpr float SwayShotgun = 0.18f;
		inline constexpr float SwayRevolver = 0.22f;
		inline constexpr float SwaySMG = 0.18f;
		inline constexpr float SwayLMG = 0.26f;
		inline constexpr float SwayLauncher = 0.24f;
		/** Their pace (Hz): a touch quicker than the scope's 0.35, lighter guns in the hands. */
		inline constexpr float SwayRate = 0.45f;
		/** A crouch steadies every gun's sway (the sniper's too) to this share. */
		inline constexpr float SwayCrouch = 0.5f;

		// The LMG.
		inline constexpr float LMGSpinUp = 0.35f;
		inline constexpr float LMGSpinStart = 0.6f;
		inline constexpr float LMGSpinDown = 0.5f;
		inline constexpr float LMGHeatPerShot = 0.012f;
		inline constexpr float LMGHeatDelay = 0.5f;
		inline constexpr float LMGHeatCool = 0.3f;
		inline constexpr float LMGHeatGlow = 6.f;

		// The launcher's rocket and its burst.
		inline constexpr float RocketSpeed = 4000.f;
		inline constexpr float RocketGravity = 120.f;
		inline constexpr float RocketLife = 5.f;
		inline constexpr float BlastRadius = 350.f;
		inline constexpr float BlastCore = 80.f;
		inline constexpr float BlastEdgeShare = 0.2f;
		inline constexpr float BlastSelfScale = 0.5f;
		inline constexpr float BlastSelfCap = 0.35f;
		inline constexpr float BlastKnockShare = 0.6f;

		// Kills by the new kinds (the creature's death, CreatureHitReactionComponent).
		inline constexpr float KillSniperBurst = 1.3f;
		inline constexpr float KillLauncherKnock = 900.f;
		inline constexpr float KillLauncherLift = 140.f;
	}

	// --- The live numbers (the console variables' current values) ---

	/** The SMG helix's glow strength, the share of the magazine under which it turns hot and pulses, and the pulse (Hz). */
	AI_LOOTER_SHOOTER_API float SMGGlow();
	AI_LOOTER_SHOOTER_API float SMGGlowLowShare();
	AI_LOOTER_SHOOTER_API float SMGGlowPulse();

	/** The bolt waits this long after a shot (the kick first), then cycles in this share of the time between shots (at most MaxSeconds). */
	AI_LOOTER_SHOOTER_API float SniperBoltDelay();
	AI_LOOTER_SHOOTER_API float SniperBoltShare();
	AI_LOOTER_SHOOTER_API float SniperBoltMaxSeconds();
	/** The sniper's spread through the scope, as a share of its hip spread (the other guns' is UPlayerViewComponent::AimSpreadMultiplier). */
	AI_LOOTER_SHOOTER_API float SniperAimedSpread();
	/** The scope's sway (degrees at its widest) and its pace (Hz). */
	AI_LOOTER_SHOOTER_API float SniperSway();
	AI_LOOTER_SHOOTER_API float SniperSwayRate();
	/**
	 * Kind's sway through its sights (degrees at its widest) and pace (Hz): the sniper's scope (Sniper.Sway, .SwayRate), every
	 * other kind its own smaller one (Looter.Gunfeel.Sway.<Kind>, one pace Sway.Rate; a gun with no kind is the rifle). 0 = none.
	 */
	AI_LOOTER_SHOOTER_API float AimSway(EWeaponKind Kind);
	AI_LOOTER_SHOOTER_API float AimSwayRate(EWeaponKind Kind);
	/** A crouch's share of the sway (Sway.Crouch). */
	AI_LOOTER_SHOOTER_API float SwayCrouch();

	/** The rules a held breath follows (FScopeBreath). */
	struct FBreathRules
	{
		/** Seconds a breath can be held. */
		float HoldSeconds = Default::SniperBreathSeconds;
		/** The sway's share while it's held. */
		float Steady = Default::SniperBreathSteady;
		/** Held to the end: winded this long, the sway this many times wider, no holding meanwhile. */
		float WindedSeconds = Default::SniperWindedSeconds;
		float WindedSway = Default::SniperWindedSway;
		/** Breath back per second of breathing (seconds of hold a second). */
		float Refill = Default::SniperBreathRefill;
	};
	AI_LOOTER_SHOOTER_API FBreathRules BreathRules();

	AI_LOOTER_SHOOTER_API float LMGSpinUp();
	AI_LOOTER_SHOOTER_API float LMGSpinStart();
	AI_LOOTER_SHOOTER_API float LMGSpinDown();
	AI_LOOTER_SHOOTER_API float LMGHeatPerShot();
	AI_LOOTER_SHOOTER_API float LMGHeatDelay();
	AI_LOOTER_SHOOTER_API float LMGHeatCool();
	AI_LOOTER_SHOOTER_API float LMGHeatGlow();

	/** The rocket's speed (cm/s), its fall (cm/s/s), and how long it flies before it bursts on its own (s). */
	AI_LOOTER_SHOOTER_API float RocketSpeed();
	AI_LOOTER_SHOOTER_API float RocketGravity();
	AI_LOOTER_SHOOTER_API float RocketLife();

	/** The burst: its reach and full-damage heart (cm), its edge's share, what its shooter takes of it, how far out the living are thrown. */
	struct FBlastRules
	{
		float Radius = Default::BlastRadius;
		float Core = Default::BlastCore;
		float EdgeShare = Default::BlastEdgeShare;
		float SelfScale = Default::BlastSelfScale;
		/** ...and never more than this share of their own health from one rocket (point-blank, it still never kills from full). */
		float SelfCap = Default::BlastSelfCap;
		float KnockShare = Default::BlastKnockShare;
	};
	AI_LOOTER_SHOOTER_API FBlastRules BlastRules();

	AI_LOOTER_SHOOTER_API float KillSniperBurst();
	AI_LOOTER_SHOOTER_API float KillLauncherKnock();
	AI_LOOTER_SHOOTER_API float KillLauncherLift();

	// --- Rules over numbers ---

	/** The helix's lit share: the rounds left of the magazine (0 empty, 1 full). */
	AI_LOOTER_SHOOTER_API float AmmoGlowFill(int32 Rounds, int32 Magazine);

	/**
	 * The helix's glow strength at Fill: Base while there's plenty; under LowShare it pulses at PulseHz (Time in seconds),
	 * between half and one and a half times Base, so the last rounds read without looking at the counter.
	 */
	AI_LOOTER_SHOOTER_API float AmmoGlowStrength(float Fill, float Base, float LowShare, float PulseHz, float Time);

	/** The LMG's rate as a share of its full one at Spin (0 cold, 1 spun up): Start at 0, full at 1. */
	AI_LOOTER_SHOOTER_API float SpinRateShare(float Spin, float Start);

	/** The spin Seconds later: up to full over UpSeconds while firing, down to nothing over DownSeconds when not. */
	AI_LOOTER_SHOOTER_API float SpinAfter(float Spin, float Seconds, bool bFiring, float UpSeconds, float DownSeconds);

	/** The LMG barrel's heat (0 cold to 1) after a shot. */
	AI_LOOTER_SHOOTER_API float HeatAfterShot(float Heat, float PerShot);

	/** The heat SinceShot seconds after the last shot: held for Delay, then cooling at Cool a second. */
	AI_LOOTER_SHOOTER_API float HeatCooled(float Heat, float SinceShot, float Delay, float Cool);

	/** When the bolt cycles after a shot: from Start seconds after it, for Seconds. */
	struct FBoltTiming
	{
		float Start = 0.f;
		float Seconds = 0.f;
	};

	/**
	 * The bolt's cycle for a gun that fires every Interval seconds: it waits Delay, then takes Share of what's left of the
	 * interval (at most MaxSeconds), so it's always home before the next shot can go.
	 */
	AI_LOOTER_SHOOTER_API FBoltTiming BoltTiming(float Interval, float Delay, float Share, float MaxSeconds);

	/** How far through its cycle the bolt is SinceShot seconds after a shot (0 to 1), or -1 before it starts and after it ends. */
	AI_LOOTER_SHOOTER_API float BoltCycleProgress(float SinceShot, const FBoltTiming& Timing);

	/**
	 * The scope's sway at Time (seconds aimed): a slow figure of eight, Amplitude degrees across at its widest (yaw), two
	 * thirds of that up and down at twice the pace, at Rate (Hz).
	 */
	AI_LOOTER_SHOOTER_API FRotator SwayAt(float Time, float Amplitude, float Rate);

	/**
	 * The sway's scale the aim eases toward: Breathing (the sniper's held breath, FScopeBreath::Step; 1 for the other guns)
	 * times CrouchShare while crouched. Never below 0.
	 */
	AI_LOOTER_SHOOTER_API float SwayShare(float Breathing, bool bCrouched, float CrouchShare);

	/** A held breath through the scope: how long it's been held, and how long the shooter is still winded. */
	struct AI_LOOTER_SHOOTER_API FScopeBreath
	{
		float Held = 0.f;
		float Winded = 0.f;
		bool bHolding = false;

		/** Dt seconds on, the shooter wanting to hold (bWant): returns the sway's scale now (Steady held, WindedSway winded, 1). */
		float Step(bool bWant, float Dt, const FBreathRules& Rules);

		/** The share of the breath still left to hold (1 full). */
		float Left(const FBreathRules& Rules) const;
	};

	/**
	 * The spread's scale for Kind at AimAlpha: the sniper's own aimed share (SniperAimed) through the scope, nothing at the
	 * hip (it never gets the others' halving), the others' Aimed share (UPlayerViewComponent's) as before.
	 */
	AI_LOOTER_SHOOTER_API float AimSpreadScale(EWeaponKind Kind, float AimAlpha, float Aimed, float SniperAimed);

	/** A burst's damage share at Distance from its middle: full within Core, down to EdgeShare at Radius, none past it. */
	AI_LOOTER_SHOOTER_API float BlastFalloff(float Distance, float Core, float Radius, float EdgeShare);
}
