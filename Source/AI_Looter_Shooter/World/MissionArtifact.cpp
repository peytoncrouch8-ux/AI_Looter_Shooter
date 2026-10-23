// AMissionArtifact: a found thing that offers a mission: picking it up, the offer card, gone once taken.

#include "World/MissionArtifact.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "UI/HUD/LooterHUD.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

#define LOCTEXT_NAMESPACE "MissionArtifact"

AMissionArtifact::AMissionArtifact()
{
	// Nothing about it moves: it listens to the missions.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// A small thing lying about: the Interact key's line and its sight check find it (Visibility), nothing else does. The
	// player walks over a tag; shots and the camera pass it by.
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);

	Title = LOCTEXT("Title", "Something left behind");
	Prompt = LOCTEXT("Prompt", "Pick up");
}

void AMissionArtifact::BeginPlay()
{
	Super::BeginPlay();
	bShownNow = true;
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	// Taken already (an earlier session, an earlier visit) it's gone from the start; otherwise it waits for its mission
	// to be taken.
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		ChangedHandle = Runner->OnChanged.AddUObject(this, &AMissionArtifact::HandleRunnerChanged);
	}
	Refresh();
}

void AMissionArtifact::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

// ---------------------------------------------------------------------------
// Its missions
// ---------------------------------------------------------------------------

const UMissionDefinition* AMissionArtifact::GetOffer() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const TArray<const UMissionDefinition*> Offers = Runner ? Runner->GetOffersAt(this) : TArray<const UMissionDefinition*>();
	return Offers.IsEmpty() ? nullptr : Offers[0];
}

bool AMissionArtifact::IsTaken() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!bTakeOnAccept || !Runner)
	{
		return false;
	}
	// Every mission it offers taken (or finished): nothing is left here to pick up. One not offered yet keeps it lying there,
	// waiting for its time.
	bool bOffersAny = false;
	for (const UMissionDefinition* Mission : Runner->GetDefinitions())
	{
		if (!Mission || !Mission->IsOffered() || !ActorHasTag(Mission->Offer.GiverTag))
		{
			continue;
		}
		bOffersAny = true;
		const FName MissionId = Mission->GetMissionId();
		if (!Runner->IsAccepted(MissionId) && !Runner->IsCompleted(MissionId))
		{
			return false;
		}
	}
	return bOffersAny;
}

void AMissionArtifact::Refresh()
{
	SetShown(!IsTaken());
}

void AMissionArtifact::HandleRunnerChanged()
{
	Refresh();
}

void AMissionArtifact::SetShown(bool bShown)
{
	if (bShown == bShownNow)
	{
		return;
	}
	bShownNow = bShown;
	SetActorHiddenInGame(!bShown);
	SetActorEnableCollision(bShown);
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		if (bShown)
		{
			Registry->Register(this);
		}
		else
		{
			Registry->Unregister(this);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("%s: %s."), *GetActorNameOrLabel(), bShown ? TEXT("back where it lay") : TEXT("taken"));
}

// ---------------------------------------------------------------------------
// The Interact key
// ---------------------------------------------------------------------------

FInteractionOptions AMissionArtifact::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const bool bTurnIn = Runner && Runner->FindTurnInAt(this);
	Options.bUsable = bShownNow && (bTurnIn || GetOffer() != nullptr);
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = bTurnIn ? LOCTEXT("TurnInPrompt", "Turn in") : Prompt;
	Options.Reach = Reach;
	return Options;
}

bool AMissionArtifact::Interact(UInteractionComponent& User, bool bHeld)
{
	return !bHeld && Use(User.GetOwner());
}

TOptional<FVector> AMissionArtifact::GetInteractionLocation() const
{
	// The middle of what's seen; the actor's spot while it has no mesh yet.
	return Mesh && Mesh->GetStaticMesh() ? TOptional<FVector>(Mesh->Bounds.Origin) : TOptional<FVector>(GetActorLocation());
}

bool AMissionArtifact::Use(AActor* Player)
{
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner || !bShownNow)
	{
		return false;
	}
	const FVector Where = GetInteractionLocation().Get(GetActorLocation());
	// A mission turned in here comes first, as at someone's door: the talk's event finishes it.
	if (const UMissionDefinition* Waiting = Runner->FindTurnInAt(this))
	{
		LooterSound::PlayAt(this, LooterSoundCue::Giver::ArtifactRead, Where);
		// A turn-in with a choice of two (Back Pay at the tag board) asks first; the card turns it in.
		if (Waiting->HasTurnInChoice())
		{
			ALooterHUD* ChoiceHUD = ALooterHUD::FindFor(Player);
			return ChoiceHUD && ChoiceHUD->OpenTurnInChoice(Waiting, this);
		}
		Runner->NotifyEvent(FMissionEvent::Talked(this));
		return true;
	}
	const UMissionDefinition* Offer = GetOffer();
	ALooterHUD* HUD = ALooterHUD::FindFor(Player);
	if (!Offer || !HUD || HUD->IsMenuOpen())
	{
		return false;
	}
	// Picked up and read: its words on the card. Taken with its mission, it goes as the card closes.
	LooterSound::PlayAt(this, LooterSoundCue::Giver::ArtifactRead, Where);
	if (!HUD->OpenMissionOffer(Offer, this))
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: picked up, offering %s."), *GetActorNameOrLabel(), *Offer->GetMissionId().ToString());
	return true;
}

#undef LOCTEXT_NAMESPACE

TOptional<FInteractionBox> AMissionArtifact::GetInteractionBox() const
{
	// What it is (a letter, a tag, a note's card over its paper), at least a hand's size round it, from what it lies on up.
	if (!Mesh || !Mesh->GetStaticMesh())
	{
		return TOptional<FInteractionBox>();
	}
	return InteractionFocus::GrownBox(Mesh->Bounds.GetBox(), InteractionFocus::SmallThingHalfWidth, InteractionFocus::SmallThingHalfHeight);
}
