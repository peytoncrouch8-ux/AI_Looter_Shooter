#pragma once

#include "CoreMinimal.h"
#include "World/FaunaActor.h"
#include "CliffSwallowFlock.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;

/** What one cliff swallow is doing. */
enum class ESwallowState : uint8
{
	/** In its nest: out of sight, or its head peeking from the mouth. */
	Inside,
	/** Clinging at the nest's mouth, facing the hole, looking about. */
	Clinging,
	/** Out on a loop: off the mouth, round its waypoints, back to just in front of the nest. */
	Looping,
	/** The last dart into the mouth. */
	Arriving,
	/** Startled at the nest, about to burst out (its delay staggers the colony). */
	Startled,
	/** Circling over the cliff until it's calm. */
	Circling,
};

/** One swallow (in play only). */
struct FCliffSwallow
{
	ESwallowState State = ESwallowState::Inside;
	/** Its nest's mouth (an index into the flock's mouths). */
	int32 Mouth = 0;
	/** In the nest: its head showing at the mouth. */
	bool bPeeking = false;

	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float Yaw = 0.f;
	float Pitch = 0.f;
	float Roll = 0.f;
	float Scale = 1.f;
	float Speed = 1000.f;

	/** The flight's leg under way: from P0 leaving at V0 to P1 arriving at V1 in Duration (FaunaRules::Hermite). */
	FVector P0 = FVector::ZeroVector;
	FVector V0 = FVector::ZeroVector;
	FVector P1 = FVector::ZeroVector;
	FVector V1 = FVector::ZeroVector;
	float LegTime = 0.f;
	float LegDuration = 1.f;
	/** The loop's waypoints still to fly through (the last: just in front of the nest). */
	TArray<FVector, TInlineAllocator<6>> Route;

	/** How long it stays where it is (s), or waits before bursting out. */
	float Timer = 0.f;

	/** On its way to the circle (flying its leg there), not yet round it. */
	bool bJoining = false;
	/** On the circle: where (radians), how wide and how high over its middle (cm), its own bob. */
	float CircleAngle = 0.f;
	float CircleRadius = 0.f;
	float CircleHeight = 0.f;
	float BobPhase = 0.f;

	/** The wings: the beat's phase (radians) and rate (a second), 0 gliding to 1 beating, the bout's time left; the hands swept back (0-1). */
	float FlapPhase = 0.f;
	float FlapRate = 12.f;
	float FlapBlend = 1.f;
	float FlapGoal = 1.f;
	float FlapBout = 0.f;
	float Sweep = 0.f;

	/** Clinging or peeking: the head's turn (degrees), where it goes next, when. */
	float HeadYaw = 0.f;
	float HeadGoal = 0.f;
	float HeadTimer = 0.f;

	FRandomStream Random;
};

