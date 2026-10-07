#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Story/StoryCondition.h"
#include "Train.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UMissionRunner;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
struct FCampaignRecord;

/**
 * The train standing at an area's platform (Docs/Areas/RansomsRest.md: Stations and the train; Track and train collision),
 * assembled from Art/Models/Vehicles/Train.py's parts: Locomotive B, the passenger car and Tilly's hearse car, coupled
 * where their Coupler sockets meet, each axle (its Axle_<n> sockets) on the wheel set whose tread puts it on the rails
 * (picked by the socket's height over the rail top), the locomotive's coupling rods on its cranks (Rod_L, Rod_R) and the
 * hearse car's side door on its hinge (Door). The actor's origin is the hearse car's, on the ground under its middle, and
 * its +X is the way out, where the locomotive faces; the platform is on its -Y side, where the hearse car's door and its
 * SOCKET_Arrival are. Every car's hull is one box reaching its coupler plane, so the gaps between them are closed.
 *
 * It stands cold and shut until the story warms it (WarmWhen: from Main 7, "The Lantern Leans", on): then the locomotive
 * has steam up (the smoke plume on its stack), the lamps (every body's LanternGlow slot, ColdGlass while cold) are lit and
 * Tilly's car's door stands open. It moves only in its two short shots (Scenes/TrainShots.h): SetTravel slides it along
 * its track and turns every wheel by the distance, the rods riding on the drivers' cranks. It never ticks. Tagged Obstacle
 * for the minimap, like the buildings. Tools/Unreal/build_area_depot.py places it where the layout parks the hearse car.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ATrain : public AActor
{
	GENERATED_BODY()

public:
	/** The tag the level's train carries (the missions and the console find it by it). */
	static const FName TrainTag;

	/** The cars, back to front: the hearse car at the origin, the passenger car coupled ahead, the locomotive at the front. */
	static constexpr int32 NumCars = 3;

	/**
	 * How a wheel of tread Radius (cm) turns as the train covers Distance along its +X: about the axle (the car's Y), by the
	 * distance over the radius, so the tread rolls on the rail without slipping.
	 */
	static FQuat WheelSpin(float Distance, float Radius);

	/** The level's train (the first, tagged TrainTag), or null. */
	static ATrain* FindIn(const UWorld* World);

	/** The train nearest Station (its platform's), within MaxDistance (cm), or null. */
	static ATrain* FindNear(const AActor* Station, float MaxDistance = 8000.f);

	ATrain();

	virtual void OnConstruction(const FTransform& Transform) override;

	// --- The story ---

	/** Warm (steam up, lamps lit, the hearse car's door open) or cold and shut, now. */
	void SetWarm(bool bInWarm);
	bool IsWarm() const { return bWarm; }

	/** WarmWhen holds in Campaign (with the missions running in this level). */
	bool ShouldBeWarm(const FCampaignRecord& Campaign, const UMissionRunner* Runner = nullptr) const;

	/** Reads the story again: warm from Main 7 on, cold before (a story taken back from the console goes cold again). */
	void RefreshStory();

	/** The look stays as set, whatever the story says, until this level ends (Looter.Train.Warm). */
	void HoldLook(bool bInWarm);

	// --- Moving (the shots) ---

	/** Slides it Distance (cm) along its track from where it stands parked (out: positive), every wheel turned to match. */
	void SetTravel(float Distance);

	/** How far it stands out along its track from its parked spot (cm). */
	float GetTravel() const { return Travel; }

	/** Where it stands parked: where it was placed, as play began. */
	FTransform GetParked() const;

	/** The way out along the track (its +X, flat). */
	FVector GetForward() const;

	// --- Where things are, parked ---

	/** Where a trip on it arrives: the hearse car's SOCKET_Arrival on the platform, the feet's spot and the way to face. */
	FTransform GetArrivalSpot() const;

	/** The shots' fixed camera on the platform (ShotCamera, in the world). */
	FTransform GetShotCameraTransform() const;

	/** Each car's place along the train from the hearse car's middle (cm), back to front, as they're coupled. */
	float GetCarOffset(int32 Car) const;

	/** The body of car Car (0 the hearse car, 1 the passenger car, 2 the locomotive), or null. */
	UStaticMeshComponent* GetCar(int32 Car) const;

	/** The axles laid out, and how many of them run on drivers. */
	int32 GetNumAxles() const { return Axles.Num(); }
	int32 GetNumDriverAxles() const;

	/** The hearse car's door stands open (its hinge turned by DoorOpenYaw). */
	bool IsDoorOpen() const;

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** Tilly's funeral car, at the actor's origin: Ellis rides in it between areas. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HearseCar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PassengerCar;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Locomotive;

	/** The hearse car's side door, on its hinge (the car's Door socket): shut while cold, open from Main 7. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HearseDoor;

	/** Steam from the locomotive's stack (its Smoke socket), while it's warm. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Smoke;

	/** The wheel sets: the locomotive's drivers, and every other axle's carriage wheels; and the two coupling rods. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> DriverWheels;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> CarriageWheels;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Rods;

	// --- The models (Art/Models/Vehicles/Train.py, Art/Models/Props/Smoke.py) ---

	UPROPERTY(EditAnywhere, Category = "Train|Art")
	TObjectPtr<UStaticMesh> HearseCarMesh;

	UPROPERTY(EditAnywhere, Category = "Train|Art")
	TObjectPtr<UStaticMesh> PassengerCarMesh;

	UPROPERTY(EditAnywhere, Category = "Train|Art")
	TObjectPtr<UStaticMesh> LocomotiveMesh;

	UPROPERTY(EditAnywhere, Category = "Train|Art")
	TObjectPtr<UStaticMesh> DoorMesh;

	UPROPERTY(EditAnywhere, Category = "Train|Art")
	TObjectPtr<UStaticMesh> DriverWheelMesh;

	UPROPERTY(EditAnywhere, Category = "Train|Art")
	TObjectPtr<UStaticMesh> CarriageWheelMesh;

	UPROPERTY(EditAnywhere, Category = "Train|Art")
	TObjectPtr<UStaticMesh> RodMesh;

	UPROPERTY(EditAnywhere, Category = "Train|Art")
	TObjectPtr<UStaticMesh> SmokeMesh;

	/** The lamps' glass slot on every body, and what it shows while cold: the trim's dark window glass (Train.py's notes). */
	UPROPERTY(EditAnywhere, Category = "Train|Art")
	FName GlassSlot = TEXT("LanternGlow");

	UPROPERTY(EditAnywhere, Category = "Train|Art")
	TObjectPtr<UMaterialInterface> ColdGlass;

	/** How far the hearse car's door turns on its hinge to stand open (degrees): flat against the panel behind it. */
	UPROPERTY(EditAnywhere, Category = "Train|Art")
	float DoorOpenYaw = 180.f;

	/** Which way the steam leans, in the world (degrees of yaw; the chimneys' plumes lean the same way downwind). */
	UPROPERTY(EditAnywhere, Category = "Train|Art")
	float SmokeYaw = 19.f;

	// --- The running gear (the rail contract: Train.py) ---

	/** The rail heads' height over the ground the bodies stand on (cm). */
	UPROPERTY(EditAnywhere, Category = "Train|Running Gear", meta = (ClampMin = "0"))
	float RailTop = 25.f;

	/** The wheel sets' tread radii (cm): an axle whose socket stands nearer the drivers' height over the rail takes them. */
	UPROPERTY(EditAnywhere, Category = "Train|Running Gear", meta = (ClampMin = "1"))
	float DriverRadius = 52.f;

	UPROPERTY(EditAnywhere, Category = "Train|Running Gear", meta = (ClampMin = "1"))
	float CarriageRadius = 42.f;

	// --- The shots ---

	/**
	 * The shots' fixed camera, in the train's own frame as it stands parked: on the platform beside the hearse car's back
	 * end, at a standing eye's height, looking out along the train toward the gap (it pulls away from it, and backs in
	 * toward it). The build script may set it from the level.
	 */
	UPROPERTY(EditAnywhere, Category = "Train|Shots")
	FTransform ShotCamera;

	UPROPERTY(EditAnywhere, Category = "Train|Shots", meta = (ClampMin = "10", ClampMax = "120"))
	float ShotFieldOfView = 55.f;

	// --- The story ---

	/** When it's warm: steam up, lamps lit, the hearse car's door open (from Main 7, "The Lantern Leans": after Main 6). */
	UPROPERTY(EditAnywhere, Category = "Train|Story")
	FStoryCondition WarmWhen;

	/** As the level shows it before the story says (cold and shut, as it stands until Main 7). */
	UPROPERTY(EditAnywhere, Category = "Train|Story")
	bool bWarm = false;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** One axle laid out: its car, its middle in the actor's frame at rest, its rest turn, and the wheel set on it. */
	struct FAxle
	{
		FVector Middle = FVector::ZeroVector;
		FQuat Rest = FQuat::Identity;
		float Radius = 0.f;
		bool bDriver = false;
	};

	/** A coupling rod: where its driver's axle is (at the rod's own side), and its crank pin's place off it at rest. */
	struct FRodPin
	{
		FVector Axle = FVector::ZeroVector;
		FVector RestOffset = FVector::ZeroVector;
		FQuat Rest = FQuat::Identity;
	};

	/** Every mesh on its part, the cars coupled, the door and the steam on their sockets, the running gear laid out. */
	void ApplyArt();

	/** Couples the cars where their couplers meet (fixed spacings without the models). */
	void CoupleCars();

	/** Finds every axle and rod on the bodies' sockets and puts a wheel set or a rod on each (at rest). */
	void LayOutRunningGear();

	/** The wheels and rods turned for the train having covered Distance. */
	void TurnRunningGear(float Distance);

	/** The smoke, the lamps' glass and the door as bWarm says. */
	void ApplyLook();

	void HandleMissionsChanged();

	/** Its placed transform, kept as play begins (the shots move it from there). */
	FTransform Parked = FTransform::Identity;
	bool bParkedSet = false;
	float Travel = 0.f;
	bool bLookHeld = false;

	TArray<FAxle> Axles;
	TArray<FRodPin> RodPins;

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle MissionsChangedHandle;
};
