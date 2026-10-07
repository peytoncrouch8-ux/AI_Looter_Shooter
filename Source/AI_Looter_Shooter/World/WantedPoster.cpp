// AWantedPoster: its make-up, its look (the decal's cell, size and box), being used, and reading Calder's note.

#include "World/WantedPoster.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "World/MinimapSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const FName AWantedPoster::WantedTag(TEXT("WantedPoster"));
const FName AWantedPoster::NoteTag(TEXT("CalderNote"));

namespace
{
	/** What build_decal_materials.py makes, and the engine's plane the scrap is cut from. */
	const TCHAR* DecalMaterialPath = TEXT("/Game/Art/Materials/MI_PosterDecal.MI_PosterDecal");
	const TCHAR* ScrapMaterialPath = TEXT("/Game/Art/Materials/MI_PosterScrap.MI_PosterScrap");
	const TCHAR* PlanePath = TEXT("/Engine/BasicShapes/Plane.Plane");

	/** M_PosterDecal's cell: (U0, V0, U1, V1) of the atlas. */
	const FName CellParameter(TEXT("Cell"));

	/**
	 * How far the decal reaches into the surface and out of it (cm). Into it only 2: the back of a thin board (the notice
	 * board's face is 4 cm thick) never shows the paper. Out of it 8: over clapboard laps, stone and planks.
	 */
	constexpr double ReachIn = 2.0;
	constexpr double ReachOut = 8.0;

	/** The box the Interact key's line finds: 1 cm thick, a hair proud of the surface (cm). */
	constexpr double BoxHalfThickness = 0.5;
	constexpr double BoxGap = 0.2;

	/**
	 * An asset made after this code (build_decal_materials.py's), found only once it exists, so the class works, and its
	 * tests run, without it. In a constructor.
	 */
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

	/** The same at play time: for posters placed in the session that made the asset (the class's defaults were set before). */
	template <typename T>
	T* LoadIfMade(const TCHAR* ObjectPath)
	{
		return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath)))
			? LoadObject<T>(nullptr, ObjectPath) : nullptr;
	}

	/**
	 * The decal's turn in the poster's frame (+X out of the surface toward whoever reads it, +Z up). A decal projects
	 * along its own +X, and its picture's U runs along its +Z and V along its +Y (DeferredDecal.usf). So: +X into the
	 * surface, +Z to the reader's right (the poster's -Y, as they face it) and +Y down: the atlas shows upright and
	 * unmirrored, V = 0 at the top as the art draws it.
	 */
	FRotator DecalTurn()
	{
		return FRotationMatrix::MakeFromXZ(FVector(-1.0, 0.0, 0.0), FVector(0.0, -1.0, 0.0)).Rotator();
	}

	FText HobName()
	{
		// As his speaker point names him (AHobBird).
		return NSLOCTEXT("LooterStory", "HobName", "Hob");
	}

	/**
	 * Calder's note in her words as the art writes them on it (Tools/Blender/looter_posters.py, the atlas's Note cell):
	 * that picture and these lines are the only places the caches are named. Hob has the last word, which seeds Ruth.
	 */
	TArray<FStoryLine> CalderNoteLines()
	{
		const FText Calder = NSLOCTEXT("LooterPoster", "CalderName", "R. Calder");
		return {
			FStoryLine::Make(Calder, NSLOCTEXT("LooterPoster", "NoteCaches",
				"Rangers: three caches laid on the Rest. Under the windmill, the Sink rim, the bluff path.")),
			FStoryLine::Make(Calder, NSLOCTEXT("LooterPoster", "NoteTake", "Take what you need.")),
			FStoryLine::Make(HobName(), NSLOCTEXT("LooterPoster", "NoteHob",
				"Calder. Same hand that signed your poster, sunshine. She shoots first and reads the small print after.")),
		};
	}

	/** Hob's remarks as the posters come down: the first, the third, and the sixth (Side 1's count), pointing to the board. */
	TArray<FWantedPosterRemark> HobRemarks()
	{
		auto Remark = [](int32 TornCount, const FText& Words)
		{
			FWantedPosterRemark Made;
			Made.TornCount = TornCount;
			Made.Lines.Add(FStoryLine::Make(HobName(), Words));
			return Made;
		};
		return {
			Remark(1, NSLOCTEXT("LooterPoster", "RemarkFirst",
				"Somebody wrote 'already' under 'dead or alive'. This town has a sense of humor after all.")),
			Remark(3, NSLOCTEXT("LooterPoster", "RemarkThird", "Nobody collects on a corpse, sunshine. You could leave a few up.")),
			Remark(6, NSLOCTEXT("LooterPoster", "RemarkSixth",
				"That's most of them. The Rangers' board by the sheriff's has a note on it, if you want more news of yourself.")),
		};
	}
}

