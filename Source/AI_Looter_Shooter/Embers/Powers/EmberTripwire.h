#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberTripwire.generated.h"

class ACreatureBase;
class AEmberPropActor;

/** A wire laid out before it's strung: its two stakes' feet on the ground. */
struct AI_LOOTER_SHOOTER_API FTripwirePlan
{
	FVector A = FVector::ZeroVector;
	FVector B = FVector::ZeroVector;
	float Length = 0.f;
	bool IsValid(float MinLength) const { return Length >= MinLength; }
};

/**
 * Tripwire, Pike's Craft power: a wire strung between two stakes, standing for its duration (20 s).
 *  - Where (PlanWire): across the aim (square to it, flat), centred where the aim meets the ground within Reach (15 m),
 *    else on the ground FallbackAhead (4 m) in front of him; each half runs out to Range / 2 (6 m all told) in steps over
 *    the ground (stopping at a drop, a ledge or anything solid at the wire's height). Shorter than MinLength (1.5 m), or no
 *    ground at all, and the press is refused.
 *  - Two stakes knocked in (the fence's post, baked, scaled to a surveyor's stake: AEmberPropActor), the wire WireHeight
 *    (28 cm) over the ground between them, glinting in the power's colour; Ember.Tripwire.Set.
 *  - A creature crossing it (its feet's path from one frame to the next across the wire, or standing on it) trips, once per
 *    wire: Damage x the level's scale (30 at level 1; UEmberDamageType, no gun's notch), thrown down a short way along its
 *    going (UCreatureKnockComponent::Knock, TripThrow cm) and left lying stunned Stun (2 s) once it lands. A boss takes
 *    the hit, never the fall. The wire twangs (it shivers, Ember.Tripwire.Trip), dust where the creature goes down.
 *  - At the end (or cut short) the wire goes slack and the stakes are pulled (Ember.Tripwire.Slack).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberTripwire : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual EEmberDenied CanActivate(const FEmberContext& Context) const override;
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** Where a wire of up to Range from Character would be strung now (see the class comment; Length 0: nowhere). */
	static FTripwirePlan PlanWire(const ACharacter& Character, float Range);

	/** One frame of the wire: who crosses it trips (its Tick; the tests call it). */
	void StepWire(float DeltaSeconds);

	bool IsStrung() const { return bStrung; }
	const FTripwirePlan& GetPlan() const { return Plan; }
	/** The creatures it has tripped. */
	const TArray<TWeakObjectPtr<ACreatureBase>>& GetTripped() const { return Tripped; }

	static constexpr float DefaultDamage = 30.f;
	static constexpr float DefaultRange = 600.f;
	static constexpr float DefaultStun = 2.f;
	static constexpr float Reach = 1500.f;
	static constexpr float FallbackAhead = 400.f;
	static constexpr float MinLength = 150.f;
	static constexpr float WireHeight = 28.f;
	/** The wire is laid out in steps this long over the ground (cm). */
	static constexpr float StepLength = 25.f;
	/** How far a trip throws it (cm) and the arc's top. */
	static constexpr float TripThrow = 140.f;
	static constexpr float TripHeight = 45.f;
	static constexpr const TCHAR* StakePath = TEXT("/Game/Art/Props/SM_FencePost.SM_FencePost");

protected:
	virtual bool Linger(float DeltaSeconds) override;

private:
	void Trip(ACreatureBase& Creature, const FVector& Going);
	void PullStakes();
	FVector WireAt(float U) const;

	FTripwirePlan Plan;
	float Damage = DefaultDamage;
	float Stun = DefaultStun;
	bool bStrung = false;
	float Time = 0.f;
	/** The wire's shiver after a trip (seconds since; negative: still), and where along it. */
	float Twang = -1.f;
	float TwangAt = 0.5f;
	TWeakObjectPtr<AEmberPropActor> StakeA;
	TWeakObjectPtr<AEmberPropActor> StakeB;
	TMap<TWeakObjectPtr<ACreatureBase>, FVector> LastFeet;
	TArray<TWeakObjectPtr<ACreatureBase>> Tripped;
	/** After it's pulled: the slack wire falling (seconds since; negative: none). */
	float Slack = -1.f;
	EmberFX::FEmberSparks Sparks;
	FRandomStream Random{ 0x781e };
};
