#pragma once

#include "CoreMinimal.h"
#include "Story/StoryCondition.h"
#include "Subsystems/WorldSubsystem.h"
#include "CarriedLantern.generated.h"

class APawn;
class ASoulFollowers;
class UHudCarriedLanternWidget;

/**
 * Abel carried home in his own lantern (Main 7, "Given to the Wind", step 4 on: MAINS_2_7.md section 11, "Carry Abel home to
 * Saint Ada"; lane MAIN67's CONTRACT67.md). From FreeAbel's end, when Sully takes the lit lantern down off the keeper's post,
 * until it's set on the Reliquary (AReliquaryLantern), Abel's light rides at Sully's hip. The Company men who bailed out of
 * the barge want it: their hose-men latch the lantern, not Sully (UDredgerHoseComponent asks IsCarried), and each line on
 * it drains his light (CarriedLanternRules: never out; nothing fails); free of lines a moment, it fills again. A small HUD
 * meter (UHudCarriedLanternWidget, the LooterUI kit, bottom middle) shows "ABEL'S LIGHT" while it's carried: it dims and
 * pulses while drained.
 *
 * It also keeps the souls freed from the barge's fallen coal (ASoulCoalLump) as a trail of lights behind the lantern
 * (ASoulFollowers, spawned the first time one is freed), which waits at the Keeper's Gate through Abel's fight and rises
 * west with the six dead when Saint Ada wakes.
 *
 * A world subsystem rather than an actor: nothing is placed for it, and the hose-men, the HUD, the Reliquary and the scenes
 * find it the same way in any level. It ticks only while the lantern is carried, drained, shown or followed.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UCarriedLanternSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UCarriedLanternSubsystem* Get(const UObject* WorldContextObject);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	// --- The lantern ---

	/** Abel's lantern is at Sully's hip now: the story's step (CarryWhen), or forced (the console, the tests), and not set down. */
	bool IsCarried() const;

	/** Forces it carried (1), not (0), or back to the story's word (-1). */
	void ForceCarried(int32 Mode);

	/** Set on the Reliquary (AReliquaryLantern): carried no more this visit, the meter goes. */
	void SetDown();
	bool IsSetDown() const { return bSetDown; }

	/** His light now (0-1, never under CarriedLanternRules::MinLight while carried). */
	float GetLight() const { return Light; }
	void SetLight(float InLight);

	/** A line takes this share now (a burst's knock, the console). */
	void Drain(float Share, AActor* By);

	/** A hose-man's line holds the lantern (bLatched) or let go. The first latch for good says Finch's word. */
	void SetLatched(AActor* By, bool bLatched);
	int32 NumLatched() const;
	bool IsDraining() const { return NumLatched() > 0; }

	/** Where the lantern hangs on Carrier (world): their right hip, where a hose's line holds it. */
	static FVector LanternPoint(const APawn* Carrier);

	/** Moves the light, the meter and the trail on (the tick's work; a test level never ticks, so tests call it). */
	void Step(float DeltaSeconds);

	// --- The freed souls (ASoulFollowers) ---

	/** A soul freed at From joins the trail (bAtOnce: already there, a load). Returns how many follow now. */
	int32 AddFollower(const FVector& From, bool bAtOnce = false);
	int32 NumFollowers() const;
	ASoulFollowers* GetFollowers() const { return Followers; }

	/** The Gravewind takes them west (SaintAdaWakes), with Extra lights rising from those points too (the six dead). */
	void ReleaseFollowers(const TArray<FVector>& Extra);

	/** When the lantern is carried by the story (Main 7 from step 4 until Saint Ada wakes). */
	FStoryCondition CarryWhen;

private:
	bool StoryHolds(const FStoryCondition& Condition) const;
	void UpdateWidget(float DeltaSeconds);
	ASoulFollowers* EnsureFollowers();

	int32 ForcedMode = -1;
	bool bSetDown = false;
	float Light = 1.f;
	float SinceFree = 10.f;
	/** The meter's pulse clock. */
	float Clock = 0.f;
	/** Read a few times a second (the story's condition walks the campaign record). */
	mutable float CarriedCheckLeft = 0.f;
	mutable bool bCarriedCached = false;

	TArray<TWeakObjectPtr<AActor>> Latched;

	UPROPERTY(Transient)
	TObjectPtr<ASoulFollowers> Followers;

	UPROPERTY(Transient)
	TObjectPtr<UHudCarriedLanternWidget> Meter;
};
