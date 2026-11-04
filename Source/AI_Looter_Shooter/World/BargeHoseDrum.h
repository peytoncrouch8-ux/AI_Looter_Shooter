#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "BargeHoseDrum.generated.h"

class ABargeBoarding;
class UAudioComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * The barge's hose drum gantry on her top deck (Main 6 step 3's optional, "Jam the hose drum": MAINS_2_7.md section 10): the
 * winch housing over the drum under her keel that pays her hose out toward the chapel's roof, its drum turning and
 * ratcheting while it does, and a jam lever on its side. Holding Interact for HoldSeconds (2 s) while JamWhen holds throws
 * the lever into the gears: the drum shrieks to a stop in a burst of sparks, the deck shudders, and the hose stops where it
 * is, short of Saint Ada's roof (ABargeBoarding::NoteDrumJammed: the story flag Pilot.DrumJammed, Finch's thanks; Delia
 * hears of it at Main 7's turn-in). The missions hear the held Interact on the actor tagged Barge_HoseDrum. Jammed stays
 * jammed (the flag). It ticks only while it turns and the player is within earshot.
 *
 * Its look is the PROPS lane's gantry, drum and jam lever (BargeLooks::DrumGantry, HoseDrum, DrumLever: the lever turns
 * about its pin); an iron housing, a drum and a lever from engine shapes stand in until they're imported.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ABargeHoseDrum : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ABargeHoseDrum();

	static const FName DrumTag;

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	virtual void Tick(float DeltaSeconds) override;

	void SetBarge(ABargeBoarding* InBarge);

	/** Jams it as the hold does (bForce: whatever JamWhen says). False when it's jammed already or can't be now. */
	bool Jam(AActor* By, bool bForce = false);

	/** Jammed or turning at once, without its sounds (a load); the barge reads the flag for it. */
	void SetJammedNow(bool bJammed);

	bool IsJammed() const { return bJammed; }
	bool CanJam() const;

	/** It turns (paying hose out) or stands still (her hose stopped, or the barge dying). */
	void SetTurning(bool bTurning);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drum")
	FStoryCondition JamWhen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drum", meta = (ClampMin = "0.1", Units = "s"))
	float HoldSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drum")
	FText Prompt;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Housing;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Drum;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Lever;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool StoryHolds(const FStoryCondition& Condition) const;

	TWeakObjectPtr<ABargeBoarding> Barge;
	bool bJammed = false;
	bool bTurning = true;
	float DrumAngle = 0.f;
	float RatchetWait = 0.f;
	float LeverAlpha = 0.f;
};
