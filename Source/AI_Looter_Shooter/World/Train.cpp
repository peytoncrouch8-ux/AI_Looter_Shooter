// ATrain: the train at the platform put together from its parts, its story (cold and shut until Main 7) and sliding it
// along its track (its running gear is TrainRunningGear.cpp's).

#include "World/Train.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/MinimapSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const FName ATrain::TrainTag(TEXT("Train"));

namespace
{
	/** Train.py's parts (imported to /Game/Art/Vehicles), Smoke.py's plume, and the trim's dark glass for cold lamps. */
	const TCHAR* HearsePath = TEXT("/Game/Art/Vehicles/SM_HearseCar.SM_HearseCar");
	const TCHAR* PassengerPath = TEXT("/Game/Art/Vehicles/SM_PassengerCar.SM_PassengerCar");
	const TCHAR* LocomotivePath = TEXT("/Game/Art/Vehicles/SM_Locomotive_B.SM_Locomotive_B");
	const TCHAR* DoorPath = TEXT("/Game/Art/Vehicles/SM_HearseCarDoor.SM_HearseCarDoor");
	const TCHAR* DriverPath = TEXT("/Game/Art/Vehicles/SM_TrainWheelSet_Driver.SM_TrainWheelSet_Driver");
	const TCHAR* CarriagePath = TEXT("/Game/Art/Vehicles/SM_TrainWheelSet_Carriage.SM_TrainWheelSet_Carriage");
	const TCHAR* RodPath = TEXT("/Game/Art/Vehicles/SM_TrainCouplingRod.SM_TrainCouplingRod");
	const TCHAR* SmokePath = TEXT("/Game/Art/Props/SM_SmokePlume.SM_SmokePlume");
	const TCHAR* ColdGlassPath = TEXT("/Game/Art/Materials/MI_HouseTrim.MI_HouseTrim");

	/** Main 7, "The Lantern Leans", starts as Main 6 ends: from then on the train has steam up. */
	const FName WarmAfter(TEXT("Main6"));

	const FName CouplerFront(TEXT("Coupler_Front"));
	const FName CouplerBack(TEXT("Coupler_Back"));
	const FName DoorSocket(TEXT("Door"));
	const FName SmokeSocket(TEXT("Smoke"));
	const FName ArrivalSocket(TEXT("Arrival"));

	/** Without the models: each car's middle along the train from the hearse car's (Train.py's lengths over couplers, cm). */
	constexpr float FallbackCarOffsets[ATrain::NumCars] = { 0.f, 1128.f, 2181.f };

	/**
	 * Without the hearse car's Arrival socket: where Train.py puts it, on the platform at the doorway's middle (Blender
	 * (2.35, -0.675, 0.40) m, which the importer turns to Unreal (-Y, -X, Z)).
	 */
	const FVector FallbackArrival(67.5, -235.0, 40.0);

	/** The shots' camera, until the build script sets one: on the platform by the hearse car's back end, at eye height. */
	const FTransform DefaultShotCamera(FRotator(0.0, 7.0, 0.0), FVector(-450.0, -330.0, 205.0));

	/** An asset found only once it's in this checkout, so the class works, and its tests run, without it. In a constructor. */
	template <typename T>
	T* FindIfMade(const TCHAR* ObjectPath)
	{
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath))))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<T> Finder(ObjectPath);
		return Finder.Object;
	}

	/** A part that never stops anything: the door, the steam, the running gear (the bodies' hulls cover them). */
	void MakeLoose(UPrimitiveComponent& Part)
	{
		Part.SetMobility(EComponentMobility::Movable);
		Part.SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Part.SetGenerateOverlapEvents(false);
		Part.SetCanEverAffectNavigation(false);
	}
}

ATrain* ATrain::FindIn(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ATrain> It(World); It; ++It)
	{
		if (!It->IsActorBeingDestroyed())
		{
			return *It;
		}
	}
	return nullptr;
}

ATrain* ATrain::FindNear(const AActor* Station, float MaxDistance)
{
	const UWorld* World = Station ? Station->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	ATrain* Nearest = nullptr;
	double NearestDistance = FMath::Square(static_cast<double>(MaxDistance));
	for (TActorIterator<ATrain> It(World); It; ++It)
	{
		const double Distance = FVector::DistSquared(It->GetActorLocation(), Station->GetActorLocation());
		if (!It->IsActorBeingDestroyed() && Distance <= NearestDistance)
		{
			Nearest = *It;
			NearestDistance = Distance;
		}
	}
	return Nearest;
}

