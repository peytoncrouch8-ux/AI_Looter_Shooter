#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Creatures/CreatureStatusPose.h"
#include "CreatureStatusReactComponent.generated.h"

/**
 * A creature's answer to the element statuses on it (the "Elements matter" job, 2026-10-29): its status pose, eased frame
 * by frame (FCreatureStatusPoseTracker, read through CreatureStatusPose::Of), and its burn's panic run (ACreatureBase::
 * UpdateStatusBrain keeps the run's clock here). Added the first time a status takes it (UpdateStatusBrain) and kept;
 * it ticks on its own clock, so its pose eases on while the brain waits (a jolt's stagger is when its jerk shows), and
 * stops ticking once there's nothing on it and nothing left to ease.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UCreatureStatusReactComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCreatureStatusReactComponent();

	static UCreatureStatusReactComponent* Find(const AActor* Creature);
	static UCreatureStatusReactComponent* FindOrAdd(AActor& Creature);

	/** Its pose now (before Looter.Element.StatusPose: CreatureStatusPose::Of scales it). */
	const FCreatureStatusPose& GetPose() const { return Tracker.Get(); }

	/** Ticks again (a status took it); it stops on its own once all is at rest. */
	void Wake();

	/** Moves its pose on by DeltaSeconds from its owner's statuses (its tick calls it; the tests may). */
	void Advance(float DeltaSeconds);

	/** Everything off (a new life): the pose and the panic. */
	void ResetReactions();

	/** The burn's panic run, kept for ACreatureBase::UpdateStatusBrain. */
	struct FBurnPanic
	{
		/** Seconds left of the run (0: not running), and where it runs to. */
		float Left = 0.f;
		FVector To = FVector::ZeroVector;
		/** Seconds until it may panic again. */
		float CooldownLeft = 0.f;
		/** The burns on it at the last look: one more is a burn taking it, which rolls for a panic. */
		int32 LastBurns = 0;
	};
	FBurnPanic& GetPanic() { return Panic; }
	const FBurnPanic& GetPanic() const { return Panic; }
	bool IsPanicking() const { return Panic.Left > 0.f; }

	/** Its own dice for the panic's roll (seeded from its owner, so a level plays the same way each load). */
	FRandomStream& GetRolls() { return Rolls; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	FCreatureStatusPoseTracker Tracker;
	FBurnPanic Panic;
	FRandomStream Rolls;
};
