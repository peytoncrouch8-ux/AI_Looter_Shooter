#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "SoulCoal.generated.h"

class UInstancedStaticMeshComponent;
class UMissionRunner;
class USceneComponent;
class UStaticMeshComponent;

/**
 * A lump of soul coal the barge rained over the Rest as it went down (Main 6's BargeFall) with a soul still pressed in it
 * (Main 7, MAINS_2_7.md section 11 step 0: "Optional: free the souls in the fallen coal. [LastingInteract SoulCoal 12, held
 * 1 s] Each lump cracks and a soul rises and follows the lantern"). Twelve lie along the two ways to Gravewind Point (the
 * west road and the keeper's path; Tools/Unreal/build_area_main7.py), tagged SoulCoal, each with its own LumpId.
 *
 * Whole, it's a black faceted lump (SM_SoulCoal, the user's Tripo model, TRIPO_MODELS.md; until then a small boulder in
 * charcoal) with a cyan soul-light pulsing in its cracks (the game's glow on a camera-facing quad, drawn only near) and a
 * thread of smoke now and then. Holding Interact a second ("Free the soul") cracks it: a sharp crack and the soul's chime,
 * cyan tatters and sparks, the glow lifting out of it as a soul that joins the trail behind Abel's lantern
 * (UCarriedLanternSubsystem::AddFollower). The first ever says Hob's word. Cracked it stays cracked (the campaign's
 * SoulCoal.<Id> flag; IsUsedUp, so Main 7's lasting objective counts it whenever it looks), its glow out, dull grey.
 *
 * It's there from Main 6's end until Saint Ada wakes (ShownWhen: after Main 6, without Main7.SaintAdaAwake), and can be
 * cracked while Main 7 runs (FreeWhen). Hidden it has no collision and doesn't tick.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ASoulCoalLump : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASoulCoalLump();

	/** The Tripo soul coal (one path), used in place of the stand-in boulder once it's imported. */
	static const TCHAR* const ModelPath;

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;
	/** Cracked for good: Main 7's lasting objective counts it. */
	virtual bool IsUsedUp() const override { return bCracked; }

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Cracks it as a held Interact does (the console, the tests): the soul out to the trail. False when it can't now. */
	bool Crack(AActor* ByWhom, bool bForce = false);
	bool IsCracked() const { return bCracked; }
	bool CanFree() const;
	bool IsShownNow() const { return bShown; }

	/** Reads the story again: shown or not, cracked or not. */
	void RefreshStory();

	/** Moves its glow and smoke on (the tick's work; tests call it). */
	void Advance(float DeltaSeconds);

	/** Its id in the campaign (SoulCoal.<Id>): unique, never renamed (set by the build script). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul Coal")
	FName LumpId;

	/** There while this holds (after Main 6, until Saint Ada wakes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul Coal")
	FStoryCondition ShownWhen;

	/** It can be cracked while this holds (Main 7 running). Empty: whenever it's there. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul Coal")
	FStoryCondition FreeWhen;

	/** How long Interact is held to crack it (s), and how far from the player's eyes (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul Coal", meta = (ClampMin = "0.1", Units = "s"))
	float HoldSeconds = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul Coal", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 260.f;

	/** Its size across (cm): a fist-and-a-half of coal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Soul Coal", meta = (ClampMin = "5", Units = "cm"))
	float Size = 32.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Lump;

	/** Its soul's glow in the cracks: one camera-facing quad. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Glow;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool StoryHolds(const FStoryCondition& Condition) const;
	void HandleMissionsChanged();
	void SetShown(bool bInShown);
	/** Fits the lump's mesh to Size, its foot on the actor's origin. */
	void FitLump();
	void ApplyCrackedLook();
	FVector GlowPoint() const;

	bool bShown = true;
	bool bCracked = false;
	/** Its soul joined the trail this visit (a load's cracked lumps add theirs at once). */
	bool bSoulAdded = false;
	float Clock = 0.f;
	float SmokeWait = 0.f;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
};
