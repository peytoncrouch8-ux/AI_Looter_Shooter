#pragma once

#include "CoreMinimal.h"
#include "Weapons/ReloadTuning.h"
#include "Weapons/WeaponTypes.h"

/** A moment of a reload that makes a sound. */
enum class EReloadStep : uint8
{
	/** The old magazine comes free. */
	MagOut,
	/** The fresh one is slapped home. */
	MagIn,
	/** The charging handle. */
	Bolt,
	/** A shell pushed into the tube. */
	ShellIn,
	/** The pump racked. */
	Pump,
	/** The revolver's latch pushed and its cylinder swung out on the crane. */
	CylinderOut,
	/** The ejector rod punched: the six empties tumble out. */
	Eject,
	/** The speedloader's rounds dropped into the chambers and let go. */
	RoundsIn,
	/** The cylinder snapped home into the frame. */
	CylinderIn,
	// The four frontier-tech kinds (Phase 12.3; appended). Their cues are Weapon.<Kind>.<Step> (WeaponSounds::ReloadStep).
	/** The sniper's bolt turned up and drawn back. */
	BoltOpen,
	/** ...and run home and turned down. */
	BoltClose,
	/** The LMG's belt box comes free... */
	BoxOut,
	/** ...a fresh one hung on. */
	BoxIn,
	/** The belt's first rounds laid in the feed tray and the cover slapped shut. */
	BeltLay,
	/** The next rocket drawn from the back. */
	RocketOut,
	/** ...slid into the launcher's mouth until it seats. */
	RocketIn,
	/** The SMG's helix comes free... */
	HelixOut,
	/** ...a fresh one clicked home. */
	HelixIn
};

/** Where in a reload (its progress, 0 to 1) a step happens. */
struct FReloadStepAt
{
	float Progress;
	EReloadStep Step;
};

/** Where in a reload (its choreography progress) the first-person gun is knocked: a part seating, or a handle, bolt or pump slammed home. */
struct FReloadJoltAt
{
	float Progress;
	bool bSlam;
};

/**
 * The reload's spring curves (Phase 3 change 12, 2026-11-03; Docs/Work/2026-11-03-Phase3/RELOAD.md): plain math over a move's
 * window (T 0 to 1), each starting and ending at rest. An authored reload snaps: the hand cocks back before the slap, the
 * part overshoots and settles; every curve here was SmoothStep, so a slap was a soft symmetric bump. Shapes from
 * ReloadTuning::FSnapShape. The progress forms at the end are what the choreography and the hands call: with
 * Looter.Gunfeel.Reload.Snap 0 they are exactly the eased curves before.
 */
namespace ReloadCurves
{
	/**
	 * A move from 0 to 1: winds up to -Anticipate over Lead (eased, from rest to rest), swings to 1 + Overshoot on an
	 * underdamped spring's half swing (fast first, arriving at rest), then settles onto 1 over the last Settle, critically
	 * damped (no second swing). 0 from T 0 down, exactly 1 from T 1 up.
	 */
	AI_LOOTER_SHOOTER_API float Snap(float T, const ReloadTuning::FSnapShape& Shape);

	/**
	 * A move into a hard stop (a magazine seating, a rocket home, a pump or bolt bottoming): winds up as Snap does, then
	 * speeds up all the way into 1 and stops dead there at T 1 (the stop takes the overshoot; the gun's beat shows it).
	 * Never past 1.
	 */
	AI_LOOTER_SHOOTER_API float SnapInto(float T, const ReloadTuning::FSnapShape& Shape);

	/**
	 * A knock out and back (the gun as a part slaps home): 0, wound up to -Anticipate by T = Lead (the hit), kicked to 1 at
	 * BeatPeakAt, rebounding past 0 to -Overshoot, settled on 0 by T 1 (critically damped). 0 outside 0-1.
	 */
	AI_LOOTER_SHOOTER_API float SnapBeat(float T, const ReloadTuning::FSnapShape& Shape);
	AI_LOOTER_SHOOTER_API float BeatPeakAt(const ReloadTuning::FSnapShape& Shape);

