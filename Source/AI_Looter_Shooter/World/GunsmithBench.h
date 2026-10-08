#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "GunsmithBench.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * A plain gunsmith's bench (the user's call, 2026-10-08: "a plain gunsmith's bench now, in Skyreach's village and in Ransom's
 * Rest, usable from the start; Ozias takes it over when the Gilded Lily comes"). A tap of Interact at it opens the bench's
 * screen (ALooterHUD::OpenBench, UBenchWidget): the player scraps guns they carry for one part each, kept in the parts box
 * (UWeaponManagerComponent), and fits parts from the box onto guns of the same kind (WeaponPartSwap has the rules).
 *
 * Its model is SM_GunsmithBench (Art/Models/Props/GunsmithBench.py), found only once it's imported; until then plain shapes
 * stand in at its size: a block for the bench and a small one for the parts box on its top. The model's sockets:
 * SOCKET_Interact (its front at hand height, where the player looks to use it), SOCKET_Gun (where a gun lies on its top)
 * and SOCKET_Box (the parts box); without the model they're where they would be on it (GetSocketTransform).
 *
 * Solid and tagged an obstacle (the minimap, the scatter) and GunsmithBench (missions find it by that tag: the player's
 * interaction component tells them of every use). It never ticks. The build scripts place it; leave about 1.5 m of clear,
 * level floor in front of it, where the player stands.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AGunsmithBench : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	/** The tag missions find benches by. */
	static const FName BenchTag;

	/** Art/Models/Props/GunsmithBench.py's bench. */
	static const TCHAR* const ModelPath;

	/** The model's sockets: where it's used from, where a gun lies, and the parts box. */
	static const FName InteractSocket;
	static const FName GunSocket;
	static const FName BoxSocket;

	AGunsmithBench();

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	/**
	 * Opens the bench's screen for Player (their pawn or controller) as a tap of Interact does. False without a local
	 * player's HUD to show it, or while the pause menu is up. Missions hear of a use from the interaction component.
	 */
	UFUNCTION(BlueprintCallable, Category = "Bench")
	bool Use(AActor* Player);

	/** One of the model's sockets in the world; without the model (or the socket), where it would be on it. */
	FTransform GetSocketTransform(FName Socket) const;

	/** The model is imported: the bench shows it rather than the plain shapes. */
	bool HasModel() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The bench: SM_GunsmithBench, or a plain block at its size until it's imported. Its pivot is on the floor at its middle. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Bench;

	/** The parts box on its top while the plain block stands in; empty once the model (which has its own box) is there. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StandInBox;

	/** The prompt's words. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bench")
	FText Prompt;

	/** How far from the player's eyes it can be used (cm); 0: the player's reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bench", meta = (ClampMin = "0", Units = "cm"))
	float Reach = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
