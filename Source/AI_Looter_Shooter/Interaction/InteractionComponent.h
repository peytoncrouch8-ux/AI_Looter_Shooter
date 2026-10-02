#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/InteractionTypes.h"
#include "Player/PawnInputBinding.h"
#include "InteractionComponent.generated.h"

class AController;
class APawn;
class UInputAction;
struct FCollisionQueryParams;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractionFocusChanged, AActor*, Focused);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractionUsed, AActor*, Target, bool, bHeld);

/**
 * The player's Interact key, for everything it uses: loot (offered by the weapon manager, IInteractionSource), doors and
 * windows, headboards, the bell, lantern posts, hay bales, posters, the skiff's gangplank (IInteractable, found through
 * UInteractionSubsystem).
 *
 * Every frame it finds what the player means to use: what the crosshair's line meets, or else the best of what lies in a
 * forgiving cone within reach of the eyes and in plain sight (InteractionFocus::Select). It owns the key's tap and hold:
 * a thing with only a tap is used as the key goes down; one with a hold fills while the key is held and is used when the
 * hold runs its time, and letting go early is its tap when it takes one (loot's rule: tap to pick up, hold to equip).
 * Looking away or letting go early cancels a hold. Every use that does something is told to the mission runner once
 * (FMissionEvent::Interaction). The HUD reads what's focused, its words and the hold's progress.
 *
 * The ALooterCharacter makes it; it only looks around for the local player.
 */
UCLASS(ClassGroup = (Looter), meta = (BlueprintSpawnableComponent))
class AI_LOOTER_SHOOTER_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	/** What the player would use now (an interactable, or loot), or null. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	/** How the focused thing can be used (its words, tap and hold), as of the last look. */
	const FInteractionOptions& GetFocusedOptions() const { return FocusedOptions; }

	/** How far through the hold the key is (0-1); 0 while it isn't held on something with a hold. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	float GetHoldProgress() const;

	/** The key is held on the focused thing, waiting for its hold (or the release that taps it). */
	bool IsHolding() const { return bPressed; }

	/** The Interact key went down. The key's binding calls it, and so can tests. */
	void PressInteract();

	/** The Interact key came up. */
	void ReleaseInteract();

	/** Looks for what the player means to use and moves a hold on by DeltaSeconds: every frame for the local player; tests call it. */
	void UpdateInteraction(float DeltaSeconds);

	/** Where the player looks from (the crosshair's line, as the gun aims) and reaches from (the eyes). False without a pawn. */
	bool GetView(FInteractionView& OutView) const;

	/** How far from the player's eyes things can be used (cm), unless they say otherwise. Loot uses the weapon manager's PickupRange. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0"))
	float Reach = 250.f;

	/**
	 * How directly the player must look at something to use it: the cosine of its angle off the crosshair (0.8 is about 37
	 * degrees, loot's rule all along). Something the crosshair is right on always counts.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "-1", ClampMax = "1"))
	float AimThreshold = 0.8f;

	/**
	 * The Interact action. Its key ("Pick up / interact" in the settings, E) is mapped in the weapon controls (IMC_Weapons),
	 * which the weapon manager adds; this binds only the action.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction|Input")
	TObjectPtr<UInputAction> InteractAction;

	/** What the player would use changed (null: nothing). */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractionFocusChanged OnFocusChanged;

	/** Something was used: tapped, or held its full time. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractionUsed OnInteracted;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	void SetupInput(AController* Controller);
	void TeardownInput();

	/** The key is really down (a release lost to a menu or alt-tab shows here). True without a player's input to ask. */
	bool IsInteractKeyDown() const;

	/** Uses the focused thing: through the source that offered it, or the interactable itself. Tells the missions. */
	bool UseFocused(bool bHeld);

	/** The hold ran its time on what the key went down on. */
	void CompleteHold();

	/** Forgets the press (the key may still be down: nothing happens until it's pressed again). */
	void ResetPress();

	// --- Finding what's meant (InteractionComponentFocus.cpp) ---

	/** Looks again and focuses the best candidate (or nothing). */
	void RefreshFocus();
	void GatherCandidates(const FInteractionView& View, TArray<FInteractionCandidate>& OutCandidates);

	/** Adds Actor if it's an interactable not too far to ask, at AtPoint (where the crosshair meets it) or its own point. */
	FInteractionCandidate* AddInteractable(const FInteractionView& View, AActor* Actor, const FVector* AtPoint, TArray<FInteractionCandidate>& OutCandidates) const;

	/** What the crosshair's line meets, when it's an interactable: looked straight at, wherever its middle is. */
	void AddLookedAt(const FInteractionView& View, TArray<FInteractionCandidate>& OutCandidates) const;

	/** Nothing solid stands between the eyes and the candidate's point. */
	bool IsInSight(const FInteractionView& View, const FInteractionCandidate& Candidate) const;

	/** Traces skip the player and what they carry. */
	void IgnorePlayer(FCollisionQueryParams& Params) const;

	void SetFocus(const FInteractionCandidate* Candidate);

	/** The owner's components that offer things to use (the weapon manager). */
	void FindSources();

	FPawnInputBinding InputBinding;

	TArray<TWeakObjectPtr<UObject>> Sources;
	bool bSourcesFound = false;

	TWeakObjectPtr<AActor> FocusedActor;
	/** The source that offered the focused thing, or null for an interactable. */
	TWeakObjectPtr<UObject> FocusedSource;
	FInteractionOptions FocusedOptions = FInteractionOptions::None();

	/** The key went down on PressedActor, which takes a hold: waiting for the hold's time or the release. */
	bool bPressed = false;
	/** Letting go before the hold's time is a tap. */
	bool bPressTaps = false;
	TWeakObjectPtr<AActor> PressedActor;
	float PressHeldSeconds = 0.f;
	float PressHoldSeconds = 0.f;
};
