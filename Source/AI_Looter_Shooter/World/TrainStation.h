#pragma once

#include "CoreMinimal.h"
#include "Areas/StationBoard.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "TrainStation.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * An area's station: on Ransom's Rest the depot by the undertaker's yard, with the departures board hung beside its door
 * (the depot's StationBoard socket). Holding Interact at the board opens the station board (UStationBoardWidget): every
 * opened area, the blank line naming the mission that opens the next, and "Skyreach (practice)" once the player has left
 * it; each a plain trip. It carries its landing (Landing_Depot), the spot on the platform where trips into the area arrive,
 * set by the area's build script (LandingTransform). Every mesh is a setting; the defaults are the imported depot and
 * board (/Game/Art/Buildings, /Game/Art/Props). Tagged Obstacle like other buildings (the minimap).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ATrainStation : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ATrainStation();

	virtual void OnConstruction(const FTransform& Transform) override;

	// --- IInteractable ---
	virtual FInteractionOptions GetInteractionOptions(const UInteractionComponent& User) const override;
	virtual bool Interact(UInteractionComponent& User, bool bHeld) override;
	virtual TOptional<FVector> GetInteractionLocation() const override;

	/** The board's words here: "Departures", "Travel", "The line to {0} isn't open yet." */
	FStationBoardWords GetBoardWords() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The station's building (the depot). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Building;

	/** The departures board, on the building's BoardSocket (at the station's origin without one). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Board;

	/** Where trips into the area arrive (tagged LandingName): on the platform, by the hearse car's door. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Landing;

	UPROPERTY(EditAnywhere, Category = "Station|Art")
	TObjectPtr<UStaticMesh> BuildingMesh;

	UPROPERTY(EditAnywhere, Category = "Station|Art")
	TObjectPtr<UStaticMesh> BoardMesh;

	/** The building's socket the board hangs on. */
	UPROPERTY(EditAnywhere, Category = "Station|Art")
	FName BoardSocket = TEXT("StationBoard");

	/** The board's socket in front of its slate, the point looked at to read it. */
	UPROPERTY(EditAnywhere, Category = "Station|Art")
	FName InteractSocket = TEXT("Interact");

	/** The landing it carries: its area's first landing (UAreaDefinition::Landings). */
	UPROPERTY(EditAnywhere, Category = "Station|Travel")
	FName LandingName = TEXT("Landing_Depot");

	/**
	 * Where the landing is, in the station's space: a spot on the platform (0.40 m up), facing the way the player should
	 * look on arriving. The build script sets it by the hearse car's door.
	 */
	UPROPERTY(EditAnywhere, Category = "Station|Travel")
	FTransform LandingTransform;

	/** What holding Interact at the board does, in the prompt's words. */
	UPROPERTY(EditAnywhere, Category = "Station|Travel")
	FText Prompt;

	UPROPERTY(EditAnywhere, Category = "Station|Travel", meta = (ClampMin = "0.05"))
	float HoldSeconds = 1.f;

	/** How far from the player's eyes the board can be used (cm); 0: the player's reach. */
	UPROPERTY(EditAnywhere, Category = "Station|Travel", meta = (ClampMin = "0"))
	float Reach = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Each mesh on its part, the board on its socket, the landing where it's set and named. */
	void ApplyArt();
};
