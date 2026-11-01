#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "LandfallGrapple.generated.h"

class ACompanyTender;
class ALamplighterShip;
class UAudioComponent;
class USceneComponent;
class UStaticMeshComponent;

/** Where a grapple is in its short life. */
UENUM()
enum class ELandfallGrappleState : uint8
{
	/** Fired from the tender's launcher, flying for the Lamplighter's rail. */
	Flying,
	/** Bitten into the rail, its line taut to the tender: it pins her. */
	Biting,
	/** Cut: the hook torn off the rail, falling away on its line. */
	Cut,
};

/**
 * One of the Company tender's two grapples (Main 1 step 2, "Cut the grapple lines": Interact held, tag Landfall_Grapple,
 * count 2). Fired from a launcher on the tender's bow, it flies for the Lamplighter's rail and bites in with a clang and
 * sparks; its iron line stays taut to the tender, straining, and the two of them hold her dead still in the air. Once the
 * deck is clear the player holds Interact on the hook (~1.2 s, under the tender riflemen's fire) to cut it: the line parts
 * with a crack and sparks, the hook tears off the rail and drops away on its line. When both are cut the ship breaks free
 * (ALamplighterShip::NotifyGrappleCut). Stand-in look: an iron hook built from engine shapes, the line a stretched iron
 * cylinder (one mesh each, MeshPaths in the .cpp; the Tripo grapple hook replaces the hook).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ALandfallGrapple : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ALandfallGrapple();

	/**
	 * Fires one from FromTender's launcher Launcher at ForShip's rail point InBiteLocal (ship-local cm): it flies FlightSeconds
	 * and bites. Null without a world. (Not named Ship, Tender, BiteLocal: the members' names.)
	 */
	static ALandfallGrapple* Fire(ALamplighterShip& ForShip, ACompanyTender& FromTender, int32 Launcher, const FVector& InBiteLocal);

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;
	virtual TOptional<FInteractionBox> GetInteractionBox() const override;

	/** Moves its flight, its line and its fall on: the Lamplighter calls it right after moving herself. */
	void Advance(float DeltaSeconds);

	/** Bites into the rail now (its flight's end; the dev jumps). */
	void Bite();

	/** Cuts it, as the player's hold does. False unless it's biting and the ship lets it be cut (the deck clear). */
	bool Cut();

	ELandfallGrappleState GetState() const { return State; }
	bool IsBiting() const { return State == ELandfallGrappleState::Biting; }
	bool IsCut() const { return State == ELandfallGrappleState::Cut; }

	/** Seconds a grapple flies from the launcher to the rail. */
	static constexpr float FlightSeconds = 0.4f;
	/** Seconds a cut grapple falls before it's gone. */
	static constexpr float FallSeconds = 6.f;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The hook: its shaft back along the line, its three prongs in the rail (the crosshair finds these). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Shaft;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Prongs;

	/** The iron line to the tender's launcher (world-placed every frame; not a child of the hook's turn). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Line;

	/** How long Interact is held to cut it (seconds). */
	UPROPERTY(EditAnywhere, Category = "Grapple", meta = (ClampMin = "0.1"))
	float HoldSeconds = 1.2f;

	UPROPERTY(EditAnywhere, Category = "Grapple")
	FText CutPrompt;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The launcher's end of the line now (the tender's launcher, or where the line's end fell to). */
	FVector GetLineTail() const;

	/** Stretches the line from Head (the hook's eye) to Tail. */
	void PlaceLine(const FVector& Head, const FVector& Tail);

	/** The hook's eye: where its line ties on, at the back of its shaft. */
	FVector GetEye() const;

	TWeakObjectPtr<ALamplighterShip> Ship;
	TWeakObjectPtr<ACompanyTender> Tender;
	int32 LauncherIndex = 0;
	FVector BiteLocal = FVector::ZeroVector;

	ELandfallGrappleState State = ELandfallGrappleState::Flying;
	FVector FlightFrom = FVector::ZeroVector;
	float Clock = 0.f;
	FVector FallVelocity = FVector::ZeroVector;
	FVector FallSpin = FVector::ZeroVector;

	TWeakObjectPtr<UAudioComponent> StrainSound;
};
