// AHayBale: one of Amos's hay bales, out in the field or loaded into the stack by his barn (Side 2).

#include "World/HayBale.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Session/SessionSubsystem.h"
#include "Story/CaptionSubsystem.h"
#include "Story/StoryLineSet.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const FName AHayBale::BaleTag(TEXT("HayBale"));
const TCHAR* const AHayBale::ModelPath = TEXT("/Game/Art/Props/SM_HayBale_Square.SM_HayBale_Square");

namespace
{
	/** Side 2, "Unfinished Business", and its step that loads the hay (counted from 0). */
	const FName SideTwo(TEXT("Side2"));
	constexpr int32 LoadStep = 1;

	/** Without the model: the engine's cube (1 m, about its middle) at the bale's size, 0.46 x 0.95 x 0.36 m, on the ground. */
	const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const FVector StandInScale(0.46, 0.95, 0.36);

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

	/** A bale's mesh: the model, pivot on the ground at its middle, or the cube standing in at its size. */
	UStaticMeshComponent* MakeBale(AActor* Owner, const TCHAR* Name, USceneComponent* Parent, UStaticMesh* Model, UStaticMesh* Cube)
	{
		UStaticMeshComponent* Made = Owner->CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Made->SetupAttachment(Parent);
		Made->SetMobility(EComponentMobility::Movable);
		// Solid, so it's stood on and found by the Interact key's line; world dynamic, so the ground traces (a creature's
		// footing, the scatter, settling props) never take it for the ground.
		Made->SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);
		Made->SetGenerateOverlapEvents(false);
		Made->SetCanEverAffectNavigation(false);
		Made->SetStaticMesh(Model ? Model : Cube);
		return Made;
	}
}

AHayBale::AHayBale()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static UStaticMesh* const Model = FindIfMade<UStaticMesh>(ModelPath);
	static UStaticMesh* const Cube = FindIfMade<UStaticMesh>(CubePath);
	Bale = MakeBale(this, TEXT("Bale"), Root, Model, Cube);
	Stacked = MakeBale(this, TEXT("Stacked"), Root, Model, Cube);
	if (!Model)
	{
		for (UStaticMeshComponent* Part : { Bale.Get(), Stacked.Get() })
		{
			Part->SetRelativeLocation(FVector(0.0, 0.0, StandInScale.Z * 50.0));
			Part->SetRelativeScale3D(StandInScale);
		}
	}

	Prompt = NSLOCTEXT("LooterHay", "LoadPrompt", "Load the hay bale");
	// Once Amos has asked (Side 2 from its second step); in the stack for good once Side 2 is done. The build script sets
	// the same.
	LoadWhen.DuringMission = SideTwo;
	LoadWhen.FromStep = LoadStep;
	LoadedWhen.AfterMissions = { SideTwo };
	Tags.Add(BaleTag);
}

void AHayBale::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// The editor shows both: the bale in the field and its place in the stack.
	ShowLoaded(bLoaded);
}

void AHayBale::BeginPlay()
{
	Super::BeginPlay();
	ShowLoaded(bLoaded);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &AHayBale::HandleMissionsChanged);
	}
	RefreshStory();
}

void AHayBale::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	MissionsChangedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// In the field, or in the stack
// ---------------------------------------------------------------------------

void AHayBale::ShowLoaded(bool bInStack)
{
	// Hidden in the game only: the editor shows the bale and its place in the stack together.
	Bale->SetHiddenInGame(bInStack);
	Bale->SetCollisionEnabled(bInStack ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	Stacked->SetHiddenInGame(!bInStack);
	Stacked->SetCollisionEnabled(bInStack ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

bool AHayBale::CanLoad() const
{
	if (bLoaded)
	{
		return false;
	}
	if (LoadWhen.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && LoadWhen.IsMet(Runner->GetCampaign(), Runner);
}

bool AHayBale::Load(AActor* ByWhom, bool bForce)
{
	if (bLoaded || (!bForce && !CanLoad()))
	{
		return false;
	}
	bLoaded = true;
	ShowLoaded(true);
	const int32 LoadedNow = CountLoaded(GetWorld());
	UE_LOG(LogLooter, Log, TEXT("%s: hay bale loaded into the stack by Whitlock's barn%s; %d of the level's are in."), *GetActorNameOrLabel(),
		ByWhom ? *FString::Printf(TEXT(" by %s"), *ByWhom->GetName()) : TEXT(""), LoadedNow);
	SayRemark(LoadedNow);
	// Progress: the session keeps the loaded bales with this map's world (FSavedMapWorld::LoadedBales).
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->SaveSoon();
	}
	return true;
}

void AHayBale::RestoreLoaded()
{
	if (bLoaded)
	{
		return;
	}
	bLoaded = true;
	ShowLoaded(true);
}

void AHayBale::PutBack()
{
	bLoaded = false;
	ShowLoaded(false);
}

int32 AHayBale::CountLoaded(const UWorld* World)
{
	int32 Loaded = 0;
	if (World)
	{
		for (TActorIterator<AHayBale> It(World); It; ++It)
		{
			Loaded += It->IsLoaded() ? 1 : 0;
		}
	}
	return Loaded;
}

void AHayBale::SayRemark(int32 LoadedNow)
{
	const FHayBaleRemark* Remark = LoadRemarks.FindByPredicate([LoadedNow](const FHayBaleRemark& Each) { return Each.LoadedCount == LoadedNow; });
	UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this);
	if (!Remark || !Captions)
	{
		return;
	}
	const TArray<FStoryLine>& Lines = Remark->LineSet ? Remark->LineSet->Lines : Remark->Lines;
	if (!Lines.IsEmpty())
	{
		// A remark on something done: after whatever is being said.
		Captions->Play(Lines, ECaptionPlay::Queue);
	}
}

void AHayBale::RefreshStory()
{
	if (bLoaded || LoadedWhen.IsEmpty())
	{
		return;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (Runner && LoadedWhen.IsMet(Runner->GetCampaign(), Runner))
	{
		// The hay's in: the story is past it, whatever this level saw.
		RestoreLoaded();
	}
}

void AHayBale::HandleMissionsChanged()
{
	RefreshStory();
}

// ---------------------------------------------------------------------------
// Being used
// ---------------------------------------------------------------------------

FInteractionOptions AHayBale::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	// Only a hold, and only once Amos has asked: before that it's just hay rotting in a field.
	Options.bUsable = CanLoad();
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = LoadHoldSeconds;
	Options.HoldPrompt = Prompt;
	Options.Reach = Reach;
	return Options;
}

bool AHayBale::Interact(UInteractionComponent& User, bool bHeld)
{
	return bHeld && Load(User.GetOwner());
}

TOptional<FVector> AHayBale::GetInteractionLocation() const
{
	// The bale's middle in the field, where hands go to lift it.
	return Bale ? TOptional<FVector>(Bale->Bounds.Origin) : TOptional<FVector>();
}
