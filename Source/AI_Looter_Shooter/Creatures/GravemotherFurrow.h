#pragma once

#include "CoreMinimal.h"
#include "Creatures/GravemotherTypes.h"
#include "GameFramework/Actor.h"
#include "GravemotherFurrow.generated.h"

class AGroundCrack;
class UDecalComponent;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;

/**
 * The ground the Gravemother's Furrow tears up (the user's call, 2026-10-19: "a 2 s tell: the ground heaves and splits in a
 * line toward the player, dust jetting from the crack; then she ploughs the line head-low and throws up a berm each side
 * (cover, 20 s, then they crumble)"). Her code drives it along her line:
 *  - the heave (SetHeave): the split crawls out from under her forelegs, a crest of earth riding its front, dust jetting from
 *    its newest stretch, the crack's decal (MI_Decal_HeaveCrack, its end cap at the far end) laid tile by tile behind it;
 *  - the plough (SetPlough): the ploughed strip (MI_Decal_Furrow) laid as she passes, earth thrown out both sides of her,
 *    and a berm (SM_GraveBerm) rising each side once her body is past, every BermEvery, turned and sized a little apart
 *    (GravemotherFurrowBerms.cpp);
 *  - then it stands: the berms are cover for BermSeconds (they block bullets, sight, walking, lobs, and her own next lane),
 *    then crumble back into the ground with the decals fading, and it's gone.
 * A tell broken off before the plough (Cancel) lets the crack fade at once and raises no berms. Until the decals are
 * imported the crack is a dark AGroundCrack (no glow), and until the berm is, berms are earth-coloured slabs.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AGravemotherFurrow : public AActor
{
	GENERATED_BODY()

public:
	AGravemotherFurrow();

	/** Lays a furrow's line on the ground from InStart (her forelegs' ground) along InDirection for InLength (cm); begun. */
	static AGravemotherFurrow* Spawn(UWorld* World, const FVector& InStart, const FVector& InDirection, float InLength,
		const FGravemotherFurrowSettings& InSettings);

	virtual void Tick(float DeltaSeconds) override;

	/** Moves it on (its tick; the tests call it). */
	void Advance(float DeltaSeconds);

	/** The split has crawled this far along the line (cm). */
	void SetHeave(float InFront);
	/** She ploughs it now; her middle is InCovered (cm) along it. */
	void BeginPlough();
	void SetPlough(float InCovered);
	/** She slammed down at SlamAt: its scar; the berms stand their time from now. */
	void EndPlough(const FVector& SlamAt, float SlamRadius);
	/** Her tell broken off before the plough: the crack fades, no berms. */
	void Cancel();

	FVector GetStart() const { return Start; }
	FVector GetDirection() const { return Direction; }
	float GetLength() const { return Length; }
	float GetFront() const { return Front; }
	float GetCovered() const { return Covered; }
	/** Where the split's front is on the ground (the heave's sound and crest ride it). */
	USceneComponent* GetFrontPoint() const { return FrontPoint; }
	/** Its berm spots both sides, those risen so far, and whether they stand or crumble now. */
	int32 NumBermSpots() const { return Berms.Num(); }
	int32 NumBermsRisen() const;
	/** A risen berm's middle on the ground (world), for the tests (berms in order of spot; INDEX_NONE-safe). */
	TArray<FVector> GetRisenBermSpots() const;
	bool IsPloughed() const { return bPloughed; }
	bool IsStanding() const { return Phase == EPhase::Standing; }
	bool IsCrumbling() const { return Phase == EPhase::Crumbling; }
	bool IsCancelled() const { return Phase == EPhase::Cancelled; }
	/** The berms' instances (one component, both sides; collision on the risen ones). */
	UInstancedStaticMeshComponent* GetBermInstances() const { return BermSet; }
	/** Its crack is the stand-in AGroundCrack (the decal isn't imported). */
	bool UsesStandInCrack() const { return StandInCrack.IsValid(); }
	/** Its decals laid so far (crack tiles, strip tiles, end cap, scar). */
	int32 NumDecals() const;

	/** The crack's and the strip's tiles (world cm): the strip texture's 200 x 50 and 200 x 100 at their widths. */
	static constexpr float CrackTileLength = 280.f;
	static constexpr float StripTileLength = 380.f;
	static constexpr float StripWidth = 190.f;
	static constexpr float CrackWidth = 70.f;

private:
	enum class EPhase : uint8 { Heave, Plough, Standing, Crumbling, Cancelled };

	/** A decal laid along the line: its middle on the ground, how far along it is, how much of it shows. */
	struct FTile
	{
		FVector Center = FVector::ZeroVector;
		float Along = 0.f;
		float Shown = 0.f;
		bool bLaid = false;
		UDecalComponent* Decal = nullptr;
	};

	/** A berm's spot beside the line, and its rise. */
	struct FBerm
	{
		FVector Ground = FVector::ZeroVector;
		float Along = 0.f;
		float Side = 1.f;
		float Yaw = 0.f;
		float Height = 1.f;
		/** Seconds since it began to rise (negative: not yet). */
		float Rise = -1.f;
		int32 Instance = INDEX_NONE;
	};

	/** The ground under Point (the world's static geometry), looked for from Rise over it (cm; 0: the default reach), or
	 *  Point itself where there's none (a test level). */
	FVector OnGround(const FVector& Point, float Rise = 0.f) const;
	void Lay();
	void TickHeave(float DeltaSeconds);
	void TickPlough(float DeltaSeconds);
	/** Fades every decal to Opacity (the crack's tiles only once laid). */
	void FadeDecals(float Opacity);

	// --- The berms, the earth thrown, the crest, the crumble (GravemotherFurrowBerms.cpp) ---
	void MakeBermLooks();
	void RaiseBerms();
	void TickBerms(float DeltaSeconds);
	FTransform BermTransform(const FBerm& Berm, float Share, float Sink) const;
	void ThrowEarthAt(float Along);
	void PlaceCrest();
	void BeginCrumble();
	void TickCrumble(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Furrow")
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Furrow")
	TObjectPtr<USceneComponent> FrontPoint;
	UPROPERTY(VisibleAnywhere, Category = "Furrow")
	TObjectPtr<UInstancedStaticMeshComponent> BermSet;
	/** The heave's crest riding the split's front: the berm's mesh, low, no collision. */
	UPROPERTY(VisibleAnywhere, Category = "Furrow")
	TObjectPtr<UInstancedStaticMeshComponent> Crest;
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> EndCap;
	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> Scar;

	TArray<FTile> CrackTiles;
	TArray<FTile> StripTiles;
	TArray<FBerm> Berms;
	TWeakObjectPtr<AGroundCrack> StandInCrack;
	FGravemotherFurrowSettings Settings;
	FVector Start = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	FVector Across = FVector::RightVector;
	float Length = 0.f;
	EPhase Phase = EPhase::Heave;
	float Front = 0.f;
	float Covered = 0.f;
	bool bPloughed = false;
	/** Seconds the berms have stood, or crumbled, or the cancelled crack faded. */
	float PhaseTime = 0.f;
	/** The stand-in berm's mesh is the engine cube (its pivot in its middle): lifted half its height. */
	bool bStandInBerm = false;
	float BermLength = 230.f;
	float BermHeight = 74.f;
	float JetIn = 0.f;
	float SprayIn = 0.f;
	float LastSprayAlong = -1.f;
};
