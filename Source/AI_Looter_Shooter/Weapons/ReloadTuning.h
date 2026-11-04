#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

/**
 * One reload as planned when it starts: from empty or not (an empty one is slower: the charging handle, the bolt or the
 * pump is worked too), and for a pump gun how many shells it pushes in (re-planned when a trigger press cuts it short).
 */
struct FReloadPlan
{
	/** The chamber was empty when it started (no round left to fire). */
	bool bEmpty = false;
	/** Shells a pump gun pushes in, one at a time (0 for any other reload). */
	int32 Shells = 0;
};

/**
 * The reload's numbers (the gun feel pass, Docs/Work/2026-10-22-Gunfeel/PLAN.md "GF-RELOAD"), every one a live
 * Looter.Gunfeel.* console variable read when asked, and the plain rules over them:
 *  - a tactical reload (rounds still in) is quicker than an empty one, by kind (Apex's way; the user's 2026-10-22 yes);
 *  - the rounds are in once the magazine seats (a swap after that keeps them), and the gun is ready a beat before the
 *    reload's motion ends (Destiny's way);
 *  - the Ranchhand loads shell by shell, each a round as it seats, pumps only from empty, and a trigger press finishes
 *    the shell in hand and fires; between shots it racks its pump (the user's 2026-10-22 yes).
 * The pump gun's seconds are its base ones times its reload scale (its rolled ReloadTime over its definition's), so
 * Reload parts and makers still count.
 */
namespace ReloadTuning
{
	namespace Default
	{
		// Each kind's reload time scale, tactical / empty.
		inline constexpr float TacticalRifle = 0.9f;
		inline constexpr float EmptyRifle = 1.1f;
		inline constexpr float TacticalSMG = 0.9f;
		inline constexpr float EmptySMG = 1.1f;
		inline constexpr float TacticalSniper = 0.9f;
		inline constexpr float EmptySniper = 1.f;
		inline constexpr float TacticalLMG = 1.f;
		inline constexpr float EmptyLMG = 1.f;
		inline constexpr float TacticalRevolver = 1.f;
		inline constexpr float EmptyRevolver = 1.f;
		inline constexpr float TacticalLauncher = 1.f;
		inline constexpr float EmptyLauncher = 1.f;

		inline constexpr bool CommitAtSeat = true;
		inline constexpr float ReadyEarly = 0.08f;

		// The shotgun's shells (base seconds).
		inline constexpr float ShotgunStart = 0.3f;
		inline constexpr float ShotgunShell = 0.42f;
		inline constexpr float ShotgunPump = 0.35f;
		inline constexpr bool ShotgunInterrupt = true;

		// The rack between shots: this share of the time between shots, within Min..Max seconds, after Delay (the kick first).
		inline constexpr float RackShare = 0.45f;
		inline constexpr float RackMin = 0.22f;
		inline constexpr float RackMax = 0.5f;
		inline constexpr float RackDelay = 0.1f;

		// The spring slaps (Phase 3 change 12, 2026-11-03; ReloadCurves in ReloadMotion.h): a beat winds up Anticipate of its
		// move the other way over Lead of its window, springs past by Overshoot and settles over the last Settle of it. Off
		// (Snap false) every curve is the SmoothStep before.
		inline constexpr bool Snap = true;
		inline constexpr float SnapAnticipate = 0.25f;
		inline constexpr float SnapOvershoot = 0.12f;
		inline constexpr float SnapLead = 0.2f;
		inline constexpr float SnapSettle = 0.4f;

		// The seat's and the slam's jolt on the first-person gun and the hands on it (cm at its peak, back and down; the
		// muzzle's dip in degrees per cm of it), on the gun recoil's spring: 9 Hz times the gun's Snappiness, damping 0.55
		// (FWeaponRecoil's), so it rings out in about 0.13 s.
		inline constexpr float SnapJolt = 1.f;
		inline constexpr float SnapJoltSeat = 1.8f;
		inline constexpr float SnapJoltSlam = 2.2f;
		inline constexpr float SnapJoltPitch = 0.6f;
		inline constexpr float SnapJoltHz = 9.f;
		inline constexpr float SnapJoltDamping = 0.55f;

