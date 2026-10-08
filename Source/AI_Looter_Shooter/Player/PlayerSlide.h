#pragma once

#include "CoreMinimal.h"

/**
 * A slide: crouching out of a sprint on the ground carries the player along the line they were running, 10% faster
 * than they went in, for a short fixed time, low enough to pass under anything a crouch fits under. Over its last
 * moments it eases to the speed the player goes on at (the sprint, when they hold forward; the crouched walk; or a stop),
 * so it hands over without a dip. It ends early if the player runs into something or leaves the ground. Pure rules
 * (starting, the speed over time, ending), so they are unit tested; UPlayerLocomotionComponent moves the character and
 * poses the body from them.
 */
struct FPlayerSlide
{
	/** The user's call (2026-10-07): a slide is 10% faster than the sprint that started it. */
	static constexpr float SpeedMultiplier = 1.1f;
	/** Seconds a slide lasts: about six and a half metres at the player's sprint. */
	static constexpr float Duration = 0.8f;
	/**
	 * Over its last this-many seconds it eases to the exit speed, on a smooth curve (no sudden braking as the ease starts
	 * or ends). Long enough that coming to a stop out of a slide is gentler than stopping out of a walk.
	 */
	static constexpr float EaseOutTime = 0.3f;
	/** Moving at under this share of what the last move allowed means it ran into something: it ends there. */
	static constexpr float StallShare = 0.5f;

	enum class EEnd : uint8 { None, Time, Stalled, Airborne, Cancelled };

	/** Only a sprint on the ground turns a crouch into a slide. */
	static bool CanStart(bool bSprinting, bool bOnGround) { return bSprinting && bOnGround; }

	/**
	 * Starts along the horizontal Velocity (Facing if standing still) at SpeedMultiplier times its speed. TopSpeed is the
	 * most the slide holds (the full sprint's speed times SpeedMultiplier).
	 */
	void Start(const FVector& Velocity, const FVector& Facing, float TopSpeed)
	{
		const FVector Flat(Velocity.X, Velocity.Y, 0.0);
		Direction = Flat.GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = FVector(Facing.X, Facing.Y, 0.0).GetSafeNormal();
		}
		StartSpeed = static_cast<float>(Flat.Size()) * SpeedMultiplier;
		Top = FMath::Max(TopSpeed, StartSpeed);
		Speed = Top;
		Elapsed = 0.f;
		bActive = true;
		LastEnd = EEnd::None;
	}

	/**
	 * Moves the slide on by DeltaTime, given how fast the character actually moved (horizontally) on the speed the slide
	 * last allowed, whether it is on the ground, and the speed the player will go on at once it ends (ExitSpeed, read
	 * each frame: the keys can change). Returns false once the slide is over.
	 */
	bool Advance(float DeltaTime, float ActualSpeed, bool bOnGround, float ExitSpeed)
	{
		if (!bActive)
		{
			return false;
		}
		Elapsed += DeltaTime;
		if (!bOnGround)
		{
			return Finish(EEnd::Airborne);
		}
		if (Elapsed >= Duration)
		{
			return Finish(EEnd::Time);
		}
		if (ActualSpeed < FMath::Min(StartSpeed, Speed) * StallShare)
		{
			return Finish(EEnd::Stalled);
		}
		Speed = SpeedAt(Elapsed, ExitSpeed);
		return true;
	}

	/** Ends it now (a jump, the player losing control). */
	void Stop()
	{
		if (bActive)
		{
			Finish(EEnd::Cancelled);
		}
	}

	/** The slide's speed at Time into it: its top speed, then over the last EaseOutTime a smooth ease to ExitSpeed. */
	float SpeedAt(float Time, float ExitSpeed) const
	{
		const float EaseStart = Duration - EaseOutTime;
		if (Time <= EaseStart)
		{
			return Top;
		}
		const float Alpha = FMath::Clamp((Time - EaseStart) / EaseOutTime, 0.f, 1.f);
		return FMath::Lerp(Top, FMath::Clamp(ExitSpeed, 0.f, Top), FMath::SmoothStep(0.f, 1.f, Alpha));
	}

	/** The most the slide lets the character move at on its next move. */
	float GetSpeed() const { return Speed; }
	bool IsActive() const { return bActive; }
	/** In the last EaseOutTime: the body starts coming up out of the slide pose. */
	bool IsEasingOut() const { return bActive && Elapsed > Duration - EaseOutTime; }
	const FVector& GetDirection() const { return Direction; }
	float GetStartSpeed() const { return StartSpeed; }
	/** The most it holds (its speed until it eases out). */
	float GetTopSpeed() const { return Top; }
	float GetElapsed() const { return Elapsed; }
	EEnd GetLastEnd() const { return LastEnd; }

private:
	bool Finish(EEnd Reason)
	{
		bActive = false;
		LastEnd = Reason;
		return false;
	}

	FVector Direction = FVector::ForwardVector;
	float StartSpeed = 0.f;
	float Top = 0.f;
	float Speed = 0.f;
	float Elapsed = 0.f;
	bool bActive = false;
	EEnd LastEnd = EEnd::None;
};
