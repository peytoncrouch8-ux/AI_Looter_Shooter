#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryLine.h"
#include "World/OreLiftMotion.h"
#include "OreLift.generated.h"

class AOreLiftBellPull;
class UAudioComponent;
class UMaterialInterface;
class UMissionRunner;
class USplineMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FCampaignRecord;

/** The lift's two landings. */
UENUM(BlueprintType)
enum class EOreLiftEnd : uint8
{
	/** The Old Quarry's floor, under the winch house's sheave. */
	Bottom,
	/** The lift head on the granite shoulder behind Larkspur's crest. */
	Top,
};

/** What a landing's bell pull does now. */
UENUM(BlueprintType)
enum class EOreLiftBell : uint8
{
	/** The lift doesn't run yet: a tap gives the pull's dull clunk and the reason why. */
	Closed,
	/** The cage waits at this landing: hold to ride it. */
	Ride,
	/** The cage is at the other landing: hold to call it here. */
	Call,
	/** The cage is on its way, or someone rides it. */
	Busy,
};

/**
 * The Providence Mining Company's ore lift (Docs/Areas/RansomsRest_Expansion.md, "Dead Lift" and section 6 "The ore lift"):
 * an open ore cage hung from its cable, from the Old Quarry's floor over one timber tower to the lift head behind Larkspur's
 * crest. The actor stands where the cage rests at the bottom; the top landing and the tower's sheave are its settings in its
 * own space (Tools/Unreal/build_area_lift.py sets them from the art's sockets). The winch house, tower and head are scenery
 * the level's build places; the lift owns the cage, its two spans of cable and the winch's sound.
 *  - It runs once its mission says so (OpensWithMission at OpensAtStep: Dead Lift's "ride it up"; or OpenFlag): until then
 *    each bell pull (AOreLiftBellPull) gives a dull clunk and says why it won't run.
 *  - Open, a held bell pull rides the cage waiting at its landing (Ride), or calls it from the other (CallTo: an empty cage
 *    comes in about 8 s, its bell answering as it lands).
 *  - A ride is a scene (Scenes/OreLiftRide: carried, look only, skippable, about 24 s): the missions hear Board.Lift as it
 *    starts and Lift.Arrived.<Top|Bottom> as the player steps off. The autosave waits through it. The player's own body
 *    waits unseen on the far landing the whole way while the view rides the cage, so a save, a quit or a death mid-ride
 *    always finds them at the ride's end, never in a cage hung over the ridge face.
 *  - The motion is OreLiftMotion's plain math: eased ends, the cage swinging on its grip, bobbing on the cable and
 *    clanking over the tower's three sheaves, lifting off one landing and settling onto the other.
 * OreLift.cpp is the lift's pieces and where the cage hangs; OreLiftRuns.cpp its rules, calls and rides;
 * OreLiftUnderWay.cpp the run under way (the cage moved, its sounds, the cable's shiver).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API AOreLift : public AActor
{
	GENERATED_BODY()

public:
	AOreLift();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Where things are (OreLift.cpp) ---

	/** Where the cage rests at End's landing (its pivot: the floor's middle), in the world. */
	FTransform GetCageRest(EOreLiftEnd End) const;

	/** Where a rider's feet go in the cage at rest at End's landing, in the world. */
	FVector GetRiderSpot(EOreLiftEnd End) const;

	/** The cable's line through the tower, from the bottom landing's grip to the top's (world). */
	TArray<FVector> GetCablePoints() const;

	/** The cable's length between the landings' grips (cm). */
	float GetCableLength() const;

	/** Where the grip is Distance along the cable from From's landing (world). */
	FVector GetGripAt(EOreLiftEnd From, float Distance) const;

	/** Where the cage hangs now (world): its rest, or wherever the run under way has it. */
	FTransform GetCageTransform() const;

	/** The rider's eye in the cage as it hangs now, for a rider whose eyes sit EyeOffset over their feet (in their own yaw's
	 *  frame) and who looks along Yaw: what the ride's camera shows. */
	FVector GetRiderEye(const FVector& EyeOffset, float Yaw) const;

	/** The run's heading over the ground (from its start toward its end; bottom to top at rest), what the sway turns about. */
	FVector GetRunHeading() const;

	/** The cage's swing now as a turn in the world (none at rest): what tips the rider's head with it. */
	const FQuat& GetSwayTurn() const { return SwayTurn; }

	/** The cage's sway now (the run under way's, none at rest). */
	const FOreLiftSway& GetSway() const { return Sway; }

	// --- The rules (OreLiftRuns.cpp) ---

	/**
	 * It runs in this story: Mission is finished or waiting to be turned in, or it's being played at AtStep or later, or
	 * Flag is set. With neither a mission nor a flag, always. Apart from the world, for the tests.
	 */
	static bool IsOpenFor(const FCampaignRecord& Campaign, const UMissionRunner* Runner, FName Mission, int32 AtStep, FName Flag);

	/** It runs now (the story here, or forced open from the console). A level without a story runs it unless it has a gate. */
	bool IsOpen() const;

	/** Why it won't run, in words for the caption (empty when it runs). */
	FText GetClosedReason() const;

	/** What End's bell pull does now. */
	EOreLiftBell GetBellState(EOreLiftEnd End) const;

	/** The landing the cage rests at, or last left. */
	EOreLiftEnd GetCageEnd() const { return CageEnd; }

	/** On the move: called, or ridden. */
	bool IsMoving() const { return bCalled || bRiding; }
	bool IsRiding() const { return bRiding; }
	bool IsCalled() const { return bCalled; }

	/** The autosave waits on this ride. */
	bool IsHoldingSaves() const { return bHoldingSaves; }

	/** The rider of the ride under way, or null. */
	AActor* GetRider() const { return Rider.Get(); }

	/** This would be the story's first ride (its RiddenFlag not set yet). */
	bool IsFirstRide() const;

	// --- Using it (OreLiftRuns.cpp) ---

	/**
	 * Sends the empty cage to End's landing (it comes in CallSeconds, about 8). False when it's closed, on the move, or
	 * there already.
	 */
	bool CallTo(EOreLiftEnd End);

	/**
	 * Rides Rider from From's landing (where the cage must wait) to the other: the ride's scene, the mission events, the
	 * autosave held. With scenes off (tour and perf runs) the ride is over at once. False when it can't go now: closed, the
	 * cage elsewhere or moving, a scene already playing, or Rider missing or dead.
	 */
	bool Ride(EOreLiftEnd From, AActor* InRider);

	/** The closed bell pull at End tugged: its dull clunk and the reason in a caption. */
	void RingClosed(EOreLiftEnd End);

	/** Forces it open, or back to what the story says (Looter.Lift.Open). */
	void SetForcedOpen(bool bInForced) { bForcedOpen = bInForced; }

	/** Puts the cage at rest at End's landing at once, and ends any call (a test, the console, a level beginning). */
	void SetResting(EOreLiftEnd End);

	/** Moves a call on, and watches the rider of a ride: the tick calls it while it's on the move; tests call it. */
	void Advance(float DeltaSeconds);

	/** A bell pull at End of this lift joins it (its BeginPlay). */
	void RegisterBellPull(AOreLiftBellPull& Pull, EOreLiftEnd End);

	// --- The run under way (OreLiftUnderWay.cpp; the ride's scene drives a ride's, the tick a call's) ---

	/** The run's shape (timing, cable) for a run from From: a ride's or a call's timing. */
	FOreLiftRunShape MakeRun(EOreLiftEnd From, bool bRide) const;

	/** The run under way, and how long it takes. */
	const FOreLiftRunShape& GetRun() const { return Run; }
	EOreLiftEnd GetRunFrom() const { return RunFrom; }

	/**
	 * Hangs the cage where the run under way has it Seconds in, with its sound as it passes each moment (the take-up, each
	 * clank, the landing); bQuiet leaves the sounds out (a skip).
	 */
	void SetRunTime(float Seconds, bool bQuiet);

	/** The ride is over (the scene ended, played or skipped; OreLiftRuns.cpp): the rider let go on the far landing, the
	 *  missions told. */
	void FinishRide();

	/** Lays the rider down on End's landing for the ride, unseen and still, until FinishRide lets them go (OreLiftRuns.cpp). */
	void ParkRider(EOreLiftEnd End);

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** The ore cage (its pivot the floor's middle, its front +X); it moves, and stops the player as a cage does. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Cage;

	/** Stand-ins while the cage's model isn't made: a floor slab and its hanger up to the grip. */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CageFloorStandIn;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HangerStandIn;

	/** The cable's two spans: the bottom landing's grip up to the tower's sheave, and on to the top landing's. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineMeshComponent> CableLower;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USplineMeshComponent> CableUpper;

	/** Where the winch's work is heard: the winch house's drum (its sound hangs on it). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Winch;

	// --- The line (the actor's own space; the build script sets them from the art's sockets) ---

	/** Where the cage rests at the bottom landing (usually the actor itself) and at the top. */
	UPROPERTY(EditAnywhere, Category = "Lift|Line", meta = (MakeEditWidget))
	FTransform BottomRest;

	UPROPERTY(EditAnywhere, Category = "Lift|Line", meta = (MakeEditWidget))
	FTransform TopRest;

	/** Where the cable rides over the tower's sheaves: the grip passes here. */
	UPROPERTY(EditAnywhere, Category = "Lift|Line", meta = (MakeEditWidget))
	FVector TowerSheave = FVector(1200.0, 300.0, 2900.0);

	// --- Art ---

	UPROPERTY(EditAnywhere, Category = "Lift|Art")
	TObjectPtr<UStaticMesh> CageMesh;

	/** The cable: a short length along its longest side, stretched along each span. */
	UPROPERTY(EditAnywhere, Category = "Lift|Art")
	TObjectPtr<UStaticMesh> CableMesh;

	UPROPERTY(EditAnywhere, Category = "Lift|Art")
	TObjectPtr<UMaterialInterface> CableMaterial;

	/** The haul rope's thickness (cm). */
	UPROPERTY(EditAnywhere, Category = "Lift|Art", meta = (ClampMin = "0.5"))
	float CableThickness = 3.2f;

	/** The cage's sockets: the grip on the cable (the top of its bail) and where the rider stands. */
	UPROPERTY(EditAnywhere, Category = "Lift|Art")
	FName GripSocket = TEXT("Grip");

	UPROPERTY(EditAnywhere, Category = "Lift|Art")
	FName RiderSocket = TEXT("Rider");

	/** Without those sockets (the stand-in): the grip 3.5 m over the floor, the rider on it. */
	UPROPERTY(EditAnywhere, Category = "Lift|Art")
	FVector FallbackGrip = FVector(0.0, 0.0, 350.0);

	UPROPERTY(EditAnywhere, Category = "Lift|Art")
	FVector FallbackRider = FVector(0.0, 0.0, 8.0);

	// --- Motion ---

	/** A ride: the take-up, the travel (eased at both ends), the settle; about 24 s with the boarding. */
	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "0"))
	float RideTakeUpSeconds = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "1"))
	float RideTravelSeconds = 21.f;

	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "0"))
	float RideEaseSeconds = 3.5f;

	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "0"))
	float RideSettleSeconds = 1.4f;

	/** An empty cage called: brisker, 8 s from bell to landing. */
	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "0"))
	float CallTakeUpSeconds = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "1"))
	float CallTravelSeconds = 6.5f;

	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "0"))
	float CallEaseSeconds = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "0"))
	float CallSettleSeconds = 1.f;

	/** How the cage hangs: its pendulum, damping and the most it swings; the breeze; the lift-off, clanks and landing. */
	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "50"))
	float PendulumLength = 240.f;

	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "0"))
	float SwingDamping = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "0"))
	float MaxSwingDegrees = 8.f;

	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "0"))
	float WindDegrees = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "0"))
	float ClankBob = 2.2f;

	/** How far the cable shivers (cm, at a span's middle) as the grip clanks over the tower, dying away. */
	UPROPERTY(EditAnywhere, Category = "Lift|Motion", meta = (ClampMin = "0"))
	float CableShiver = 6.f;

	// --- The story ---

	/** The mission that opens it, from this step of it on (counted from 0): Dead Lift's "ride the lift to the top". */
	UPROPERTY(EditAnywhere, Category = "Lift|Story")
	FName OpensWithMission = TEXT("DeadLift");

	UPROPERTY(EditAnywhere, Category = "Lift|Story", meta = (ClampMin = "0"))
	int32 OpensAtStep = 3;

	/** Or a campaign flag that opens it (None: none). */
	UPROPERTY(EditAnywhere, Category = "Lift|Story")
	FName OpenFlag;

	/** Set on the first ride's arrival: rides after it are plain, and the story knows the mountain is reached. */
	UPROPERTY(EditAnywhere, Category = "Lift|Story")
	FName RiddenFlag = TEXT("OreLift.Ridden");

	/** What a ride tells the missions: Board.<Vehicle> as it starts, <ArrivedEvent>.<Top|Bottom> as it lands. */
	UPROPERTY(EditAnywhere, Category = "Lift|Story")
	FName Vehicle = TEXT("Lift");

	UPROPERTY(EditAnywhere, Category = "Lift|Story")
	FString ArrivedEvent = TEXT("Lift.Arrived");

	/** Why the closed bell pull gets no answer: before its mission (a dead lift), and during it, before she has steam. */
	UPROPERTY(EditAnywhere, Category = "Lift|Story", meta = (MultiLine = true))
	FText ClosedColdLine;

	UPROPERTY(EditAnywhere, Category = "Lift|Story", meta = (MultiLine = true))
	FText ClosedSteamLine;

	/** Said as the first ride begins (Hiram's "Hold the rail."). */
	UPROPERTY(EditAnywhere, Category = "Lift|Story")
	TArray<FStoryLine> FirstRideLines;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	// OreLift.cpp
	/** Each mesh on its component, the stand-ins shown while the cage's model isn't there, the cable laid out. */
	void ApplyArt();

	/** The cage's own grip and rider points (its sockets, else the fallbacks), in its space. */
	FVector GetGripLocal() const;
	FVector GetRiderLocal() const;

	/** The cable's points in the actor's space: the two landings' grips and the tower's sheave between. */
	TArray<FVector> GetCablePointsLocal() const;

	/** Hangs the cage Distance along the cable from From, swayed by InSway (the actor's space); the swing's turn and the run's
	 *  heading over the ground come out too. */
	FTransform CageAt(EOreLiftEnd From, float Distance, const FOreLiftSway& InSway, FQuat& OutSwing, FVector& OutHeading) const;

	/** Lays the cable's spans along the line, shivering by Shiver (cm at a span's middle) as the grip clanks over. */
	void LayCable(float Shiver);

	// OreLiftUnderWay.cpp
	/** Starts a run from From (a ride's, or a call's): its shape, the swing from rest, its moments' sounds still to come. */
	void BeginRun(EOreLiftEnd From, bool bRide);

	/** The cable's song on the cage and the winch's work in its house, while the cage runs. */
	void StartLoops();
	void StopLoops();

	// OreLiftRuns.cpp
	void HoldSaves();
	void ReleaseSaves();

	/** The bell pull at End, when it has joined. */
	AOreLiftBellPull* FindBellPull(EOreLiftEnd End) const;

	/** Ticks only while on the move. */
	void RefreshTick();

	/** Tells the missions Name happened here. */
	void TellMissions(FName Name);

	/** The engine's shapes the stand-ins are made of. */
	UPROPERTY()
	TObjectPtr<UStaticMesh> StandInCube;

	UPROPERTY()
	TObjectPtr<UStaticMesh> StandInCylinder;

	EOreLiftEnd CageEnd = EOreLiftEnd::Bottom;
	EOreLiftEnd RunFrom = EOreLiftEnd::Bottom;
	FOreLiftRunShape Run;
	FOreLiftSwingState SwingState;
	FOreLiftSway Sway;
	/** The cage's swing as a turn in the world, and the run's heading over the ground (world). */
	FQuat SwayTurn = FQuat::Identity;
	FVector RunHeading = FVector::ForwardVector;
	/** The cable's last shiver, so a still cable isn't laid again every frame. */
	float LastShiver = 0.f;
	TArray<float> ClankTimes;
	/** Seconds into the run under way, and how many of its clanks have sounded. */
	float RunTime = -1.f;
	int32 ClanksHeard = 0;
	bool bTakeUpHeard = false;
	bool bLandHeard = false;

	bool bCalled = false;
	bool bRiding = false;
	bool bForcedOpen = false;
	bool bHoldingSaves = false;

	/** The rider of the ride under way, and whether they were lost on the way (dead or gone: no arrival for the missions). */
	TWeakObjectPtr<AActor> Rider;
	bool bRiderLost = false;

	TWeakObjectPtr<AOreLiftBellPull> BottomPull;
	TWeakObjectPtr<AOreLiftBellPull> TopPull;

	TWeakObjectPtr<UAudioComponent> CableLoop;
	TWeakObjectPtr<UAudioComponent> WinchLoop;
};
