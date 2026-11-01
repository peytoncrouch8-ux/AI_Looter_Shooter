#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "IntakeCharge.generated.h"

class ASinkIntake;
class UAudioComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * One of the three clamp spots on the barge's intake collar in the Sink (Main 5 "Choke Point", the set piece "the Intake":
 * Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 7): an iron lug on the collar, chalked with a Warden's cross where Junie
 * said to put it. Holding Interact for HoldSeconds there while SetWhen holds sets one of Junie's salt charges on it: three
 * salt jars wrapped in wire with a magnet clamp, slammed onto the iron with a clank and a magnet's thunk; the missions hear
 * a held Interact on it (Main 5's fourth step counts three, one a spot: a set one can't be set again), and its intake
 * (IntakeTag) is told (the drops, Vane's word, the fuse after the third).
 *
 * Lit (its intake's fuse burning), its fuse spits sparks and a little red light and hisses; blown with the intake, it's
 * gone. A session loaded mid-step finds it bare again (the step's count starts over with it). It ticks only while lit.
 *
 * Stand-in look: an iron lug (the engine's cube) with the charge as three white cylinders and a black disc; the user's
 * Tripo charge (TRIPO_MODELS.md M45-2, ChargeModelPath) replaces the charge once it's in.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AIntakeCharge : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AIntakeCharge();

	/** Every clamp spot carries it (Main 5's Interact objective counts by it). */
	static const FName ChargeTag;

	/** The user's Tripo charge (one path). */
	static const TCHAR* const ChargeModelPath;

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	virtual void Tick(float DeltaSeconds) override;

	/** Sets a charge on it as the hold does (the console: bForce whatever its story). False when it's set already or can't be now. */
	bool SetCharge(AActor* By, bool bForce = false);

	/** Its fuse lights (the intake's third charge set): sparks, a red light, a hiss until it blows. */
	void LightFuse();

	/** It blew with the intake (or its story is past it): the charge gone, the lug blackened. */
	void Blow();

	/** Back to bare (a new visit, the console). */
	void ResetCharge();

	bool IsSet() const { return bSet; }
	bool IsLit() const { return bLit; }
	bool CanSet() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The collar's lug (always there). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Lug;

	/** The charge: the Tripo model, else the stand-in's three jars and clamp. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Charge;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Jars;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> FuseLight;

	/** Its intake (by tag), told as a charge goes on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charge")
	FName IntakeTag = TEXT("Intake_Sink");

	/** A charge can be set while this holds (Main 5's fourth step). Empty: always. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charge")
	FStoryCondition SetWhen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charge", meta = (ClampMin = "0.1", Units = "s"))
	float HoldSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Charge")
	FText Prompt;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	ASinkIntake* FindIntake() const;
	void ShowCharge(bool bShown);
	FVector GetFuse() const;

	bool bSet = false;
	bool bLit = false;
	bool bBlown = false;
	float SparkClock = 0.f;
	TWeakObjectPtr<UAudioComponent> HissSound;
};
