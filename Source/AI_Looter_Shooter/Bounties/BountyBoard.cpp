// ABountyBoard: the board at the depot: its model and sheets, its floating mark, and Interact opening its screen.

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
#include "Components/DecalComponent.h"
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
// BOUNTIES on its header (Art/Models/Props/Boardwalk.py NoticeBoard_Bounty); the NOTICES board until that is imported.
const TCHAR* const ABountyBoard::ModelPath = TEXT("/Game/Art/Props/SM_NoticeBoard_Bounty.SM_NoticeBoard_Bounty");
const TCHAR* const ABountyBoard::FallbackModelPath = TEXT("/Game/Art/Props/SM_NoticeBoard_Town.SM_NoticeBoard_Town");

namespace
{
	const TCHAR* const PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* const CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* const CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* const SurfacePath = TEXT("/Game/Environment/Materials/M_StylizedSurface.M_StylizedSurface");
	/** What build_bounty_sheet_materials.py makes: M_PosterDecal's instances for the sheets' art and the printed names. */
	const TCHAR* const SheetDecalPath = TEXT("/Game/Art/Materials/MI_BountySheetDecal.MI_BountySheetDecal");
	const TCHAR* const NameDecalPath = TEXT("/Game/Art/Materials/MI_BountyNameDecal.MI_BountyNameDecal");
	const FName CellParameter(TEXT("Cell"));

	/**
	 * The art's layout (Tools/Blender/looter_bounty_sheets.py: change both together). T_BountySheets_BC is 2048 square, a kind's
	 * 1024 cell two to a row, its sheet 680 x 920 px centred in it. T_BountyNames_BC is 2048 x 512: 512 x 64 cells four to a row,
	 * a name 480 x 60 px centred in its cell.
	 */
	constexpr float SheetAtlas = 2048.f;
	constexpr float SheetCellPx = 1024.f;
	constexpr float SheetPxW = 680.f;
	constexpr float SheetPxH = 920.f;
	constexpr float NameAtlasW = 2048.f;
	constexpr float NameAtlasH = 512.f;
	constexpr float NameCellW = 512.f;
	constexpr float NameCellH = 64.f;
	constexpr float NamePxW = 480.f;
	constexpr float NamePxH = 60.f;
	constexpr int32 NamesPerKind = 8;

	/** A sheet is 34 x 46 cm; the printed name's band is 30 x 3.75 cm, its middle 8.5 cm under the sheet's (art: BAND_Y). */
	constexpr double SheetWidth = 34.0;
	constexpr double SheetHeight = 46.0;
	constexpr double NameWidth = 30.0;
	constexpr double NameHeight = 3.75;
	constexpr double NameDown = 8.5;

	/**
	 * How the decals reach (cm, in the board's frame, +X out of its face): from ReachIn behind the face's surface to ReachOut in
	 * front of it. The surface is taken a centimetre behind FaceOffset.X (the sheets float just above it); the reach is wide
	 * enough for the model's face to be a centimetre either side of that.
	 */
	constexpr double SurfaceBehind = 1.0;
	constexpr double ReachIn = 3.0;
	constexpr double ReachOut = 6.0;

	/** The decal's turn in the board's frame, as AWantedPoster's (DecalTurn): +X into the face, V down the picture, U to the reader's right. */
	FQuat DecalTurn()
	{
		return FRotationMatrix::MakeFromXZ(FVector(-1.0, 0.0, 0.0), FVector(0.0, -1.0, 0.0)).ToQuat();
	}

	int32 KindIndex(FName Kind)
	{
		if (Kind == TEXT("Spider"))
		{
			return 1;
		}
		if (Kind == TEXT("Spitter"))
		{
			return 2;
		}
		if (Kind == TEXT("DeadMiner"))
		{
			return 3;
		}
		return 0;
	}

	/** A sheet (cm): an upright poster, a little proud of the face, and how far each leans. */
	const FVector SheetSize(46.0, 34.0, 1.0);
	constexpr double SheetProud = 0.6;
	const FVector SealSize(5.0, 5.0, 1.2);
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

