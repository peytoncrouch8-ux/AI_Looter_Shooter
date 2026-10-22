// ABountyBoard: the board in town: its model and sheets, its floating mark, and Interact opening its screen.

#include "Bounties/BountyBoard.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Bounties/BountyBoardSubsystem.h"
#include "Bounties/BountyRules.h"
#include "Creatures/CreatureRankSettings.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "UI/World/BountyBoardWidget.h"
#include "UI/World/GiverMarkActor.h"
#include "UI/World/GiverMarkSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LooterBounty"

const FName ABountyBoard::BoardTag(TEXT("BountyBoard"));
const TCHAR* const ABountyBoard::ModelPath = TEXT("/Game/Art/Props/SM_NoticeBoard_Town.SM_NoticeBoard_Town");

namespace
{
	const TCHAR* const PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* const CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* const CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* const SurfacePath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");

	/** A sheet (cm): an upright poster, a little proud of the face; its seal near its top, and how far each leans. */
	const FVector SheetSize(46.0, 34.0, 1.0);
	constexpr double SheetProud = 0.6;
	const FVector SealSize(5.0, 5.0, 1.2);
	constexpr double SealUp = 15.0;
	constexpr float SheetLean[] = { -2.5f, 1.5f, 3.f };

	/** The board without its model (a test level, before the import): a plank face on two posts, cm. */
	const FVector StandInBoard(12.0, 176.0, 96.0);

	/** Old newsprint, a world colour (sRGB #E4D8BC), not the UI's. */
	const FLinearColor PaperColor = FLinearColor::FromSRGBColor(FColor(0xE4, 0xD8, 0xBC));

	/** An asset found only once it's imported, so the class and its tests work without it. In a constructor. */
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

	UStaticMeshComponent* MakeLookPart(AActor* Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Mesh)
	{
		UStaticMeshComponent* Part = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Part->SetupAttachment(Parent);
		Part->SetMobility(EComponentMobility::Movable);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetGenerateOverlapEvents(false);
		Part->SetCanEverAffectNavigation(false);
		Part->SetCastShadow(false);
		Part->SetStaticMesh(Mesh);
		return Part;
	}
}

ABountyBoard::ABountyBoard()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;

	static UStaticMesh* const Model = FindIfMade<UStaticMesh>(ModelPath);
	static UStaticMesh* const Plane = FindIfMade<UStaticMesh>(PlanePath);
	static UStaticMesh* const Cylinder = FindIfMade<UStaticMesh>(CylinderPath);
	static UStaticMesh* const Cube = FindIfMade<UStaticMesh>(CubePath);
	static UMaterialInterface* const Surface = FindIfMade<UMaterialInterface>(SurfacePath);
	SurfaceBase = Surface;

	// Solid, so it's stood against and the Interact key's line finds it; world dynamic, so ground traces never take it for
	// the ground.
	Board = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Board"));
	Board->SetupAttachment(Root);
	Board->SetMobility(EComponentMobility::Static);
	Board->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
	Board->SetGenerateOverlapEvents(false);
	Board->SetCanEverAffectNavigation(false);
	Board->SetStaticMesh(Model ? Model : Cube);
	if (!Model)
	{
		Board->SetRelativeLocation(FVector(0.0, 0.0, FaceOffset.Z));
		Board->SetRelativeScale3D(StandInBoard / 100.0);
	}

	// The sheets and their seals, laid out on the face (LayOutSheets).
	for (int32 Slot = 0; Slot < BountyRules::SlotCount; ++Slot)
	{
		UStaticMeshComponent* Sheet = MakeLookPart(this, Root, *FString::Printf(TEXT("Sheet%d"), Slot + 1), Plane);
		Sheet->SetVisibility(false);
		Sheets.Add(Sheet);
		UStaticMeshComponent* Seal = MakeLookPart(this, Root, *FString::Printf(TEXT("Seal%d"), Slot + 1), Cylinder);
		Seal->SetVisibility(false);
		Seals.Add(Seal);
	}
	LayOutSheets();

	Title = LOCTEXT("BoardTitle", "Bounties");
	Subtitle = LOCTEXT("BoardSubtitle", "Posted by the Rim Rangers. Paid from the town's purse, on proof.");
	Prompt = LOCTEXT("BoardPrompt", "Read the bounties");
	// Ransom's Rest's story (the build script sets them as well): the town safe after Main 3, harder marks after Main 5 and 6.
	OpenWhen.AfterMissions = { TEXT("Main3") };
	DangerousWhen.AfterMissions = { TEXT("Main5") };
	DeadlyWhen.AfterMissions = { TEXT("Main6") };
	Tags.Add(BoardTag);
}

void ABountyBoard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	LayOutSheets();
}

void ABountyBoard::LayOutSheets()
{
	// The engine's plane faces up: turned to face the board's front (+X), its length up the face, each leaning a little as
	// if pinned by hand; its seal near its top, a little proud of it.
	for (int32 Slot = 0; Slot < Sheets.Num() && Slot < Seals.Num(); ++Slot)
	{
		const double Across = (Slot - (Sheets.Num() - 1) * 0.5) * SheetSpacing;
		if (UStaticMeshComponent* Sheet = Sheets[Slot])
		{
			Sheet->SetRelativeLocation(FVector(FaceOffset.X + SheetProud, Across, FaceOffset.Z));
			// Turned to face front first, then leant about the face's own normal (its front, +X), so it stays flat on it.
			const FQuat Lean(FVector::ForwardVector, FMath::DegreesToRadians(SheetLean[Slot % UE_ARRAY_COUNT(SheetLean)]));
			Sheet->SetRelativeRotation(Lean * FRotator(-90.f, 0.f, 0.f).Quaternion());
			Sheet->SetRelativeScale3D(SheetSize / 100.0);
		}
		if (UStaticMeshComponent* Seal = Seals[Slot])
		{
			Seal->SetRelativeLocation(FVector(FaceOffset.X + SheetProud + SealSize.Z * 0.5, Across, FaceOffset.Z + SealUp));
			Seal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
			Seal->SetRelativeScale3D(SealSize / 100.0);
		}
	}
}

