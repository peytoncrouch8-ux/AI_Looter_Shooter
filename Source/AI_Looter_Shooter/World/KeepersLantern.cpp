// AKeepersLantern: the Keeper's Lantern hanging dark in the Sink's webbing, and taking it.

#include "World/KeepersLantern.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "UObject/ConstructorHelpers.h"

const FName AKeepersLantern::LanternTag(TEXT("Lantern_Keeper"));
const FName AKeepersLantern::Mission(TEXT("Main5"));

namespace
{
	/** Sink.py's tangle, BurialDeck.py's lantern, and the trim's window glass its glass shows while dark. */
	const TCHAR* SnarePath = TEXT("/Game/Art/Props/SM_Web_Snare.SM_Web_Snare");
	const TCHAR* LanternPath = TEXT("/Game/Art/Props/SM_KeepersLantern.SM_KeepersLantern");
	const TCHAR* DarkGlassPath = TEXT("/Game/Art/Materials/MI_HouseTrim.MI_HouseTrim");

	/**
	 * Without the models: where Sink.py's cord ends under the tangle's middle (Blender (0.06, -0.02, -0.34)), and the
	 * lantern's grip over its foot (BurialDeck.py: 0.427 m), in the actor's and the lantern's frames (cm).
	 */
	const FVector FallbackHang(2.0, -6.0, -34.0);
	const FVector FallbackGrip(0.0, 0.0, 42.7);

	/** The box round the lantern, out past its iron guard (half sizes, cm); its height follows the lantern's. */
	constexpr double BoxHalfWidth = 16.0;
	constexpr double BoxPad = 4.0;

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
}

bool AKeepersLantern::IsTaken(const FCampaignRecord& Campaign, const UMissionRunner* Runner)
{
	// Main 5 finished, or on a step after the one that takes it.
	FStoryCondition Finished;
	Finished.AfterMissions = { Mission };
	FStoryCondition Past;
	Past.DuringMission = Mission;
	Past.FromStep = TakeStep + 1;
	return Finished.IsMet(Campaign, Runner) || Past.IsMet(Campaign, Runner);
}

bool AKeepersLantern::IsTakenIn(const UObject* WorldContextObject)
{
	const UMissionRunner* Runner = UMissionRunner::Get(WorldContextObject);
	return Runner && IsTaken(Runner->GetCampaign(), Runner);
}

AKeepersLantern::AKeepersLantern()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// A web card: nothing meets it, it casts no shadow (the art's rule for every card).
	static UStaticMesh* const SnareModel = FindIfMade<UStaticMesh>(SnarePath);
	Snare = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Snare"));
	Snare->SetupAttachment(Root);
	Snare->SetMobility(EComponentMobility::Movable);
	Snare->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Snare->SetGenerateOverlapEvents(false);
	Snare->SetCanEverAffectNavigation(false);
	Snare->SetCastShadow(false);
	Snare->SetStaticMesh(SnareModel);

	// Small, and hanging in the pit's shade: no shadow; the box below is what's found.
	static UStaticMesh* const LanternModel = FindIfMade<UStaticMesh>(LanternPath);
	Lantern = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lantern"));
	Lantern->SetupAttachment(Root);
	Lantern->SetMobility(EComponentMobility::Movable);
	Lantern->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Lantern->SetGenerateOverlapEvents(false);
	Lantern->SetCanEverAffectNavigation(false);
	Lantern->SetCastShadow(false);
	Lantern->SetStaticMesh(LanternModel);

	// What the Interact key's line (the Visibility channel) finds; nothing else meets it. World dynamic, so the traces that
	// look for the ground among world-static things never do.
	Grip = CreateDefaultSubobject<UBoxComponent>(TEXT("Grip"));
	Grip->SetupAttachment(Root);
	Grip->SetMobility(EComponentMobility::Movable);
	Grip->InitBoxExtent(FVector(BoxHalfWidth, BoxHalfWidth, FallbackGrip.Z * 0.5 + BoxPad));
	Grip->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Grip->SetCollisionObjectType(ECC_WorldDynamic);
	Grip->SetCollisionResponseToAllChannels(ECR_Ignore);
	Grip->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Grip->SetGenerateOverlapEvents(false);
	Grip->SetCanEverAffectNavigation(false);
	Grip->SetHiddenInGame(true);

	static UMaterialInterface* const TrimGlass = FindIfMade<UMaterialInterface>(DarkGlassPath);
	DarkGlass = TrimGlass;

	Prompt = NSLOCTEXT("LooterSink", "TakeLantern", "Take the Keeper's Lantern");
	// Main 5's third step (from 0: 2) only; the build script sets the same.
	TakeWhen.DuringMission = Mission;
	TakeWhen.FromStep = TakeStep;
	TakeWhen.BeforeStep = TakeStep + 1;
	Tags.Add(LanternTag);
}

void AKeepersLantern::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	// Placed or edited: hanging from the cord, dark, as the level shows it before Main 5's third step.
	HangLantern();
	ApplyGlass();
}

void AKeepersLantern::BeginPlay()
{
	Super::BeginPlay();
	HangLantern();
	ApplyGlass();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &AKeepersLantern::HandleMissionsChanged);
	}
	ShowInSnare(true);
	RefreshStory();
}

