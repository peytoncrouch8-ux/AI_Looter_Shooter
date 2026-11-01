#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "LamplighterLauncher.generated.h"

class ALamplighterShip;
class UAudioComponent;
class USceneComponent;
class UStaticMeshComponent;

/** Where one of Junie's launchers is. */
UENUM()
enum class ELauncherState : uint8
{
	/** Loaded on the rail: usable once the sortie says she's alongside. */
	Ready,
	/** Fired: the grapple flying for the barge's rail, its chain paying out behind it. */
	Flying,
	/** Bitten into the barge's rail, its chain taut: with both, the chains haul her in. */
	Biting,
	/** Cut loose (step 7): the hook torn free, falling away on its chain. */
	Cut,
};

/**
 * One of the two grapple launchers Junie re-rigged on the Lamplighter's starboard rail from the Company tender's own
 * (Main 6, "The Pilot Light", step 2: "Their grapples were still in my hull, so I re-rigged them. Waste not."; tag
 * Lamplighter_GrappleLauncher, Interact held 1 s, count 2). The sortie spawns them on her deck (fore and aft of the plank's
 * gap) and fixes them to her, so they ride her; until she's alongside the barge they're only looked at. Alongside, a hold
 * fires one: the barrel kicks back with a heavy report and a puff of smoke, the hook arcs across the gap paying out its
 * chain and bites into the barge's rail with a clang and sparks; the chain stays taut between them (straining while the
 * two haul her in). Step 7 cuts it: the chain parts with a crack and the hook drops away.
 *
 * Its barrel turns to aim at the barge's rail while it's ready, so the player sees what it's for. It never ticks: the sortie
 * moves it on each frame (Advance) right after moving her, so hook and chain never lag her. Stand-in look: a timber block,
 * an iron barrel, a hook of three cones and a chain stretched from an engine cylinder (the Tripo grapple launcher replaces
 * them: TRIPO_MODELS.md 37).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALamplighterLauncher : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ALamplighterLauncher();

	/** Spawns one on Ship's deck at DeckLocal (her frame, cm, Z from the deck's top), fixed to her deck, facing her starboard. */
	static ALamplighterLauncher* SpawnOn(ALamplighterShip& Ship, const FVector& DeckLocal);

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	/** Moves its aim, its grapple's flight, its chain and its recoil on, its hook's target at Anchor (world): the sortie's frame. */
	void Advance(float DeltaSeconds, const FVector& Anchor);

	/** Lets a hold fire it (the sortie: she's alongside), or not. */
	void SetArmed(bool bInArmed) { bArmed = bInArmed; }
	bool IsArmed() const { return bArmed; }

	/** It can be fired now: armed, and loaded. */
	bool CanFire() const { return bArmed && State == ELauncherState::Ready; }

	/** Fires it at Anchor, as a hold does (the console too). False when it can't now. */
	bool Fire(const FVector& Anchor);

	/** Bitten into Anchor at once (a load in the berth, the dev jumps). */
	void BiteNow(const FVector& Anchor);

	/** Its chain strains (the two hauling her in): the strain's groan while it does. */
	void SetStraining(bool bInStraining);

	/** Cut loose: the chain parts, the hook drops away. False unless it's flying or biting. */
	bool Cut();

	/** Loaded again on the rail (a jump back from the console). */
	void Reload();

	ELauncherState GetState() const { return State; }
	bool IsBiting() const { return State == ELauncherState::Biting; }

	/** The barrel's mouth, where the chain leaves it (world). */
	FVector GetMuzzle() const;

	/** Its hook's place now (world). */
	FVector GetHookLocation() const;

	/** Seconds its grapple flies, and how high its arc lifts at the middle (cm). */
	static constexpr float FlightSeconds = 0.55f;
	static constexpr float ArcLift = 160.f;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The timber block bolted to her deck, and the iron barrel on it (what the player looks at to use it). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mount;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Barrel;

	/** The grapple: a shaft and three prongs (world-placed while it flies, bites and falls). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Hook;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> HookParts;

	/** The chain from the barrel's mouth to the hook (world-placed every frame). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Chain;

	/** How long Interact is held to fire it (s). */
	UPROPERTY(EditAnywhere, Category = "Launcher", meta = (ClampMin = "0.1"))
	float HoldSeconds = 1.f;

	UPROPERTY(EditAnywhere, Category = "Launcher")
	FText FirePrompt;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The barrel's pivot (world): where it turns on the block. */
	FVector GetPivot() const;
	void AimAt(const FVector& Target, float DeltaSeconds);
	void PlaceHook(const FVector& At, const FVector& Facing);
	void PlaceChain(const FVector& From, const FVector& To);

	ELauncherState State = ELauncherState::Ready;
	bool bArmed = false;
	float Clock = 0.f;
	FVector FlightFrom = FVector::ZeroVector;
	FVector BiteAt = FVector::ZeroVector;
	/** The barge rail's point the sortie last gave it (what a hold fires at). */
	FVector LastAnchor = FVector::ZeroVector;
	bool bHasAnchor = false;
	FVector FallVelocity = FVector::ZeroVector;
	/** The barrel's kick (cm back along it), springing home. */
	float Recoil = 0.f;
	float RecoilSpeed = 0.f;
	/** The barrel's turn, eased toward its aim. */
	FQuat Aim = FQuat::Identity;
	bool bAimSet = false;
	TWeakObjectPtr<UAudioComponent> StrainSound;
};
