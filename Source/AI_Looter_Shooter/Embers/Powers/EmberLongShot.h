#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberLongShot.generated.h"

class ACreatureBase;
class AEmberMarkActor;
class UBulletSubsystem;
struct FBulletLanding;
struct FBulletShot;
struct FHitResult;

/**
 * Long Shot, Pike's second Sight power: for its duration (8 s) his shots hit harder the farther they fly, and the creatures
 * he has marked are always critically hit.
 *  - Distance: a shot of his deals 1 + Strength (0.15) for every 10 m it has flown, up to 1 + MaxBonus (1: double at
 *    about 67 m), on top of the gun's own falloff (UBulletSubsystem::OnBulletLanding scales it before it's rolled).
 *  - Marks: on use, the living creatures in his sights (within ConeDegrees, 30, of the aim, within Range, 60 m, in sight;
 *    the nearest the crosshair first, at most MaxMarks, 8) are marked; so is any creature a shot of his hits from MarkFrom
 *    (25 m) or farther. A marked creature hit by his shot is always critical (the gun's critical multiplier), its mark
 *    flaring. Marks float over them through walls (AEmberMarkActor, the diamond in the power's teal and the distance).
 *  - Felt: Ember.LongShot.Start (a breath held, a scope's ring turned); each hit of his a far tink pitched up by the
 *    distance (Ember.LongShot.Hit), teal sparks at the hit as big as the bonus.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberLongShot : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** A shot landing (OnBulletLanding; the tests call it): the player's scaled by its distance, critical on a marked creature. */
	void NoteLanding(const FBulletShot& Shot, const FHitResult& Hit, FBulletLanding& Landing);

	/** A shot's damage scale after flying Distance cm (1 + Strength per 10 m, held to 1 + MaxBonus). */
	float ScaleAt(float Distance) const;

	bool IsSteady() const { return bSteady; }
	bool IsMarked(const ACreatureBase& Creature) const;
	int32 NumMarked() const { return Marks.Num(); }
	/** Marks a creature now (the use's sights and long hits; public for the tests). */
	void MarkCreature(ACreatureBase& Creature);

	static constexpr float DefaultStrength = 0.15f;
	static constexpr float DefaultMaxBonus = 1.f;
	static constexpr float DefaultRange = 6000.f;
	static constexpr float DefaultCone = 30.f;
	static constexpr float DefaultMarkFrom = 2500.f;
	static constexpr int32 MaxMarks = 8;

protected:
	virtual bool Linger(float DeltaSeconds) override;

private:
	void Bind(bool bOn);
	void MarkInSights();

	struct FMarkOn
	{
		TWeakObjectPtr<ACreatureBase> Creature;
		TWeakObjectPtr<AEmberMarkActor> Tag;
	};
	TArray<FMarkOn> Marks;

	TWeakObjectPtr<UBulletSubsystem> Bullets;
	FDelegateHandle LandingHandle;
	float Strength = DefaultStrength;
	float MaxBonus = DefaultMaxBonus;
	float Range = DefaultRange;
	float Cone = DefaultCone;
	float MarkFrom = DefaultMarkFrom;
	bool bSteady = false;
	double LastTink = -10.0;
	EmberFX::FEmberSparks Sparks;
	FRandomStream Random{ 0x109a };
};
