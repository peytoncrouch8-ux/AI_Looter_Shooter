// AOreLift: its pieces (the cage, the cable's spans, the winch's spot, the stand-ins), the line through the tower and where
// the cage hangs on it. Its rules, calls and rides are OreLiftRuns.cpp's.

#include "World/OreLift.h"
#include "AI_Looter_Shooter.h"
#include "Algo/Reverse.h"
#include "Components/SceneComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "OreLift"

namespace
{
	/** The mountain art's cage and cable (Art/Models/Buildings/OreLift.py), and the engine's shapes for the stand-ins. */
	const TCHAR* CagePath = TEXT("/Game/Art/Buildings/SM_RR_LiftCage.SM_RR_LiftCage");
	const TCHAR* CablePath = TEXT("/Game/Art/Buildings/SM_RR_LiftCable.SM_RR_LiftCable");
	const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	/** The mountain's iron (looter_mountain.py's IronBlack): the stand-in cable's. */
	const TCHAR* IronPath = TEXT("/Game/Art/Materials/MI_IronBlack.MI_IronBlack");

	/** The stand-in cage: a floor slab this wide and thick (cm), and its hanger rod's thickness. */
	constexpr double StandInFloor = 160.0;
	constexpr double StandInSlab = 8.0;
	constexpr double StandInHanger = 8.0;

	/** The engine's shapes are 100 cm a side. */
	constexpr double EngineShape = 100.0;

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

	/** A socket's place in its mesh, or unset. */
	TOptional<FVector> SocketIn(const UStaticMesh* Mesh, FName Socket)
	{
		const UStaticMeshSocket* Found = Mesh && !Socket.IsNone() ? Mesh->FindSocket(Socket) : nullptr;
		return Found ? TOptional<FVector>(Found->RelativeLocation) : TOptional<FVector>();
	}

	/** The direction over the ground from From to To, straight ahead (+X) when they stand one over the other. */
	FVector Flat(const FVector& From, const FVector& To)
	{
		return FVector(To.X - From.X, To.Y - From.Y, 0.0).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	}

	/** The point Distance along a line of points (clamped to its ends). */
	FVector AlongLine(const TArray<FVector>& Points, float Distance)
	{
		if (Points.IsEmpty())
		{
			return FVector::ZeroVector;
		}
		double Left = FMath::Max(static_cast<double>(Distance), 0.0);
		for (int32 Index = 1; Index < Points.Num(); ++Index)
		{
			const FVector Span = Points[Index] - Points[Index - 1];
			const double Length = Span.Size();
			if (Left <= Length && Length > UE_KINDA_SMALL_NUMBER)
			{
				return Points[Index - 1] + Span * (Left / Length);
			}
			Left -= Length;
		}
		return Points.Last();
	}

	/** Which of a mesh's sides is its longest, as the axis a spline mesh stretches it along, and its girth across that. */
	ESplineMeshAxis::Type LongAxisOf(const UStaticMesh* Mesh, double& OutGirth)
	{
		const FBox Box = Mesh ? Mesh->GetBoundingBox() : FBox(ForceInit);
		const FVector Size = Box.IsValid ? Box.GetSize() : FVector(EngineShape);
		if (Size.Z >= Size.X && Size.Z >= Size.Y)
		{
			OutGirth = FMath::Max(FMath::Max(Size.X, Size.Y), 1.0);
			return ESplineMeshAxis::Z;
		}
		if (Size.Y >= Size.X)
		{
			OutGirth = FMath::Max(FMath::Max(Size.X, Size.Z), 1.0);
			return ESplineMeshAxis::Y;
		}
		OutGirth = FMath::Max(FMath::Max(Size.Y, Size.Z), 1.0);
		return ESplineMeshAxis::X;
	}

