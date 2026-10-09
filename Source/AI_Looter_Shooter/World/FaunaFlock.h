#pragma once

#include "CoreMinimal.h"
#include "World/FaunaActor.h"
#include "FaunaFlock.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;

/** How a flock lives. */
UENUM(BlueprintType)
enum class EFaunaFlockMode : uint8
{
	/** On perches (fences, roofs, dead trees, the ground), flushing at the player or a gunshot, wheeling, coming back. */
	Perching,
	/** Never landing: swallows hawking low over the water, hawks circling high over the valley (AerialArea). */
	Aerial,
};

/** What one bird of a flock is doing. */
enum class EFaunaBirdState : uint8
{
	/** On its perch: looking about, shuffling; on the ground walking and pecking. */
	Perched,
	/** Startled, about to go (its delay staggers the flock's take-off). */
	Startled,
	/** Leaving its perch for the circle. */
	TakingOff,
	/** Wheeling over the flock's place until it's calm there. */
	Circling,
	/** Coming back to a perch, then the landing's flare. */
	Returning,
	Landing,
	/** A short hop to another perch of the flock. */
	Hopping,
	/** Gone off over the hills while the player stays among the perches; back when it's calm. */
	Leaving,
	Away,
	/** An aerial flock's bird on its endless course. */
	Soaring,
};

/** One bird (in play only). */
struct FFaunaBird
{
	EFaunaBirdState State = EFaunaBirdState::Perched;
	/** The perch it holds or is flying to (INDEX_NONE: none). */
	int32 Perch = INDEX_NONE;

	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float Yaw = 0.f;
	float Pitch = 0.f;
	float Roll = 0.f;
	/** Its size against the model's. */
	float Scale = 1.f;

	/** The flight under way: from P0 leaving at V0 to P1 arriving at V1 in FlightDuration (FaunaRules::Hermite). */
	FVector P0 = FVector::ZeroVector;
	FVector V0 = FVector::ZeroVector;
	FVector P1 = FVector::ZeroVector;
	FVector V1 = FVector::ZeroVector;
	float FlightTime = 0.f;
	float FlightDuration = 1.f;
	/** The yaw it lands facing (the perch's), turned to over the landing. */
	float LandYaw = 0.f;

	/** Seconds before a startled bird goes. */
	float Delay = 0.f;

	/** On the circle: where (radians), how wide and high over the circle's middle (cm), its own bob. */
	float CircleAngle = 0.f;
	float CircleRadius = 0.f;
	float CircleHeight = 0.f;
	float BobPhase = 0.f;

	/** Its aerial course's own numbers: phases, the figure's turn, how fast along it. */
	FVector AerialPhase = FVector::ZeroVector;
	float AerialTurn = 0.f;
	float AerialRate = 1.f;
	float AerialTime = 0.f;

	/** The wings: the beat's phase (radians), its rate (beats a second), 0 gliding to 1 beating, the bout's time left. */
	float FlapPhase = 0.f;
	float FlapRate = 4.f;
	float FlapBlend = 0.f;
	float FlapGoal = 1.f;
	float FlapBout = 0.f;
	/** Wings raised for the landing's flare (degrees). */
	float Flare = 0.f;

	/** Perched: the head's turn and tilt (degrees, against the body), where it's heading, when it moves next. */
	float HeadYaw = 0.f;
	float HeadPitch = 0.f;
	float HeadYawGoal = 0.f;
	float HeadPitchGoal = 0.f;
	float HeadTimer = 0.f;

	/** Perched: the body's tilt (pecking, a call, preening), its goal, and the next idle thing it does. */
	float BodyPitch = 0.f;
	float BodyPitchGoal = 0.f;
	float IdleTimer = 0.f;
	int32 PecksLeft = 0;
	float PeckTimer = 0.f;
	/** Its call's moment: the head up, a bob (s left). */
	float CallTime = 0.f;

