#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GravemotherFX.generated.h"

class AActor;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/** Which dust a puff is: fresh grave earth (dug up: #806B55), the paler floor's (footfalls, the heave's edge), the stone's. */
enum class EGraveDust : uint8 { Earth, Floor, Stone };

/** What a thrown chunk is: grave debris (clump, field stone, coffin splinter, bone, root end), a granite chip, a small clod. */
enum class EGraveChunk : uint8 { Debris, StoneChip, Clod };

/** A burst of dust puffs (UGravemotherFX::Puffs). Sizes in cm, times in s, speeds in cm/s; each puff varies by about a fifth. */
struct FGraveDustBurst
{
	EGraveDust Kind = EGraveDust::Earth;
	int32 Count = 8;
	/** Puffs start within this of the centre (flat), up to SpreadUp above it. */
	float Spread = 40.f;
	float SpreadUp = 20.f;
	float StartSize = 60.f;
	float EndSize = 160.f;
	float Life = 1.2f;
	float Opacity = 0.35f;
	/** Every puff's own drift, and how fast it spreads away from the centre (flat, a little up). */
	FVector Drift = FVector(0.0, 0.0, 35.0);
	float Outward = 120.f;
	/** Gravity's share on it (dirt raining down: 0.3; dust hangs: 0). */
	float Fall = 0.f;
};

/**
 * The Gravemother's Barrow fight's effects (the sheet Saved/Concepts/Gravemother/Barrow_FX.png), drawn in code as the
 * weapons' are (FWeaponFX): camera-facing cards on instanced planes in the art's materials (MI_FX_GraveDust's 2x2 sheet of
 * fresh-earth puffs, MI_FX_FloorDust, MI_FX_GraveDustJet's upright jets, MI_FX_StoneSpark's streaks, MI_FX_GraveFlash), and
 * thrown chunks (SM_GraveDebris_1-5, SM_StoneChip_1-3, SM_GraveClod_S) that land on the floor under their burst, lie a
 * moment and shrink away. No collision, no shadows, capped (MaxCards, MaxChunks; the oldest go first), only near a viewer.
 *
 * Each card instance carries five floats the materials read (the contract with BARROW-ART): 0-2 a tint multiplying the
 * texture, 3 its opacity (dust, jets) or glow (sparks, flash), 4 its cell of the sheet (dust 0-3, jets 0-1). Until a
 * material or mesh is imported its kind falls back to FWeaponFX's own puffs, streaks, clods and chips.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGravemotherFX : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The world's, or null (a world that draws nothing). */
	static UGravemotherFX* Get(const UObject* WorldContext);

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGravemotherFX, STATGROUP_Tickables); }

	/** A burst of dust round Center. */
	void Puffs(const FVector& Center, const FGraveDustBurst& Burst);

	/** One upright dust jet from Base (it turns about its own upright to the camera), Width x Height cm, rising and thinning. */
	void Jet(const FVector& Base, float Width, float Height, float Life);

	/** Sparks glancing off a hit on stone at At: thrown along the shot's bounce off Normal. */
	void Sparks(const FVector& At, const FVector& Normal, const FVector& ShotDirection, int32 Count);

	/** A hot flash at At, Size cm across (a shot on the stone). */
	void Flash(const FVector& At, float Size);

	/** Count chunks thrown from At, up along Up and outward at about Speed (cm/s), Size times their own. */
	void Chunks(EGraveChunk Kind, const FVector& At, const FVector& Up, int32 Count, float Speed, float Size = 1.f);

	/** Moves everything on by DeltaSeconds and redraws it (its tick; the tests call it). */
	void Advance(float DeltaSeconds);

	/** Cards and chunks alive now (the tests; the caps). */
	int32 NumCards() const { return Cards.Num(); }
	int32 NumChunks() const { return Chunks_.Num(); }

	/** Tests and tools: spawn whether or not a player is near (a test level has none). */
	bool bIgnoreViewers = false;
	/** Tests: use FWeaponFX's fallbacks even where the art is imported. */
	bool bForceFallback = false;

	static constexpr int32 MaxCards = 280;
	static constexpr int32 MaxChunks = 90;
	/** Nothing is spawned farther than this (cm) from every viewer. */
	static constexpr float SeenWithin = 6000.f;

private:
	enum class ECardSet : uint8 { Dust, Floor, Jet, Spark, Flash, Count };

	struct FCard
	{
		ECardSet Set = ECardSet::Dust;
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		/** A spark's streak runs back along its velocity this long (cm); a jet's height. */
		float Length = 0.f;
		float Age = 0.f;
		float Life = 1.f;
		float StartSize = 50.f;
		float EndSize = 100.f;
		float Opacity = 1.f;
		float Roll = 0.f;
		float Cell = 0.f;
		float Fall = 0.f;
		float Drag = 1.2f;
		FLinearColor Tint = FLinearColor::White;
	};

	struct FChunk
	{
		/** Its mesh's set and index in it. */
		EGraveChunk Kind = EGraveChunk::Debris;
		int32 Mesh = 0;
		FVector Location = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		FVector Spin = FVector::ZeroVector;
		float Size = 1.f;
		float Age = 0.f;
		float Life = 2.5f;
		float FloorZ = -1.0e9f;
		uint8 Bounces = 0;
		bool bResting = false;
	};

	/** The effect components on a transient actor, made the first time anything is spawned; false: none in this world. */
	bool EnsureDrawing();
	/** Whether to spawn at Where at all (a viewer near), and whether its kind has its art (else FWeaponFX's stand-in). */
	bool ShouldSpawn(const FVector& Where) const;
	bool HasCards(ECardSet Set) const;
	bool HasChunks(EGraveChunk Kind) const;
	FCard& AddCard(ECardSet Set);
	/** The floor under At (the world's static geometry), or none far below. */
	float FloorUnder(const FVector& At) const;
	/** The dust's colour and the tint a puff of Kind takes on its sheet. */
	static FLinearColor DustTint(EGraveDust Kind);
	/** FWeaponFX's stand-ins for kinds whose art isn't imported (GravemotherFXDraw.cpp). */
	void FallbackPuffs(const FVector& Center, const FGraveDustBurst& Burst);
	void FallbackChunks(EGraveChunk Kind, const FVector& At, const FVector& Up, int32 Count, float Speed, float Size);
	void FallbackSparks(const FVector& At, const FVector& Normal, const FVector& ShotDirection);
	/** Moving and drawing (GravemotherFXDraw.cpp). */
	void Simulate(float DeltaSeconds);
	void Redraw();
	FVector GetCameraLocation() const;

	TWeakObjectPtr<AActor> Owner;
	TWeakObjectPtr<UInstancedStaticMeshComponent> CardSets[static_cast<int32>(ECardSet::Count)];
	/** One component per chunk mesh: debris 0-4, chips 5-7, the small clod 8. */
	TArray<TWeakObjectPtr<UInstancedStaticMeshComponent>> ChunkSets;
	bool bDrawingTried = false;
	bool bDrawnAnything = false;

	TArray<FCard> Cards;
	TArray<FChunk> Chunks_;
	FRandomStream Random{ 0x6a7e };
};
