#pragma once

#include "CoreMinimal.h"
#include "Embers/Powers/EmberFX.h"
#include "Embers/Powers/EmberPowerBase.h"
#include "EmberSpyglass.generated.h"

class ACreatureBase;
class AEmberMarkActor;
class UCameraComponent;
class UMaterialInstanceDynamic;

/**
 * Spyglass, Pike's Sight power (Docs/Handoffs/CloudIslandConcepts_2026-10-05.md, "Ember powers"): for its duration (10 s)
 * every creature within Range (60 m) is marked, even through walls.
 *  - The sweep: a ring of cyan light runs out over the ground from Pike to Range in SweepSeconds (1.2 s); each creature is
 *    marked as it reaches it (a soft ping each, Ember.Spyglass.Mark), and any that comes within Range later is marked too.
 *  - Marked: it takes Strength (30%) more damage from everything (UHealthComponent::SetDamageTakenScale, "Spyglass"); its
 *    mark floats over it through walls (AEmberMarkActor: the hostile diamond in the power's orange and its distance); its
 *    body is washed orange where it's seen (the hit flash's overlay). A mark dies with its creature.
 *  - The lens: the view narrows a little and the screen's edges darken through the sweep (the cameras' vignette), then
 *    clear. Heard: Ember.Spyglass.Start (the brass drawn out), Ember.Spyglass.End (shut).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberSpyglass : public UEmberPowerBase
{
	GENERATED_BODY()

public:
	virtual void Activate(const FEmberContext& Context) override;
	virtual void Tick(const FEmberContext& Context, float DeltaSeconds) override;
	virtual void End(const FEmberContext& Context, bool bInterrupted) override;

	/** One frame of it: the sweep, new marks, marks of the dead let go (its Tick; the tests call it). */
	void StepSight(float DeltaSeconds);

	bool IsLooking() const { return bLooking; }
	/** How far the sweep's ring has run (cm). */
	float GetSweepRadius() const { return SweepRadius; }
	bool IsMarked(const ACreatureBase& Creature) const;
	int32 NumMarked() const { return Marked.Num(); }

	/** The source name it scales a marked creature's damage under (UHealthComponent::SetDamageTakenScale). */
	static const FName ScaleSource;

	static constexpr float DefaultRange = 6000.f;
	static constexpr float DefaultStrength = 0.3f;
	static constexpr float SweepSeconds = 1.2f;
	/** The view narrows this much (degrees) through the sweep, and the edges darken to this (the vignette's intensity). */
	static constexpr float LensDegrees = 5.f;
	static constexpr float LensVignette = 0.85f;

private:
	void Mark(ACreatureBase& Creature);
	void Unmark(ACreatureBase& Creature, bool bRemoveFromList);
	void SetLens(float Amount);

	struct FMarked
	{
		TWeakObjectPtr<ACreatureBase> Creature;
		TWeakObjectPtr<AEmberMarkActor> Mark;
	};
	TArray<FMarked> Marked;

	float Range = DefaultRange;
	float DamageScale = 1.f + DefaultStrength;
	bool bLooking = false;
	float Time = 0.f;
	float SweepRadius = 0.f;
	float PingClock = 0.f;
	int32 Pings = 0;
	float RecheckClock = 0.f;
	/** The lens as last set (0 to 1), and the cameras it set it on, with their own vignette before. */
	float Lens = 0.f;
	struct FLensCamera
	{
		TWeakObjectPtr<UCameraComponent> Camera;
		bool bHadOverride = false;
		float Vignette = 0.4f;
	};
	TArray<FLensCamera> Lensed;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MarkLook;
	EmberFX::FEmberTint Washed;
	FRandomStream Random{ 0x5b1a };
};