	/** On the ground: walking from one spot to another (XY from its perch), over WalkDuration. */
	FVector2D WalkFrom = FVector2D::ZeroVector;
	FVector2D WalkTo = FVector2D::ZeroVector;
	float WalkTime = 0.f;
	float WalkDuration = 0.f;
	float WalkYaw = 0.f;

	FRandomStream Random;
};

/**
 * A flock of small birds drawn as instances (World/FaunaActor.h): crows on Ransom's Rest's fences, headboards, roofs and
 * dead trees; sparrows on Skyreach's; swallows and hawks in the air (Aerial). A perching flock's birds sit on its
 * perches (Tools/Unreal/build_area_fauna.py finds them on what the level has: fence rails and posts, wall tops,
 * headboards, roof ridges, a dead tree's limbs, open ground beside them; never within 15 m of one of Hob's perches, so
 * no plain crow is seen where the story's one-eyed crow is). They look about in quick little turns, shuffle, call now
 * and then, and on the ground walk (crows) or hop (sparrows) and peck. When a player comes near (nearer creeping,
 * farther running: FaunaRules::FearOf), a hostile creature passes, a gun fires within GunfireRadius or a bullet strikes
 * near them, the flock bursts off its perches nearest-first, wheels over its place, and comes back once nothing has
 * troubled it for a while; if the player stays among the perches, the birds go off over the hills and return later.
 * Now and then one hops to another perch.
 *
 * A bird is a few pieces: perched, its body (folded wings) and its head on the body's Head socket; flying, a flying body
 * with the wings on its WingL and WingR sockets (and, for crows, the outer wings on each inner wing's Wrist socket), so
 * the wings beat, glide and flare without a skeleton: one instanced component per piece for the whole flock, every piece
 * not in use shrunk away, each piece's instances updated in one batch only when the flock is on screen. Models:
 * Art/Models/Creatures/AmbientFauna.py. Without its meshes the flock draws nothing (it still keeps its time).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AFaunaFlock : public AFaunaActor
{
	GENERATED_BODY()

public:
	AFaunaFlock();

	virtual void UpdateFauna(const FFaunaTick& Tick, const FFaunaContext& Context) override;
	virtual FVector GetFaunaCenter() const override;
	virtual float GetFaunaRadius() const override;
	virtual bool IsBusy() const override;

	/** Startles the flock as a noise or a threat at Source would: everyone goes, nearest Source first. */
	void Startle(const FVector& Source);

	/** The flock's birds (in play). */
	const TArray<FFaunaBird>& GetBirds() const { return Birds; }

	/** How many birds are on their perches (perched, not startled). */
	int32 CountPerched() const;

	/** Whether the flock is up (startled, in the air, away), not settled on its perches. */
	bool IsFlushed() const { return bFlushed; }

	/** Builds the birds now (as play begins it), for the tests' worlds. */
	void SetupBirds();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock")
	EFaunaFlockMode Mode = EFaunaFlockMode::Perching;

	/** Where its birds may be (world): the first BirdCount, chosen by the seed, hold a bird as play begins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock")
	TArray<FFaunaPerch> Perches;

	/** How many birds (no more than its perches, for a perching flock). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock", meta = (ClampMin = "0", ClampMax = "64"))
	int32 BirdCount = 6;

	/** An aerial flock's sky: round its Center, between MinHeight and MaxHeight over it, on courses about Radius wide. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock")
	FFaunaZone AerialArea;

	/** An aerial flock's course: 1 circles (hawks riding a thermal), 2 figure-eights (swallows over the water). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock", meta = (ClampMin = "1", ClampMax = "3"))
	int32 AerialFigure = 1;

	// --- The pieces (Art/Models/Creatures/AmbientFauna.py) ---

	/** Perched: the body with its wings folded, its feet at the pivot, and a Head socket at the neck. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Model")
	TObjectPtr<UStaticMesh> PerchedMesh;

	/** The perched bird's head, its pivot at the neck. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Model")
	TObjectPtr<UStaticMesh> HeadMesh;

	/** Flying: the body with its head and tail, sockets WingL and WingR at the shoulders. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Model")
	TObjectPtr<UStaticMesh> FlyingMesh;

	/** The wings (the inner wings, with a Wrist socket, when there are outer ones), each pivoting at its shoulder. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Model")
	TObjectPtr<UStaticMesh> WingMeshL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Model")
	TObjectPtr<UStaticMesh> WingMeshR;

	/** The outer wings (optional), each pivoting at its wrist. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Model")
	TObjectPtr<UStaticMesh> OuterWingMeshL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Model")
	TObjectPtr<UStaticMesh> OuterWingMeshR;

	/** The birds' size against the models', and how much each differs from it (a share). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Model", meta = (ClampMin = "0.1"))
	float BirdScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Model", meta = (ClampMin = "0", ClampMax = "0.5"))
	float ScaleJitter = 0.08f;

	// --- Behaviour ---

	/** A walking player this near (cm) startles it (FaunaRules::FearOf scales it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float FleeRadius = 1200.f;

	/** A gunshot this near (cm) startles it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float GunfireRadius = 5000.f;

	/** A bullet striking this near (cm) startles it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "0", Units = "cm"))
	float ImpactRadius = 1500.f;

	/** Flying speed (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "100", Units = "cm/s"))
	float FlightSpeed = 900.f;

	/** The circle it wheels on once startled: how wide (cm) and how high over its perches' middle (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "200", Units = "cm"))
	float CircleRadius = 1600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "200", Units = "cm"))
	float CircleHeight = 1400.f;

	/** How long it must be calm (no threat near its perches, no noise) before it comes back (s, from Min to Max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "0", Units = "s"))
	float CalmSecondsMin = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "0", Units = "s"))
	float CalmSecondsMax = 16.f;

	/** Circling this long (s) without it calming down, the flock goes off over the hills instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "1", Units = "s"))
	float LeaveAfterSeconds = 30.f;

	/** Seconds between one bird's hops to another perch (Min to Max), while the flock is settled and seen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "1", Units = "s"))
	float HopSecondsMin = 14.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "1", Units = "s"))
	float HopSecondsMax = 32.f;

	/** On the ground: hopping (sparrows) rather than walking (crows), and how fast (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour")
	bool bHopsOnGround = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "1", Units = "cm/s"))
	float GroundSpeed = 45.f;

	/** How long a head stays still between its quick turns (s, Min to Max): crows look about slower than sparrows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "0.05", Units = "s"))
	float HeadSecondsMin = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Behaviour", meta = (ClampMin = "0.05", Units = "s"))
	float HeadSecondsMax = 2.2f;

	// --- Wings ---

	/** Wingbeats a second, and how far each beat swings (degrees, either way of the glide's angle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Wings", meta = (ClampMin = "0.5"))
	float WingbeatsPerSecond = 4.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Wings", meta = (ClampMin = "0", ClampMax = "80", Units = "Degrees"))
	float WingbeatDegrees = 40.f;

	/** The wings' angle over level while gliding (degrees; a hawk's shallow V). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Wings", meta = (Units = "Degrees"))
	float GlideDegrees = 5.f;

	/** The share of its flying time it glides (a hawk most of it, a crow a third). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Wings", meta = (ClampMin = "0", ClampMax = "1"))
	float GlideShare = 0.35f;

	// --- Sounds (FaunaCues.h; a cue without sounds plays nothing) ---

	/** A perched or soaring bird's call, now and then while a player is within CallRange. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Sound")
	FName CallCue;

	/** The flock bursting off its perches. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Sound")
	FName TakeOffCue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Sound", meta = (ClampMin = "0", Units = "cm"))
	float CallRange = 5000.f;

	/** Seconds between calls (Min to Max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Sound", meta = (ClampMin = "0.5", Units = "s"))
	float CallSecondsMin = 7.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flock|Sound", meta = (ClampMin = "0.5", Units = "s"))
	float CallSecondsMax = 20.f;

protected:
	virtual void BeginPlay() override;

private:
	// --- The flock (FaunaFlock.cpp) ---

	/** Whether a threat or a noise startles it now. */
	void CheckAlarm(const FFaunaTick& Tick, const FFaunaContext& Context);

	/** While it's up: is it calm yet, and when may it come back (or should it leave). */
	void UpdateFlushed(float DeltaSeconds, const FFaunaContext& Context);

	/** A safe free perch for a bird coming back: its own if it can, else the nearest (INDEX_NONE: none). */
	int32 FindLandingPerch(int32 BirdIndex, const FFaunaContext& Context) const;

	/** Now and then a settled bird hops to another free perch nearby; a perched one calls. */
	void UpdateSettledLife(float DeltaSeconds, const FFaunaTick& Tick, const FFaunaContext& Context);

	void PlayCue(FName Cue, const FVector& Where) const;

	// --- Flight (FaunaFlockFlight.cpp) ---

	void UpdateBird(FFaunaBird& Bird, int32 Index, float DeltaSeconds);
	void TakeOff(FFaunaBird& Bird, int32 Index);
	void FlyCircle(FFaunaBird& Bird, float DeltaSeconds);
	void StartReturn(FFaunaBird& Bird, int32 PerchIndex);
	void StartHop(FFaunaBird& Bird, int32 PerchIndex);
	void StartLeave(FFaunaBird& Bird);
	void FlyAerial(FFaunaBird& Bird, float DeltaSeconds);
	/** Moves a bird along its flight's curve; true once it's at the end. */
	bool FollowFlight(FFaunaBird& Bird, float DeltaSeconds);
	/** Faces, pitches and banks a flying bird along its velocity. */
	void SteerBody(FFaunaBird& Bird, const FVector& NewVelocity, float DeltaSeconds);
	/** Beating and gliding in bouts. */
	void UpdateWings(FFaunaBird& Bird, float DeltaSeconds, float BeatScale = 1.f);
	void Settle(FFaunaBird& Bird, int32 PerchIndex);

	// --- Perched life and drawing (FaunaFlockPose.cpp) ---

	void UpdatePerched(FFaunaBird& Bird, float DeltaSeconds);
	void MakeComponents();
	void ReadSockets();
	/** Every piece's instance for every bird, in one batch per piece. */
	void PushTransforms();

	TArray<FFaunaBird> Birds;
	/** Which bird holds each perch (INDEX_NONE: free). */
	TArray<int32> PerchHolder;

	FVector Center = FVector::ZeroVector;
	float Reach = 0.f;
	FVector CircleCenter = FVector::ZeroVector;
	float CircleDirection = 1.f;

	bool bFlushed = false;
	bool bLeaving = false;
	float CalmTime = 0.f;
	float CalmNeeded = 10.f;
	float UpTime = 0.f;
	float HopTimer = 0.f;
	float CallTimer = 0.f;
	FRandomStream FlockRandom;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> PerchedInstances;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> HeadInstances;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> FlyingInstances;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> WingInstancesL;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> WingInstancesR;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> OuterInstancesL;
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> OuterInstancesR;

	/** The pieces' sockets (in their meshes' frames). */
	FVector HeadSocket = FVector(10.f, 0.f, 22.f);
	FVector ShoulderL = FVector(4.f, -5.f, 3.f);
	FVector ShoulderR = FVector(4.f, 5.f, 3.f);
	FVector WristL = FVector(0.f, -18.f, 0.f);
	FVector WristR = FVector(0.f, 18.f, 0.f);

	/**
	 * How high a perched body's middle stands over its feet (cm, at scale 1): the flying body's pivot is its middle, the
	 * perched one's its feet, so flights leave and reach a perch this much over it and the switch doesn't jump.
	 */
	float BodyLift = 10.f;

	/** Reused each push: one transform per bird for each piece. */
	TArray<FTransform> PerchedPose, HeadPose, FlyingPose, WingPoseL, WingPoseR, OuterPoseL, OuterPoseR;

	bool bBirdsReady = false;
};
