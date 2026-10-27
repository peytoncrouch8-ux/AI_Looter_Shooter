// AReadableProp: something to read or look at closely: its words on the reading card, what's said about it after, and the
// mission event that counts it.

#include "World/ReadableProp.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionFocus.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionGates.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Story/CaptionSubsystem.h"
#include "Story/HeroLines.h"
#include "UI/HUD/LooterHUD.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

#define LOCTEXT_NAMESPACE "ReadableProp"

const FName AReadableProp::ReadableTag(TEXT("Readable"));

AReadableProp::AReadableProp()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// A page, a slate, a book on a desk: the Interact key's line and its sight check find it, nothing else does (the plank
	// on the nave's wall is the chapel's; the player never bumps a journal).
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldDynamic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);

	Tags.Add(ReadableTag);
}

void AReadableProp::BeginPlay()
{
	Super::BeginPlay();
	// The build script sets its tags whole; these it always carries (an objective's arrow finds it by the second).
	Tags.AddUnique(ReadableTag);
	if (!ReadableId.IsNone())
	{
		Tags.AddUnique(TagFor(ReadableId));
	}
	bShownNow = true;
	// Painted words are the played hero's too: their own carving, else the one every hero but Ellis sees.
	if (UStaticMesh* HeroMesh = PickMeshFor(HeroLines::GetPlayedHeroId()); HeroMesh && Mesh)
	{
		Mesh->SetStaticMesh(HeroMesh);
	}
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
	// What's there to read follows the story: it looks again whenever the missions change.
	if (!ShownWhen.IsEmpty())
	{
		if (UMissionRunner* Runner = UMissionRunner::Get(this))
		{
			BoundRunner = Runner;
			ChangedHandle = Runner->OnChanged.AddUObject(this, &AReadableProp::HandleRunnerChanged);
		}
	}
	Refresh();
}

void AReadableProp::EndPlay(const EEndPlayReason::Type EndPlayReason)
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
// Whether it's there
// ---------------------------------------------------------------------------

bool AReadableProp::ShouldShow() const
{
	if (ShownWhen.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && ShownWhen.IsMet(Runner->GetCampaign(), Runner);
}

bool AReadableProp::CanRead() const
{
	// Asked as the key looks (every focus pass), so a step's change opens or closes it at once without a hook.
	return MissionGates::AnyMet(UsableWhen, this);
}

void AReadableProp::Refresh()
{
	SetShown(ShouldShow());
}

void AReadableProp::HandleRunnerChanged()
{
	Refresh();
}

void AReadableProp::SetShown(bool bShown)
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
}

// ---------------------------------------------------------------------------
// Reading it
// ---------------------------------------------------------------------------

FName AReadableProp::EventFor(FName ReadableId)
{
	return FName(*(TEXT("Readable.") + ReadableId.ToString()));
}

FName AReadableProp::TagFor(FName ReadableId)
{
	return FName(*(TEXT("Readable_") + ReadableId.ToString()));
}

FName AReadableProp::GetReadEvent() const
{
	return EventFor(ReadableId.IsNone() ? GetFName() : ReadableId);
}

TArray<FStoryLine> AReadableProp::GetLinesToSay() const
{
	TArray<FStoryLine> Said = Lines;
	for (FStoryLine& Line : Said)
	{
		if (Line.Speaker.IsEmpty())
		{
			Line.Speaker = Title;
		}
	}
	return Said;
}

UStaticMesh* AReadableProp::PickMeshFor(FName HeroId) const
{
	if (HeroLines::IsDefaultHeroId(HeroId))
	{
		return nullptr;
	}
	if (const TObjectPtr<UStaticMesh>* Own = HeroMeshes.Find(HeroId); Own && *Own)
	{
		return Own->Get();
	}
	return OtherMesh.Get();
}

FInteractionOptions AReadableProp::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	// Before its step (or after a step-only one: the eleven's tags once taken) it's never focused, so no prompt lingers.
	Options.bUsable = bShownNow && CanRead();
	const FText Words = !Prompt.IsEmpty() ? Prompt : Text.IsEmpty() ? LOCTEXT("Look", "Look") : LOCTEXT("Read", "Read");
	// A step's reading is a short hold; lore a tap (MissionGates).
	MissionGates::SetUse(Options, Words, bHoldToRead);
	Options.Reach = Reach;
	return Options;
}

bool AReadableProp::Interact(UInteractionComponent& User, bool /*bHeld*/)
{
	// The interaction component has already matched the press to what GetInteractionOptions offers (a tap or a hold).
	return Read(User.GetOwner());
}

TOptional<FVector> AReadableProp::GetInteractionLocation() const
{
	return Mesh && Mesh->GetStaticMesh() ? TOptional<FVector>(Mesh->Bounds.Origin) : TOptional<FVector>(GetActorLocation());
}

bool AReadableProp::Read(AActor* Player)
{
	if (!bShownNow || !CanRead())
	{
		return false;
	}
	ALooterHUD* HUD = ALooterHUD::FindFor(Player);
	if (HUD && HUD->IsMenuOpen())
	{
		return false;
	}
	// Its words on the reading card (the offer card with no mission: ALooterHUD::OpenMissionOffer); without a HUD (a test, a
	// script) it's read all the same.
	if (!Text.IsEmpty() && HUD && !HUD->OpenMissionOffer(nullptr, this))
	{
		return false;
	}
	LooterSound::PlayAt(this, LooterSoundCue::MissionFeatures::Read, GetInteractionLocation().Get(GetActorLocation()));
	// What's said about it: after the card is put away (the captions wait under a menu), or at once when there's no card.
	const TArray<FStoryLine> Said = GetLinesToSay();
	if (!Said.IsEmpty())
	{
		if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
		{
			Captions->Play(Said, ECaptionPlay::Interrupt);
		}
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->NotifyEvent(FMissionEvent::Named(GetReadEvent(), this));
		// Kept by the story, so what reading it opens (a Ledger page) stays open across saves; SetFlag ignores a flag it holds.
		if (!SetsFlag.IsNone())
		{
			Runner->SetFlag(SetsFlag);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("%s: %s read (%s)."), *GetActorNameOrLabel(), *Title.ToString(), *GetReadEvent().ToString());
	return true;
}

#undef LOCTEXT_NAMESPACE

TOptional<FInteractionBox> AReadableProp::GetInteractionBox() const
{
	// What it is, at least a hand's size round it, from what it lies on up (a page, a letter, a badge on a desk).
	if (!Mesh || !Mesh->GetStaticMesh())
	{
		// No model of its own (the barricade's tags on their nail, words on a wall): a hand's room round its spot.
		return InteractionFocus::BoxAround(GetActorLocation(), FVector(InteractionFocus::SmallThingHalfWidth * 1.5), GetActorQuat());
	}
	return InteractionFocus::GrownBox(Mesh->Bounds.GetBox(), InteractionFocus::SmallThingHalfWidth, InteractionFocus::SmallThingHalfHeight);
}