	/**
	 * The gun recoil's spring knocked from rest (FWeaponRecoil::AddShot's impulse, in closed form so it is the same at any
	 * frame rate): 0 at Seconds 0 and before, 1 at its peak, a rebound past 0, and 0 again from JoltSeconds on.
	 */
	AI_LOOTER_SHOOTER_API float Jolt(float Seconds, float Hz, float Damping);
	/** When Jolt has rung down under 1% of its peak for good. */
	AI_LOOTER_SHOOTER_API float JoltSeconds(float Hz, float Damping);

	// --- Over a reload's progress, on the live Looter.Gunfeel.Reload.Snap* numbers ---

	/** Looter.Gunfeel.Reload.Snap is on. */
	AI_LOOTER_SHOOTER_API bool On();
	/** 0 before A, 1 from B: a Snap (on) or SmoothStep (off). */
	AI_LOOTER_SHOOTER_API float Rise(float P, float A, float B);
	/** Up A to B, held, down C to D, each a Rise (on, the fall overshoots past 0 and settles). */
	AI_LOOTER_SHOOTER_API float Window(float P, float A, float B, float C, float D);
	/** Into a stop A to B: a SnapInto (on; bWindUp false for a part that starts at a stop) or SmoothStep (off). */
	AI_LOOTER_SHOOTER_API float Into(float P, float A, float B, bool bWindUp);
	/**
	 * A knock whose hit is at At, HalfWidth either side as the old bump had: on, a SnapBeat over
	 * [At - Lead x 2 HalfWidth, At + (1 - Lead) x 2 HalfWidth] (wound up before the hit, rung out after it); off, the old
	 * symmetric bump peaking at At.
	 */
	AI_LOOTER_SHOOTER_API float Beat(float P, float At, float HalfWidth);
}

/**
 * One reload, choreographed over its progress (0 = started, 1 = done). The weapon moves its part from this and the
 * first-person view moves the whole gun from it, so the two always line up whatever the weapon's reload time.
 */
namespace LooterReload
{
	/**
	 * The reload's sounds where its motion makes them, in order: the magazine out, the fresh one seated, the charging
	 * handle; each shell pushed in, then the pump; or the cylinder out, the empties punched out, the fresh rounds in, the
	 * cylinder home. Empty for None.
	 */
	AI_LOOTER_SHOOTER_API TConstArrayView<FReloadStepAt> Steps(EWeaponReloadPart Part);

	/**
	 * How far the magazine is out of its well (cm, along the well's axis), and whether it can be seen: the old one
	 * drops out of sight before the fresh one comes up from below.
	 */
	AI_LOOTER_SHOOTER_API float MagazineTravel(float Progress, bool& bOutVisible);

	/** How far the pump is pulled back (cm). */
	AI_LOOTER_SHOOTER_API float PumpTravel(float Progress);

	/**
	 * How far the revolver's cylinder has swung out on its crane (degrees, out to the gun's left): it flicks open early,
	 * stays out through the ejecting and loading, and snaps home near the end with a little bounce.
	 */
	AI_LOOTER_SHOOTER_API float CylinderSwing(float Progress);

	/** The cylinder's swing when fully out (degrees), clear of the frame. */
	inline constexpr float CylinderOpenDegrees = 100.f;

	// --- The four frontier-tech kinds (Phase 12.3) ---

	/**
	 * How far out of its well the reload part of a gun of Part is (cm along its -Z) and whether it shows: the magazine's
	 * choreography (MagazineTravel) for the SMG's helix and the LMG's box, a little later for the sniper's box (its bolt
	 * opens first). 0 for the parts that don't slide out.
	 */
	AI_LOOTER_SHOOTER_API float WellTravel(EWeaponReloadPart Part, float Progress, bool& bOutVisible);

	/** The sniper's box comes out this much later in its reload than a rifle's magazine (its bolt is opened first). */
	inline constexpr float BoltMagazineLag = 0.06f;

	/** How open the sniper's bolt is in its reload (0 home, 1 turned up and drawn fully back): open at the start, home at the end. */
	AI_LOOTER_SHOOTER_API float BoltOpenInReload(float Progress);

