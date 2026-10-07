#pragma once

#include "CoreMinimal.h"

/**
 * A slide: crouching out of a sprint on the ground carries the player along the line they were running, 10% faster
 * than they went in, for a short fixed time, low enough to pass under anything a crouch fits under. It ends early if
 * the player runs into something or leaves the ground. Pure rules (starting, the speed over time, ending), so they are
 * unit tested; UPlayerLocomotionComponent moves the character and poses the body from them.
 */
struct FPlayerSlide
{
	/** The user's call (2026-10-07): a slide is 10% faster than the sprint that started it. */
	static constexpr float SpeedMultiplier = 1.1f;
	/** Seconds a slide lasts: about six and a half metres at the player's sprint. */
	static constexpr float Duration = 0.8f;
	/** Over its last this-many seconds it slows to the crouched walk instead of stopping dead. */
	static constexpr float EaseOutTime = 0.2f;
	/** Moving at under this share of the slide's speed means it ran into something: it ends there. */
	static constexpr float StallShare = 0.5f;

	enum class EEnd : uint8 { None, Time, Stalled, Airborne, Cancelled };

	/** Only a sprint on the ground turns a crouch into a slide. */
	static bool CanStart(bool bSprinting, bool bOnGround) { return bSprinting && bOnGround; }

	/**
	 * Starts along the horizontal Velocity (Facing if standing still) at SpeedMultiplier times its speed. TopSpeed is the
	 * most the slide holds (the full sprint's speed times SpeedMultiplier); ExitSpeed is what it eases down to at the end.
	 */
	void Start(const FVector& Velocity, const FVector& Facing, float TopSpeed, float InExitSpeed)
	{
		const FVector Flat(Velocity.X, Velocity.Y, 0.0);
		Direction = Flat.GetSafeNormal();
		if (Direction.IsNearlyZero())
		{
			Direction = FVector(Facing.X, Facing.Y, 0.0).GetSafeNormal();
		}
		StartSpeed = static_cast<float>(Flat.Size()) * SpeedMultiplier;
		Top = FMath::Max(TopSpeed, StartSpeed);
		ExitSpeed = FMath::Min(InExitSpeed, Top);
		Elapsed = 0.f;
		bActive = true;
		LastEnd = EEnd::None;
	}

	/**
	 * Moves the slide on by DeltaTime, given how fast the character actually moves now (horizontally) and whether it is
	 * on the ground. Returns false once the slide is over.
	 */
	bool Advance(float DeltaTime, float ActualSpeed, bool bOnGround)
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
		if (ActualSpeed < FMath::Min(StartSpeed, GetSpeed()) * StallShare)
		{
			return Finish(EEnd::Stalled);
		}
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

	/** The most the slide moves at right now: its top speed, easing down to the exit speed over the last EaseOutTime. */
	float GetSpeed() const
	{
		const float EaseStart = Duration - EaseOutTime;
		if (Elapsed <= EaseStart)
		{
			return Top;
		}
		return FMath::Lerp(Top, ExitSpeed, FMath::Clamp((Elapsed - EaseStart) / EaseOutTime, 0.f, 1.f));
	}

	bool IsActive() const { return bActive; }
	/** In the last EaseOutTime: the body starts coming up out of the slide pose. */
	bool IsEasingOut() const { return bActive && Elapsed > Duration - EaseOutTime; }
	const FVector& GetDirection() const { return Direction; }
	float GetStartSpeed() const { return StartSpeed; }
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
	float ExitSpeed = 0.f;
	float Elapsed = 0.f;
	bool bActive = false;
	EEnd LastEnd = EEnd::None;
};
