#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Bosses/BossTypes.h"
#include "Loot/LootLibrary.h"
#include "Math/RandomStream.h"
#include "Weapons/AmmoTypes.h"
#include "BossLootShower.generated.h"

class ULootTable;

/**
 * A dead boss's loot thrown out Borderlands-style (FBossLootShowerSettings; UBossComponent throws it in place of its loot drop
 * component's toss): its table rolled at the death as the kill rolls it (the kill gun's ammo leaning, a Greedy iron's
 * luck, a practice area's guns held back), its bonus ammo added, then popped out one piece at a time from where it burst,
 * each in a high arc on its own heading round the ring (BossRules::ShowerThrow): the ammo first, quick, then the guns with
 * a beat before each, the rarest last. A flash and a burst cue as it starts, a pop at each piece. It waits out its delay
 * first, and with bAfterScene any scene that plays (Abel's loot comes after he sits with Ellis); it goes once it's thrown
 * everything. Nothing is thrown in a level that doesn't tick (a test level), so the tests check its plan (Plan, Order).
 */
UCLASS(NotPlaceable)
class AI_LOOTER_SHOOTER_API ABossLootShower : public AActor
{
	GENERATED_BODY()

public:
	ABossLootShower();

	/**
	 * Rolls Boss's loot (its loot drop component's table, level and luck) and starts a shower bursting from From. Null when
	 * it has no loot drop component or nothing could spawn.
	 */
	static ABossLootShower* Throw(AActor& Boss, const FVector& From, const FBossLootShowerSettings& Settings);

	/** What a shower throws: Table rolled as a kill's (RollLoot), then BonusAmmo chest-full boxes leaning to KillAmmo. */
	static FLootRoll Plan(const ULootTable* Table, int32 Level, float Luck, FRandomStream& Random, TOptional<EAmmoType> KillAmmo,
		bool bWeapons, int32 BonusAmmo);

	/**
	 * The order its pieces fly: every ammo box first (as -1 - its index in Roll.Ammo), then the guns (their index in
	 * Roll.Weapons) from the least rare to the rarest, so the best comes last.
	 */
	static TArray<int32> Order(const FLootRoll& Roll);

	virtual void Tick(float DeltaSeconds) override;

	/** Moves it on by DeltaSeconds (its tick does). */
	void Advance(float DeltaSeconds);

	int32 NumPieces() const { return Queue.Num(); }
	int32 NumThrown() const { return Thrown; }
	const FLootRoll& GetRoll() const { return Roll; }

	/** After a scene it waited for ends, the first piece waits at least this long (s): the player has their eyes back. */
	static constexpr float AfterSceneWait = 0.7f;

	/** Pieces leave from this far over where it bursts (cm). */
	static constexpr float PopHeight = 60.f;

private:
	/** Throws the queue's piece Piece (Order's code), as the Index-th piece thrown. */
	void ThrowPiece(int32 Piece, int32 Index);
	/** The flash and the cue as the first piece goes. */
	void Burst();

	/** Rolled at the death, kept here (its guns' definitions with it) until each piece is thrown. */
	UPROPERTY(Transient)
	FLootRoll Roll;

	TArray<int32> Queue;
	FBossLootShowerSettings Settings;
	float Wait = 0.f;
	int32 Thrown = 0;
	float StartYaw = 0.f;
	bool bBurst = false;
};