ATrain::ATrain()
{
	PrimaryActorTick.bCanEverTick = false;

	static UStaticMesh* const HearseModel = FindIfMade<UStaticMesh>(HearsePath);
	static UStaticMesh* const PassengerModel = FindIfMade<UStaticMesh>(PassengerPath);
	static UStaticMesh* const LocomotiveModel = FindIfMade<UStaticMesh>(LocomotivePath);
	static UStaticMesh* const DoorModel = FindIfMade<UStaticMesh>(DoorPath);
	static UStaticMesh* const DriverModel = FindIfMade<UStaticMesh>(DriverPath);
	static UStaticMesh* const CarriageModel = FindIfMade<UStaticMesh>(CarriagePath);
	static UStaticMesh* const RodModel = FindIfMade<UStaticMesh>(RodPath);
	static UStaticMesh* const SmokeModel = FindIfMade<UStaticMesh>(SmokePath);
	static UMaterialInterface* const TrimGlass = FindIfMade<UMaterialInterface>(ColdGlassPath);
	HearseCarMesh = HearseModel;
	PassengerCarMesh = PassengerModel;
	LocomotiveMesh = LocomotiveModel;
	DoorMesh = DoorModel;
	DriverWheelMesh = DriverModel;
	CarriageWheelMesh = CarriageModel;
	RodMesh = RodModel;
	SmokeMesh = SmokeModel;
	ColdGlass = TrimGlass;

	// Movable throughout: it stands still but for its two shots, and a part fixed in place couldn't go with it then.
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// The bodies are solid by their hulls: nobody climbs on, squeezes between the cars or walks under them.
	auto MakeBody = [this](const TCHAR* Name)
	{
		UStaticMeshComponent* Body = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Body->SetupAttachment(Root);
		Body->SetMobility(EComponentMobility::Movable);
		Body->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		return Body;
	};
	HearseCar = MakeBody(TEXT("HearseCar"));
	PassengerCar = MakeBody(TEXT("PassengerCar"));
	Locomotive = MakeBody(TEXT("Locomotive"));

	HearseDoor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HearseDoor"));
	HearseDoor->SetupAttachment(HearseCar, DoorSocket);
	MakeLoose(*HearseDoor);

	// Steam is a card: no shadow.
	Smoke = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Smoke"));
	Smoke->SetupAttachment(Locomotive, SmokeSocket);
	MakeLoose(*Smoke);
	Smoke->SetCastShadow(false);

	auto MakeGear = [this](const TCHAR* Name, bool bShadow)
	{
		UInstancedStaticMeshComponent* Gear = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Gear->SetupAttachment(Root);
		MakeLoose(*Gear);
		Gear->SetCastShadow(bShadow);
		return Gear;
	};
	DriverWheels = MakeGear(TEXT("DriverWheels"), true);
	CarriageWheels = MakeGear(TEXT("CarriageWheels"), true);
	// Thin bars by the wheels: their shadow reads as nothing at any distance.
	Rods = MakeGear(TEXT("Rods"), false);

	ShotCamera = DefaultShotCamera;
	WarmWhen.AfterMissions = { WarmAfter };
	Tags.Add(TrainTag);
	Tags.Add(MinimapTags::Obstacle);
}

void ATrain::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Placed or edited: coupled, on its wheels, cold, as the level shows it before Main 7.
	ApplyArt();
}

void ATrain::BeginPlay()
{
	Super::BeginPlay();
	Parked = GetActorTransform();
	bParkedSet = true;
	Travel = 0.f;
	// The running gear's layout isn't kept with the level: lay it out again on the bodies as they are.
	ApplyArt();
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &ATrain::HandleMissionsChanged);
	}
	RefreshStory();
}

void ATrain::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	MissionsChangedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Putting it together
// ---------------------------------------------------------------------------

void ATrain::ApplyArt()
{
	HearseCar->SetStaticMesh(HearseCarMesh);
	PassengerCar->SetStaticMesh(PassengerCarMesh);
	Locomotive->SetStaticMesh(LocomotiveMesh);
	CoupleCars();

	// The door on its hinge; without the hearse car's socket it would hang in the car's middle, so it isn't shown.
	const bool bHinge = HearseCar->DoesSocketExist(DoorSocket);
	HearseDoor->SetStaticMesh(DoorMesh);
	HearseDoor->AttachToComponent(HearseCar, FAttachmentTransformRules::SnapToTargetNotIncludingScale, bHinge ? DoorSocket : FName(NAME_None));
	HearseDoor->SetVisibility(bHinge && DoorMesh != nullptr);

	// The steam rises from the stack and leans downwind, whichever way the train faces.
	Smoke->SetStaticMesh(SmokeMesh);
	Smoke->AttachToComponent(Locomotive, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		Locomotive->DoesSocketExist(SmokeSocket) ? SmokeSocket : FName(NAME_None));
	Smoke->SetWorldRotation(FRotator(0.0, SmokeYaw, 0.0));

	DriverWheels->SetStaticMesh(DriverWheelMesh);
	CarriageWheels->SetStaticMesh(CarriageWheelMesh);
	Rods->SetStaticMesh(RodMesh);
	LayOutRunningGear();
	ApplyLook();
}

