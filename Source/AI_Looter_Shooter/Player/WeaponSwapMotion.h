#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"

class AWeaponBase;

/**
 * How a swap looks and how long it takes, Borderlands' way: the gun in hand drops and tips away out of the bottom of the
 * view, the guns change places out of sight, and the next one swings up into place, a touch past its hold, and settles.
 *
 * One number says where a gun is on that path, Lowered: 0 at its hold, 1 out of view, below 0 a draw's overshoot. The
 * holster and the draw run the same path, each on its own clock (the holster's FWeaponSwapClock, which the weapon manager
 * runs; the draw's the gun's own ready time, which its first shot waits for), so a swap pressed mid-draw sets off down from
 * wherever the gun is, and one called off comes back up from wherever it got to: never a jump. The first-person view poses
 * the gun by it (PlayerLocomotionViewModel.cpp; the hands follow the gun), the third-person body lowers its arm by it
 * (FLooterCarry::Swap). Pure, so it's unit tested (Looter.Weapons.Swap.*).
 */

/** One kind of gun's swap. The view's pose is in camera space (X ahead, Y right, Z up; cm and degrees), about the grip. */
struct FWeaponSwapStyle
{
	/** Seconds a holster takes at Handling 1 (better handling, quicker: WeaponSwapMotion::HolsterSeconds). */
	float HolsterSeconds = 0.21f;
	/** The share of the draw spent swinging up (to just past the hold); the rest settles back onto it. */
	float RiseShare = 0.66f;
	/** How far past its hold the draw swings the gun, as a share of the way down. */
	float Overshoot = 0.07f;
	/** Out of view: dropped and tipped muzzle-down, turned and rolled out to the right. */
	FVector DownOffset = FVector::ZeroVector;
	FRotator DownRotation = FRotator::ZeroRotator;
	/** The draw's flourish as it swings past the hold, per unit of overshoot (the six-gun's flick of the wrist). */
	FRotator Flick = FRotator::ZeroRotator;
};

namespace WeaponSwapMotion
{
	/** A holster never takes less or more than this (seconds), however good or bad the gun's handling. */
	inline constexpr float MinHolsterSeconds = 0.14f;
	inline constexpr float MaxHolsterSeconds = 0.32f;
	/** A swap called off early still takes this share of a holster to come back up, so it never snaps. */
	inline constexpr float MinRaiseShare = 0.35f;
	/** A holster that sets off this far down is silent: the gun was still below the view, just drawn. */
	inline constexpr float QuietFrom = 0.8f;
	/** A swap called off this far down plays the gun's draw as it comes back up; less, and it never really went. */
	inline constexpr float ReturnHeardFrom = 0.5f;

	/** The kind's swap: the six-gun's, the pump shotgun's, or the bullpup's (which any other kind shares). */
	AI_LOOTER_SHOOTER_API const FWeaponSwapStyle& StyleOf(EWeaponKind Kind);

	/** A gun's kind by its definition (None without one). */
	AI_LOOTER_SHOOTER_API EWeaponKind KindOf(const AWeaponBase* Weapon);

	/** How long a whole holster of this kind takes with this handling (seconds). */
	AI_LOOTER_SHOOTER_API float HolsterSeconds(EWeaponKind Kind, float Handling);

	/** Where a draw has the gun Ready of the way through (0 just drawn, 1 up: the gun's GetReadyAlpha). */
	AI_LOOTER_SHOOTER_API float DrawLowered(EWeaponKind Kind, float Ready);

	/** Where a holster setting off from From has the gun Time of the way through: it answers the key at once, then falls away. */
	AI_LOOTER_SHOOTER_API float HolsterLowered(float From, float Time);

	/** The first-person gun's pose at Lowered, added to its hold about its grip. */
	AI_LOOTER_SHOOTER_API void ViewPose(EWeaponKind Kind, float Lowered, FVector& OutOffset, FRotator& OutRotation);

	/** How far the third-person arm lowers the gun at Lowered (0 carried as ever, 1 hanging at the side). */
	AI_LOOTER_SHOOTER_API float BodyShare(float Lowered);
}

/**
 * The holster's clock: the gun in hand on its way down out of view, or, the swap called off, back up from wherever it got
 * to (eased both ways: it turns round smoothly, then settles on its hold). Times are the world's (seconds), handed in, so
 * tests step it.
 */
struct AI_LOOTER_SHOOTER_API FWeaponSwapClock
{
	/** Sets the gun off down at Now from where it is (From: its draw's Lowered, or a return's), a whole holster taking Seconds. */
	void Lower(double Now, float From, float Seconds);

	/** Called off at Now: back up from where it is, in its share of a holster (MinRaiseShare at the least). */
	void Raise(double Now);

	/** Where the gun is at Now (0 when nothing runs). */
	float GetLowered(double Now) const;

	/** Seconds from Now until the gun is out of view (lowering) or back at its hold (raising). */
	float SecondsLeft(double Now) const;

	/** The leg under way has ended (within DoneSlack). */
	bool IsDone(double Now) const { return SecondsLeft(Now) <= DoneSlack; }

	bool IsRunning() const { return bRunning; }
	bool IsLowering() const { return bRunning && bLowering; }
	bool IsRaising() const { return bRunning && !bLowering; }

	void Reset() { *this = FWeaponSwapClock(); }

	/** A timer can wake a hair early: this close to the end counts as the end (seconds). */
	static constexpr float DoneSlack = 0.001f;

private:
	/** 0-1 through the leg under way. */
	float Progress(double Now) const;

	double Start = 0.0;
	/** Where the leg set off from, how long it takes, and how long a whole holster takes. */
	float From = 0.f;
	float Leg = 0.f;
	float Whole = 0.f;
	bool bLowering = false;
	bool bRunning = false;
};