/**
 * Cliff swallows at their mud nests under the cliffs' overhangs (the "make it alive" polish, look sheet 44): a few to a
 * nest cluster (SM_SwallowNests, placed by Tools/Unreal/build_area_cliff_life.py; this flock by
 * Tools/Unreal/build_area_animals.py, one per group of nearby clusters). Each bird has a mouth of its own (the nests'
 * SOCKET_Nest1..4, each facing out of its hole): it sits inside (now and then its head peeking out), clings at the mouth
 * facing the hole and looks about, then drops out and loops 10-40 m out over the slope on smooth curves at 9-12 m/s,
 * banking hard (40-60 degrees), in bouts of quick wingbeats (10-14 a second) and glides with the hands swept back, diving
 * now and then, and darts back into its mouth; 2-8 s later it's out again. A gunshot within GunfireRadius, a bullet's
 * strike near them or a player right under the nests bursts them all out to circle over the cliff until it's calm, then
 * each comes home. Calls (World.Fauna.Swallow.Twitter) now and then while a player is within CallRange.
 *
 * Drawn as AFaunaFlock draws its birds, in pieces (Art/Models/Creatures/Swallow.py): perched, the clinging body and its
 * head; flying, the body and the inner and outer wings on their sockets (WingL/R, Wrist): one instanced component per
 * piece for the whole flock, every piece not in use shrunk away, moved in one batch per piece only while on screen.
 * Nothing ticks: UFaunaSubsystem updates it near the player. Without its meshes it draws nothing (it still keeps time).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ACliffSwallowFlock : public AFaunaActor
{
	GENERATED_BODY()

public:
	ACliffSwallowFlock();

	virtual void UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context) override;
	virtual FVector GetFaunaCenter() const override;
	virtual float GetFaunaRadius() const override;
	virtual bool IsBusy() const override;

	/** Builds the mouths and the birds now (as play begins it), for the tests' worlds. */
	void SetupBirds();

	const TArray<FCliffSwallow>& GetBirds() const { return Birds; }

	/** Every nest mouth (world): its front out of the hole, its top up. */
	const TArray<FTransform>& GetMouths() const { return Mouths; }

	/** How many are at their nests (inside or clinging, not startled). */
	int32 CountHome() const;

	/** Whether they're up and circling (or not yet calm). */
	bool IsAlarmed() const { return bAlarmed; }

	/** Bursts them out as a noise or a threat at Source would. */
	void Alarm(const FVector& Source);

	/** The nest clusters (world transforms of SM_SwallowNests as placed: its pivot where wall meets overhang, +X out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows")
	TArray<FTransform> Nests;

	/** The nests' model, for its mouths (SOCKET_Nest1..4). Without it (or its sockets) each cluster has four about where the model has them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows")
	TObjectPtr<UStaticMesh> NestMesh;

	/** Birds to a cluster (no more than its mouths). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows", meta = (ClampMin = "0", ClampMax = "4"))
	int32 BirdsPerNest = 3;

	// --- The pieces (Art/Models/Creatures/Swallow.py) ---

	/** Clinging at a mouth: the body upright, wings folded, its feet gripping the rim at the pivot; SOCKET_Head at the neck. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Model")
	TObjectPtr<UStaticMesh> PerchedMesh;

	/** The head, its pivot at the neck: on the clinging body, or alone peeking out of a mouth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Model")
	TObjectPtr<UStaticMesh> HeadMesh;

	/** Flying: the body, head and square tail (pivot at its middle), SOCKET_WingL and SOCKET_WingR at the shoulders. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Model")
	TObjectPtr<UStaticMesh> FlyingMesh;

	/** The arms of the wings (pivot at the shoulder, SOCKET_Wrist), and the long hands (pivot at the wrist). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Model")
	TObjectPtr<UStaticMesh> WingInnerMeshL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Model")
	TObjectPtr<UStaticMesh> WingInnerMeshR;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Model")
	TObjectPtr<UStaticMesh> WingOuterMeshL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Model")
	TObjectPtr<UStaticMesh> WingOuterMeshR;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Model", meta = (ClampMin = "0.1"))
	float BirdScale = 1.f;

	// --- Flight ---

	/** Flying speed (cm/s, each bird its own between Min and Max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "100", Units = "cm/s"))
	float SpeedMin = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "100", Units = "cm/s"))
	float SpeedMax = 1200.f;

	/** How far out a loop goes (cm, Min to Max), and how far under and over its nest it ranges (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "0", Units = "cm"))
	float LoopOutMin = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "0", Units = "cm"))
	float LoopOutMax = 4000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "0", Units = "cm"))
	float LoopBelow = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "0", Units = "cm"))
	float LoopAbove = 900.f;

	/** The widest bank (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "0", ClampMax = "80", Units = "Degrees"))
	float MaxBank = 60.f;

	/** Wingbeats a second (Min to Max, each bird its own), how far each swings (degrees), the share of time spent gliding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "1"))
	float WingbeatsMin = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "1"))
	float WingbeatsMax = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "0", ClampMax = "80", Units = "Degrees"))
	float WingbeatDegrees = 48.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "0", ClampMax = "1"))
	float GlideShare = 0.4f;

	/** How long one stays at its nest between loops (s, Min to Max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "0", Units = "s"))
	float StaySecondsMin = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Flight", meta = (ClampMin = "0", Units = "s"))
	float StaySecondsMax = 8.f;

	// --- Alarm ---

	/** A player this near a nest (cm; FaunaRules::FearOf scales it, height counts less) bursts them out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Alarm", meta = (ClampMin = "0", Units = "cm"))
	float FleeRadius = 600.f;

	/** A gunshot this near (cm), or a bullet's strike this near, bursts them out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Alarm", meta = (ClampMin = "0", Units = "cm"))
	float GunfireRadius = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Alarm", meta = (ClampMin = "0", Units = "cm"))
	float ImpactRadius = 1200.f;

	/** The circle over the cliff: how far out in front of the nests (cm), how wide, how high over them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Alarm", meta = (ClampMin = "0", Units = "cm"))
	float CircleOut = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Alarm", meta = (ClampMin = "200", Units = "cm"))
	float CircleRadius = 1100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Alarm", meta = (ClampMin = "0", Units = "cm"))
	float CircleHeight = 900.f;

	/** How long it must be calm before they come home (s, Min to Max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Alarm", meta = (ClampMin = "0", Units = "s"))
	float CalmSecondsMin = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Alarm", meta = (ClampMin = "0", Units = "s"))
	float CalmSecondsMax = 14.f;

	// --- Sound ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Sound")
	FName CallCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Sound", meta = (ClampMin = "0", Units = "cm"))
	float CallRange = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Sound", meta = (ClampMin = "0.5", Units = "s"))
	float CallSecondsMin = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Swallows|Sound", meta = (ClampMin = "0.5", Units = "s"))
	float CallSecondsMax = 10.f;

protected:
	virtual void BeginPlay() override;

private:
	// --- The colony (CliffSwallowFlock.cpp) ---

	void ReadMouths();
	void CheckAlarm(const FFaunaTick& Tick, const FFaunaContext& Context);
	void UpdateCalm(float DeltaSeconds, const FFaunaContext& Context);
	void UpdateCalls(float DeltaSeconds, const FFaunaContext& Context);
	void PlayCue(FName Cue, const FVector& Where) const;
	/** Its mouth's front flattened: the way out from the cliff. */
	FVector OutFrom(int32 Mouth) const;

	// --- Flight and drawing (CliffSwallowFlockFlight.cpp) ---

	void UpdateBird(FCliffSwallow& Bird, float DeltaSeconds);
	void StartLoop(FCliffSwallow& Bird);
	void StartCircle(FCliffSwallow& Bird);
	void StartHome(FCliffSwallow& Bird);
	void Settle(FCliffSwallow& Bird);
	/** A waypoint out from its nest that a line from Previous reaches clear of the cliff, over the ground. */
	FVector LoopPoint(FCliffSwallow& Bird, const FVector& Previous, float Out, float Rise, float Turn) const;
	/** The next leg of its route (Catmull-Rom tangents through the waypoints); false when the route is done. */
	bool NextLeg(FCliffSwallow& Bird);
	bool FollowLeg(FCliffSwallow& Bird, float DeltaSeconds);
	void SteerBody(FCliffSwallow& Bird, const FVector& NewVelocity, float DeltaSeconds);
	void UpdateWings(FCliffSwallow& Bird, float DeltaSeconds);
	void MakeComponents();
	void ReadSockets();
	void PushTransforms();

	TArray<FCliffSwallow> Birds;
	TArray<FTransform> Mouths;

	FVector Center = FVector::ZeroVector;
	float Reach = 0.f;
	FVector CircleCenter = FVector::ZeroVector;
	float CircleDirection = 1.f;

	bool bAlarmed = false;
	float CalmTime = 0.f;
	float CalmNeeded = 10.f;
	float CallTimer = 0.f;
	FRandomStream FlockRandom;
	bool bBirdsReady = false;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> PerchedInstances;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> HeadInstances;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> FlyingInstances;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> InnerInstancesL;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> InnerInstancesR;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> OuterInstancesL;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> OuterInstancesR;

	/** The pieces' sockets (in their meshes' frames; a swallow's proportions until read). */
	FVector HeadSocket = FVector(3.f, 0.f, 5.f);
	FVector ShoulderL = FVector(1.8f, -1.6f, 0.9f);
	FVector ShoulderR = FVector(1.8f, 1.6f, 0.9f);
	FVector WristL = FVector(0.f, -4.8f, 0.16f);
	FVector WristR = FVector(0.f, 4.8f, 0.16f);

	/** Reused each push: one transform per bird for each piece. */
	TArray<FTransform> PerchedPose, HeadPose, FlyingPose, InnerPoseL, InnerPoseR, OuterPoseL, OuterPoseR;
};
