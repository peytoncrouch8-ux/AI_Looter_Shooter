#pragma once

#include "CoreMinimal.h"
#include "Creatures/AmbushSpawner.h"
#include "Creatures/CreatureRank.h"
#include "Engine/TimerHandle.h"
#include "Templates/SubclassOf.h"
#include "SetPieceSpawner.generated.h"

class ABossSeal;
class AController;
class ACreatureBase;
class APawn;
class UHealthComponent;
class ULootDropComponent;
class USpeakerPointComponent;

/** One beat of a set piece: a push's own spots and how it's announced (a caption, a sting, a shake, dust from the roof). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FSetPieceBeat
{
	GENERATED_BODY()

	/** The push it plays with: its wave, counted from 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece", meta = (ClampMin = "1"))
	int32 Wave = 1;

	/**
	 * Where this push's creatures come from, relative to the spawner as its SpawnPoints are (the alleys ahead, then the ones
	 * behind). Empty: round its spawn center (round the player, for one that springs round them).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece", meta = (MakeEditWidget = true))
	TArray<FVector> Spots;

	/** Words said as it comes (a caption line): who says them (none: a notice) and what. No words: no caption. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	FText Speaker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece", meta = (MultiLine = true))
	FText Line;

	/** A sting heard as it comes (2D), and a sound at the player's spot in the world. None: none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	FName StingCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	FName WorldCue;

	/** The view's shake (0-1, the boss shakes' scale) and how long it dies away (s). 0: none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece", meta = (ClampMin = "0", ClampMax = "1"))
	float Shake = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece", meta = (ClampMin = "0", Units = "s"))
	float ShakeSeconds = 0.8f;

	/** Puffs of dust and grit sifting down from the roof round the player (a cave-in; none where there's no roof). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece", meta = (ClampMin = "0", ClampMax = "24"))
	int32 DustPuffs = 0;
};

/** A creature a set piece stands at a spot of its own, off its ground's spots (the saloon balcony's Bell-Ringer, the high ground's Spitters). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FSetPiecePlaced
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	TSubclassOf<ACreatureBase> CreatureClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	ECreatureRank Rank = ECreatureRank::Basic;

	/** Where its feet go, in the world (the build finds the balcony's deck or the high ground's top). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	float Yaw = 0.f;

	/** The push it comes with, counted from 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece", meta = (ClampMin = "1"))
	int32 Wave = 1;
};

/** One fog wall of an arena set piece, across one of the arena's openings: its two ends in the world, each at its ground (cm). */
USTRUCT(BlueprintType)
struct AI_LOOTER_SHOOTER_API FSetPieceGate
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	FVector Start = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	FVector End = FVector::ZeroVector;
};