void AKeepersLantern::EndPlay(const EEndPlayReason::Type EndPlayReason)
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
// Hanging in the webbing, dark
// ---------------------------------------------------------------------------

void AKeepersLantern::HangLantern()
{
	// Where the snare's cord ends, in the actor's frame.
	FVector Hang = Snare->GetRelativeTransform().TransformPosition(FallbackHang);
	if (Snare->GetStaticMesh() && Snare->DoesSocketExist(HangSocket))
	{
		Hang = GetActorTransform().InverseTransformPosition(Snare->GetSocketLocation(HangSocket));
	}
	// The lantern's grip, from its foot.
	FVector GripOffset = FallbackGrip;
	if (Lantern->GetStaticMesh() && Lantern->DoesSocketExist(GripSocket))
	{
		GripOffset = Lantern->GetSocketTransform(GripSocket, RTS_Component).GetLocation();
	}
	// Upright, its grip on the cord's end; the box round it from its foot to its grip.
	Lantern->SetRelativeLocationAndRotation(Hang - GripOffset, FRotator::ZeroRotator);
	const double HalfHeight = FMath::Max(GripOffset.Z, 10.0) * 0.5 + BoxPad;
	Grip->SetBoxExtent(FVector(BoxHalfWidth, BoxHalfWidth, HalfHeight));
	Grip->SetRelativeLocationAndRotation(Hang - GripOffset + FVector(0.0, 0.0, GripOffset.Z * 0.5), FRotator::ZeroRotator);
}

int32 AKeepersLantern::FindGlassSlot() const
{
	const UStaticMesh* Model = Lantern ? Lantern->GetStaticMesh() : nullptr;
	if (!Model)
	{
		return INDEX_NONE;
	}
	const int32 Named = Lantern->GetMaterialIndex(GlassSlot);
	if (Named != INDEX_NONE)
	{
		return Named;
	}
	// The slot named otherwise: the one whose own material is the shared LanternGlow (MI_LanternGlow).
	for (int32 Slot = 0; Slot < Lantern->GetNumMaterials(); ++Slot)
	{
		const UMaterialInterface* Own = Model->GetMaterial(Slot);
		if (Own && Own->GetName().Contains(GlassSlot.ToString()))
		{
			return Slot;
		}
	}
	return INDEX_NONE;
}

void AKeepersLantern::ApplyGlass()
{
	const int32 Slot = FindGlassSlot();
	if (Slot == INDEX_NONE)
	{
		return;
	}
	if (!LitGlass)
	{
		LitGlass = Lantern->GetStaticMesh()->GetMaterial(Slot);
	}
	UMaterialInterface* Wanted = bLit ? LitGlass.Get() : DarkGlass.Get();
	if (Wanted && Lantern->GetMaterial(Slot) != Wanted)
	{
		Lantern->SetMaterial(Slot, Wanted);
	}
}

void AKeepersLantern::SetLit(bool bInLit)
{
	bLit = bInLit;
	ApplyGlass();
}

void AKeepersLantern::ShowInSnare(bool bShown)
{
	Lantern->SetVisibility(bShown);
	Grip->SetCollisionEnabled(bShown ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}

// ---------------------------------------------------------------------------
// Taking it
// ---------------------------------------------------------------------------

FInteractionOptions AKeepersLantern::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	// Not before its step (it's only a lantern in a web until the sacs are down), and never once it's gone.
	Options.bUsable = CanTake();
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = Prompt;
	Options.Reach = Reach;
	return Options;
}

bool AKeepersLantern::Interact(UInteractionComponent& User, bool bHeld)
{
	return !bHeld && Take(User.GetOwner());
}

TOptional<FVector> AKeepersLantern::GetInteractionLocation() const
{
	// The lantern's middle, where a hand closes on it.
	return Grip ? TOptional<FVector>(Grip->GetComponentLocation()) : TOptional<FVector>();
}

bool AKeepersLantern::CanTake() const
{
	if (bTaken)
	{
		return false;
	}
	if (TakeWhen.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && TakeWhen.IsMet(Runner->GetCampaign(), Runner);
}

bool AKeepersLantern::Take(AActor* ByWhom, bool bForce)
{
	if (bTaken || (!bForce && !CanTake()))
	{
		return false;
	}
	bTaken = true;
	ShowInSnare(false);
	UE_LOG(LogLooter, Log, TEXT("%s: the Keeper's Lantern taken from the webbing%s. It's dark."), *GetActorNameOrLabel(),
		ByWhom ? *FString::Printf(TEXT(" (by %s)"), *ByWhom->GetName()) : TEXT(""));
	return true;
}

void AKeepersLantern::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner)
	{
		return;
	}
	if (IsTaken(Runner->GetCampaign(), Runner))
	{
		// Ellis has it: the snare is empty, whatever this level saw.
		if (!bTaken)
		{
			bTaken = true;
			ShowInSnare(false);
		}
		return;
	}
	if (bTaken && !TakeWhen.IsEmpty() && !TakeWhen.IsMet(Runner->GetCampaign(), Runner))
	{
		// The story went back before its step (the console starting Main 5 over): it hangs there again.
		bTaken = false;
		ShowInSnare(true);
	}
}

void AKeepersLantern::HandleMissionsChanged()
{
	RefreshStory();
}
