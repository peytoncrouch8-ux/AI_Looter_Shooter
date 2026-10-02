#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BossSeal.generated.h"

class UBoxComponent;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EBossSealShape : uint8
{
	/** A closed ring round the actor (the test boss's arena). */
	Ring,
	/** A straight wall from the actor to GateEnd across a gap (the Keeper's Gate); the arena lies ahead of the actor. */
	Gate
};

/**
 * A boss fight's fog wall: raised when the fight starts, dropped when it's won or reset. It stops walking pawns (the
 * player and the creatures) and nothing else: bullets, the camera, the creatures' sight and the minimap pass through.
 *
 * Its walls are thin boxes along its line (a ring's chords, or the gate's one span) that answer only the Pawn channel: their
 * object type is world-dynamic, so no ground trace (world-static) ever lands on top of one, and every other channel
 * ignores them. They exist only while it stands; a character never steps up onto one.
 *
 * It's seen as a curtain of rising ghost-light: thin emissive dashes climbing from the ground along its line over a glowing
 * seam, on two instanced meshes (opaque, M_StylizedSurface's glow: no translucency). It grows up out of the ground as
 * it rises and sinks back as it drops. The boss component makes a ring round its spot (SpawnRing), or uses one placed in
 * the level (a gate) that it's pointed at.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ABossSeal : public AActor
{
	GENERATED_BODY()

public:
	ABossSeal();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seal")
	EBossSealShape Shape = EBossSealShape::Ring;

	/** The ring's radius (cm) round the actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seal",
		meta = (ClampMin = "200", Units = "cm", EditCondition = "Shape == EBossSealShape::Ring", EditConditionHides))
	float Radius = 1500.f;

	/** The gate's far end, in the actor's own frame (drag it in the level): the wall runs from the actor to here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seal",
		meta = (MakeEditWidget, EditCondition = "Shape == EBossSealShape::Gate", EditConditionHides))
	FVector GateEnd = FVector(0.0, 800.0, 0.0);

	/** How high it stands over its line (cm), and how far its walls reach under it, so a dip along the line stays closed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seal", meta = (ClampMin = "100", Units = "cm"))
	float Height = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seal", meta = (ClampMin = "0", Units = "cm"))
	float Depth = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seal", meta = (ClampMin = "10", Units = "cm"))
	float Thickness = 40.f;

	/** The ghost-light's color (linear); it glows with this hue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seal")
	FLinearColor Color = FLinearColor(0.45f, 0.75f, 1.f);

	/** A ring seal of Radius round Center, standing on Center's height, down (raise it to close it). */
	static ABossSeal* SpawnRing(UWorld* World, const FVector& Center, float RingRadius);

	/** Closes it: its walls stop pawns at once, and the curtain grows up out of the ground. */
	void Raise();

	/** Opens it: its walls let everyone through at once, and the curtain sinks away. */
	void Drop();

	bool IsRaised() const { return bRaised; }

	/** How far up the curtain stands (0 down, 1 up), for its look. */
	float GetRise() const { return Rise; }

	/**
	 * Whether a point is on the arena's side, at least Margin (cm) in from the wall: inside a ring, or ahead of a gate. The
	 * boss raises it only once the player and the boss both are, so nobody is shut out or caught in the wall.
	 */
	bool IsInside(const FVector& Point, float Margin = 0.f) const;

	/** The wall's line in world space: a ring's corners all round (it closes back to the first), or the gate's two ends. */
	TArray<FVector> GetPath() const;

	bool IsClosed() const { return Shape == EBossSealShape::Ring; }

	/** The walls of the last Raise, one per span of its line (they stop pawns only while it's raised). */
	const TArray<TObjectPtr<UBoxComponent>>& GetWalls() const { return Walls; }

	/** A ring is cut into spans about this long (cm). */
	static constexpr float SpanLength = 400.f;

protected:
	virtual void BeginPlay() override;

private:
	/** Builds the walls anew along the line as it is now. */
	void BuildWalls();
	void SetWallsBlocking(bool bBlocking);

	/** Lays out the curtain's dashes along the line, each standing on the ground under it (game worlds only). */
	void BuildCurtain();
	/** Moves the dashes up their lines and draws them, cut off at the curtain's height for its rise. */
	void DrawCurtain();
	void ClearCurtain();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Dashes;

	/** The glowing seam along the ground under the curtain. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Seam;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowBase;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> Walls;

	/** One rising dash of light: where its column stands, and how it climbs. */
	struct FDash
	{
		FVector Foot = FVector::ZeroVector;
		float Phase = 0.f;
		float Speed = 100.f;
		float Length = 100.f;
		float Width = 4.f;
	};
	TArray<FDash> Curtain;
	/** The seam's pieces along the ground, one per span. */
	TArray<FTransform> SeamPieces;

	bool bRaised = false;
	float Rise = 0.f;
	float Clock = 0.f;
};
