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

	// --- The pump racked between shots (AWeaponBase::GetPumpCycle, WeaponBasePump.cpp) ---

	/** The rack's point where the fore-end is fully back: the hull is flung out of the port. */
	inline constexpr float PumpCycleBackAt = 0.45f;

	/** How far the fore-end is pulled back through its rack (CycleProgress 0 to 1; -1 none): snapped back, a beat, shoved home. */
	AI_LOOTER_SHOOTER_API float PumpCycleTravel(float CycleProgress);

	/** The first-person gun through the rack: it rocks back and dips with the stroke and bumps forward as it's shoved home. */
	AI_LOOTER_SHOOTER_API void PumpCycleViewPose(float CycleProgress, FVector& OutOffset, FRotator& OutRotation);
}
