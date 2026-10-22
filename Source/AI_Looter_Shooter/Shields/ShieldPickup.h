#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Shields/ShieldTypes.h"
#include "ShieldPickup.generated.h"

class ULootTossComponent;
class UMaterialInstanceDynamic;
class URotatingMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UWidgetComponent;

/**
 * A ward lying as loot (a kill's or a chest's drop, one the player dropped): the ward box (SM_WardBox_<Rarity>, its coal
 * window glowing in the rarity's colour) spinning under its rarity's beam, as a gun's (LootDropMoments), with a floating
 * label (UShieldLabelWidget: its name, numbers, trait and how it compares with the ward worn). The Interact key takes it:
 * a tap picks it up (worn if none is, else a spare), a hold wears it now (the worn one becomes a spare, or drops). Rare
 * and up sting and flare their beam as they land, as guns do. It lies for LifeSeconds (longer while the player stands
 * near), and a session doesn't keep it: leaving the map leaves it (Lost Property keeps only guns for now).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AShieldPickup : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AShieldPickup();

	/** Spawns Ward lying at Location (not yet tossed). */
	static AShieldPickup* SpawnWard(UWorld* World, const FShieldInstanceData& Ward, const FVector& Location);

	/** Spawns Ward at Location and tosses it (a drop from the player's hands). */
	static AShieldPickup* DropWard(UWorld* World, const FShieldInstanceData& Ward, const FVector& Location, const FVector& Velocity);

	/** Throws it; it bounces and settles, and a Rare or better one stings and flares as it lands. */
	void Toss(const FVector& Velocity);

	const FShieldInstanceData& GetWard() const { return Ward; }

	/** Seconds left before it's gone; the session save keeps them and a ward put back goes on with them (at least 10). */
	float GetLifeLeft() const { return LifeLeft; }
	void ContinueLife(float Seconds) { LifeLeft = FMath::Max(Seconds, 10.f); }

	// IInteractable
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;

	/** Seconds it lies before it's gone, unless the player is near (the guns' 5 minutes). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield")
	float LifeSeconds = 300.f;

	/** Within this (cm) of the player it never runs out, and its label shows. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shield")
	float NearDistance = 900.f;

	/** The box shows this much bigger than it's worn, so it reads in the grass (as the grenade tins do). */
	static constexpr float ModelScale = 1.7f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UFUNCTION()
	void HandleTossStopped(const FHitResult& ImpactResult);

	/** The model, its glow and the beam, as the ward is. */
	void ApplyLook();
	/** The label's visibility and focus, the life's clock (a few times a second). */
	void Watch();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Model;

	/** The rarity's beam, on the root so it stands still while the box spins. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Beam;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWidgetComponent> Label;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<ULootTossComponent> TossMovement;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<URotatingMovementComponent> Spin;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;

	UPROPERTY(Transient)
	FShieldInstanceData Ward;

	/** The beam's steady numbers (LootDropMoments), which a landing's flare plays about. */
	float BeamGlow = 0.f;
	float BeamHeight = 0.f;
	float BeamRadius = 0.f;
	/** Seconds into a landing's flare (it ticks while under LightBeams::FlareSeconds), and whether it was thrown. */
	float FlareAge = 1000.f;
	bool bTossed = false;
	/** Seconds left before it's gone (held while the player is near). */
	float LifeLeft = 300.f;
	float WatchClock = 0.f;
};