	/**
	 * How open the bolt is through its cycle between shots (CycleProgress 0 to 1, GunKinds::BoltCycleProgress): turned
	 * up and snapped back, a beat there (the case flies), run home and turned down.
	 */
	AI_LOOTER_SHOOTER_API float BoltCycleOpen(float CycleProgress);

	/**
	 * How much of the trigger hand is on the bolt's knob (0 on the grip, 1 on the knob): through a bolt gun's reload, as
	 * the bolt is opened and again as it's run home; through its cycle between shots (CycleProgress; -1: none), all of it.
	 */
	AI_LOOTER_SHOOTER_API float BoltHandInReload(float Progress);
	AI_LOOTER_SHOOTER_API float BoltHandInCycle(float CycleProgress);

	/** The bolt's cycle point where it's fully back: the case is flung out of the port. */
	inline constexpr float BoltCaseOutAt = 0.42f;

	/** A bolt Open (0 to 1) as its part shows it: turned up about the bore first (degrees), then drawn back (cm). */
	AI_LOOTER_SHOOTER_API void BoltPose(float Open, float& OutLiftDegrees, float& OutBack);
	inline constexpr float BoltLiftDegrees = 60.f;
	inline constexpr float BoltBack = 8.5f;

	/**
	 * The launcher's reload: how far in front of the mouth the next rocket is (cm along the gun), whether it's in the hand
	 * and in view yet (bOutShown: brought up from the back, then slid in), and whether it has seated (bOutSeated: the
	 * loaded rocket's nose shows in the mouth from then on).
	 */
	AI_LOOTER_SHOOTER_API float RocketTravel(float Progress, bool& bOutShown, bool& bOutSeated);
	inline constexpr float RocketReach = 34.f;

	/** The first-person gun's motion through a bolt's cycle between shots (CycleProgress 0 to 1): the wrist rolls it as the bolt is worked. */
	AI_LOOTER_SHOOTER_API void BoltCycleViewPose(float CycleProgress, FVector& OutOffset, FRotator& OutRotation);

	/** The first-person gun's motion on top of its hold pose, in camera space (X forward, Y right, Z up; degrees). */
	AI_LOOTER_SHOOTER_API void ViewModelPose(EWeaponReloadPart Part, float Progress, FVector& OutOffset, FRotator& OutRotation);

	// --- A reload as planned (the gun feel pass, ReloadTuning; ReloadMotionPlan.cpp) ---
	// The functions above are a magazine reload from empty and the old four-shell pump reload; these follow a reload's
	// FReloadPlan (AWeaponBase::GetReloadPlan). Progress here is the reload's choreography progress
	// (AWeaponBase::GetReloadProgress, already through ChoreographyProgress), so the functions above read it as before.

	/** The rifle's charging handle (and the SMG's knob) is worked over this part of the magazine's choreography. */
	inline constexpr float HandleFrom = 0.70f;
	inline constexpr float HandleTo = 0.82f;

	/**
	 * Where a reload of Part is in its choreography at RawProgress (its share of the motion's time): a tactical magazine
	 * or helix reload skips the handle (HandleFrom jumps to HandleTo, where the gun, the magazine and the handle are all
	 * at rest, so nothing visibly jumps); every other reload's is its own.
	 */
	AI_LOOTER_SHOOTER_API float ChoreographyProgress(EWeaponReloadPart Part, const FReloadPlan& Plan, float RawProgress);

	/**
	 * The reload's sounds where its motion makes them, for Plan: tactical drops the handle (magazine, helix) or the bolt
	 * (the sniper: the box only); a pump gun pushes Plan.Shells shells (ReloadTuning's seconds) and pumps only from empty.
	 */
	AI_LOOTER_SHOOTER_API void StepsFor(EWeaponReloadPart Part, const FReloadPlan& Plan, TArray<FReloadStepAt>& OutSteps);

	/**
	 * Whether Step's seat puts the rounds in (Looter.Gunfeel.Reload.CommitAtSeat): the magazine, helix, box, rocket or
	 * speedloader home; each shell, one round, for a pump gun.
	 */
	AI_LOOTER_SHOOTER_API bool CommitsAt(EWeaponReloadPart Part, EReloadStep Step);

