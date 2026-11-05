#pragma once

#include "CoreMinimal.h"
#include "Creatures/EncounterSpawner.h"
#include "RappelSpawner.generated.h"

class ACompanyTenderDrop;
class UInstancedStaticMeshComponent;

/**
 * An encounter whose creatures come down ropes (the Rappel entrance: Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 15):
 * Vane's Dredgers sliding off a tender hanging over the chapel yard (Main 4's BellAnswered: AnchorTag names the tender, an
 * ACompanyTenderDrop, whose rope points move as it bobs) or down the barge's hose into the Sink (Main 5's IntakeDrop:
 * RopeTops, fixed points beside the hose). Its creatures are spawned at their feet's spots as any encounter's are; each is
 * then lifted to its rope's top (the ropes taken in turn, a wave's men down each one after another), held there passive,
 * and slid down over SlideSeconds (a quick drop, a brake at the bottom, a little sway), its rope drawn from the top to its
 * hands; on the ground it lands (dust, a bark) and comes for the player (bHuntOnSpawn). Killed on the rope, it falls.
 *
 * With a tender, its first wave waits for the tender to hover (the player's approach counts only then; a trigger, the
 * console's or a mission's, still starts it at once). Its ropes stay down while the tender hovers, or RopeLingerSeconds
 * after the last man lands. It ticks only while someone is on a rope or a rope still hangs.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ARappelSpawner : public AEncounterSpawner
{
	GENERATED_BODY()

public:
	ARappelSpawner();

	virtual void Tick(float DeltaSeconds) override;

	/** The tender (by its tag) its ropes hang from. None: RopeTops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Rappel")
	FName AnchorTag;

	/** Its ropes' tops (world) without a tender: beside the barge's hose. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Rappel")
	TArray<FVector> RopeTops;

	/** A slide from a rope's top to the ground (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Rappel", meta = (ClampMin = "0.3", Units = "s"))
	float SlideSeconds = 2.2f;

	/**
	 * Each man takes the rope nearest the spot he lands on (the barge's droppers, whose ropes hang over their own landings:
	 * the spawner may skip a spot near the player or too near another, and in turn he could slide from one face to
	 * another's landing); a drop's second on a rope waits behind the first. Off: the ropes taken in turn (Mains 4-5).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Rappel")
	bool bRopeNearestFeet = false;

	/** The ropes hang on this long (s) after the last man lands (without a tender to hang from). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Rappel", meta = (ClampMin = "0", Units = "s"))
	float RopeLingerSeconds = 6.f;

	/** Someone is on a rope, or waiting at its top. */
	bool IsSliding() const { return Sliders.Num() > 0; }

	/** The ropes it has now (the tender's points, or RopeTops). */
	int32 NumRopes() const;
	/** Rope Index's top now (world). */
	FVector GetRopeTop(int32 Index) const;

	/** The rope a drop's Turn-th man (from 0) takes for his landing at Feet: the nearest of Tops (flat), or Turn's in turn. Pure. */
	static int32 PickRope(TConstArrayView<FVector> Tops, const FVector& Feet, int32 Turn, bool bNearest);

	/** How far down a slide Time seconds into Seconds is (0 the top, 1 the ground): quick at first, braked at the bottom. Pure. */
	static float SlideAlpha(float Time, float Seconds);

	/** Moves its slides and its ropes on (the tick does; a test level never ticks, so tests call it). */
	void AdvanceRopes(float DeltaSeconds);

protected:
	virtual void BeginPlay() override;
	virtual bool CheckApproach() override;
	virtual void OnCreatureSpawned(ACreatureBase& Creature) override;
	virtual bool HasOwnEntrance() const override { return true; }

private:
	/** One man on (or waiting at) a rope: the rope, where he lands, how far into his slide (negative: still waiting). */
	struct FSlider
	{
		TWeakObjectPtr<ACreatureBase> Creature;
		int32 Rope = 0;
		FVector Feet = FVector::ZeroVector;
		float Clock = 0.f;
		float Sway = 0.f;
	};

	ACompanyTenderDrop* FindTender() const;
	/** Puts Slider's man where his slide has him; true once he's down. */
	bool StepSlider(FSlider& Slider, float DeltaSeconds);
	/** He's down: walking again, the dust, a bark, and on the player. */
	void Land(ACreatureBase& Creature, const FVector& Feet);
	/** The ropes drawn: from each top to whoever is on it, or to the ground while they still hang. */
	void PlaceRopes();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Ropes;

	TArray<FSlider> Sliders;
	TWeakObjectPtr<ACompanyTenderDrop> Tender;
	/** The spawn's turn (spawns this close together are one drop), and when the last came. */
	int32 Turn = 0;
	double LastSpawn = -1000.0;
	/** Seconds the ropes have hung since the last man landed; their feet (one a rope). */
	float RopesHung = 0.f;
	bool bRopesDown = false;
	TArray<FVector> RopeFeet;
	/** Men sent down each rope this drop (bRopeNearestFeet: the next on a rope waits behind them). */
	TArray<int32> DropOnRope;
};