void ATrain::CoupleCars()
{
	UStaticMeshComponent* Cars[NumCars] = { HearseCar, PassengerCar, Locomotive };
	float Offset = 0.f;
	for (int32 Car = 1; Car < NumCars; ++Car)
	{
		// Where the car behind's front coupler plane is, the one ahead's back coupler plane goes.
		const UStaticMeshComponent* Behind = Cars[Car - 1];
		const UStaticMeshComponent* Ahead = Cars[Car];
		if (Behind->DoesSocketExist(CouplerFront) && Ahead->DoesSocketExist(CouplerBack))
		{
			Offset += static_cast<float>(Behind->GetSocketTransform(CouplerFront, RTS_Component).GetLocation().X
				- Ahead->GetSocketTransform(CouplerBack, RTS_Component).GetLocation().X);
		}
		else
		{
			Offset = FallbackCarOffsets[Car];
		}
		Cars[Car]->SetRelativeLocationAndRotation(FVector(Offset, 0.0, 0.0), FRotator::ZeroRotator);
	}
	HearseCar->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
}

UStaticMeshComponent* ATrain::GetCar(int32 Car) const
{
	switch (Car)
	{
	case 0:
		return HearseCar;
	case 1:
		return PassengerCar;
	case 2:
		return Locomotive;
	default:
		return nullptr;
	}
}

float ATrain::GetCarOffset(int32 Car) const
{
	const UStaticMeshComponent* Body = GetCar(Car);
	return Body ? static_cast<float>(Body->GetRelativeLocation().X) : 0.f;
}

// ---------------------------------------------------------------------------
// Cold or warm
// ---------------------------------------------------------------------------

void ATrain::ApplyLook()
{
	Smoke->SetVisibility(bWarm && SmokeMesh != nullptr);
	HearseDoor->SetRelativeRotation(FRotator(0.0, bWarm ? DoorOpenYaw : 0.0, 0.0));

	// Every body's lamps: their own glow while warm, the trim's dark glass while cold.
	for (int32 Car = 0; Car < NumCars; ++Car)
	{
		UStaticMeshComponent* Body = GetCar(Car);
		const UStaticMesh* Model = Body ? Body->GetStaticMesh() : nullptr;
		const int32 Slot = Model ? Body->GetMaterialIndex(GlassSlot) : INDEX_NONE;
		if (Slot == INDEX_NONE)
		{
			continue;
		}
		UMaterialInterface* Wanted = bWarm ? Model->GetMaterial(Slot) : ColdGlass.Get();
		if (Wanted && Body->GetMaterial(Slot) != Wanted)
		{
			Body->SetMaterial(Slot, Wanted);
		}
	}
}

void ATrain::SetWarm(bool bInWarm)
{
	if (bWarm == bInWarm)
	{
		return;
	}
	bWarm = bInWarm;
	ApplyLook();
	UE_LOG(LogLooter, Log, TEXT("%s: %s"), *GetActorNameOrLabel(), bWarm ? TEXT("steam up, lamps lit, the hearse car's door open")
		: TEXT("cold and shut"));
}

bool ATrain::ShouldBeWarm(const FCampaignRecord& Campaign, const UMissionRunner* Runner) const
{
	return WarmWhen.IsMet(Campaign, Runner);
}

void ATrain::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner || bLookHeld)
	{
		return;
	}
	SetWarm(ShouldBeWarm(Runner->GetCampaign(), Runner));
}

void ATrain::HoldLook(bool bInWarm)
{
	bLookHeld = true;
	SetWarm(bInWarm);
}

bool ATrain::IsDoorOpen() const
{
	return HearseDoor && !FMath::IsNearlyZero(HearseDoor->GetRelativeRotation().Yaw, 1.0);
}

void ATrain::HandleMissionsChanged()
{
	RefreshStory();
}

// ---------------------------------------------------------------------------
// Moving
// ---------------------------------------------------------------------------

FTransform ATrain::GetParked() const
{
	return bParkedSet ? Parked : GetActorTransform();
}

FVector ATrain::GetForward() const
{
	FVector Forward = GetParked().GetUnitAxis(EAxis::X);
	Forward.Z = 0.0;
	return Forward.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
}

void ATrain::SetTravel(float Distance)
{
	if (!bParkedSet)
	{
		Parked = GetActorTransform();
		bParkedSet = true;
	}
	Travel = Distance;
	SetActorLocation(Parked.GetLocation() + GetForward() * Distance, /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
	TurnRunningGear(Distance);
}

FTransform ATrain::GetArrivalSpot() const
{
	FVector Local = FallbackArrival;
	if (HearseCar->GetStaticMesh() && HearseCar->DoesSocketExist(ArrivalSocket))
	{
		Local = (HearseCar->GetSocketTransform(ArrivalSocket, RTS_Component) * HearseCar->GetRelativeTransform()).GetLocation();
	}
	// Stepping out of the door onto the platform: facing away from the car, across the boards (its -Y).
	const FTransform At = GetParked();
	return FTransform(FRotator(0.0, At.Rotator().Yaw - 90.0, 0.0), At.TransformPosition(Local));
}

FTransform ATrain::GetShotCameraTransform() const
{
	return ShotCamera * GetParked();
}
