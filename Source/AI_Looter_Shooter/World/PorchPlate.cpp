// APorchPlate: Delia's porch plate: the jars of grave salt taken once (Main 1), the biscuits eaten later (Main 7's optional),
// both kept by the campaign's story flags.

#include "World/PorchPlate.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Player/PlayerThrowComponent.h"
#include "Story/CaptionSubsystem.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#define LOCTEXT_NAMESPACE "PorchPlate"

const FName APorchPlate::PlateTag(TEXT("PorchPlate"));

namespace
{
	/** A small prop's collision: the Interact key's line finds it, nothing else does. */
	void SetupPiece(UStaticMeshComponent* Piece, bool bFoundByInteract)
	{
		Piece->SetMobility(EComponentMobility::Movable);
		Piece->SetCollisionEnabled(bFoundByInteract ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
		Piece->SetCollisionObjectType(ECC_WorldDynamic);
		Piece->SetCollisionResponseToAllChannels(ECR_Ignore);
		if (bFoundByInteract)
		{
			Piece->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}
		Piece->SetGenerateOverlapEvents(false);
		Piece->SetCanEverAffectNavigation(false);
	}

	/** The pawn behind Player (a pawn or its controller). */
	APawn* PawnOf(AActor* Player)
	{
		if (APawn* Pawn = Cast<APawn>(Player))
		{
			return Pawn;
		}
		const AController* Controller = Cast<AController>(Player);
		return Controller ? Controller->GetPawn() : nullptr;
	}
}

APorchPlate::APorchPlate()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Plate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plate"));
	Plate->SetupAttachment(Root);
	SetupPiece(Plate, /*bFoundByInteract*/ true);
	Jars = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Jars"));
	Jars->SetupAttachment(Root);
	SetupPiece(Jars, /*bFoundByInteract*/ true);
	Biscuits = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Biscuits"));
	Biscuits->SetupAttachment(Root);
	SetupPiece(Biscuits, /*bFoundByInteract*/ true);

	TakePrompt = LOCTEXT("TakePrompt", "Take the jars");
	EatPrompt = LOCTEXT("EatPrompt", "Eat the biscuits");
	Tags.Add(PlateTag);
}

void APorchPlate::BeginPlay()
{
	Super::BeginPlay();
	Tags.AddUnique(PlateTag);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	// The flags may change by the console or a load: the look follows the missions' changes.
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		ChangedHandle = Runner->OnChanged.AddUObject(this, &APorchPlate::HandleRunnerChanged);
	}
	RefreshLook();
}

void APorchPlate::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(ChangedHandle);
	}
	BoundRunner.Reset();
	ChangedHandle.Reset();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

void APorchPlate::HandleRunnerChanged()
{
	RefreshLook();
}

// ---------------------------------------------------------------------------
// What's on it
// ---------------------------------------------------------------------------

bool APorchPlate::IsMet(const FStoryCondition& When) const
{
	if (When.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && When.IsMet(Runner->GetCampaign(), Runner);
}

bool APorchPlate::IsTaken() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Runner->HasFlag(TakenFlag);
}

bool APorchPlate::IsEaten() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Runner->HasFlag(EatenFlag);
}

bool APorchPlate::CanTake() const
{
	return !IsTaken() && IsMet(TakeWhen);
}

bool APorchPlate::CanEat() const
{
	return bCanEat && IsTaken() && !IsEaten() && IsMet(EatWhen);
}

bool APorchPlate::IsUsedUp() const
{
	return IsTaken() && (!bCanEat || IsEaten());
}

void APorchPlate::RefreshLook()
{
	if (Jars)
	{
		Jars->SetVisibility(!IsTaken());
	}
	if (Biscuits)
	{
		Biscuits->SetVisibility(!IsEaten());
	}
}

// ---------------------------------------------------------------------------
// The Interact key
// ---------------------------------------------------------------------------

FInteractionOptions APorchPlate::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	const bool bTake = CanTake();
	Options.bUsable = bTake || CanEat();
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = bTake ? TakePrompt : EatPrompt;
	Options.Reach = Reach;
	return Options;
}

bool APorchPlate::Interact(UInteractionComponent& User, bool bHeld)
{
	if (bHeld)
	{
		return false;
	}
	return CanTake() ? Take(User.GetOwner()) : Eat(User.GetOwner());
}

TOptional<FVector> APorchPlate::GetInteractionLocation() const
{
	return Plate && Plate->GetStaticMesh() ? TOptional<FVector>(Plate->Bounds.Origin) : TOptional<FVector>(GetActorLocation());
}

void APorchPlate::Say(const TArray<FStoryLine>& Said) const
{
	if (Said.IsEmpty())
	{
		return;
	}
	if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
	{
		Captions->Play(Said, ECaptionPlay::Interrupt);
	}
}

bool APorchPlate::Take(AActor* Player)
{
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner || !CanTake())
	{
		return false;
	}
	// Delia's salt: two grenades, the counter unlocked and flashing as any gift does. Full already, the jars are still taken
	// (the slip is read either way, and nothing waits on the count).
	const APawn* Pawn = PawnOf(Player);
	UPlayerThrowComponent* Throw = Pawn ? Pawn->FindComponentByClass<UPlayerThrowComponent>() : nullptr;
	const int32 Given = Throw && Grenades > 0 ? Throw->AddGrenades(Grenades, EGrenadeChange::Given) : 0;
	Runner->SetFlag(TakenFlag);
	RefreshLook();
	LooterSound::PlayAt(this, LooterSoundCue::MissionFeatures::PorchPlateTake, GetInteractionLocation().Get(GetActorLocation()));
	Say(TakeLines);
	Runner->NotifyEvent(FMissionEvent::Named(TakenFlag, this));
	UE_LOG(LogLooter, Log, TEXT("%s: the jars taken (%d grave-salt grenade%s)."), *GetActorNameOrLabel(), Given, Given == 1 ? TEXT("") : TEXT("s"));
	return true;
}

bool APorchPlate::Eat(AActor* Player)
{
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner || !CanEat())
	{
		return false;
	}
	Runner->SetFlag(EatenFlag);
	RefreshLook();
	LooterSound::PlayAt(this, LooterSoundCue::MissionFeatures::PorchPlateEat, GetInteractionLocation().Get(GetActorLocation()));
	Say(EatLines);
	Runner->NotifyEvent(FMissionEvent::Named(EatenFlag, this));
	UE_LOG(LogLooter, Log, TEXT("%s: the biscuits eaten."), *GetActorNameOrLabel());
	return true;
}

#undef LOCTEXT_NAMESPACE
