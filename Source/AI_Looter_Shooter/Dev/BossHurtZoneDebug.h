#pragma once

#include "CoreMinimal.h"
#include "Bosses/BossAttackRules.h"
#include "Subsystems/WorldSubsystem.h"
#include "BossHurtZoneDebug.generated.h"

/**
 * Looter.Boss.HurtZones 1 (round 5; Bosses/BossAttackRules.h): draws every fighting boss's hurt zones in the world each frame
 * so its fairness can be tuned: grey through an attack's wind-up (the tell: where it will land), red while it can hurt,
 * nothing in its recovery. Discs on the ground (a quake, a stomp, a burst, a rock fall), wedges in front (a bite, a swing,
 * the buckshot's cone), lanes (a furrow's line, a lunge's path), each with its height. Each boss's line on the screen says
 * its last attack, how many in a row, and its idle time. Debug lines only: nothing in a shipping build, nothing when off.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UBossHurtZoneDebug : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The view is on (Looter.Boss.HurtZones). */
	static bool IsOn();

	/** Every boss's hurt zones in World now (its GatherHurtZones); how many bosses gave theirs. The view and the tests. */
	static int32 Gather(UWorld* World, TArray<FBossHurtZone>& Out);

	/** The colour a zone is drawn in: red live, grey in its wind-up; false for a window that isn't drawn. */
	static bool ColorFor(EBossAttackWindow Window, FColor& OutColor);

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Draws one zone for this frame. */
	static void Draw(UWorld* World, const FBossHurtZone& Zone);
};