void ABountyBoard::BeginPlay()
{
	Super::BeginPlay();
	Tags.AddUnique(BoardTag);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	// The sheets' paper and seals, each its own colour.
	SealMaterials.Reset();
	for (int32 Slot = 0; Slot < Sheets.Num(); ++Slot)
	{
		if (SurfaceBase)
		{
			UMaterialInstanceDynamic* Paper = UMaterialInstanceDynamic::Create(SurfaceBase, this);
			Paper->SetVectorParameterValue(TEXT("Color"), PaperColor);
			Paper->SetScalarParameterValue(TEXT("Variation"), 0.35f);
			Sheets[Slot]->SetMaterial(0, Paper);
			UMaterialInstanceDynamic* Wax = UMaterialInstanceDynamic::Create(SurfaceBase, this);
			Wax->SetScalarParameterValue(TEXT("Variation"), 0.1f);
			Seals[Slot]->SetMaterial(0, Wax);
			SealMaterials.Add(Wax);
		}
	}
	// Its mark floats over the world in play (a test level has no screen to float it on).
	UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		MarkActor = World->SpawnActor<AGiverMarkActor>(AGiverMarkActor::StaticClass(), GetActorTransform(), Params);
	}
	if (UBountyBoardSubsystem* Bounties = UBountyBoardSubsystem::Get(this))
	{
		Bounties->RegisterBoard(this);
	}
}

void ABountyBoard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	if (UBountyBoardSubsystem* Bounties = UBountyBoardSubsystem::Get(this))
	{
		Bounties->UnregisterBoard(this);
	}
	if (MarkActor)
	{
		MarkActor->Destroy();
		MarkActor = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

bool ABountyBoard::IsOpen() const
{
	const UBountyBoardSubsystem* Bounties = UBountyBoardSubsystem::Get(this);
	return Bounties && Bounties->IsBoardOpen(*this);
}

FInteractionOptions ABountyBoard::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	Options.bUsable = IsOpen();
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = Prompt;
	return Options;
}

bool ABountyBoard::Interact(UInteractionComponent& User, bool bHeld)
{
	if (bHeld || !IsOpen())
	{
		return false;
	}
	APawn* Pawn = Cast<APawn>(User.GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (!PC || !PC->IsLocalController())
	{
		return false;
	}
	if (!UBountyBoardWidget::OpenFor(PC, this))
	{
		return false;
	}
	LooterSound::PlayAt(this, LooterSoundCue::Bounty::BoardOpen, GetInteractionLocation().Get(GetActorLocation()));
	// Read: every posting up counts as seen (its ? goes).
	if (UBountyBoardSubsystem* Bounties = UBountyBoardSubsystem::Get(this))
	{
		Bounties->MarkRead();
	}
	return true;
}

TOptional<FVector> ABountyBoard::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorTransform().TransformPosition(FaceOffset));
}

void ABountyBoard::ShowRecord(const FBountyBoardRecord& Record, bool bOpen)
{
	bool bNew = false;
	bool bToPay = false;
	for (int32 Slot = 0; Slot < Sheets.Num(); ++Slot)
	{
		const FBountyPosting* Posting = Record.Slots.IsValidIndex(Slot) ? &Record.Slots[Slot] : nullptr;
		// A posting up shows its sheet; a taken or won one is in the player's pocket.
		const bool bUp = bOpen && Posting && Posting->IsPosting() && Posting->State == EBountyState::Posted;
		Sheets[Slot]->SetVisibility(bUp);
		Seals[Slot]->SetVisibility(bUp);
		if (bUp && SealMaterials.IsValidIndex(Slot))
		{
			const ECreatureRank Rank = BountyRules::GetTier(Posting->Tier).MarkRank;
			SealMaterials[Slot]->SetVectorParameterValue(TEXT("Color"), UCreatureRankSettings::Get(Rank).Color);
		}
		bNew |= bUp && Posting->Serial > Record.SeenUpTo;
		bToPay |= bOpen && Posting && Posting->IsPosting() && Posting->State == EBountyState::Won;
	}
	ShowMark(bToPay ? EShownMark::TurnIn : (bNew ? EShownMark::Offer : EShownMark::None));
}

bool ABountyBoard::IsSheetShown(int32 Slot) const
{
	return Sheets.IsValidIndex(Slot) && Sheets[Slot]->IsVisible();
}

void ABountyBoard::ShowMark(EShownMark Mark)
{
	if (Mark == ShownMark)
	{
		return;
	}
	ShownMark = Mark;
	if (!MarkActor)
	{
		return;
	}
	if (Mark == EShownMark::None)
	{
		MarkActor->Retire();
		return;
	}
	FGiverMark Shown;
	Shown.Giver = this;
	Shown.Location = UGiverMarkSubsystem::MarkSpot(*this);
	Shown.Mark = Mark == EShownMark::TurnIn ? MissionMarkIcons::EMark::TurnIn : MissionMarkIcons::EMark::Offer;
	Shown.bMain = false;
	MarkActor->Show(Shown);
}

#undef LOCTEXT_NAMESPACE