// ---------------------------------------------------------------------------
// The atlas
// ---------------------------------------------------------------------------

FVector2D FWantedPosterAtlas::SizeOf(const FVector4& Cell) const
{
	return FVector2D(FMath::Abs(Cell.Z - Cell.X), FMath::Abs(Cell.W - Cell.Y)) * Centimeters;
}

// ---------------------------------------------------------------------------
// Its make-up and life
// ---------------------------------------------------------------------------

AWantedPoster::AWantedPoster()
{
	// Ticks only while a torn-off scrap falls.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// The paper, projected into the surface behind it; its cell, size and spot are set by RefreshLook.
	Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Decal"));
	Decal->SetupAttachment(Root);
	Decal->SetMobility(EComponentMobility::Movable);
	Decal->SetRelativeRotation(DecalTurn());

	// What the Interact key's line (the Visibility channel) finds; nothing else meets it. World dynamic, so the traces
	// that look for the ground among world-static things (the minimap's bake, the scatter, settling props) never do.
	Hitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("Hitbox"));
	Hitbox->SetupAttachment(Root);
	Hitbox->SetMobility(EComponentMobility::Movable);
	Hitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Hitbox->SetCollisionObjectType(ECC_WorldDynamic);
	Hitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
	Hitbox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Hitbox->SetGenerateOverlapEvents(false);
	Hitbox->SetCanEverAffectNavigation(false);
	Hitbox->SetHiddenInGame(true);

	TearPrompt = NSLOCTEXT("LooterPoster", "TearPrompt", "Tear down the poster");
	ReadPrompt = NSLOCTEXT("LooterPoster", "ReadPrompt", "Read the note");
	NoteLines = CalderNoteLines();
	TearRemarks = HobRemarks();

	static UMaterialInterface* const DecalAsset = FindIfMade<UMaterialInterface>(DecalMaterialPath);
	static UMaterialInterface* const ScrapAsset = FindIfMade<UMaterialInterface>(ScrapMaterialPath);
	static UStaticMesh* const PlaneAsset = FindIfMade<UStaticMesh>(PlanePath);
	DecalMaterial = DecalAsset;
	ScrapMaterial = ScrapAsset;
	ScrapMesh = PlaneAsset;

	Tags.Add(WantedTag);
	Tags.Add(MinimapTags::Obstacle);
	// The whole poster's shape from the start; its material instance comes once it's placed or spawned.
	RefreshLook();
}

void AWantedPoster::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Placed, spawned or edited: the right tag, cell and box at once (the editor shows the note as the note).
	RefreshTags();
	RefreshLook();
}

void AWantedPoster::BeginPlay()
{
	Super::BeginPlay();
	RefreshTags();
	RefreshLook();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AWantedPoster::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	EndScrap();
	Super::EndPlay(EndPlayReason);
}

void AWantedPoster::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void AWantedPoster::SetVariant(EWantedPosterVariant NewVariant)
{
	Variant = NewVariant;
	// Calder's note is never down.
	bTorn = bTorn && Variant == EWantedPosterVariant::Wanted;
	RefreshTags();
	RefreshLook();
}

void AWantedPoster::RefreshTags()
{
	Tags.Remove(WantedTag);
	Tags.Remove(NoteTag);
	Tags.AddUnique(Variant == EWantedPosterVariant::CalderNote ? NoteTag : WantedTag);
	Tags.AddUnique(MinimapTags::Obstacle);
}

// ---------------------------------------------------------------------------
// Its look
// ---------------------------------------------------------------------------

FVector4 AWantedPoster::GetWholeCell() const
{
	return Variant == EWantedPosterVariant::CalderNote ? Atlas.Note : Atlas.Poster;
}

FVector4 AWantedPoster::GetShownCell() const
{
	return Variant == EWantedPosterVariant::Wanted && bTorn ? Atlas.Remnant : GetWholeCell();
}