		// The thrown magazine (UWeaponMagazineDrop): lies this long (s) before it sinks away.
		inline constexpr bool DropMag = true;
		inline constexpr float DropMagRest = 20.f;

		/** The shipped tactical / empty scale for Kind (1 for the shotgun: its time is its shells). */
		AI_LOOTER_SHOOTER_API float TimeScale(EWeaponKind Kind, bool bEmpty);
	}

	/**
	 * A spring slap's shape (ReloadCurves): a move of 1 winds up Anticipate the other way over the first Lead of its window,
	 * then springs to 1 + Overshoot and settles back to 1 over the last Settle of it (critically damped: no second swing).
	 */
	struct FSnapShape
	{
		float Anticipate = Default::SnapAnticipate;
		float Overshoot = Default::SnapOvershoot;
		float Lead = Default::SnapLead;
		float Settle = Default::SnapSettle;

		/** The same without its wind-up: a part already at a stop (a pump forward, a bolt home) can't move the other way first. */
		FSnapShape NoWindUp() const
		{
			FSnapShape Shape = *this;
			Shape.Anticipate = 0.f;
			Shape.Lead = 0.f;
			return Shape;
		}
	};

	/** The seat's and the slam's jolt (cm, degrees per cm, the spring's Hz before the gun's Snappiness, its damping), times Scale. */
	struct FJoltRules
	{
		float Scale = Default::SnapJolt;
		float Seat = Default::SnapJoltSeat;
		float Slam = Default::SnapJoltSlam;
		float Pitch = Default::SnapJoltPitch;
		float Hz = Default::SnapJoltHz;
		float Damping = Default::SnapJoltDamping;
	};

	/** The shotgun's beats in base seconds: getting the gun up and the first shell from the belt, each shell, the pump. */
	struct FShellTiming
	{
		float Start = Default::ShotgunStart;
		float Shell = Default::ShotgunShell;
		float Pump = Default::ShotgunPump;
	};

	/** The rack between shots: Start seconds after the shot it begins, and takes Seconds. */
	struct FRackTiming
	{
		float Start = 0.f;
		float Seconds = 0.f;
		float End() const { return Start + Seconds; }
	};

	struct FRackRules
	{
		float Share = Default::RackShare;
		float Min = Default::RackMin;
		float Max = Default::RackMax;
		float Delay = Default::RackDelay;
	};

	// --- The live numbers ---

	/** Looter.Gunfeel.Reload.Tactical.<Kind> / .Empty.<Kind>: the kind's reload time scale (1 for the shotgun and None). */
	AI_LOOTER_SHOOTER_API float TimeScale(EWeaponKind Kind, bool bEmpty);
	/** Looter.Gunfeel.Reload.CommitAtSeat: the rounds go in as the magazine seats (else only once the reload is done). */
	AI_LOOTER_SHOOTER_API bool CommitAtSeat();
	/** Looter.Gunfeel.Reload.ReadyEarly: the gun can fire this long (s) before the reload's motion ends. */
	AI_LOOTER_SHOOTER_API float ReadyEarly();
	/** Looter.Gunfeel.Reload.Shotgun.Start / .Shell / .Pump. */
	AI_LOOTER_SHOOTER_API FShellTiming ShellTiming();
	/** Looter.Gunfeel.Reload.Shotgun.Interrupt: a trigger press cuts a shell reload short (after the shell in hand). */
	AI_LOOTER_SHOOTER_API bool ShotgunInterrupt();
	/** Looter.Gunfeel.Shotgun.RackShare / .RackMin / .RackMax / .RackDelay. */
	AI_LOOTER_SHOOTER_API FRackRules RackRules();
	/** Looter.Gunfeel.Reload.Debug: log every reload's plan, times, commit and finish to LogLooter. */
	AI_LOOTER_SHOOTER_API bool Debug();
	/** Looter.Gunfeel.Reload.Snap [1]: the reload's beats spring (anticipation, overshoot, settle) and the seats jolt the gun; 0 = the eased curves before. */
	AI_LOOTER_SHOOTER_API bool SnapOn();
	/** Looter.Gunfeel.Reload.Snap.Anticipate / .Overshoot / .Lead / .Settle, kept to shapes that still start and end at rest. */
	AI_LOOTER_SHOOTER_API FSnapShape SnapShape();
	/** Looter.Gunfeel.Reload.Snap.Jolt (scale; 0 none) / .JoltSeat / .JoltSlam / .JoltPitch / .JoltHz / .JoltDamping. */
	AI_LOOTER_SHOOTER_API FJoltRules JoltRules();
	/** Looter.Gunfeel.Reload.DropMag [1]: a magazine, helix or box reload throws the old one out as a falling piece. */
	AI_LOOTER_SHOOTER_API bool DropMagOn();
	/** Looter.Gunfeel.Reload.DropMag.Rest [20]: seconds a thrown magazine lies before it sinks away. */
	AI_LOOTER_SHOOTER_API float DropMagRest();