	/** The pump's travel through a planned shell reload (cm back): racked near its end, only from empty. */
	AI_LOOTER_SHOOTER_API float PumpTravel(float Progress, const FReloadPlan& Plan);

	/** The sniper's bolt through a planned reload: as BoltOpenInReload / BoltHandInReload from empty, home and untouched in a tactical one. */
	AI_LOOTER_SHOOTER_API float BoltOpenInReload(float Progress, const FReloadPlan& Plan);
	AI_LOOTER_SHOOTER_API float BoltHandInReload(float Progress, const FReloadPlan& Plan);

	/** The first-person gun through a planned reload: a pump gun's shells and pump as planned; a tactical sniper's without the bolt's work. */
	AI_LOOTER_SHOOTER_API void ViewModelPose(EWeaponReloadPart Part, float Progress, const FReloadPlan& Plan, FVector& OutOffset,
		FRotator& OutRotation);

	/** ...a planned shell reload's (Plan.Shells > 0): up for the shells, a nudge as each seats, rocked back by the pump. */
	AI_LOOTER_SHOOTER_API void PumpViewModelPose(float Progress, const FReloadPlan& Plan, FVector& OutOffset, FRotator& OutRotation);

	/** How far the pump is pulled when racked (cm). */
	inline constexpr float PumpStroke = 7.f;

	// --- The seat's jolt and the thrown magazine (Phase 3 change 12; ReloadMotionPlan.cpp, ReloadMotionSnap.cpp) ---

	/** The old magazine is clear of its well here (the magazine's choreography): the hand lets go and it falls (UWeaponMagazineDrop). */
	inline constexpr float MagazineClearAt = 0.28f;
	/** ...and the fresh one first shows under the well here. */
	inline constexpr float FreshMagazineAt = 0.46f;
	/** The sniper's bolt is home again here in a reload from empty. */
	inline constexpr float BoltHomeAt = 0.88f;

	/**
	 * Where a reload of Part as planned knocks the first-person gun, in order: each part seating (the magazine, helix, box,
	 * rocket, cylinder; not a shell), and from empty the charging handle or the belt cover home (HandleTo), the sniper's bolt
	 * home (BoltHomeAt), the pump shoved home.
	 */
	AI_LOOTER_SHOOTER_API void JoltsFor(EWeaponReloadPart Part, const FReloadPlan& Plan, TArray<FReloadJoltAt>& OutJolts);

	/** ChoreographyProgress backwards: the motion's share where a reload of Part reaches Progress (inside a skipped handle, its start). */
	AI_LOOTER_SHOOTER_API float RawProgressOf(EWeaponReloadPart Part, const FReloadPlan& Plan, float Progress);

	/**
	 * The first-person gun's knock RawProgress into a reload Motion seconds long: every seat and slam passed, rung on the
	 * gun recoil's spring (Rules' Hz times the gun's Snappiness), summed; in the gun's space, back (-X) and down (-Z) with
	 * the muzzle dipped. Zero with Looter.Gunfeel.Reload.Snap 0, or once each has rung out.
	 */
	AI_LOOTER_SHOOTER_API void JoltPose(EWeaponReloadPart Part, const FReloadPlan& Plan, float RawProgress, float MotionSeconds,
		float Snappiness, const ReloadTuning::FJoltRules& Rules, FVector& OutOffset, FRotator& OutRotation);

	// --- The pump racked between shots (AWeaponBase::GetPumpCycle, WeaponBasePump.cpp) ---

	/** The rack's point where the fore-end is fully back: the hull is flung out of the port. */
	inline constexpr float PumpCycleBackAt = 0.45f;

	/** How far the fore-end is pulled back through its rack (CycleProgress 0 to 1; -1 none): snapped back, a beat, shoved home. */
	AI_LOOTER_SHOOTER_API float PumpCycleTravel(float CycleProgress);

	/** The first-person gun through the rack: it rocks back and dips with the stroke and bumps forward as it's shoved home. */
	AI_LOOTER_SHOOTER_API void PumpCycleViewPose(float CycleProgress, FVector& OutOffset, FRotator& OutRotation);
}
