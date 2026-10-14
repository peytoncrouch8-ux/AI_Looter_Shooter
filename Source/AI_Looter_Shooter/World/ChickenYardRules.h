#pragma once

#include "CoreMinimal.h"
#include "ChickenYardRules.generated.h"

/** Something standing in a pen that the birds walk round: a box on the ground (world XY, cm), turned by Yaw (degrees). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FChickenPenBlock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens")
	FVector2D Center = FVector2D::ZeroVector;

	/** Half its length along its turned X and half its width along its turned Y (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens")
	FVector2D HalfSize = FVector2D(50.0, 50.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens")
	float Yaw = 0.f;
};

/**
 * A pen chickens keep to: an outline (world XY, cm, its corners in order; the picket yard 40 cm inside its fence, or the
 * coop's own lath run) and what stands in it (the coop, the run's ramp). A pen holds its own birds: they never leave it.
 */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FChickenPen
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens")
	TArray<FVector2D> Outline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens")
	TArray<FChickenPenBlock> Blocks;

	/** How many hens live in it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens", meta = (ClampMin = "0", ClampMax = "8"))
	int32 Hens = 3;

	/** Whether the rooster does (a yard has one, in one of its pens). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chickens")
	bool bRooster = false;
};

/**
 * Where chickens may stand and run in a pen, as plain functions (the tests check them without a level): inside its
 * outline and out of its blocks by a body's room, a way between two spots that stays so, and the far side of the pen
 * from what scares them (straight there, or round a block's corner).
 */
namespace ChickenPens
{
	/** Whether P is inside the outline (and out of every block) with Margin cm to spare. */
	AI_LOOTER_SHOOTER_API bool IsInside(const FChickenPen& Pen, const FVector2D& P, float Margin);

	/** Whether a bird can go straight from A to B: every point of the way inside with Margin to spare. */
	AI_LOOTER_SHOOTER_API bool IsWayClear(const FChickenPen& Pen, const FVector2D& A, const FVector2D& B, float Margin);

	/** The pen's box (world XY). */
	AI_LOOTER_SHOOTER_API FBox2D Bounds(const FChickenPen& Pen);

	/** The spots a bird can stand on, every Step cm across the pen (inside with Margin to spare). */
	AI_LOOTER_SHOOTER_API TArray<FVector2D> Spots(const FChickenPen& Pen, float Step, float Margin);

	/** The corners to go round each block by: just off each of its corners, where they're inside. */
	AI_LOOTER_SHOOTER_API TArray<FVector2D> Corners(const FChickenPen& Pen, float Margin);

	/** A spot near the pen's middle (the first spot nearest it), for a bird that finds itself outside. */
	AI_LOOTER_SHOOTER_API FVector2D Middle(const FChickenPen& Pen, float Margin);

	/** A run away from Threat: where to (and through Via first, when bVia), the best of Spots that a way reaches. */
	struct FFleeRun
	{
		FVector2D Target = FVector2D::ZeroVector;
		FVector2D Via = FVector2D::ZeroVector;
		bool bVia = false;
		bool bFound = false;
	};

	/**
	 * The best place to run to from From, away from Threat: far from it, not too far to run, not on top of another bird
	 * (Others), reached straight or round one corner. Spots and Corners are the pen's (Spots(), Corners()).
	 */
	AI_LOOTER_SHOOTER_API FFleeRun FindFleeRun(const FChickenPen& Pen, TConstArrayView<FVector2D> InSpots, TConstArrayView<FVector2D> InCorners,
		const FVector2D& From, const FVector2D& Threat, TConstArrayView<FVector2D> Others, float Margin, FRandomStream& Random);
}
