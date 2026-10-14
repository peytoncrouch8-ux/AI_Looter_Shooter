#pragma once

#include "CoreMinimal.h"
#include "Player/ViewEase.h"

/**
 * How the third-person body carries its gun right now, as plain numbers: shouldered at the crosshair (all zero), lowered
 * to a low-ready while walking or running at ease, or carried in a sprint (a long gun across the chest, a six-gun low at
 * the side in one hand). The body's Anim Blueprint works it out (ULooterCharacterAnimInstance), the view turns the gun by
 * it (LooterCarry::CarriedGun, from UPlayerViewComponent::GetHeldWeaponRotation) and the stance layer moves the hands and
 * turns them with the gun (LooterStancePoseCarry.cpp), so the gun never turns in the palm. First person has its own
 * camera-held gun and ignores it.
 */
struct FLooterCarry
{
	/** How far into the low-ready (0-1): muzzle dropped, the hands a little lower and closer. */
	float LowReady = 0.f;
	/** How far into the sprint's carry (0-1). */
	float Sprint = 0.f;
	/** A short gun (a six-gun, held out by its definition's HoldReach): one-handed at the side in a sprint, not across the chest. */
	bool bShortGun = false;
	/**
	 * How far a swap has the gun lowered (0-1, WeaponSwapMotion::BodyShare): the arm hanging at the side, muzzle to the ground,
	 * the other hand let go. The guns change places at the bottom; the next comes up the same way.
	 */
	float Swap = 0.f;

	/** How far the gun is from shouldered, every carry together (0: at the crosshair, 1: fully carried). */
	float Weight() const
	{
		return 1.f - (1.f - FMath::Clamp(LowReady, 0.f, 1.f)) * (1.f - FMath::Clamp(Sprint, 0.f, 1.f)) * (1.f - FMath::Clamp(Swap, 0.f, 1.f));
	}

	/** How far the support hand has let go of the gun (a six-gun in a sprint, any gun in a swap; its arm swings with the run instead). */
	float OneHanded() const { return FMath::Max(bShortGun ? FMath::Clamp(Sprint, 0.f, 1.f) : 0.f, FMath::Clamp(Swap, 0.f, 1.f)); }
};

/**
 * When the gun goes to the low-ready: once the player has gone LooterCarry::ReadyDelay seconds without aiming, shooting or
 * striking and is on the move, it eases down over LowerSeconds. Aiming, shooting or striking brings it back to the shoulder
 * in RaiseSeconds (the one deliberate quick move, whatever it was doing); standing still for StillGrace lets it settle back
 * up in SettleSeconds. On FViewEase's curve, so it never jolts. Pure, so it's unit tested.
 */
struct FLooterReadyClock
{
	/** One frame: bAlert (aiming, shooting or striking), bMoving (walking or running, LooterCarry::MovingSpeed). */
	void Update(float DeltaTime, bool bAlert, bool bMoving);

	/** 0 shouldered, 1 at the low-ready. */
	float GetAlpha() const { return FMath::Clamp(Ease.GetValue(), 0.f, 1.f); }

	/** Shouldered, as just drawn: the low-ready waits its full delay. */
	void Reset();

private:
	FViewEase Ease;
	float SinceAlert = 0.f;
	float StillFor = 0.f;
	/** The plan under way is an alert's quick raise. */
	bool bSnapping = false;
};

/**
 * The armed upper body's ride on the hips (ULooterCharacterAnimInstance's held pose sits on whatever the legs' clip does):
 * the run's quick bounce mostly smoothed out, a share of it kept so the body still looks alive, slow changes (a crouch, the
 * jog's lower hips) passed through. Component space, cm. Pure, so it's unit tested.
 */
struct FLooterBobFilter
{
	/** Where the upper body's root sits this frame, given where the hips carry it (Raw) and the seconds since the last. */
	FVector Update(const FVector& Raw, float DeltaTime);

	void Reset() { bStarted = false; }

	/** How quickly the smoothed spot follows the hips (seconds), and the share of the bounce taken out. */
	static constexpr float FollowSeconds = 0.15f;
	static constexpr float SmoothShare = 0.75f;
	/** The smoothed spot never trails the hips by more than this (cm): a landing's dip still carries the body down with it. */
	static constexpr float MaxLag = 4.f;

private:
	FVector Smoothed = FVector::ZeroVector;
	bool bStarted = false;
};

/** The carry's numbers and the gun's turn, shared by the view (the gun) and the stance layer (the hands). */
namespace LooterCarry
{
	/** No aim, shot or strike for this long (seconds) before the gun is lowered. */
	inline constexpr float ReadyDelay = 0.8f;
	/** Seconds to ease into the low-ready, to snap back to the shoulder, and to settle back up after stopping. */
	inline constexpr float LowerSeconds = 0.45f;
	inline constexpr float RaiseSeconds = 0.12f;
	inline constexpr float SettleSeconds = 0.35f;
	/** Standing still this long (seconds) raises the gun again; a stop that short (a strafe turning round) doesn't. */
	inline constexpr float StillGrace = 0.25f;
	/** On the move (world cm/s over the ground). */
	inline constexpr float MovingSpeed = 60.f;

	/** The low-ready's hands: this much lower, closer to the body and in toward its middle (cm, the full-size body's). */
	inline constexpr float LowDrop = 7.f;
	inline constexpr float LowBack = 4.f;
	inline constexpr float LowIn = 2.f;

	/**
	 * The sprint's right wrist from the right shoulder (cm, the full-size body's, in its facing): a long gun's low on the
	 * chest, in toward the middle, the gun a hand's width out in front of the belly...
	 */
	inline constexpr float ChestAhead = 20.f;
	inline constexpr float ChestIn = 9.f;
	inline constexpr float ChestDown = 30.f;
	/** ...a six-gun's low at the side, the arm hanging nearly straight, the hand a little ahead and out, clear of the thigh. */
	inline constexpr float SideAhead = 9.f;
	inline constexpr float SideOut = 7.f;
	inline constexpr float SideDown = 50.f;

	/** The low-ready's turn of the gun in its own frame: muzzle down 18 degrees, a little in toward the body's middle. */
	FRotator LowReadyTurn();

	/**
	 * The sprint's carry, in the frame Yaw (the facing) is in: a long gun across the chest, muzzle down and to the left, its
	 * top rolled a little toward the body; a six-gun hanging at the side, muzzle to the ground ahead of the foot.
	 */
	FRotator SprintHold(float Yaw, bool bShortGun);

	/** A swap's lowered gun, in the frame Yaw is in: hanging at the side, muzzle to the ground a little ahead and out. */
	FRotator SwapHold(float Yaw, bool bShortGun);

	/**
	 * The gun as carried: Aiming (shouldered at the crosshair, any kick in it) turned down to the low-ready, on to the
	 * sprint's carry and down to a swap's by Carry's shares. Yaw is the facing's, in Aiming's frame. The sprint's carry
	 * doesn't follow the aim's pitch (the torso doesn't either in a sprint), nor does a swap's.
	 */
	FQuat CarriedGun(const FQuat& Aiming, float Yaw, const FLooterCarry& Carry);
}
