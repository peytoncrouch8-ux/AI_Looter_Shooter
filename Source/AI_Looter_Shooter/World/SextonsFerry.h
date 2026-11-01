#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SextonsFerry.generated.h"

class UAudioComponent;
class UBoxComponent;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Mister Sexton's ferry and its landing, in the canyon fog under Gravewind Point (the user's call, 2026-10-31: "Sexton
 * sells the plan at his ferry in the fog under Gravewind Point"; Docs/Work/2026-10-31-Wardens/MAINS_2_7.md, Main 2):
 * a timber landing stage hung on chains under the burial deck's open west end, and beside it the Ferryman's long black
 * ferry hanging on rotted chains over nothing, a lantern on its prow pole, its hull down in a fog bank. Sexton sits in its stern
 * (Passenger: snapped to its Seat as play begins, so he sways with it), talked to from the landing. The player comes down
 * by the rope ladder from the deck's edge (AFerryLadder, its own actor).
 *
 * The landing is safe ground over a drop: waist-high rails round all four sides, with blockers over them a player can't
 * jump past, so nobody steps off it into the canyon (it stands outside the playable area, where a fall would only be
 * caught by the long-fall rule). It stands 3.8 m under the deck (ClimbDrop), less than the fall recovery's open-edge
 * drop, so standing on it never reads as a fall either.
 *
 * Alive without costing much: the ferry sways on its chains (a slow roll, a slower turn, a bob), its chains re-strung each
 * frame and its timbers creaking, only while the player is within SwayReach; otherwise it never ticks. One soft path for
 * the Tripo ferry (FerryModelPath, TRIPO_MODELS.md "Mains 2-3"): the stand-in is engine cubes in charcoal-black wood.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ASextonsFerry : public AActor
{
	GENERATED_BODY()

public:
	ASextonsFerry();

	/** The Tripo ferry (one path): used in place of the stand-in's hull once it's in the project. */
	static const TCHAR* const FerryModelPath;

	/** The landing's size (cm: out from the cliff, along it) and how far under the deck's boards its planks are. */
	static constexpr float LandingDepth = 400.f;
	static constexpr float LandingWidth = 600.f;
	static constexpr float ClimbDrop = 380.f;

	/** The ferry's length and beam (cm), and how far out from the landing's middle its middle hangs (its near side 0.5 m off the rail). */
	static constexpr float FerryLength = 900.f;
	static constexpr float FerryBeam = 160.f;
	static constexpr float FerryOut = 330.f;
	/** ...and this far along the cliff, so its stern, where Sexton sits, is across from the landing's middle. */
	static constexpr float FerryAlong = 250.f;

	virtual void Tick(float DeltaSeconds) override;

	/** Its sway now (s into it): the ferry's frame turned and bobbed (the tick's work; tests call it). */
	void Sway(float Seconds);

	/** Where a player stands on the landing, Along cm from its middle along the cliff, InFromRail cm in from its outer rail. */
	FVector GetLandingSpot(float Along, float InFromRail) const;

	/** Sexton's seat in the world (where Passenger is snapped). */
	FTransform GetSeatTransform() const;

	/** Who sits in its stern (Mister Sexton): snapped to the seat as play begins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ferry")
	TObjectPtr<AActor> Passenger;

	/** Where the deck's open front edge is, relative to the landing's middle (cm, along its X: behind it, negative). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ferry")
	float DeckFrontX = -100.f;

	/** It sways while the player is within this (cm), and creaks within the second. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ferry", meta = (ClampMin = "0", Units = "cm"))
	float SwayReach = 8000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ferry", meta = (ClampMin = "0", Units = "cm"))
	float CreakReach = 3000.f;

	// --- Its parts ---

	/** The landing's middle, on its planks. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Landing;

	/** The rails round it (seen) and the blockers over them (unseen, pawns only). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Rails;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UBoxComponent>> Blockers;

	/** The landing's four chains up to the deck. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> LandingChains;

	/** The ferry's frame (it sways): the hull, the seat, the prow's lantern hang on it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> FerryFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TArray<TObjectPtr<UStaticMeshComponent>> Hull;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Model;

	/** Sexton's seat on the stern thwart, facing the landing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Seat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Lantern;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPointLightComponent> LanternLight;

	/** The ferry's two chains, bow and stern, up to the deck's edge (re-strung as it sways). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BowChain;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> SternChain;

	/** The fog bank round the ferry's waterline (always there: the Ferryman's fog, whatever the light). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Fog;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The landing's chains, and the ferry's from its chain eyes to the deck's edge. */
	void StringChains();

	/** The fog bank's place and size, always from the code's constants (over whatever a placed ferry saved). */
	void PlaceFog();

	bool IsPlayerWithin(double Reach) const;

	float SwayClock = 0.f;
	TWeakObjectPtr<UAudioComponent> Creak;
};
