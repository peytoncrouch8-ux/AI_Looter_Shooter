#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Story/StoryCondition.h"
#include "ChapelWindowHose.generated.h"

class ACompanyHoseCart;
class AChapelReliquary;
class UAudioComponent;
class UInstancedStaticMeshComponent;
class UMissionRunner;
class USceneComponent;
class UStaticMeshComponent;

/**
 * The Company's hose in Saint Ada's side window (Main 4 "A Coal Under Ash", Docs/Work/2026-10-31-Wardens/MAINS_2_7.md
 * section 6): the door Abel held won't open, so the crew ran a hose from their cart in the yard over the sill of the west
 * window and across the sanctuary to the Reliquary, to drink the saint. While it drinks, pulses of her ember's light run
 * out along it toward the window (the reason for the step, seen), and the Reliquary's ember is dimmer (SetDrinking).
 *
 * The actor stands on the sill's outer edge, its +X out of the wall. The hose is drawn as a chain of short sections (one
 * instanced mesh: a draw call) along a curve from the sill, down the inside of the wall to the nave's floor
 * (InsideFloorDrop under the sill) and up onto the Reliquary at InsideEnd (play aims at the Reliquary's own ember, found by
 * ReliquaryTag). The cart (ACompanyHoseCart, CartTag) draws the outside length from its reel to the sill.
 *
 * Pulled (Interact held HoldSeconds at the sill while PullableWhen holds, Main 4's step 1): it whips back through the
 * window like a snake, its nozzle out over the sill and down onto the yard (PulledRest), coughing black smog at the window,
 * the snap and the whip heard, the view kicked; its cart is stopped (the spill, Vane's complaint), and the missions hear a
 * held Interact on it (the step's objective) and PulledEvent. If its cart is stopped first (valve or tank), the hose bucks
 * and whips out on its own and tells the missions the same: either way the saint stops being drunk. Shown while ShownWhen
 * holds; out of the window from the start while any of PulledWhen holds. It ticks only while drinking near the player or
 * whipping. Topics: ChapelWindowHose.cpp (story, pull, sounds), ChapelWindowHoseLine.cpp (its line, pulses and whip).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AChapelWindowHose : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AChapelWindowHose();

	/** Its tag (the mission's Interact objective counts a held Interact on it), and what the missions hear as it comes out. */
	static const FName HoseTag;
	static const FName PulledEvent;

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Pulls it out of the window as the hold does: the whip, the smog, its cart stopped, the missions told. By is who
	 * pulled it (null: its cart's stop freed it). False unless it's in the window and its story lets it be pulled now
	 * (bForce: whatever its story says, as the console does).
	 */
	bool Pull(AController* By, bool bForce = false);

	/** It's in the window (drinking); out once pulled or its story is past it. */
	bool IsInWindow() const { return !bOut; }
	/** Its whip is still playing. */
	bool IsWhipping() const { return bOut && WhipClock < WhipSeconds; }
	/** It can be pulled now: in the window and PullableWhen holds. */
	bool CanPull() const;

	/** Reads its story again (the missions' changes call it; so can tests). */
	void RefreshStory();

	/** Moves its pulses or its whip on (the tick does; a test level never ticks, so tests call it). */
	void Advance(float DeltaSeconds);

	/** Where its nozzle is now (world): on the Reliquary while drinking, out on the yard's ground once pulled. */
	FVector GetNozzle() const;

	/**
	 * The hose's line from the sill at Alpha (0 the sill, 1 its end) in its own frame: down the inside of the wall, along the
	 * floor and up to End. Pure, for tests.
	 */
	static FVector InsideCurve(const FVector& End, float FloorDrop, float Alpha);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The hose's sections, laid along its curve. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Sections;

	/** The brass nozzle at its end. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Nozzle;

	/** Her light drawn out along it: glow quads (the game's additive glow) running toward the window. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Pulses;

	/** Its end inside, on the Reliquary (relative; play aims at the Reliquary's ember when it finds it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose", meta = (MakeEditWidget = true))
	FVector InsideEnd = FVector(-200.0, 0.0, -60.0);

	/** How far under the sill the nave's floor is inside (cm): the hose runs down the wall to it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose", meta = (ClampMin = "0", Units = "cm"))
	float InsideFloorDrop = 132.f;

	/** Pulled, its nozzle lies here (relative: out on the yard's ground under the window). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose", meta = (MakeEditWidget = true))
	FVector PulledRest = FVector(260.0, -120.0, -170.0);

	/** The Reliquary it drinks from (by tag), and its cart (by tag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose")
	FName ReliquaryTag = TEXT("Reliquary_Chapel");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose")
	FName CartTag;

	/** Interact is held this long (s) to pull it: a heavy hose, braced against the sill. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose", meta = (ClampMin = "0.1", Units = "s"))
	float HoldSeconds = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose")
	FText Prompt;

	/** It's in the window while this holds (Main 4); hidden otherwise. Empty: always. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose|Story")
	FStoryCondition ShownWhen;

	/** It can be pulled while this holds (Main 4's step 1). Empty: whenever it's in the window. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose|Story")
	FStoryCondition PullableWhen;

	/** It lies out of the window from the start while any of these holds (its story past it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hose|Story")
	TArray<FStoryCondition> PulledWhen;

	/** The whip's length (s): back through the window, then out and down onto the yard. */
	static constexpr float WhipSeconds = 0.9f;
	/** Sections along it. */
	static constexpr int32 SectionCount = 14;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The points along it now (its own frame): in the window, or where the whip has it. */
	void BuildLine(TArray<FVector>& OutPoints) const;
	/** Lays the sections and the nozzle along the line. */
	void PlaceSections();
	/** The pulses of her light running out along it while it drinks. */
	void PlacePulses();
	/** Its end in its own frame: the Reliquary's ember when found, else InsideEnd. */
	FVector EndLocal() const;
	/** The whip's line at Alpha (0-1): its end drawn back along the curve, out over the sill, down onto PulledRest, snaking. */
	FVector WhipEnd(float Alpha) const;
	/** Smog coughing out of the window, the snap, the kick. */
	void CoughAndSnap();
	bool IsPlayerNear(double Within) const;
	void HandleRunnerChanged();
	ACompanyHoseCart* FindCart() const;
	AChapelReliquary* FindReliquary() const;
	void SetReliquaryDrinking(bool bDrinking) const;

	bool bShown = false;
	bool bOut = false;
	float WhipClock = WhipSeconds;
	float PulseClock = 0.f;
	TWeakObjectPtr<AChapelReliquary> Reliquary;
	TWeakObjectPtr<UAudioComponent> DrinkSound;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle ChangedHandle;
};
