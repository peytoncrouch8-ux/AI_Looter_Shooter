// AGunsmithBench: the plain gunsmith's bench, its model or the shapes standing in for it, and using it.

#include "World/GunsmithBench.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "World/MinimapSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const FName AGunsmithBench::BenchTag(TEXT("GunsmithBench"));
const TCHAR* const AGunsmithBench::ModelPath = TEXT("/Game/Art/Props/SM_GunsmithBench.SM_GunsmithBench");
const FName AGunsmithBench::InteractSocket(TEXT("Interact"));
const FName AGunsmithBench::GunSocket(TEXT("Gun"));
const FName AGunsmithBench::BoxSocket(TEXT("Box"));

namespace
{
	/** Without the model: the engine's cube (1 m, about its middle) at the bench's size, 0.7 deep, 1.6 long and 0.9 m high. */
	const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const FVector StandInSize(70.0, 160.0, 90.0);
	/** The stand-in parts box: 30 x 40 x 18 cm, on the top at the bench's right end. */
	const FVector StandInBoxSize(30.0, 40.0, 18.0);
	const FVector StandInBoxAt(0.0, 52.0, 90.0);

	/** Where the sockets would be on the model (the actor's space: +X its front, the floor at Z 0). */
	FTransform StandInSocket(FName Socket)
	{
		if (Socket == AGunsmithBench::GunSocket)
		{
			// Lying on the top along the bench, left of the box.
			return FTransform(FRotator(0.0, 90.0, 0.0), FVector(0.0, -18.0, StandInSize.Z));
		}
		if (Socket == AGunsmithBench::BoxSocket)
		{
			return FTransform(StandInBoxAt);
		}
		// The front edge, at the height hands work at.
		return FTransform(FVector(StandInSize.X * 0.5 + 5.0, 0.0, 95.0));
	}

	/** An asset found only once it's imported, so the class works, and its tests run, without it. In a constructor. */
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
}

AGunsmithBench::AGunsmithBench()
{
	// It never changes on its own: a screen opens, nothing moves.
	PrimaryActorTick.bCanEverTick = false;

	// Movable like the other props the build scripts place and the console spawns in play.
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static UStaticMesh* const Model = FindIfMade<UStaticMesh>(ModelPath);
	static UStaticMesh* const Cube = FindIfMade<UStaticMesh>(CubePath);

	// Solid, so it's stood against and found by the Interact key's line; world dynamic, so the traces that look for the
	// ground among world-static things (a creature's footing, the scatter, settling props) never take it for the ground.
	Bench = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bench"));
	Bench->SetupAttachment(Root);
	Bench->SetMobility(EComponentMobility::Movable);
	Bench->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Bench->SetGenerateOverlapEvents(false);
	Bench->SetCanEverAffectNavigation(false);
	Bench->SetStaticMesh(Model ? Model : Cube);

	// Small and on the top: nothing to stand on, so no collision of its own.
	StandInBox = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StandInBox"));
	StandInBox->SetupAttachment(Root);
	StandInBox->SetMobility(EComponentMobility::Movable);
	StandInBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StandInBox->SetGenerateOverlapEvents(false);
	StandInBox->SetCanEverAffectNavigation(false);
	if (!Model)
	{
		// The cube's pivot is its middle: lift each block so it stands on what's under it.
		Bench->SetRelativeLocation(FVector(0.0, 0.0, StandInSize.Z * 0.5));
		Bench->SetRelativeScale3D(StandInSize / 100.0);
		StandInBox->SetStaticMesh(Cube);
		StandInBox->SetRelativeLocation(StandInBoxAt + FVector(0.0, 0.0, StandInBoxSize.Z * 0.5));
		StandInBox->SetRelativeScale3D(StandInBoxSize / 100.0);
	}

	Prompt = NSLOCTEXT("LooterBench", "UsePrompt", "Use the gunsmith's bench");
	Tags.Add(BenchTag);
	Tags.Add(MinimapTags::Obstacle);
}

void AGunsmithBench::BeginPlay()
{
	Super::BeginPlay();
	// The build script sets its tags whole; these it always carries.
	Tags.AddUnique(BenchTag);
	Tags.AddUnique(MinimapTags::Obstacle);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AGunsmithBench::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// Its model
// ---------------------------------------------------------------------------

bool AGunsmithBench::HasModel() const
{
	const UStaticMesh* Mesh = Bench ? Bench->GetStaticMesh() : nullptr;
	return Mesh && Mesh->GetPathName() == ModelPath;
}

FTransform AGunsmithBench::GetSocketTransform(FName Socket) const
{
	if (HasModel() && Bench->DoesSocketExist(Socket))
	{
		return Bench->GetSocketTransform(Socket);
	}
	return StandInSocket(Socket) * GetActorTransform();
}

// ---------------------------------------------------------------------------
// Being used
// ---------------------------------------------------------------------------

FInteractionOptions AGunsmithBench::GetInteractionOptions(const UInteractionComponent& User) const
{
	// A tap opens the screen, any time: the bench is plain and open to everyone from the start.
	FInteractionOptions Options;
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = Prompt;
	Options.Reach = Reach;
	return Options;
}

bool AGunsmithBench::Interact(UInteractionComponent& User, bool bHeld)
{
	return !bHeld && Use(User.GetOwner());
}

TOptional<FVector> AGunsmithBench::GetInteractionLocation() const
{
	// Its front at hand height, where a gunsmith stands to work.
	return TOptional<FVector>(GetSocketTransform(InteractSocket).GetLocation());
}

bool AGunsmithBench::Use(AActor* Player)
{
	ALooterHUD* HUD = ALooterHUD::FindFor(Player);
	if (!HUD || !HUD->OpenBench(this))
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: the gunsmith's bench is in use%s."), *GetActorNameOrLabel(),
		Player ? *FString::Printf(TEXT(" by %s"), *Player->GetName()) : TEXT(""));
	return true;
}
