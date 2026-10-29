#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FindGlints.generated.h"

class UInstancedStaticMeshComponent;

/** One star to draw: where, how bright (its envelope times FindRules::GlintStrength), and its own seed (its spikes' unevenness). */
struct FFindGlintStar
{
	FVector Location = FVector::ZeroVector;
	float Strength = 0.f;
	float Seed = 0.f;
};

/**
 * The finds' glints (UFindSubsystem's tells): a pool of FindRules::MaxGlints starbursts on M_FX_Shape quads (the effects'
 * additive master: fine here, these are no Nanite meshes), each facing the eye, an unused one shrunk to nothing. Made by the
 * subsystem the first time a glint shows; never saved, no collision, no shadow. Without M_FX_Shape it draws nothing.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AFindGlints : public AActor
{
	GENERATED_BODY()

public:
	static const TCHAR* ShapeMaterialPath;

	AFindGlints();

	/** Draws Stars (at most MaxGlints; the rest ignored) facing ViewLocation; none hides the pool. */
	void SetStars(const TArray<FFindGlintStar>& Stars, const FVector& ViewLocation);

	/** How many stars it draws now. */
	int32 GetDrawn() const { return Drawn; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Cards;

private:
	/** Fills the pool the first time it's asked to draw; false without the material or the plane. */
	bool MakePool();

	bool bTriedPool = false;
	bool bPool = false;
	int32 Drawn = 0;
};
