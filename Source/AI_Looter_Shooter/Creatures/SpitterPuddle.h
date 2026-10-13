#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "SpitterPuddle.generated.h"

class ACharacter;
class AController;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

/** A venom puddle's make: how wide and how long, how hard it burns and how often, its color, and whose venom it is. */
struct FSpitterPuddleSpec
{
	/** Its reach round its middle (cm). */
	float Radius = 130.f;
	/** How long it lies before it has dried up (s). */
	float Seconds = 5.f;
	/** One burn's damage before the per-hit roll, and the time between burns while someone stands in it (s). */
	float Damage = 2.f;
	float Every = 0.5f;
	/** The venom's glow (the core's and the bubbles'); the pool is a dark shade of it. */
	FLinearColor Color = FLinearColor(0.45f, 1.f, 0.12f);
	/** The Spitter whose venom it is (never burned by it) and who gets the blame for its burns. */
	TWeakObjectPtr<AActor> Spitter;
	TWeakObjectPtr<AController> Instigator;
};

/**
 * A short-lived pool of venom where a Spitter's glob landed (ASpitterGlob): it splashes out, lies glowing and bubbling for
 * a few seconds and dries up, and burns any player standing in it every half second (only players: creatures walk through
 * it). Drawn in code like the pellets (M_StylizedSurface's opaque glow, no translucency or decal): a few flattened blobs of
 * dark venom, each laid on its own patch of ground so a puddle on a bumpy slope still hugs it, a brighter core, and small
 * bubbles that swell and pop. Its burn and its sizzle are LooterSoundCue::SpitterBurn and SpitterSizzle.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API ASpitterPuddle : public AActor
{
	GENERATED_BODY()

public:
	ASpitterPuddle();

	/** Lays a puddle on the ground at Ground, facing Normal (the ground's up there). Null when it couldn't spawn. */
	static ASpitterPuddle* Spawn(UWorld* World, const FVector& Ground, const FVector& Normal, const FSpitterPuddleSpec& Spec);

	virtual void Tick(float DeltaSeconds) override;

	/** Whether Body stands in it now: its feet in its reach and on its level (SpitterRules::PuddleTouches). */
	bool Touches(const ACharacter& Body) const;

	/** Seconds since it landed, and its whole life. */
	float GetAge() const { return Age; }
	float GetLife() const { return Spec.Seconds; }
	float GetRadius() const { return Spec.Radius; }

	/** Dried up: it burns nobody and is going. */
	bool IsDry() const { return Age >= Spec.Seconds; }

	/** Burns it has dealt (one per body per burn): the tests' measure, as a test level's pawns take no damage. */
	int32 GetBurns() const { return Burns; }

	/** How much of it shows now (0 to 1: spreading, lying, drying). */
	float GetSpread() const;

	/** The blobs it lies in (its dark pool), for the tests: each sits on its own ground. */
	int32 NumBlobs() const { return PoolBlobs.Num(); }

protected:
	virtual void BeginPlay() override;

private:
	/** One flattened blob of venom on its patch of ground. */
	struct FBlob
	{
		FVector Location = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		float Radius = 10.f;
	};

	/** A bubble rising in it: where (on the main blob's plane, from the middle), how far through its short life, how big. */
	struct FBubble
	{
		FVector2D Offset = FVector2D::ZeroVector;
		float Age = 0.f;
		float Life = 0.6f;
		float Size = 3.f;
	};

	/** Lays its blobs on the ground round its middle (once, as it lands). */
	void Lay(const FVector& Normal);

	/** A blob of radius Radius at Spot (on its middle's plane), put down on the ground under it, or nothing where it's off the puddle's level. */
	bool LayBlob(const FVector& Spot, float Radius, FBlob& Out) const;

	/** Burns every player standing in it. */
	void Burn();

	/** Its blobs and bubbles as they are now, spread by GetSpread. */
	void Draw(float DeltaSeconds);

	void NewBubble(FBubble& Bubble);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Pool;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Core;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Bubbles;

	/** M_StylizedSurface, which its parts wear in the venom's colors. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PoolLook;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> CoreLook;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BubbleLook;

	FSpitterPuddleSpec Spec;
	TArray<FBlob> PoolBlobs;
	TArray<FBlob> CoreBlobs;
	TArray<FBubble> BubbleState;
	/** Its middle's ground plane: where the bubbles rise from. */
	FVector Up = FVector::UpVector;
	FQuat Flat = FQuat::Identity;
	float Age = 0.f;
	/** Seconds to its next burn. */
	float BurnIn = 0.f;
	int32 Burns = 0;
	bool bLaid = false;
	FRandomStream Random;
};
