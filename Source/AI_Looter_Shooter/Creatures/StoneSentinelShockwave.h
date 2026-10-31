#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "Creatures/StoneSentinelMoveRules.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"
#include "StoneSentinelShockwave.generated.h"

class AActor;
class ACharacter;
class AController;
class UInstancedStaticMeshComponent;
class UStaticMesh;

/** What a shockwave needs as it leaves the fists (AStoneSentinelShockwave::Spawn): copied, so it plays out whatever becomes of its maker. */
struct FSentinelShockLaunch
{
	/** Its middle on the ground (world), its maker's size, the hurt it does (already times the fight's difficulty). */
	FVector Center = FVector::ZeroVector;
	float Size = 1.f;
	float Damage = 0.f;
	/** Its gaps' middles (degrees, world bearings from Center; StoneSentinelMoves::MakeGaps). */
	TArray<float> Gaps;
	/** Something of its maker's the front passes over (its plinth): never a wall to it. */
	TWeakObjectPtr<AActor> PassesOver;
	/** Who sent it (never hurt by it) and who gets the blame for its damage. */
	TWeakObjectPtr<AActor> Causer;
	TWeakObjectPtr<AController> Instigator;
	FSentinelShockTuning Tuning;
	int32 Seed = 0;
};

/**
 * A Stone Sentinel's ground-slam shockwave (the user's pick, 2026-10-31: "a ground-slam shockwave up close";
 * StoneSentinelCreatureShock.cpp sends it): a ring of rock spikes erupting out along the ground from where its fists went
 * in, its front running out at Speed to MaxRadius, gaps in it never where its target stood. Each row of spikes is laid as the
 * front reaches it, on the ground found by one trace a sector (Sectors round), so it climbs and dips with the land; a wall
 * or a step too high stops it on that side. A player the front's band crosses on the ground is hurt, thrown up and back, and
 * shaken; one in the air over it, in a gap, behind a wall or on another level is passed by. It sounds as it nears each player.
 *
 * The spikes are the level's own layered rock (SM_Rock_C slabs, SM_Rock_D chunks), stretched tall and leaning out, on two
 * instanced sets: opaque, no shadows, no collision; each rises in RiseSeconds, stands, and sinks back in. Until those rocks are
 * imported, the engine's cube in granite stands in. Dust rolls along the front (FWeaponFX::SpawnWaveLift) with puffs and
 * debris (UGravemotherFX), few and never at the lens (StoneSentinelFX). Its sets are pools that only grow (spare instances hidden),
 * and its sentinel keeps it to send again (Relaunch): idle between waves; a spare one, or one whose sentinel is gone, removes
 * itself once its last spike has sunk.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AStoneSentinelShockwave : public AActor
{
	GENERATED_BODY()

public:
	AStoneSentinelShockwave();

	/** A new wave, sent at once. Kept (bInKeep: its sentinel holds it and sends it again), it idles when it's done instead of going. */
	static AStoneSentinelShockwave* Spawn(UWorld* World, const FSentinelShockLaunch& InLaunch, bool bInKeep = false);

	/** Sends this wave again from InLaunch's middle (its spike sets and their instances reused: nothing new made). */
	void Relaunch(const FSentinelShockLaunch& InLaunch);

	/** Done and waiting to be sent again (a kept wave). */
	bool IsIdle() const { return bIdle; }

	virtual void Tick(float DeltaSeconds) override;

	/** Moves it on by DeltaSeconds (its tick; the tests call it). */
	void Advance(float DeltaSeconds);

	/** Its front's radius now (cm), and whether it has run its way out. */
	float GetFrontRadius() const { return Radius; }
	bool HasRunOut() const { return bRunOut; }
	/** Players it hurt, spikes standing now, rows laid, sectors a wall or a step stopped. */
	int32 GetPlayersHit() const { return PlayersHit; }
	int32 NumSpikes() const { return Spikes.Num(); }
	int32 NumRows() const { return NextRow; }
	int32 NumStoppedSectors() const;

	/** The rocks its spikes are made of (the level's layered cliff rock). */
	static const TCHAR* const SlabPath;
	static const TCHAR* const ChunkPath;

private:
	/** One way out from its middle: the ground its last row found, or where a wall or a step stopped it. */
	struct FSector
	{
		FVector Ground = FVector::ZeroVector;
		bool bHasGround = false;
		bool bStopped = false;
		float StopRadius = 0.f;
	};

	/** A rock spike: its foot on the ground, its lean and turn, its size, its life. */
	struct FSpike
	{
		int32 Set = 0;
		FVector Base = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		float Width = 50.f;
		float Height = 100.f;
		float Age = 0.f;
	};

	/** Lays row Row (its radius StartRadius + Row x RowSpacing): the ground along each sector, then its spikes and dust. */
	void LayRow(int32 Row);
	/** Finds the ground at Point along a sector from its last (or none, or a wall, or a step: false). */
	bool FindRowGround(FSector& Sector, const FVector& Point, float RowRadius, FVector& OutGround) const;
	/** Hurts every player the front crossed as it ran from FromRadius to ToRadius, and sounds as it nears them. */
	void SweepPlayers(float FromRadius, float ToRadius);
	/** The spikes on screen as they stand now (rising, standing, sinking). */
	void DrawSpikes();
	/** A bearing (degrees) round its middle, and the sector it falls in. */
	float BearingOf(const FVector& Where) const;
	int32 SectorOf(float Bearing) const;

	UPROPERTY(VisibleAnywhere, Category = "Shockwave")
	TObjectPtr<UInstancedStaticMeshComponent> SlabSet;
	UPROPERTY(VisibleAnywhere, Category = "Shockwave")
	TObjectPtr<UInstancedStaticMeshComponent> ChunkSet;
	UPROPERTY()
	TObjectPtr<UStaticMesh> SlabMesh;
	UPROPERTY()
	TObjectPtr<UStaticMesh> ChunkMesh;

	FSentinelShockLaunch Launch;
	FCollisionQueryParams WorldQuery;
	FRandomStream Random;
	TArray<FSector> Sectors;
	TArray<FSpike> Spikes;
	/** Each set's mesh foot (its bounds' bottom, cm), size across and height, so a spike stands on its own foot whatever the mesh. */
	float SetBottom[2] = { 0.f, 0.f };
	float SetAcross[2] = { 100.f, 100.f };
	float SetTall[2] = { 100.f, 100.f };
	TArray<TWeakObjectPtr<ACharacter>> Struck;
	TArray<TWeakObjectPtr<ACharacter>> Heard;
	float Age = 0.f;
	float Radius = 0.f;
	int32 NextRow = 0;
	int32 PlayersHit = 0;
	bool bRunOut = false;
	bool bDrawn = false;
	/** Its sentinel keeps it to send again; idle between its waves. */
	bool bKeep = false;
	bool bIdle = false;
};
