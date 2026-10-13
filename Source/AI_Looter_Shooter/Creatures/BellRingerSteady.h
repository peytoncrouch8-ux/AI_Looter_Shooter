#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BellRingerSteady.generated.h"

class ABellRingerRing;
class ACreatureBase;

/**
 * A Bell-Ringer's toll steadying one of its pack (ABellRingerCreature, when there's none fallen to raise): for a few
 * seconds it chases faster and attacks sooner, a faint pale ring pulsing at its feet (ABellRingerRing's aura). Added to
 * the creature by the toll and gone by itself, putting its pace back: a second toll while it lasts only makes it last
 * longer, never stacks. Its pace goes back by division, so whatever else changed the creature's speed meanwhile stays.
 */
UCLASS(ClassGroup = (Looter))
class AI_LOOTER_SHOOTER_API UBellRingerSteady : public UActorComponent
{
	GENERATED_BODY()

public:
	UBellRingerSteady();

	/**
	 * Steadies Creature for Seconds: its chase InSpeedScale times as fast, the wait between its attacks InCooldownScale as
	 * long (a living creature only). The one it has already just lasts Seconds again. Returns it, or null.
	 */
	static UBellRingerSteady* Apply(ACreatureBase& Creature, float Seconds, float InSpeedScale, float InCooldownScale);

	/** The steadying on Creature, if it has one. */
	static UBellRingerSteady* Find(const ACreatureBase& Creature);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Moves it on by DeltaSeconds: when its time runs out (or its creature dies) it ends. Its tick calls it; the tests call it. */
	void Advance(float DeltaSeconds);

	float GetTimeLeft() const { return TimeLeft; }
	bool IsSteadying() const { return bApplied; }

	/** The aura's pale soul-light. */
	static FLinearColor AuraColor();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Begin(float Seconds, float InSpeedScale, float InCooldownScale);
	/** Puts the creature's pace back and lets go of the aura; then the component goes. */
	void End();
	void PutPaceBack();

	float TimeLeft = 0.f;
	float SpeedScale = 1.f;
	float CooldownScale = 1.f;
	bool bApplied = false;
	TWeakObjectPtr<ABellRingerRing> Aura;
};