	USplineMeshComponent* MakeSpan(AActor& Owner, const TCHAR* Name, USceneComponent* Parent)
	{
		USplineMeshComponent* Span = Owner.CreateDefaultSubobject<USplineMeshComponent>(Name);
		Span->SetupAttachment(Parent);
		Span->SetMobility(EComponentMobility::Movable);
		// A haul rope high overhead: nothing walks into it, and its shadow is too slight to pay for.
		Span->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		Span->SetGenerateOverlapEvents(false);
		Span->SetCanEverAffectNavigation(false);
		Span->SetCastShadow(false);
		return Span;
	}
}

AOreLift::AOreLift()
{
	// Ticks only while the cage is on the move (RefreshTick).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	static UStaticMesh* const CageModel = FindIfMade<UStaticMesh>(CagePath);
	static UStaticMesh* const CableModel = FindIfMade<UStaticMesh>(CablePath);
	static UStaticMesh* const Cube = FindIfMade<UStaticMesh>(CubePath);
	static UStaticMesh* const Cylinder = FindIfMade<UStaticMesh>(CylinderPath);
	static UMaterialInterface* const Iron = FindIfMade<UMaterialInterface>(IronPath);
	CageMesh = CageModel;
	CableMesh = CableModel ? CableModel : Cylinder;
	CableMaterial = CableModel ? nullptr : Iron;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// The cage moves, and stops the player as a cage does: they ride standing on its floor.
	Cage = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cage"));
	Cage->SetupAttachment(Root);
	Cage->SetMobility(EComponentMobility::Movable);
	Cage->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Cage->SetCanEverAffectNavigation(false);
	Cage->SetStaticMesh(CageModel);

	CageFloorStandIn = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CageFloorStandIn"));
	CageFloorStandIn->SetupAttachment(Cage);
	CageFloorStandIn->SetMobility(EComponentMobility::Movable);
	CageFloorStandIn->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	CageFloorStandIn->SetCanEverAffectNavigation(false);
	CageFloorStandIn->SetStaticMesh(Cube);

	HangerStandIn = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HangerStandIn"));
	HangerStandIn->SetupAttachment(Cage);
	HangerStandIn->SetMobility(EComponentMobility::Movable);
	HangerStandIn->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	HangerStandIn->SetCanEverAffectNavigation(false);
	HangerStandIn->SetCastShadow(false);
	HangerStandIn->SetStaticMesh(Cylinder);
	StandInCube = Cube;
	StandInCylinder = Cylinder;

	CableLower = MakeSpan(*this, TEXT("CableLower"), Root);
	CableUpper = MakeSpan(*this, TEXT("CableUpper"), Root);

	// The winch house's drum, about 9 m east-south-east of the bottom landing and up off the floor (the design's winch house
	// at (28, 110) beside the landing at (20, 114)); the build script puts it on the house's Winch socket.
	Winch = CreateDefaultSubobject<USceneComponent>(TEXT("Winch"));
	Winch->SetupAttachment(Root);
	Winch->SetMobility(EComponentMobility::Movable);
	Winch->SetRelativeLocation(FVector(-400.0, 800.0, 250.0));

	// The design's top landing from the bottom one: 32 m north, 7 m east and 46 m up (Docs/Areas/RansomsRest_Expansion.md,
	// "The ore lift": the landing at (20, 114) +1 m, the head at (27, 146) +47 m), facing as the bottom does.
	TopRest = FTransform(FVector(3200.0, 700.0, 4600.0));

	ClosedColdLine = LOCTEXT("ClosedCold", "Nothing answers. The lift's boiler has been cold for years.");
	ClosedSteamLine = LOCTEXT("ClosedSteam", "No steam in her yet. Hiram needs her crank, coal and clamp pin first.");
	FirstRideLines = { FStoryLine::Make(LOCTEXT("Hiram", "Hiram Coyle"), LOCTEXT("HoldTheRail", "Hold the rail."), 2.4f) };
}

void AOreLift::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyArt();
}

void AOreLift::BeginPlay()
{
	Super::BeginPlay();
	ApplyArt();
	// A level begins with the cage at the quarry: nothing keeps where it was, and a call brings it in 8 s.
	SetResting(EOreLiftEnd::Bottom);
	if (!CageMesh)
	{
		UE_LOG(LogLooter, Log, TEXT("%s: no cage model (SM_RR_LiftCage) yet, so it runs with a stand-in floor."), *GetActorNameOrLabel());
	}
}

