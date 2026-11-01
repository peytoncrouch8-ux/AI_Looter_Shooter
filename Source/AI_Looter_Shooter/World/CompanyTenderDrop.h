#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "CompanyTenderDrop.generated.h"

class UAudioComponent;
class UMissionRunner;
class USceneComponent;
class UStaticMeshComponent;

/** Where a drop tender is in its run. */
UENUM(BlueprintType)
enum class ETenderDropState : uint8
{
	/** Not called (its story doesn't hold, or its run is over): hidden. */
	Away,
	/** Flying in from the barge. */
	Coming,
	/** Hanging over the drop, its ropes down: its spawner's men slide. */
	Hovering,
	/** Its men are down: flying back to the barge. */
	Leaving,
};

/**
 * A Vane Consolidated tender sent off the barge to drop men by rope (Main 4 "A Coal Under Ash", the barge's answer to the
 * bell: Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 6; the Rappel entrance, section 15). Landfall's boarding tender
 * (ACompanyTender) belongs to the Lamplighter; this one flies on its own: when ComeWhen begins to hold it swings off the
 * barge's side at ComeFrom, flies in over FlySeconds on an arc to HoverAt (rising over the ridge between, easing in, turning
 * from its heading to HoverYaw as it slows), and hangs there bobbing, its engine droning, its ropes' tops at RopeAnchors
 * (its own frame). ARappelSpawner (its AnchorTag) waits for it to hover and slides its men down from those points. Once
 * that spawner has brought all its men down (DropSpawnerId), it lingers LeaveAfter seconds and flies back the way it came,
 * and is gone. A level loaded while ComeWhen holds starts the run again (the step's fight starts over with it).
 *
 * Stand-in look: the user's Tripo tender (SM_CompanyTender, TRIPO_MODELS.md no. 14, the boarding tender's), else the gang's
 * skiff (SM_Skiff_A_Gang) scaled to about 12 m. Seen only: nothing collides with it (it hangs 15 m up). It ticks only while
 * it's out.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ACompanyTenderDrop : public AActor
{
	GENERATED_BODY()

public:
	ACompanyTenderDrop();

	virtual void Tick(float DeltaSeconds) override;

	/** Hanging over the drop: its spawner may slide men down its ropes. */
	bool IsHovering() const { return State == ETenderDropState::Hovering; }
	ETenderDropState GetDropState() const { return State; }

	/** Rope Index's top now (world): it moves as the tender bobs. */
	FVector GetRopeAnchor(int32 Index) const;
	int32 NumRopeAnchors() const { return RopeAnchors.Num(); }

	/** Starts its run in now (the console: bForce whatever its story; the story when ComeWhen begins to hold). */
	void Come(bool bForce = false);
	/** Puts it hovering at once (a skipped approach, the tests). */
	void HoverNow();
	/** Its men are down (or the story moved on): it flies back. */
	void Leave();

	/** Moves its run on (the tick does; a test level never ticks, so tests call it). */
	void Advance(float DeltaSeconds);

	/** Reads its story again (the missions' changes call it). */
	void RefreshStory();

	/** Where it is Alpha (0-1) along its run in from ComeFrom to HoverAt (world), the arc's rise over the midway. Pure. */
	static FVector RunAt(const FVector& From, const FVector& To, float Alpha, float Rise);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Hull;

	/** It comes while this holds (Main 4 from the bell's answer): it starts its run as it begins to hold. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tender|Story")
	FStoryCondition ComeWhen;

	/** Where its run starts (world: off the barge's side) and where it hangs over the drop (world), facing HoverYaw. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tender")
	FVector ComeFrom = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tender")
	FVector HoverAt = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tender")
	float HoverYaw = 0.f;

	/** Its run in (s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tender", meta = (ClampMin = "1", Units = "s"))
	float FlySeconds = 9.f;

	/** Where its ropes hang from (its own frame, cm: +X its bow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tender", meta = (MakeEditWidget = true))
	TArray<FVector> RopeAnchors;

	/** The spawner (by id) whose men it drops; once they're all down it leaves. None: it hovers until its story ends. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tender")
	FName DropSpawnerId;

	/** It lingers this long (s) once its men are down, then flies back. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tender", meta = (ClampMin = "0", Units = "s"))
	float LeaveAfter = 8.f;

	/** The run in's arc rises this high over the straight line, midway (cm): over the ridge between. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tender", meta = (ClampMin = "0", Units = "cm"))
	float RunRise = 1800.f;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Its place and turn now, from its state and clock. */
	void PlaceNow();
	/** Its spawner has brought all its men down. */
	bool IsDropDone() const;
	void SetState(ETenderDropState NewState);
	void HandleRunnerChanged();

	ETenderDropState State = ETenderDropState::Away;
	float Clock = 0.f;
	float BobClock = 0.f;
	float LingerClock = 0.f;
	/** The story held when it last looked (its run starts as it begins to hold). */
	bool bStoryHeld = false;
	TWeakObjectPtr<UAudioComponent> EngineSound;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
};
