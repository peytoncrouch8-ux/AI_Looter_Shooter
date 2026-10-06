#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SceneCloudBank.generated.h"

class UInstancedStaticMeshComponent;
class UWorld;

/**
 * A bank of soft cloud for a scene to sail into (Docs/Story.md: the cloud bank the tutorial's skiff enters; later the
 * evening cloud the gang's skiff comes out of): a dozen big puffs of the game's smoke (M_FX_Smoke, unlit and translucent)
 * on camera-facing quads, all one draw, whiter on top and bluer underneath. A puff thins out as the camera comes close,
 * so flying through one never pops, and they're drawn back to front so they blend right.
 *
 * Cheap on purpose: no volumetrics, a handful of quads, and only while a scene has it out. Its front is at its origin and
 * it reaches back along its forward axis. Looter.Scene.CloudBrightness tunes how bright it reads (0 leaves it out).
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API ASceneCloudBank : public AActor
{
	GENERATED_BODY()

public:
	ASceneCloudBank();

	/** A bank at Where (its front, facing along its forward axis); null when it can't be drawn or is turned off. */
	static ASceneCloudBank* Spawn(UWorld& World, const FTransform& Where);

	virtual void Tick(float DeltaSeconds) override;

	/** How many puffs it has. */
	int32 GetPuffCount() const { return Puffs.Num(); }

private:
	struct FPuff
	{
		/** From the bank's front: ahead, to starboard, up (cm). */
		FVector Offset = FVector::ZeroVector;
		float Size = 3000.f;
		/** Turned in its own plane, so no two look alike. */
		float Roll = 0.f;
		/** 0 at the bank's base to 1 at its top: shaded below, lit above. */
		float Height = 0.5f;
	};

	/** Lays the puffs out and draws them. False when the smoke material or the quad is missing. */
	bool Build();

	/** Faces every puff toward Viewer, thins those near it, and orders them back to front. */
	void Redraw(const FVector& Viewer);

	/** Where the camera is (or, with no player, a spot well in front of the bank). */
	FVector FindViewer() const;

	TArray<FPuff> Puffs;

	UPROPERTY(VisibleAnywhere, Category = "Cloud")
	TObjectPtr<UInstancedStaticMeshComponent> Cards;
};