void AOreLift::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// A level ending mid-ride (a quit, the end of Play-In-Editor) takes the rider's body with it from the far landing,
	// where the ride laid it: the session saved them there. Nothing waits on this lift any more.
	StopLoops();
	ReleaseSaves();
	Super::EndPlay(EndPlayReason);
}

void AOreLift::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// The pieces
// ---------------------------------------------------------------------------

void AOreLift::ApplyArt()
{
	Cage->SetStaticMesh(CageMesh);
	// Without the cage's model, a floor slab to stand on and a rod up to the grip, so the lift runs before its art.
	const bool bStandIn = CageMesh == nullptr;
	const FVector Grip = GetGripLocal();
	CageFloorStandIn->SetStaticMesh(StandInCube);
	CageFloorStandIn->SetRelativeLocation(FVector(0.0, 0.0, FallbackRider.Z - 0.5 * StandInSlab));
	CageFloorStandIn->SetRelativeScale3D(FVector(StandInFloor / EngineShape, StandInFloor / EngineShape, StandInSlab / EngineShape));
	CageFloorStandIn->SetVisibility(bStandIn);
	CageFloorStandIn->SetCollisionEnabled(bStandIn ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	HangerStandIn->SetStaticMesh(StandInCylinder);
	HangerStandIn->SetRelativeLocation(FVector(Grip.X, Grip.Y, 0.5 * (FallbackRider.Z + Grip.Z)));
	HangerStandIn->SetRelativeScale3D(FVector(StandInHanger / EngineShape, StandInHanger / EngineShape,
		FMath::Max(Grip.Z - FallbackRider.Z, 1.0) / EngineShape));
	HangerStandIn->SetVisibility(bStandIn);

	// The cable stretched along each span, as thick as a haul rope whatever the mesh's own girth.
	double Girth = EngineShape;
	const ESplineMeshAxis::Type Along = LongAxisOf(CableMesh, Girth);
	const FVector2D Thickness(CableThickness / Girth);
	for (USplineMeshComponent* Span : { CableLower.Get(), CableUpper.Get() })
	{
		Span->SetStaticMesh(CableMesh);
		if (CableMaterial)
		{
			Span->SetMaterial(0, CableMaterial);
		}
		Span->SetForwardAxis(Along, /*bUpdateMesh*/ false);
		Span->SetStartScale(Thickness, false);
		Span->SetEndScale(Thickness, false);
	}

	if (!IsMoving())
	{
		Cage->SetRelativeTransform(CageEnd == EOreLiftEnd::Bottom ? BottomRest : TopRest);
	}
	LayCable(0.f);
}

FVector AOreLift::GetGripLocal() const
{
	return SocketIn(CageMesh, GripSocket).Get(FallbackGrip);
}

FVector AOreLift::GetRiderLocal() const
{
	return SocketIn(CageMesh, RiderSocket).Get(FallbackRider);
}

// ---------------------------------------------------------------------------
// The line
// ---------------------------------------------------------------------------

TArray<FVector> AOreLift::GetCablePointsLocal() const
{
	const FVector Grip = GetGripLocal();
	return { BottomRest.TransformPosition(Grip), TowerSheave, TopRest.TransformPosition(Grip) };
}

TArray<FVector> AOreLift::GetCablePoints() const
{
	TArray<FVector> Points = GetCablePointsLocal();
	const FTransform& ToWorld = GetActorTransform();
	for (FVector& Point : Points)
	{
		Point = ToWorld.TransformPosition(Point);
	}
	return Points;
}

float AOreLift::GetCableLength() const
{
	const TArray<FVector> Points = GetCablePoints();
	double Length = 0.0;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		Length += FVector::Dist(Points[Index - 1], Points[Index]);
	}
	return static_cast<float>(Length);
}

