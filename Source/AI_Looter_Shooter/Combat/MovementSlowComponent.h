#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MovementSlowComponent.generated.h"

class ACharacter;

/**
 * A character's slows (a Gravebound Unpaid's shriek): while one lasts, it holds the character's walking speeds (walking,
 * sprinting and aiming alike, and crouching) down to a share of what they'd be, and gives them back when it ends. The
 * strongest slow on it wins, and a new one never cuts an old one short. Added to the character by the first slow (Apply)
 * and kept; it ticks only while it holds a slow.
 *
 * The player's locomotion sets the walking speed every frame (sprint, aim): this ticks after it and takes what it set as
 * the character's own speed, so the slow always comes on top. On a plain character it keeps the speed it found.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UMovementSlowComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMovementSlowComponent();

	/** Slows Victim's walking to SlowShare of its speed for Seconds, adding the component if it has none. Null without a victim. */
	static UMovementSlowComponent* Apply(ACharacter* Victim, float SlowShare, float Seconds);

	/** A slow on top of any it has: the stronger share, and the longer time left. */
	void AddSlow(float SlowShare, float Seconds);

	bool IsSlowed() const { return TimeLeft > 0.f; }

	/** Seconds of slow left (0: none). */
	float GetTimeLeft() const { return TimeLeft; }

	/** The share of its speed it walks at now (1: not slowed). */
	float GetMultiplier() const { return IsSlowed() ? Multiplier : 1.f; }

	/** Moves the slow on by DeltaSeconds: it holds the speeds down while it lasts and gives them back when it ends. Its tick calls it; tests call it. */
	void Advance(float DeltaSeconds);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void OnRegister() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Holds the speeds at the slow's share of the character's own (what anything else set since it last wrote them). */
	void HoldSpeeds();
	/** Gives back the speeds it held, unless something else has changed them since. */
	void Release();

	float Multiplier = 1.f;
	float TimeLeft = 0.f;
	bool bHolding = false;
	/** The character's own speeds (unslowed), and what this last wrote. */
	float OwnWalkSpeed = 0.f;
	float OwnCrouchedSpeed = 0.f;
	float WrittenWalkSpeed = 0.f;
	float WrittenCrouchedSpeed = 0.f;
};
