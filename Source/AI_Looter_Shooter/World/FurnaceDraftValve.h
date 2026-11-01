#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "FurnaceDraftValve.generated.h"

class ABargeBoarding;
class USceneComponent;
class UStaticMeshComponent;

/**
 * One of the soul furnace's three draft valves at the furnace hall's wall (Main 6 step 5, "Kill the furnace: shut the three
 * draft valves": MAINS_2_7.md section 10): a wheel valve on the pipe run that feeds the furnace its draught. Holding Interact
 * for HoldSeconds (1.5 s) while ShutWhen holds turns it shut: the wheel spins down under the player's hands, the pipe
 * shrieks and vents steam, and the barge is told (ABargeBoarding::NoteValveShut: the furnace dims a notch, the grate ring
 * cools, a line). The missions hear the held Interact on the actor tagged Furnace_DraftValve (three spots, one each: a shut
 * valve can't be shut again). A session loaded on step 5 finds it open again (the count starts over with it); past it, shut.
 *
 * Stand-in look: an iron pipe and a wheel from engine shapes (Tripo 34's draft valve replaces both: ModelPath). The barge
 * spawns it at the wall (BargeRules::ValveAngles) and moves it with her. It ticks only while its wheel turns.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AFurnaceDraftValve : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AFurnaceDraftValve();

	static const FName ValveTag;

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	virtual void Tick(float DeltaSeconds) override;

	void SetBarge(ABargeBoarding* InBarge);

	/** Shuts it as the hold does (bForce: whatever ShutWhen says). False when it's shut already or can't be now. */
	bool Shut(AActor* By, bool bForce = false);

	/** Shut or open at once, without its sounds or the barge's word (a load, a reset). */
	void SetShutNow(bool bShut);

	bool IsShut() const { return bShut; }
	bool CanShut() const;

	/** Its wheel's turn (0 open, 1 shut) as it spins down. */
	float GetTurn() const { return Turn; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	FStoryCondition ShutWhen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve", meta = (ClampMin = "0.1", Units = "s"))
	float HoldSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Valve")
	FText Prompt;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Pipe;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Wheel;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool StoryHolds(const FStoryCondition& Condition) const;
	void PoseWheel();

	TWeakObjectPtr<ABargeBoarding> Barge;
	bool bShut = false;
	float Turn = 0.f;
	float SteamWait = 0.f;
};