	static UStaticMesh* const BountiesModel = FindIfMade<UStaticMesh>(ModelPath);
	static UStaticMesh* const Model = BountiesModel ? BountiesModel : FindIfMade<UStaticMesh>(FallbackModelPath);
	static UStaticMesh* const Plane = FindIfMade<UStaticMesh>(PlanePath);
	static UStaticMesh* const Cylinder = FindIfMade<UStaticMesh>(CylinderPath);
	static UStaticMesh* const Cube = FindIfMade<UStaticMesh>(CubePath);
	static UMaterialInterface* const Surface = FindIfMade<UMaterialInterface>(SurfacePath);
	static UMaterialInterface* const SheetDecalAsset = FindIfMade<UMaterialInterface>(SheetDecalPath);
	static UMaterialInterface* const NameDecalAsset = FindIfMade<UMaterialInterface>(NameDecalPath);
	SurfaceBase = Surface;
	SheetDecalBase = SheetDecalAsset;
	NameDecalBase = NameDecalAsset;

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
		// The art's sheet and its printed name, projected onto the face (as the wall posters are); shown instead of the plain
		// plane once the art is imported. The name over the sheet.
		UDecalComponent* SheetDecal = CreateDefaultSubobject<UDecalComponent>(*FString::Printf(TEXT("SheetDecal%d"), Slot + 1));
		SheetDecal->SetupAttachment(Root);
		SheetDecal->SetMobility(EComponentMobility::Movable);
		SheetDecal->SetVisibility(false);
		SheetDecal->SetSortOrder(0);
		SheetDecals.Add(SheetDecal);
		UDecalComponent* NameDecal = CreateDefaultSubobject<UDecalComponent>(*FString::Printf(TEXT("NameDecal%d"), Slot + 1));
		NameDecal->SetupAttachment(Root);
		NameDecal->SetMobility(EComponentMobility::Movable);
		NameDecal->SetVisibility(false);
		NameDecal->SetSortOrder(1);
		NameDecals.Add(NameDecal);
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
	// if pinned by hand; its seal on the ring printed for it, a little proud of it. (The plane is the plain paper shown until the
	// art is imported; the art is a decal per sheet, below.)
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
		// The sheet's middle on the face, and a spot on the sheet (cm right and up) leant with it.
		const FQuat LeanTurn(FVector::ForwardVector, FMath::DegreesToRadians(SheetLean[Slot % UE_ARRAY_COUNT(SheetLean)]));
		const FVector Middle(FaceOffset.X, Across, FaceOffset.Z);
		const auto OnSheet = [&](double Right, double Up) { return Middle + LeanTurn.RotateVector(FVector(0.0, -Right, Up)); };
		if (UStaticMeshComponent* Seal = Seals[Slot])
		{
			FVector SealAt = OnSheet(SealOffset.X, SealOffset.Y);
			SealAt.X = FaceOffset.X + SheetProud + SealSize.Z * 0.5;
			Seal->SetRelativeLocation(SealAt);
			Seal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
			Seal->SetRelativeScale3D(SealSize / 100.0);
		}
		// The decals: their box from ReachIn behind the face to ReachOut before it, the picture across the other two axes
		// (Y runs down it, Z across it: DecalTurn). The name's band is under the picture's middle.
		const double HalfDepth = (ReachIn + ReachOut) * 0.5;
		const double BoxX = FaceOffset.X - SurfaceBehind + (ReachOut - ReachIn) * 0.5;
		if (SheetDecals.IsValidIndex(Slot) && SheetDecals[Slot])
		{
			SheetDecals[Slot]->DecalSize = FVector(HalfDepth, SheetHeight * 0.5, SheetWidth * 0.5);
			SheetDecals[Slot]->SetRelativeLocation(FVector(BoxX, Middle.Y, Middle.Z));
			SheetDecals[Slot]->SetRelativeRotation(LeanTurn * DecalTurn());
			SheetDecals[Slot]->MarkRenderStateDirty();
		}
		if (NameDecals.IsValidIndex(Slot) && NameDecals[Slot])
		{
			FVector NameAt = OnSheet(0.0, -NameDown);
			NameAt.X = BoxX;
			NameDecals[Slot]->DecalSize = FVector(HalfDepth, NameHeight * 0.5, NameWidth * 0.5);
			NameDecals[Slot]->SetRelativeLocation(NameAt);
			NameDecals[Slot]->SetRelativeRotation(LeanTurn * DecalTurn());
			NameDecals[Slot]->MarkRenderStateDirty();
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
	// Each sheet's decals take an instance of their own, for the cell (the sheet's kind, the mark's name).
	SheetDecalMaterials.Reset();
	NameDecalMaterials.Reset();
	for (int32 Slot = 0; Slot < SheetDecals.Num(); ++Slot)
	{
		UMaterialInstanceDynamic* SheetLook = SheetDecalBase ? UMaterialInstanceDynamic::Create(SheetDecalBase, this) : nullptr;
		UMaterialInstanceDynamic* NameLook = NameDecalBase ? UMaterialInstanceDynamic::Create(NameDecalBase, this) : nullptr;
		if (SheetLook)
		{
			SheetDecals[Slot]->SetDecalMaterial(SheetLook);
		}
		if (NameLook && NameDecals.IsValidIndex(Slot))
		{
			NameDecals[Slot]->SetDecalMaterial(NameLook);
		}
		SheetDecalMaterials.Add(SheetLook);
		NameDecalMaterials.Add(NameLook);
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
	// Read: every posting up counts as seen (its ? goes). A strongbox an older save still owes is handed over here, heard.
	if (UBountyBoardSubsystem* Bounties = UBountyBoardSubsystem::Get(this))
	{
		Bounties->MarkRead();
		if (Bounties->HandOverOwed(Pawn) > 0)
		{
			LooterSound::Play2D(this, LooterSoundCue::Bounty::Paid);
		}
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
		// The art's decals when they're made, else the plain paper.
		Sheets[Slot]->SetVisibility(bUp && !UsesDecals());
		Seals[Slot]->SetVisibility(bUp);
		if (SheetDecals.IsValidIndex(Slot) && SheetDecals[Slot])
		{
			SheetDecals[Slot]->SetVisibility(bUp && UsesDecals());
			if (bUp && SheetDecalMaterials.IsValidIndex(Slot) && SheetDecalMaterials[Slot])
			{
				const FVector4 Cell = SheetCell(Posting->Kind);
				SheetDecalMaterials[Slot]->SetVectorParameterValue(CellParameter, FLinearColor(Cell.X, Cell.Y, Cell.Z, Cell.W));
			}
		}
		if (NameDecals.IsValidIndex(Slot) && NameDecals[Slot])
		{
			const bool bNamed = NameDecalMaterials.IsValidIndex(Slot) && NameDecalMaterials[Slot];
			NameDecals[Slot]->SetVisibility(bUp && UsesDecals() && bNamed);
			if (bUp && bNamed)
			{
				const FVector4 Cell = NameCell(Posting->Kind, Posting->NameIndex);
				NameDecalMaterials[Slot]->SetVectorParameterValue(CellParameter, FLinearColor(Cell.X, Cell.Y, Cell.Z, Cell.W));
			}
		}
		if (bUp && SealMaterials.IsValidIndex(Slot))
		{
			const ECreatureRank Rank = BountyRules::GetTier(Posting->Tier).MarkRank;
			SealMaterials[Slot]->SetVectorParameterValue(TEXT("Color"), UCreatureRankSettings::Get(Rank).Color);
		}
		bNew |= bUp && Posting->Serial > Record.SeenUpTo;
		bToPay |= bOpen && Posting && Posting->IsPosting() && Posting->State == EBountyState::Won;
	}
	// An older save's strongbox still owed waits here too (handed over as the board is read).
	bToPay |= bOpen && !Record.OwedChests.IsEmpty();
	ShowMark(bToPay ? EShownMark::TurnIn : (bNew ? EShownMark::Offer : EShownMark::None));
}

bool ABountyBoard::IsSheetShown(int32 Slot) const
{
	if (UsesDecals())
	{
		return SheetDecals.IsValidIndex(Slot) && SheetDecals[Slot] && SheetDecals[Slot]->IsVisible();
	}
	return Sheets.IsValidIndex(Slot) && Sheets[Slot]->IsVisible();
}

FVector4 ABountyBoard::SheetCell(FName Kind)
{
	const int32 Index = KindIndex(Kind);
	const float X0 = (static_cast<float>(Index % 2) * SheetCellPx + (SheetCellPx - SheetPxW) * 0.5f) / SheetAtlas;
	const float Y0 = (static_cast<float>(Index / 2) * SheetCellPx + (SheetCellPx - SheetPxH) * 0.5f) / SheetAtlas;
	return FVector4(X0, Y0, X0 + SheetPxW / SheetAtlas, Y0 + SheetPxH / SheetAtlas);
}

FVector4 ABountyBoard::NameCell(FName Kind, int32 NameIndex)
{
	const int32 Index = KindIndex(Kind) * NamesPerKind + FMath::Clamp(NameIndex, 0, NamesPerKind - 1);
	const int32 Columns = static_cast<int32>(NameAtlasW / NameCellW);
	const float X0 = (static_cast<float>(Index % Columns) * NameCellW + (NameCellW - NamePxW) * 0.5f) / NameAtlasW;
	const float Y0 = (static_cast<float>(Index / Columns) * NameCellH + (NameCellH - NamePxH) * 0.5f) / NameAtlasH;
	return FVector4(X0, Y0, X0 + NamePxW / NameAtlasW, Y0 + NamePxH / NameAtlasH);
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
