#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

class AActor;
class UStaticMeshComponent;
class UWorld;
struct FRandomStream;

/**
 * An Unpaid's hat knocked off its head (HITS, 2026-10-28; the user's pick: by hard head hits and every head kill, lying
 * 6 s): it flies off along the shot and up, tumbling, falls, bounces once, lies a while, sinks into the ground and is gone
 * until the Unpaid comes back. A plain flight (no physics, no new mesh: the hat's own component moves), stepped with a
 * trace the caller gives it, so the tests fly it over a flat floor without a world.
 */
struct AI_LOOTER_SHOOTER_API FUnpaidHatFlight
{
	enum class EStage : uint8
	{
		/** On its head (or never knocked off). */
		OnHead,
		Flying,
		/** Lying where it came down. */
		Lying,
		/** Sinking into the ground, then gone. */
		Sinking,
		/** Hidden until the Unpaid comes back. */
		Gone
	};

	/**
	 * What's in the way of a move from From to To (world): true and the hit's point and surface normal when something is.
	 * Play traces world-static geometry (LooterWorld::StaticGeometryParams); the tests a flat floor.
	 */
	using FTrace = TFunctionRef<bool(const FVector& From, const FVector& To, FVector& OutPoint, FVector& OutNormal)>;

	EStage Stage = EStage::OnHead;
	/** Its pivot (world) and its turn; its velocity (cm/s), the axis it tumbles about and how fast (degrees a second). */
	FVector Location = FVector::ZeroVector;
	FQuat Turn = FQuat::Identity;
	FVector Velocity = FVector::ZeroVector;
	FVector SpinAxis = FVector::RightVector;
	float SpinSpeed = 0.f;
	/** Seconds in this stage, and how many times it has bounced. */
	float Time = 0.f;
	int32 Bounces = 0;
	/** How far its lowest point sits under its pivot (cm): it lies with that point on the ground. */
	float Brim = 0.f;
	/** Where it was knocked off (world): one that falls far below it went off a cliff and is gone. */
	float StartZ = 0.f;

	/**
	 * Knocked off now from Start (world, turned StartTurn) along Way (the shot's way, a unit vector): 350-500 cm/s along it
	 * (level), 250 up, tumbling at 540 degrees a second. Brim: its lowest point's distance under its pivot (cm).
	 */
	void Launch(const FVector& Start, const FQuat& StartTurn, const FVector& Way, float InBrim, FRandomStream& Dice);

	/** Dt seconds on: flying, lying or sinking (Trace finds the ground and walls). Whether it's still to be seen. */
	bool Step(float Dt, FTrace Trace);

	/** Off its head (flying, lying, sinking or gone). */
	bool IsOff() const { return Stage != EStage::OnHead; }

	/** Off its head and still to be seen. */
	bool IsShown() const { return Stage == EStage::Flying || Stage == EStage::Lying || Stage == EStage::Sinking; }

	/** Back on its head (a respawn). */
	void Reset() { *this = FUnpaidHatFlight(); }

	/** Down, it lies this long (s), then sinks this long (s) this far (cm). */
	static constexpr float LieSeconds = 6.f;
	static constexpr float SinkSeconds = 1.f;
	static constexpr float SinkDepth = 18.f;
	/** Flying longer than this without coming down (s), or falling this far below where it started (cm), it's gone. */
	static constexpr float MaxFlightSeconds = 3.f;
	static constexpr float MaxFall = 2500.f;
};

/** Knocking a real hat off and flying it (UnpaidHat.cpp): the creature's hat component and its flight. */
namespace UnpaidHat
{
	/** At most this many hats are off heads in a world at once (flying, lying, sinking): one more is just hidden, with a puff of dust. */
	inline constexpr int32 MaxOff = 4;

	/**
	 * Knocks Hat off along Way (a unit vector; zero: straight back off the head): detached, flying, counted against MaxOff (past
	 * it, hidden at once with a puff of dust). False when it isn't on a head to knock off (hidden, no mesh, off already).
	 */
	AI_LOOTER_SHOOTER_API bool KnockOff(UStaticMeshComponent& Hat, FUnpaidHatFlight& Flight, const FVector& Way, FRandomStream& Dice);

	/** Moves a knocked-off hat on by Dt (a trace for the ground each frame), and hides it once it's gone. Nothing on a head. */
	AI_LOOTER_SHOOTER_API void Tick(UStaticMeshComponent& Hat, FUnpaidHatFlight& Flight, float Dt);

	/** Forgets the flight (the Unpaid coming back): the caller puts the hat on its head again (AUnpaidCreature::PutOnHat). */
	AI_LOOTER_SHOOTER_API void PutBack(UStaticMeshComponent& Hat, FUnpaidHatFlight& Flight);

	/** Hats off heads in World now (flying, lying, sinking): the cap's count. */
	AI_LOOTER_SHOOTER_API int32 NumOff(const UWorld* World);
}
