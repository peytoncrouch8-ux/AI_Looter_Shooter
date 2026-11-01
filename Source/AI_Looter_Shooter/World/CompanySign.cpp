// ACompanySign: Vane Consolidated's words put up in the Rest (the checkpoint's board, the chalk on Main Street's doors).

#include "World/CompanySign.h"
#include "AI_Looter_Shooter.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/StoryProp.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* const ACompanySign::BoardModelPath = TEXT("/Game/Art/Tripo/SM_CompanyCheckpointSign.SM_CompanyCheckpointSign");

namespace
{
	const TCHAR* SignCubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* SignCylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* SignBoardPaintPath = TEXT("/Game/Art/Materials/MI_PaintBlack.MI_PaintBlack");
	const TCHAR* SignPostWoodPath = TEXT("/Game/Art/Materials/MI_LanternWood.MI_LanternWood");

	/** Company white on its black boards, and chalk: a little warmer, a little greyer. */
	const FColor StencilWhite(232, 228, 214);
	const FColor ChalkWhite(222, 219, 204);
	/** The board's thickness, its posts' girth and how far the words stand off its face (cm). */
	constexpr float BoardThick = 6.f;
	constexpr float PostGirth = 12.f;
	constexpr float WordsOff = 4.f;

	UStaticMesh* SignModelIfMade()
	{
		const FString Path(ACompanySign::BoardModelPath);
		if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
		{
			return nullptr;
		}
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(ACompanySign::BoardModelPath);
		return Finder.Object;
	}
}

ACompanySign::ACompanySign()
{
	PrimaryActorTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(SignCubePath);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(SignCylinderPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Paint(SignBoardPaintPath);
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Wood(SignPostWoodPath);
	static UStaticMesh* const Model = SignModelIfMade();

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Board = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Board"));
	Board->SetupAttachment(Root);
	Board->SetStaticMesh(Model ? Model : Cube.Object.Get());
	if (!Model)
	{
		Board->SetMaterial(0, Paint.Object);
	}
	Board->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Board->SetCanEverAffectNavigation(false);

	auto MakePost = [&](const TCHAR* Name)
	{
		UStaticMeshComponent* Post = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Post->SetupAttachment(Root);
		Post->SetStaticMesh(Cylinder.Object);
		Post->SetMaterial(0, Wood.Object);
		Post->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Post->SetCanEverAffectNavigation(false);
		return Post;
	};
	PostLeft = MakePost(TEXT("PostLeft"));
	PostRight = MakePost(TEXT("PostRight"));

	Words = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Words"));
	Words->SetupAttachment(Root);
	Words->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	Words->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	Words->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Words->SetCastShadow(false);
}

void ACompanySign::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Arrange();
}

void ACompanySign::Arrange()
{
	const bool bModel = Board && Board->GetStaticMesh() && Board->GetStaticMesh()->GetPathName() == FString(BoardModelPath);
	const bool bBoard = Look == ECompanySignLook::Board;
	const float Middle = BoardLift + static_cast<float>(BoardSize.Y) * 0.5f;
	if (Board)
	{
		Board->SetVisibility(bBoard);
		Board->SetCollisionEnabled(bBoard ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		if (!bModel)
		{
			Board->SetRelativeLocation(FVector(0.0, 0.0, Middle));
			Board->SetRelativeScale3D(FVector(BoardThick / 100.0, BoardSize.X / 100.0, BoardSize.Y / 100.0));
		}
	}
	for (const float Side : { -1.f, 1.f })
	{
		UStaticMeshComponent* Post = Side < 0.f ? PostLeft.Get() : PostRight.Get();
		if (!Post)
		{
			continue;
		}
		const bool bShown = bBoard && !bModel;
		const float Tall = BoardLift + static_cast<float>(BoardSize.Y);
		Post->SetVisibility(bShown);
		Post->SetCollisionEnabled(bShown ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
		Post->SetRelativeLocation(FVector(-BoardThick, Side * (BoardSize.X * 0.5f - 18.f), Tall * 0.5f));
		Post->SetRelativeScale3D(FVector(PostGirth / 100.0, PostGirth / 100.0, Tall / 100.0));
	}
	if (Words)
	{
		// Drawn on its face (the Tripo board comes blank: TRIPO_MODELS.md M23-4), or on the wall, chalked.
		Words->SetVisibility(true);
		Words->SetText(Text);
		Words->SetWorldSize(LetterSize);
		Words->SetTextRenderColor(bBoard ? StencilWhite : ChalkWhite);
		Words->SetRelativeLocationAndRotation(FVector(bBoard ? BoardThick * 0.5f + WordsOff * 0.25f : WordsOff, 0.0, bBoard ? Middle : BoardLift),
			FRotator(0.0, 0.0, bBoard ? 0.0 : Slant));
	}
}

void ACompanySign::BeginPlay()
{
	Super::BeginPlay();
	Arrange();
	if (ShownWhen.Num() > 0 || HiddenWhen.Num() > 0)
	{
		if (UMissionRunner* Runner = UMissionRunner::Get(this))
		{
			BoundRunner = Runner;
			ChangedHandle = Runner->OnChanged.AddUObject(this, &ACompanySign::HandleRunnerChanged);
		}
	}
	Refresh();
}

void ACompanySign::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(ChangedHandle);
	}
	BoundRunner.Reset();
	ChangedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

bool ACompanySign::ShouldShow() const
{
	if (ShownWhen.Num() == 0 && HiddenWhen.Num() == 0)
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return AStoryProp::Applies(ShownWhen, HiddenWhen, Runner ? &Runner->GetCampaign() : nullptr, Runner);
}

void ACompanySign::Refresh()
{
	const bool bShown = ShouldShow();
	SetActorHiddenInGame(!bShown);
	SetActorEnableCollision(bShown);
}

void ACompanySign::HandleRunnerChanged()
{
	Refresh();
}