FVector AOreLift::GetGripAt(EOreLiftEnd From, float Distance) const
{
	TArray<FVector> Points = GetCablePoints();
	if (From == EOreLiftEnd::Top)
	{
		Algo::Reverse(Points);
	}
	return AlongLine(Points, Distance);
}

FTransform AOreLift::GetCageRest(EOreLiftEnd End) const
{
	return (End == EOreLiftEnd::Bottom ? BottomRest : TopRest) * GetActorTransform();
}

FVector AOreLift::GetRiderSpot(EOreLiftEnd End) const
{
	return GetCageRest(End).TransformPosition(GetRiderLocal());
}

FTransform AOreLift::GetCageTransform() const
{
	return Cage->GetComponentTransform();
}

FVector AOreLift::GetRunHeading() const
{
	return RunHeading;
}

FVector AOreLift::GetRiderEye(const FVector& EyeOffset, float Yaw) const
{
	// Their feet on the cage's floor as it hangs now, and their head over them turned their way, tipped as the cage is.
	const FVector Feet = Cage->GetComponentTransform().TransformPosition(GetRiderLocal());
	return Feet + SwayTurn.RotateVector(FRotator(0.0, Yaw, 0.0).RotateVector(EyeOffset));
}

FTransform AOreLift::CageAt(EOreLiftEnd From, float Distance, const FOreLiftSway& InSway, FQuat& OutSwing, FVector& OutHeading) const
{
	TArray<FVector> Points = GetCablePointsLocal();
	const FTransform& FromRest = From == EOreLiftEnd::Bottom ? BottomRest : TopRest;
	const FTransform& ToRest = From == EOreLiftEnd::Bottom ? TopRest : BottomRest;
	if (From == EOreLiftEnd::Top)
	{
		Algo::Reverse(Points);
	}
	double Length = 0.0;
	for (int32 Index = 1; Index < Points.Num(); ++Index)
	{
		Length += FVector::Dist(Points[Index - 1], Points[Index]);
	}
	const FVector Grip = AlongLine(Points, Distance);
	const double Progress = Length > UE_KINDA_SMALL_NUMBER ? FMath::Clamp(Distance / Length, 0.0, 1.0) : 1.0;

	// Hung from its grip: turned from one landing's way to the other's, swung about the grip along the travel and across it.
	const FVector Heading = Flat(Points[0], Points.Last());
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Heading);
	const FQuat Turn = FQuat::Slerp(FromRest.GetRotation(), ToRest.GetRotation(), Progress);
	// Along > 0 tips its floor back against the travel; Side > 0 swings its floor to the travel's left.
	const FQuat Swing = FQuat(Right, FMath::DegreesToRadians(InSway.Along)) * FQuat(Heading, -FMath::DegreesToRadians(InSway.Side));
	const FQuat Hang = Swing * Turn;
	const FVector Location = Grip - Hang.RotateVector(GetGripLocal()) + FVector(0.0, 0.0, InSway.Bob);
	OutSwing = Swing;
	OutHeading = Heading;
	return FTransform(Hang, Location);
}

void AOreLift::LayCable(float Shiver)
{
	const TArray<FVector> Points = GetCablePointsLocal();
	USplineMeshComponent* Spans[] = { CableLower.Get(), CableUpper.Get() };
	for (int32 Index = 0; Index < 2 && Index + 1 < Points.Num(); ++Index)
	{
		if (!Spans[Index])
		{
			continue;
		}
		// Straight, as a haul rope under load reads from the ground; a clank sends a shiver down it (a cubic whose end
		// tangents lean by 4 x the shiver bows it that far at its middle), the two spans out of step.
		const FVector Start = Points[Index];
		const FVector End = Points[Index + 1];
		const FVector Span = End - Start;
		const FVector Across = FVector::CrossProduct(Span, FVector::CrossProduct(FVector::UpVector, Span)).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		const FVector Lean = Across * (4.0 * Shiver * (Index == 0 ? 1.0 : -0.7));
		Spans[Index]->SetStartAndEnd(Start, Span - Lean, End, Span + Lean, /*bUpdateMesh*/ true);
	}
}

#undef LOCTEXT_NAMESPACE
