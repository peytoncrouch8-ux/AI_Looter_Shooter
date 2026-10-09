#pragma once

#include "CoreMinimal.h"
#include "World/FaunaActor.h"
#include "FaunaSwarm.generated.h"

class UAudioComponent;
class UInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMesh;

/** What kind of insect a swarm is: each has its own way of moving. */
UENUM(BlueprintType)
enum class EFaunaInsect : uint8
{
	/** Fluttering wanders over flowers and gardens, now and then resting with the wings closed up (by day). */
	Butterfly,
	/** Hovering still over the water, then darting off and stopping dead (by day). */
	Dragonfly,
	/** Slow drifting points of light that blink (at dusk). */
	Firefly,
	/** Tight, quick, erratic loops over an outhouse or a carcass, with their buzz. */
	Fly,
};

/** One insect (in play only). */
struct FFaunaInsect
{
	/** The zone it belongs to (INDEX_NONE: not out). */
	int32 Zone = INDEX_NONE;
	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	FVector Goal = FVector::ZeroVector;
	float Yaw = 0.f;
	float Pitch = 0.f;
	float Roll = 0.f;
	float Scale = 1.f;
	/** Until its next goal, dart or blink (s). */
	float Timer = 0.f;
	/** A dragonfly's dart: where from, how long, how far along (s). */
	FVector DartFrom = FVector::ZeroVector;
	float DartDuration = 0.f;
	float DartTime = 0.f;
	/** A butterfly at rest on a flower (s left). */
	float Rest = 0.f;
	/** Fluttering wings: the beat's phase (radians) and rate (beats a second). */
	float FlapPhase = 0.f;
	float FlapRate = 9.f;
	/** A firefly's light (0 dark to 1), lit or between flashes. */
	float Glow = 0.f;
	bool bLit = false;
	/** Its own phases for the drift's slow wander. */
	FVector Phase = FVector::ZeroVector;
	float Age = 0.f;
};

/**
 * A level's insects of one kind and look (World/FaunaActor.h): butterflies over the flower drifts and gardens, dragonflies
 * over the creek, the ponds and the Wallow's pools, fireflies at dusk on Ransom's Rest, flies over the outhouses and the
 * den's larder. Its zones (Tools/Unreal/build_area_fauna.py) say where and how many; only the zones near the view
 * (ActiveRadius) have insects out, no more than MaxActive at once, so a few dozen fly at a time near the player and none
 * anywhere else. Every insect is an instance (a body, and a butterfly's two wings on their own components), moved only
 * while the swarm is on screen. They veer off from a player who comes close and never touch anything.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AFaunaSwarm : public AFaunaActor
{
	GENERATED_BODY()

public:
	AFaunaSwarm();

	virtual void UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context) override;
	virtual FVector GetFaunaCenter() const override;
	virtual float GetFaunaRadius() const override;
	virtual bool IsBusy() const override { return ActiveCount > 0; }

	/** How many insects are out now. */
	int32 GetActiveCount() const { return ActiveCount; }

	const TArray<FFaunaInsect>& GetInsects() const { return Insects; }

	/** Builds its pool now (as play begins it), for the tests' worlds. */
	void SetupSwarm();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm")
	EFaunaInsect Kind = EFaunaInsect::Butterfly;

	/** Where it lives, and how many each place holds while the player is near. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm")
	TArray<FFaunaZone> Zones;

	/** The most out at once, whatever the zones ask. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm", meta = (ClampMin = "0", ClampMax = "128"))
	int32 MaxActive = 16;

	/** A zone has its insects out while the view is within this of its edge (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm", meta = (ClampMin = "0", Units = "cm"))
	float ActiveRadius = 3500.f;

	/** Its body (a butterfly's without wings), and a butterfly's wings, each pivoting at the body's middle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm|Model")
	TObjectPtr<UStaticMesh> BodyMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm|Model")
	TObjectPtr<UStaticMesh> WingMeshL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm|Model")
	TObjectPtr<UStaticMesh> WingMeshR;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm|Model", meta = (ClampMin = "0.1"))
	float InsectScale = 1.f;

	/** How fast it flies (cm/s): a butterfly's flutter, a dragonfly's dart, a firefly's drift, a fly's loops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm", meta = (ClampMin = "1", Units = "cm/s"))
	float Speed = 120.f;

	/** A player this near (cm) sends it off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm", meta = (ClampMin = "0", Units = "cm"))
	float FleeRadius = 250.f;

	/** Flies' buzz (a loop near the nearest zone), or a dragonfly's rattle as it darts past (FaunaCues.h). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm|Sound")
	FName SoundCue;

	/** The flies' buzz starts within this of a zone (cm) and stops past 1.5 times it; a dart's rattle plays within it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swarm|Sound", meta = (ClampMin = "0", Units = "cm"))
	float SoundRange = 800.f;

protected:
	virtual void BeginPlay() override;
	virtual void OnFaunaShownChanged(bool bShown) override;

private:
	/** Brings insects out in the zones near the view and puts away those whose zone is no longer near. */
	void UpdateZones(const FFaunaContext& Context);
	void Spawn(FFaunaInsect& Insect, int32 Zone);

	void MoveButterfly(FFaunaInsect& Insect, float DeltaSeconds, const FFaunaContext& Context);
	void MoveDragonfly(FFaunaInsect& Insect, float DeltaSeconds, const FFaunaContext& Context);
	void MoveFirefly(FFaunaInsect& Insect, float DeltaSeconds);
	void MoveFly(FFaunaInsect& Insect, float DeltaSeconds);

	/** A random point in a zone, between its heights. */
	FVector PointIn(const FFaunaZone& Zone);
	/** Where a threat sends an insect at Where (INDEX_NONE: none near). */
	int32 ThreatNear(const FVector& Where, const FFaunaContext& Context) const;
	/** Keeps the flies' buzz going near their zone while the player is close. */
	void UpdateLoop(const FFaunaContext& Context);
	void PushTransforms();

	TArray<FFaunaInsect> Insects;
	int32 ActiveCount = 0;
	FRandomStream Random;
	FVector ActiveCenter = FVector::ZeroVector;
	float ActiveReach = 0.f;
	float DartSoundCooldown = 0.f;
	bool bReady = false;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> BodyInstances;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> WingInstancesL;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> WingInstancesR;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> SoundAnchor;

	TWeakObjectPtr<UAudioComponent> Loop;
	TArray<FTransform> BodyPose, WingPoseL, WingPoseR;
};