	// --- The plain rules ---

	/** Shells a pump gun pushes in: what its tube lacks, as many as the pool has. */
	AI_LOOTER_SHOOTER_API int32 ShellsNeeded(int32 Magazine, int32 MagazineSize, int32 Reserve);

	/** The plan for a reload of a gun whose reload works Part, starting with Magazine of MagazineSize in and Reserve in the pool. */
	AI_LOOTER_SHOOTER_API FReloadPlan PlanFor(EWeaponReloadPart Part, int32 Magazine, int32 MagazineSize, int32 Reserve);

	/** How much a gun's parts and maker move its reload: its rolled ReloadTime over its definition's base (1 without one). */
	AI_LOOTER_SHOOTER_API float GunScale(float ReloadTime, float BaseReloadTime);

	/** A shell reload's length in base seconds: Start, a Shell each, the Pump only from empty. */
	AI_LOOTER_SHOOTER_API float ShellReloadSeconds(const FReloadPlan& Plan, const FShellTiming& Timing);

	/** When the Nth shell (from 0) seats, in base seconds: the middle of its turn, where the thumb pushes it home. */
	AI_LOOTER_SHOOTER_API float ShellSeatSeconds(int32 Shell, const FShellTiming& Timing);

	/**
	 * A reload's whole motion (seconds, before the holder's time scale): a pump gun's shells times its GunScale; any
	 * other its ReloadTime times its kind's KindScale (tactical or empty).
	 */
	AI_LOOTER_SHOOTER_API float MotionSeconds(EWeaponReloadPart Part, const FReloadPlan& Plan, float ReloadTime, float GunScaleValue,
		float KindScale, const FShellTiming& Timing);

	/** When the gun can fire again, seconds into a reload of Motion: Early before its end, never before half of it. */
	AI_LOOTER_SHOOTER_API float ReadySeconds(float Motion, float Early);

	/**
	 * A trigger press SecondsIn (base seconds) into a shell reload: the shells it keeps, those seated and the one in
	 * hand (the first one if the hand is still on its way to the belt), never more than planned.
	 */
	AI_LOOTER_SHOOTER_API int32 ShellsKept(const FReloadPlan& Plan, float SecondsIn, const FShellTiming& Timing);

	/**
	 * The rack after a shot, ShotInterval seconds before the gun can fire again: Share of it within Min..Max, starting
	 * Delay after the shot (or sooner, so it's home before the next shot can go).
	 */
	AI_LOOTER_SHOOTER_API FRackTiming RackTiming(float ShotInterval, const FRackRules& Rules);
	AI_LOOTER_SHOOTER_API FRackTiming RackTiming(float ShotInterval);
}
