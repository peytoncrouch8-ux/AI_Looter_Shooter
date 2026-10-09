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

/** The fog wall's look as plain functions, apart from the world (the tests check them). */
namespace BossSealFog
{
	/** How much of its full strength a bit of fog Z cm up shows on a wall Height cm high: 1 at the foot, a quarter at the top. */
	AI_LOOTER_SHOOTER_API float Density(float Z, float Height);

	/** The flare at Distance (cm) from a player: 1 touching the wall, fading to none from about four metres out. */
	AI_LOOTER_SHOOTER_API float Flare(float Distance);

	/**
	 * The point on a path (a closed ring, or an open line) nearest Point, seen from above: its height is the path's there.
	 * Returns the distance to it (cm) flat on the ground plane.
	 */
	AI_LOOTER_SHOOTER_API float NearestOnPath(const TArray<FVector>& Path, bool bClosed, const FVector& Point, FVector& OutNearest);
}

/**
 * A boss fight's fog wall: raised when the fight starts, dropped when it's won or reset. It stops walking pawns (the
 * player and the creatures) and nothing else: bullets, the camera, the creatures' sight and the minimap pass through.
 *
 * Its walls are thin boxes along its line (a ring's chords, or the gate's one span) that answer only the Pawn channel: their
 * object type is world-dynamic, so no ground trace (world-static) ever lands on top of one, and every other channel
 * ignores them. They exist only while it stands; a character never steps up onto one.
 *
 * It's seen as a wall of grave-fog (BossSealCurtain.cpp), all of it camera-facing quads on three instanced meshes, so no
 * assets and few draws: tall faint veils that give the wall its body, wide dense banks hugging the ground, wisps that
 * climb from the foot curling slowly and thinning with height (all the game's soft smoke, M_FX_Smoke), halos along the foot
 * and specks of ghost-light climbing (the additive M_FX_Glow), over an opaque glowing seam on the ground (M_StylizedSurface).
 * The fog near the player flares, and a glow gathers where the wall is touched. It grows up out of the ground as it rises
 * and sinks back as it drops. The boss component makes a ring round its spot (SpawnRing), or uses one placed in the level
 * (a gate) that it's pointed at.
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

	/** What a piece of the curtain is: how it looks and how it moves (the first three are fog, the others light). */
	enum class EPiece : uint8
	{
		/** A tall, faint panel standing on the line: the wall's body, brightest at the foot and gone by the top. */
		Veil,
		/** A wide, low billow hugging the ground: the dense foot. */
		Bank,
		/** A wisp climbing from the foot, curling and thinning as it goes. */
		Plume,
		/** A soft glow along the foot. */
		Halo,
		/** A speck of ghost-light that climbs and winks out. */
		Mote
	};

	/** One piece of the curtain: where its column stands on the ground, and how it climbs and drifts. */
	struct FPiece
	{
		EPiece Kind = EPiece::Plume;
		FVector Foot = FVector::ZeroVector;
		/** The wall's direction there, flat (a piece drifts along it and across it). */
		FVector Along = FVector::ForwardVector;
		/** How high its trip climbs (cm), how long it takes (s), and where in it the piece is at time zero (0 to 1). */
		float Reach = 100.f;
		float Period = 8.f;
		float Phase = 0.f;
		/** Its width at the top of its trip (cm). */
		float Size = 100.f;
		/** The circle it drifts round: its radius (cm), turn (rad/s, signed) and where on it at time zero (rad). */
		float Curl = 40.f;
		float CurlSpeed = 0.4f;
		float CurlPhase = 0.f;
		/** For its shimmer, so no two pulse together (0 to 1). */
		float Seed = 0.f;
	};

	/** What a frame's drawing needs to know: where the camera and the player are, and how far up the curtain is. */
	struct FCurtainView
	{
		FVector Viewer = FVector::ZeroVector;
		FVector Player = FVector::ZeroVector;
		bool bPlayer = false;
		/** The curtain's top (cm) for its rise, and how strongly it shows (it fades in over the first of its rise). */
		float Top = 0.f;
		float Strength = 1.f;
	};

	/** Lays out the curtain's pieces along the line, each standing on the ground under it (game worlds only). */
	void BuildCurtain();
	/** Moves every piece, turns it to face the camera and draws it, cut off at the curtain's height for its rise. */
	void DrawCurtain();
	void DrawFog(const FCurtainView& View);
	void DrawLights(const FCurtainView& View);
	void ClearCurtain();

	/** Where a climbing piece is now, how far through its trip (0 to 1) and how it's moving (cm/s). */
	FVector Climb(const FPiece& Piece, float& OutTrip, FVector& OutVelocity) const;

	/** The soft smoke: veils, banks and plumes. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Fog;

	/** The additive light: halos, motes and the glow where the wall is touched. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Glow;

	/** The glowing seam along the ground under the curtain. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Seam;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> GlowBase;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> Walls;

	TArray<FPiece> FogPieces;
	TArray<FPiece> LightPieces;
	/** The seam's pieces along the ground, one per span. */
	TArray<FTransform> SeamPieces;
	/** The line on the ground (one point per corner of GetPath()), for finding where the player touches it. */
	TArray<FVector> FloorPath;

	bool bRaised = false;
	float Rise = 0.f;
	float Clock = 0.f;
};
