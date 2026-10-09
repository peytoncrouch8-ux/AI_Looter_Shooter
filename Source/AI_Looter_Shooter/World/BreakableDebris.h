#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BreakableDebris.generated.h"

class AActor;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The pieces of broken props in flight (ABreakableProp): each a pre-broken piece modeled where it sat in the whole prop
 * (Art/Models/Props/Lootables.py), thrown from there, tumbling, bouncing twice on the ground under it, lying a few seconds
 * and shrinking away. Nothing is simulated by physics and no mesh is made at runtime: a piece's flight is worked out in
 * code against the one ground height traced as it's thrown.
 *
 * The pieces are drawn by a pool of static mesh components on one transient actor, made as they're first needed and used
 * again (at most MaxPieces in flight: past it, the oldest lying piece is taken), so a break spawns no actor and a level
 * full of breakables costs nothing until one breaks. It ticks only while a piece is out; a test world, which doesn't
 * tick, calls Advance.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UBreakableDebrisSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	static UBreakableDebrisSubsystem* Get(const UObject* WorldContextObject);

	/**
	 * Throws one piece of a broken prop: Mesh starts where it stood in the prop (From, the prop's transform, its origin the
	 * prop's pivot), its middle flying at Velocity (cm/s) and turning by Spin (axis times rad/s); it comes down on the ground
	 * under where it started (Ignored: the prop itself, never taken for the ground), lies Life seconds and shrinks away.
	 * False without a mesh (a piece not imported yet).
	 */
	bool Throw(UStaticMesh* Mesh, const FTransform& From, const FVector& Velocity, const FVector& Spin, float Life,
		const AActor* Ignored = nullptr);

	/** Moves every piece on by DeltaSeconds (the tick does). */
	void Advance(float DeltaSeconds);

	/** Pieces out now (flying, lying or shrinking), and the components made so far for them. */
	int32 NumPieces() const { return Pieces.Num(); }
	int32 NumComponents() const { return Pool.Num(); }

	/** How many lie still on the ground now (for the tests). */
	int32 NumLying() const;

	/** The most pieces out at once. */
	static constexpr int32 MaxPieces = 40;

	// --- UWorldSubsystem ---
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	// --- FTickableGameObject ---
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UBreakableDebrisSubsystem, STATGROUP_Tickables); }

private:
	struct FPiece
	{
		TWeakObjectPtr<UStaticMeshComponent> Mesh;
		/** The mesh's middle in its own frame (scaled): it turns about this. */
		FVector LocalMiddle = FVector::ZeroVector;
		/** Where its middle is (world), how it's turned and scaled. */
		FVector Middle = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		FVector Scale = FVector::OneVector;
		FVector Velocity = FVector::ZeroVector;
		FVector Spin = FVector::ZeroVector;
		/**
		 * The ground under where it started, as a plane (a point on it and its normal), so a piece thrown down a slope
		 * comes down on the slope; and how far its middle stays over the ground when it lies (half its thinnest side).
		 */
		FVector GroundPoint = FVector::ZeroVector;
		FVector GroundNormal = FVector::UpVector;
		float Rest = 2.f;

		/** The ground's height under its middle now. */
		double GroundZ() const;
		float Age = 0.f;
		/** Seconds it lies once down before it shrinks away (counted from when it lies). */
		float Life = 3.f;
		float LyingFor = 0.f;
		int32 BouncesLeft = 0;
		bool bLying = false;
	};

	/** A free component from the pool (made if there's none and room), else the one drawing the oldest lying piece. */
	UStaticMeshComponent* TakeComponent();

	/** Puts a piece's mesh where its state says, shrunk by Shrink (1: full size). */
	static void Pose(const FPiece& Piece, float Shrink);

	/** Hides a piece's component and gives it back to the pool. */
	void Release(FPiece& Piece);

	/**
	 * The ground under Middle (world-static only, skipping Ignored) as a point and its normal; flat at Fallback's height
	 * when none is near.
	 */
	void FindGround(const FVector& Middle, double Fallback, const AActor* Ignored, FVector& OutPoint, FVector& OutNormal) const;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PoolActor;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Pool;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Free;

	TArray<FPiece> Pieces;
};
