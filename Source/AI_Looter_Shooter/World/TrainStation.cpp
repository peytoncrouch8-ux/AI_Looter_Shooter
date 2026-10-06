#include "World/TrainStation.h"
#include "Core/LooterMenuGameMode.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "World/MinimapSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "TrainStation"

ATrainStation::ATrainStation()
{
	PrimaryActorTick.bCanEverTick = false;

	// The imported depot (Art/Models/Buildings/Depot.py) and its board (Art/Models/Props/Railway.py); each a setting.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DepotAsset(TEXT("/Game/Art/Buildings/SM_Depot.SM_Depot"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BoardAsset(TEXT("/Game/Art/Props/SM_StationBoard.SM_StationBoard"));
	BuildingMesh = DepotAsset.Object;
	BoardMesh = BoardAsset.Object;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;

	Building = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Building"));
	Building->SetMobility(EComponentMobility::Static);
	Building->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Building->SetStaticMesh(BuildingMesh);
	Building->SetupAttachment(Root);

	// Solid, so the crosshair's line finds it (it blocks Visibility) and the player reads it from in front.
	Board = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Board"));
	Board->SetMobility(EComponentMobility::Static);
	Board->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Board->SetStaticMesh(BoardMesh);
	Board->SetupAttachment(Building, BoardSocket);

	Landing = CreateDefaultSubobject<USceneComponent>(TEXT("Landing"));
	Landing->SetMobility(EComponentMobility::Static);
	Landing->SetupAttachment(Root);

	// On the platform in front of the depot (the platform's 0.40 m up), until the build script sets it by the train.
	LandingTransform = FTransform(FVector(600.0, 0.0, 40.0));
	Prompt = LOCTEXT("Prompt", "Read the station board");
	Tags.Add(MinimapTags::Obstacle);
}

void ATrainStation::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyArt();
}

void ATrainStation::ApplyArt()
{
	Building->SetStaticMesh(BuildingMesh);
	Board->SetStaticMesh(BoardMesh);
	// On the building's socket when it has one; else the board stands at the station's origin.
	Board->AttachToComponent(Building, FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		Building->DoesSocketExist(BoardSocket) ? BoardSocket : FName(NAME_None));
	Landing->SetRelativeTransform(LandingTransform);

	// It carries its landing: the actor is found by the name, the spot on the platform is its Landing.
	Landing->ComponentTags = { LandingName };
	Tags.RemoveAll([](FName Tag) { return Tag.ToString().StartsWith(TEXT("Landing_")); });
	Tags.AddUnique(LandingName);
}

void ATrainStation::BeginPlay()
{
	Super::BeginPlay();
	Tags.AddUnique(LandingName);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void ATrainStation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

FInteractionOptions ATrainStation::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	// Behind the main menu nobody travels.
	Options.bUsable = !ALooterMenuGameMode::IsMenuWorld(GetWorld());
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = HoldSeconds;
	Options.HoldPrompt = Prompt;
	Options.Reach = Reach;
	return Options;
}

bool ATrainStation::Interact(UInteractionComponent& User, bool bHeld)
{
	if (!bHeld || !GetInteractionOptions(User).bUsable)
	{
		return false;
	}
	ALooterHUD* HUD = ALooterHUD::FindFor(User.GetOwner());
	return HUD && HUD->OpenStationBoard(this, GetBoardWords());
}

TOptional<FVector> ATrainStation::GetInteractionLocation() const
{
	// In front of the slate, where the player reads it; else the middle of the board.
	if (Board && Board->DoesSocketExist(InteractSocket))
	{
		return Board->GetSocketLocation(InteractSocket);
	}
	return Board ? TOptional<FVector>(Board->Bounds.Origin) : TOptional<FVector>();
}

FStationBoardWords ATrainStation::GetBoardWords() const
{
	return FStationBoardWords::Station();
}

#undef LOCTEXT_NAMESPACE
