#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "BargeFurnace.generated.h"

class ABargeBoarding;
class UInstancedStaticMeshComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * The barge's soul furnace in the middle of its furnace hall (Main 6 steps 5-6, "Kill the furnace" and "Open the soul
 * cage": MAINS_2_7.md section 10): a 6 m riveted iron furnace, pipes up into the ceiling, and at its heart a glass-windowed
 * soul cage where Abel's light beats like a heart, flaring black at its edges (his soul burning as the barge's pilot light).
 * ABargeBoarding spawns it and keeps it in her frame (its origin the hall floor's middle, its +X the cage's window).
 *
 *  - The floor grate ring round it: twelve iron wedges that heat like a roulette wheel (BargeRules::HotMask: 4 of 12 glow a
 *    second, then burn, the pattern stepping a wedge every 3 s). A player on a burning wedge burns (a share of their health
 *    a second, in pulses). Each draft valve shut cools it a notch (4, 3, 2 hot), and dims the furnace's light and its heart;
 *    with all three shut the ring is cold and the cage can be opened.
 *  - The cage: holding Interact for HoldSeconds (3 s) at its window while OpenWhen holds (Main 6 step 6) and Sully carries
 *    Abel's lantern (AKeepersLantern::IsTakenIn) bursts it (Scenes/CageBurst). The missions hear the held Interact on the
 *    actor tagged Furnace_SoulCage. Burst, its heart is gone and the glass shattered; a load past step 6 finds it so.
 *
 * Stand-in look from engine shapes in the barge's iron (Tripo 33's furnace replaces the body once imported: BodyPath;
 * Tripo 34's grate wedge: GratePath). Its one light is shadowless. It ticks while the barge does (her tick moves it on).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ABargeFurnace : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ABargeFurnace();

	static const FName CageTag;
	static const TCHAR* const BodyPath;
	static const TCHAR* const GratePath;

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	/** Its barge (set as she spawns it). */
	void SetBarge(ABargeBoarding* InBarge);

	/** Moves its heart, its light and the ring on (the barge's tick). */
	void Advance(float DeltaSeconds);

	// --- The valves and the ring ---

	/** How many draft valves are shut (0-3): its light, its heart, the ring's heat. */
	void SetValvesShut(int32 Shut);
	int32 GetValvesShut() const { return ValvesShut; }

	/** The ring runs (hot wedges) only while this is on: the barge lit and the furnace fight not over. */
	void SetRingRunning(bool bRunning);
	bool IsRingRunning() const { return bRingRunning; }

	/** The ring's clock (s) and the wedges hot now (a bit each), and whether they burn (past the turn's tell). */
	float GetRingClock() const { return RingClock; }
	uint32 GetHotMask() const;
	bool IsRingBurning() const;

	// --- The cage ---

	/** It can be opened now: OpenWhen holds, the ring is cold, the cage whole and Abel's lantern carried (bForce: the story aside). */
	bool CanOpenCage(bool bForce = false) const;

	/** Bursts it: the CageBurst scene (or its end state with scenes off). False when it can't now. */
	bool BurstCage(AActor* By, bool bForce = false);

	/** The cage burst, now or on a load: its heart gone, the glass in shards, its light out. */
	void SetCageBurst(bool bBurst);
	bool IsCageBurst() const { return bCageBurst; }

	/** Every light of it out (the barge dying): the heart dark too. */
	void PutOut();

	/** Its cage's heart (world), the way its window faces (world, flat), and the hall's wall behind it (where Abel tears out). */
	FVector GetCageLocation() const;
	FVector GetCageFacing() const;
	FVector GetBackWall() const;

	/** Its opening's lines while it's shut and wanted: OpenWhen (Main 6 step 6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace")
	FStoryCondition OpenWhen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace", meta = (ClampMin = "0.1", Units = "s"))
	float HoldSeconds = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace")
	FText Prompt;

	/** Said when the cage is tried without the lantern (a nudge, not a lock without a reason). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace")
	FText NeedsLanternPrompt;

	/** Its light: lit, and each valve's notch off it (candelas), its reach (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace|Look")
	float LitCandelas = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Furnace|Look")
	float LightRadius = 2200.f;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Crown;

	/** The pipes up into the hall's ceiling and the riveted bands (one instanced look). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Ironwork;

	/** The cage's iron window frame on the furnace's face. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CageFrame;

	/** Abel's light in the cage: its gold-white heart, beating; the black of the furnace flaring round its edges. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Heart;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HeartSmog;

	/** The burst's glass and bars, flung out (shown only once it's burst). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Shards;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> CageLight;

	/** The grate ring: cold wedges, hot ones (glowing or burning), and the fire tongues over the burning. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> GrateCold;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> GrateHot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> GrateFire;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Lays the ring's wedges into the cold, hot and fire looks for Mask (only when it changes). */
	void LayGrates(uint32 Mask, bool bBurning);
	/** A wedge's transform in its own frame (the hall floor's middle), and its fire's. */
	FTransform WedgeTransform(int32 Wedge, float Up) const;
	void HurtOnGrates(float DeltaSeconds);
	bool StoryHolds(const FStoryCondition& Condition) const;

	TWeakObjectPtr<ABargeBoarding> Barge;
	int32 ValvesShut = 0;
	bool bRingRunning = false;
	bool bCageBurst = false;
	bool bOut = false;
	float RingClock = 0.f;
	float HeartClock = 0.f;
	float HurtClock = 0.f;
	uint32 LaidMask = 0xFFFFFFFFu;
	bool bLaidBurning = false;
	bool bSizzling = false;
};
