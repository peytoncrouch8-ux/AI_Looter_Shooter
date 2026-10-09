#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
#include "World/FaunaActor.h"
#include "FaunaTumbleweeds.generated.h"

class UStaticMesh;
class UStaticMeshComponent;

/** One rolling tumbleweed (in play only). */
struct FFaunaTumble
{
	bool bActive = false;
	int32 Lane = INDEX_NONE;
	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FQuat Spin = FQuat::Identity;
	/** How far it has turned in all (radians): Spin alone can't tell, as every whole turn brings it back. */
	float Rolled = 0.f;
	float Scale = 1.f;
	float Age = 0.f;
	/** Until its next bounce off the ground (s). */
	float NextHop = 0.f;
	/** How long it has crawled slower than a walk (s): stuck, it fades. */
	float Stuck = 0.f;
	/** Going: shrinking away over its last second (0 while it rolls). */
	float Fade = 0.f;
	float SoundCooldown = 0.f;
	/** Its own place in the gusts. */
	float GustPhase = 0.f;
	/** On the ground at the last step, and the ground's slope there. */
	bool bGrounded = false;
	FVector GroundNormal = FVector::UpVector;
	/** Whether a trace has ever found ground under it (none in a test level: it rolls level). */
	bool bFoundGround = false;
};

/**
 * Tumbleweeds rolling across Ransom's Rest's open ground and Main Street now and then (World/FaunaActor.h). Each lane
 * (Tools/Unreal/build_area_fauna.py: open stretches running downwind, the way the smoke leans) may send one rolling
 * from its upwind end while a player is near it and the end is out of sight or far: blown along by gusting wind, rolling
 * as far as it travels, bouncing along the ground and over bumps, following the terrain (a trace down) and glancing off
 * fences, rocks, walls and anyone in its way (a sphere sweep on the pawn channel, so the playable area's walls turn it
 * too). It never pushes or blocks anything: it has no collision of its own. It fades away past its lane, when stuck,
 * or far from the player. A handful of traces a frame for each of at most MaxActive.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AFaunaTumbleweeds : public AFaunaActor
{
	GENERATED_BODY()

public:
	AFaunaTumbleweeds();

	virtual void UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context) override;
	virtual FVector GetFaunaCenter() const override;
	virtual float GetFaunaRadius() const override;
	virtual bool IsBusy() const override;

	const TArray<FFaunaTumble>& GetTumbles() const { return Tumbles; }

	/** Sends one rolling from a lane's upwind end now (tests, Looter.Fauna.Tumble). False without a free one. */
	bool Launch(int32 LaneIndex);

	/** Builds its pool now (as play begins it), for the tests' worlds. */
	void SetupTumbleweeds();

	/** Where they roll: each lane's start is upwind, its end downwind. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds")
	TArray<FFaunaLane> Lanes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds")
	TObjectPtr<UStaticMesh> Mesh;

	/** The most rolling at once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "0", ClampMax = "8"))
	int32 MaxActive = 2;

	/** Seconds between one setting off and the next (Min to Max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "1", Units = "s"))
	float SpawnSecondsMin = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "1", Units = "s"))
	float SpawnSecondsMax = 35.f;

	/** A lane sends one only while a player is this near its start (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "0", Units = "cm"))
	float SpawnRange = 6000.f;

	/** Nearer than this (cm), a lane's start must be out of sight to send one (it would be seen appearing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "0", Units = "cm"))
	float HiddenSpawnRange = 3000.f;

	/** The wind's average speed (cm/s) and how much it gusts (a share of it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "0", Units = "cm/s"))
	float WindSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "0", ClampMax = "1"))
	float Gustiness = 0.5f;

	/** The model's radius (cm, at scale 1) and the scales they come in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "1", Units = "cm"))
	float Radius = 38.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "0.1"))
	float ScaleMin = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "0.1"))
	float ScaleMax = 1.3f;

	/** How much of its speed into a fence or rock it keeps bouncing back (0 to 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "0", ClampMax = "1"))
	float Restitution = 0.35f;

	/** A dry bounce's scrape (FaunaCues.h). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds|Sound")
	FName BounceCue;

	/** Gone this far (cm) from the nearest player, or this long (s), it fades. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "0", Units = "cm"))
	float DespawnRange = 8000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tumbleweeds", meta = (ClampMin = "1", Units = "s"))
	float MaxLifeSeconds = 45.f;

protected:
	virtual void BeginPlay() override;

private:
	/** Picks a lane near a player whose start nobody sees close up, and sends one off it. */
	void TrySpawn(const FFaunaContext& Context);
	/** One step of a roll (at most a thirtieth of a second). */
	void Roll(FFaunaTumble& Tumble, float DeltaSeconds, const FFaunaContext& Context);
	/** The ground under a point (world static, never volumes or the playable area's walls), if any. */
	bool FindGround(const FVector& Point, float Above, float Below, FVector& OutGround, FVector& OutNormal) const;
	void Bounced(FFaunaTumble& Tumble, const FFaunaContext& Context);
	void PushTransforms();

	TArray<FFaunaTumble> Tumbles;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Bodies;

	FCollisionQueryParams GroundParams;
	FCollisionQueryParams SweepParams;
	FRandomStream Random;
	float SpawnTimer = 0.f;
	float WindTime = 0.f;
	FVector Center = FVector::ZeroVector;
	float Reach = 0.f;
	bool bReady = false;
};
