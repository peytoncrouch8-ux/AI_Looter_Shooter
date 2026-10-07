#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "GroundCrack.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * A crack in the ground (the Gravemother's charge, Docs/Areas/RansomsRest.md "Enemies by rank"): a jagged fissure from
 * Start to End with a few short twigs off it, its lips dark and a seam of soul-light glowing in it, laid on the ground by
 * traces. Its maker opens it along its line (SetOpen: the charge's wind-up, so the crack shows where the charge will run),
 * bursts the ground round a point (Burst: where the charge slams down), then lets it close (Close): it sinks back into the
 * ground and goes by itself. Two instanced meshes of the engine cube, opaque, the glow M_StylizedSurface's (as the shriek
 * ring's): no new assets, no translucency, no shadows, no collision.
 */
UCLASS(NotPlaceable)
class AI_LOOTER_SHOOTER_API AGroundCrack : public AActor
{
	GENERATED_BODY()

public:
	AGroundCrack();

	/**
	 * Lays a crack from Start to End on the ground, Width wide at its widest (cm), its seam glowing InGlowColor; Seed picks
	 * its zigzag. It starts shut (SetOpen opens it).
	 */
	static AGroundCrack* Spawn(UWorld* World, const FVector& Start, const FVector& End, float Width, const FLinearColor& InGlowColor,
		int32 Seed);

	virtual void Tick(float DeltaSeconds) override;

	/** Moves it on by DeltaSeconds: a burst spreading, closing, going. Its tick calls it; tests call it. */
	void Advance(float DeltaSeconds);

	/** How far along its line it has cracked open, 0 (shut) to 1 (all the way to End). */
	void SetOpen(float Amount);
	float GetOpen() const { return Open; }

	/** The ground breaks round At (cm round it): short cracks burst out every way from it. */
	void Burst(const FVector& At, float Radius);

	/** It starts to close AfterSeconds from now, sinks back into the ground over CloseSeconds, and goes. */
	void Close(float AfterSeconds);
	bool IsClosing() const { return bCloseAsked; }

	/** Where it runs, as laid (on the ground). */
	FVector GetStart() const { return Points.Num() > 0 ? Points[0] : GetActorLocation(); }
	FVector GetEnd() const { return Points.Num() > 0 ? Points.Last() : GetActorLocation(); }

	/** Pieces of crack drawn now (the line's, its twigs' and a burst's that are open at all). */
	int32 NumPiecesShown() const { return PiecesShown; }

	/** The burst's cracks round its point. */
	int32 NumBurstCracks() const { return BurstCracks; }

	/** Seconds it takes to sink away once it closes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crack", meta = (ClampMin = "0.05", Units = "s"))
	float CloseSeconds = 0.8f;

	/** Seconds a burst takes to spread out to its full reach. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crack", meta = (ClampMin = "0.01", Units = "s"))
	float BurstSeconds = 0.15f;

	/** It closes by itself this long after it was laid, if its maker never closed it (s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crack", meta = (ClampMin = "1", Units = "s"))
	float MaxLife = 20.f;

private:
	/** One straight piece of the crack: from A to B on the ground, Width wide at full, shown once the crack is open past Along. */
	struct FPiece
	{
		FVector A = FVector::ZeroVector;
		FVector B = FVector::ZeroVector;
		float Width = 0.f;
		/** How far along the line (0 to 1) the opening must have reached for it to show. */
		float Along = 0.f;
		/** It's part of a burst rather than the line. */
		bool bBurst = false;
	};

	/** Lays the zigzag and its twigs between the ends (on the ground under each point). */
	void Lay(const FVector& Start, const FVector& End, float Width, int32 Seed);
	/** The ground under Point (the world's static geometry), or Point itself where there's none (a test level). */
	FVector OnGround(const FVector& Point) const;
	/** Puts every piece where it shows now. */
	void Draw();
	/** How much of a piece shows now, 0 to 1. */
	float ShownShare(const FPiece& Piece) const;

	UPROPERTY(VisibleAnywhere, Category = "Crack")
	TObjectPtr<UInstancedStaticMeshComponent> Lips;

	UPROPERTY(VisibleAnywhere, Category = "Crack")
	TObjectPtr<UInstancedStaticMeshComponent> Seam;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> SurfaceBase;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LipsLook;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SeamLook;

	TArray<FVector> Points;
	TArray<FPiece> Pieces;
	FRandomStream Random;
	float Open = 0.f;
	float Age = 0.f;
	float BurstAge = 0.f;
	int32 BurstCracks = 0;
	bool bCloseAsked = false;
	float CloseIn = 0.f;
	/** 1 while open, down to 0 as it sinks away. */
	float Closing = 1.f;
	int32 PiecesShown = 0;
	/** Something changed since it was last drawn. */
	bool bDirty = true;
};