/**
 * A hand-made set piece (the user's call, 2026-10-21: a few per area, announced and rewarded; Docs/Areas/RansomsRest.md
 * "Set pieces"): an ambush whose creatures come in pushes from sides of its own, each push with a beat (a caption, a sting,
 * a shake, dust from a mine's roof), with creatures standing at spots of their own (Placed) and a loot shower at its end.
 * It plays once a session: done, it sets its story flag (DoneFlag, kept in the campaign record as any story flag), and its
 * story condition holds only without it, so a later visit or load finds it off.
 *  - Springing: as an ambush, on the player walking onto its ground (AmbushCorners); or with SpringRadius on the player
 *    coming that near its spot (a mine's drift, where the way out runs through it), not while any spawner it waits on
 *    (WaitForSpawners) is fighting, so two fights never stack.
 *  - Its first push comes as it springs: each push's spots are its beat's (Spots), or round the player (bAroundPlayer:
 *    the Webwood's spiders drop round where they stand). The next comes on top of the last's stragglers once PushWhenLeft
 *    or fewer are left (Looter.Fight.SetPiece.PushWhenLeft overrides), or WaveInterval after it at the latest.
 *  - While it's begun and not done, the speakers tagged HushSpeakerTags won't talk (Tilly keeps her window shut while the
 *    dead are in Main Street, so Main 3 can't be turned in over the fight).
 *  - Done: its ClearedBeat, the flag, and the loot shower (ABossLootShower, as a boss's but smaller: RewardGuns guns at
 *    RewardLuck and RewardAmmo pickups, Looter.Fight.SetPiece.Reward*) at RewardSpot, at the fight's level.
 *  - An arena (FIGHTS lane 6, 2026-10-28: the chapel yard, Saint Ada's Yard): with ArenaGates, a fog wall (ABossSeal, Gate
 *    shape, runtime-made) across each opening rises once it's fighting and the living player stands inside every gate by
 *    ArenaMargin (nobody is shut out or caught in a rising wall), and drops when its own creatures are all down (Cleared,
 *    so a Bell-Ringer left tolling never traps anyone), when it's done, taken away or off, on the player's death, and
 *    with Looter.Fight.SetPiece.Arena 0. Sealed, the player can't leave its ground, so the Unpaid's press rule
 *    (ACreatureBase::NotePressed: shot at from just past the edge) has nobody to press; walls sit inside the ground's edge.
 * Built by Tools/Unreal/build_area_camps.py (an ambush with a "setPiece" block) and build_area_fights_mountain.py (a fight
 * of spawner SetPieceSpawner); the layout's data is in gameplay.encounters.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ASetPieceSpawner : public AAmbushSpawner
{
	GENERATED_BODY()

public:
	ASetPieceSpawner();

	/** The story flag it sets when done ("SetPiece.MainStreet"): it's off whenever the flag is set (added to ActiveWhen.WithoutFlags as play begins). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	FName DoneFlag;

	/** Each push's beat (its spots and its announcing). A push without one comes round the usual spots, unannounced. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	TArray<FSetPieceBeat> Beats;

	/** Said and heard as it's done (its spots and Wave unused). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	FSetPieceBeat ClearedBeat;

	/** Creatures standing at spots of their own, each with its push. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	TArray<FSetPiecePlaced> Placed;

	/** The next push comes once this many or fewer of the last are left (alive and owed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece", meta = (ClampMin = "0"))
	int32 PushWhenLeft = 3;

	/** Its pushes come round where the player stood as it sprang (Spots unused when a beat has none). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	bool bAroundPlayer = false;

	/** Above 0: it springs on the player coming this near (cm) its spot, rather than walking onto its ground. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece", meta = (ClampMin = "0", Units = "cm"))
	float SpringRadius = 0.f;

	/** It doesn't spring while any of these spawners (by id) is fighting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	TArray<FName> WaitForSpawners;

	/** Speakers (by actor tag) that won't talk while it's begun and not done. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	TArray<FName> HushSpeakerTags;

	/**
	 * Fog walls that shut the arena's openings while it's fought (world ends, each at its ground). The arena lies ahead of
	 * each wall as seen from the spawner (a wall is turned so the spawner's side is the inside). None: no arena.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece|Arena")
	TArray<FSetPieceGate> ArenaGates;

	/** The player must be this far (cm) inside every gate before they rise. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece|Arena", meta = (ClampMin = "0", Units = "cm"))
	float ArenaMargin = 150.f;

	/** Its loot shower at its end. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece")
	bool bReward = true;

	/** Where the shower bursts from, relative to the spawner (its spot: the street's middle, the drift's end). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Set Piece", meta = (MakeEditWidget = true))
	FVector RewardSpot = FVector::ZeroVector;

	/** Done this visit (or before: its flag was set). */
	bool IsDone() const { return bDone; }

	/** Its placed creatures alive now, and those taken away while the player was far (owed back). */
	int32 NumPlacedAlive() const;
	int32 NumPlacedOwed() const { return PlacedOwed.Num(); }

	/** Whether a spawner it waits on is fighting now. */
	bool IsHeld() const;

	/** Whether the speakers it shuts are shut now. */
	bool IsHushing() const { return Hushed.Num() > 0; }

	/** Looks whether it's done (its creatures and its placed ones all down) and finishes it if so; the tests call it. */
	bool CheckDone();

	/** Its fog gates are up now (closing the arena). */
	bool AreGatesUp() const { return bGatesUp; }

	/** The fog wall of gate Index once it has been made (the first time the gates rise); null before. */
	ABossSeal* GetGateSeal(int32 Index) const;

	/** Whether Point is inside every gate by Margin (cm); true with no gates. */
	bool IsInsideArena(const FVector& Point, float Margin) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool CheckApproach() override;
	virtual void OnCreatureSpawned(ACreatureBase& Creature) override;
	virtual void UpdatePack(float DeltaSeconds) override;
	virtual FVector GetSpawnCenter() const override;
	virtual void OnStateChanged(EEncounterState From, EEncounterState To) override;
	virtual void OnWaveStarted(int32 Wave) override;

	/** The reward's loot table and level, for the shower (ABossLootShower::Throw reads its owner's). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<ULootDropComponent> RewardLoot;

private:
	/** One of its placed creatures out in the level, and its entry in Placed. */
	struct FPlacedLiving
	{
		TWeakObjectPtr<ACreatureBase> Creature;
		int32 Entry = INDEX_NONE;
	};

	// --- Its beats and its end (SetPieceSpawner.cpp) ---
	void PlayBeat(const FSetPieceBeat& Beat, bool bInterrupt);
	void SiftDust(const FVector& Around, int32 Puffs) const;
	void Finish();
	void ThrowReward();
	void StartDoneWatch();
	void HandleDoneWatch();

	// --- Its placed creatures and the speakers it shuts (SetPieceSpawnerPlaced.cpp) ---
	ACreatureBase* SpawnPlaced(int32 Entry);
	void SpawnPlacedForWave(int32 Wave);
	void BringPlacedBack();
	/** Takes its placed creatures away: owed back (the player went far) or gone (its story ended). */
	void RemovePlaced(bool bOweThem);
	void PrunePlaced();
	void Hush();
	void Unhush();

	// --- Its arena's fog gates (SetPieceSpawnerArena.cpp) ---
	/** Looks at the player and the fight: raises the gates or drops them (each look of its timer). */
	void UpdateArena();
	void RaiseGates(const APawn& Player);
	void DropGates();
	void DestroyGates();
	/** The player died while the gates were up: they open. */
	UFUNCTION()
	void HandleArenaPlayerDeath(AController* Killer);

	TArray<FPlacedLiving> PlacedLiving;
	/** The fog walls, one per ArenaGates entry (made the first time they rise, kept for the visit). */
	TArray<TWeakObjectPtr<ABossSeal>> GateSeals;
	/** The player's health, while the gates are up (for their death). */
	TWeakObjectPtr<UHealthComponent> WatchedPlayerHealth;
	bool bGatesUp = false;
	TArray<int32> PlacedOwed;
	TArray<TWeakObjectPtr<USpeakerPointComponent>> Hushed;
	FVector SprungAt = FVector::ZeroVector;
	bool bSprung = false;
	bool bDone = false;
	/** The highest level of the creatures it brought: its reward's. */
	int32 HighestLevel = 1;
	FTimerHandle DoneWatch;
};