void AWantedPoster::RefreshLook()
{
	if (!Decal || !Hitbox)
	{
		return;
	}
	const FVector2D Whole = Atlas.SizeOf(GetWholeCell()) * Scale;
	const FVector4 Shown = GetShownCell();
	const FVector2D Size = Atlas.SizeOf(Shown) * Scale;

	// The decal's box: its depth from ReachIn behind the surface to ReachOut in front, the picture across its other two
	// axes (Y runs down the picture, Z across it; see DecalTurn). A cell smaller than the whole hangs from the same top
	// edge, under the nails (the art draws the remnant at the poster's size, so it swaps in place).
	const double HalfDepth = (ReachIn + ReachOut) * 0.5;
	Decal->DecalSize = FVector(HalfDepth, Size.Y * 0.5, Size.X * 0.5);
	Decal->SetRelativeLocation(FVector(ReachOut - HalfDepth, 0.0, (Whole.Y - Size.Y) * 0.5));
	Decal->MarkRenderStateDirty();
	if (UMaterialInstanceDynamic* Material = FindOrMakeDecalMaterial())
	{
		Material->SetVectorParameterValue(CellParameter, FLinearColor(static_cast<float>(Shown.X), static_cast<float>(Shown.Y),
			static_cast<float>(Shown.Z), static_cast<float>(Shown.W)));
	}

	// The box over the whole sheet, a hair proud of the surface; down, there's nothing left to use.
	Hitbox->SetBoxExtent(FVector(BoxHalfThickness, Whole.X * 0.5, Whole.Y * 0.5), /*bUpdateOverlaps*/ false);
	Hitbox->SetRelativeLocation(FVector(BoxGap + BoxHalfThickness, 0.0, 0.0));
	Hitbox->SetCollisionEnabled(bTorn ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
}

UMaterialInstanceDynamic* AWantedPoster::FindOrMakeDecalMaterial()
{
	// Never for the class's defaults, nor while an instance is still being made (its constructor sets the shape only).
	if (!Decal || HasAnyFlags(RF_ClassDefaultObject | RF_NeedInitialization))
	{
		return nullptr;
	}
	UMaterialInterface* Base = DecalMaterial ? DecalMaterial.Get() : LoadIfMade<UMaterialInterface>(DecalMaterialPath);
	if (!Base)
	{
		return nullptr;
	}
	// One instance per poster for its cell, kept for its life (and saved with a placed one, so the editor shows its cell).
	UMaterialInstanceDynamic* Current = Cast<UMaterialInstanceDynamic>(Decal->GetDecalMaterial());
	if (Current && Current->Parent == Base)
	{
		return Current;
	}
	UMaterialInstanceDynamic* Made = UMaterialInstanceDynamic::Create(Base, this);
	Decal->SetDecalMaterial(Made);
	return Made;
}

UStaticMesh* AWantedPoster::GetScrapMesh() const
{
	return ScrapMesh ? ScrapMesh.Get() : LoadIfMade<UStaticMesh>(PlanePath);
}

UMaterialInterface* AWantedPoster::GetScrapMaterial() const
{
	return ScrapMaterial ? ScrapMaterial.Get() : LoadIfMade<UMaterialInterface>(ScrapMaterialPath);
}

// ---------------------------------------------------------------------------
// Being used
// ---------------------------------------------------------------------------

FInteractionOptions AWantedPoster::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	Options.Reach = Reach;
	if (Variant == EWantedPosterVariant::CalderNote)
	{
		Options.bUsable = CanRead();
		Options.bTap = true;
		Options.bHold = false;
		Options.TapPrompt = ReadPrompt;
		return Options;
	}
	// Only a hold: a poster isn't torn by brushing the key on the way past.
	Options.bUsable = CanTear();
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = TearHoldSeconds;
	Options.HoldPrompt = TearPrompt;
	return Options;
}

bool AWantedPoster::Interact(UInteractionComponent& User, bool bHeld)
{
	if (Variant == EWantedPosterVariant::CalderNote)
	{
		return !bHeld && Read(User.GetOwner());
	}
	return bHeld && Tear(User.GetOwner());
}

TOptional<FVector> AWantedPoster::GetInteractionLocation() const
{
	// The middle of the sheet, a centimetre in front of its box: the sight line from the eyes ends there, short of the wall.
	return GetActorLocation() + GetActorForwardVector() * (BoxGap + 2.0 * BoxHalfThickness + 1.0);
}

bool AWantedPoster::CanTear() const
{
	return Variant == EWantedPosterVariant::Wanted && !bTorn;
}

bool AWantedPoster::CanRead() const
{
	return Variant == EWantedPosterVariant::CalderNote && !IsBeingRead();
}

bool AWantedPoster::IsBeingRead() const
{
	const UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	return Captions && Conversation != 0 && Captions->IsPlaying(Conversation);
}

// ---------------------------------------------------------------------------
// Reading Calder's note
// ---------------------------------------------------------------------------

bool AWantedPoster::Read(AActor* Reader)
{
	if (!CanRead())
	{
		return false;
	}
	if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
	{
		// Read like a talk at a door: the reading has the floor, and whatever was being said stops.
		Conversation = Captions->Play(NoteLines, ECaptionPlay::Interrupt);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: Calder's note read (%d lines)%s."), *GetActorNameOrLabel(), NoteLines.Num(),
		Reader ? *FString::Printf(TEXT(" by %s"), *Reader->GetName()) : TEXT(""));
	return true;
}

int32 AWantedPoster::CountTorn(const UWorld* World)
{
	int32 Torn = 0;
	if (World)
	{
		for (TActorIterator<AWantedPoster> It(World); It; ++It)
		{
			Torn += It->IsTorn() ? 1 : 0;
		}
	}
	return Torn;
}
