// AVendor: a shop's door the player trades at, and opening its screen.

#include "Economy/Vendor.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Story/SpeakerPointComponent.h"
#include "UI/HUD/LooterHUD.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

const FName AVendor::VendorTag(TEXT("Vendor"));

namespace
{
	/** The story has business at this vendor's voice now (a mission to turn in or offer): it comes ahead of the shop. */
	bool VoiceHasBusiness(const USpeakerPointComponent* Voice)
	{
		return Voice && Voice->HasMissionBusiness();
	}
}

AVendor::AVendor()
{
	// Nothing about it moves or changes by itself: a screen opens.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	// The story's voice at the shop: quiet unless a mission is offered or waits to be turned in here, so a shop with no
	// missions behaves exactly as before.
	Voice = CreateDefaultSubobject<USpeakerPointComponent>(TEXT("Voice"));
	Voice->SetupAttachment(Root);
	Voice->bMissionsOnly = true;

	Title = NSLOCTEXT("LooterVendor", "DefaultTitle", "General Store");
	Prompt = NSLOCTEXT("LooterVendor", "DefaultPrompt", "Trade");
	Tags.Add(VendorTag);
}

void AVendor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Before any actor's play begins, so a mission looking for the voice's tag finds it whatever starts first.
	ApplyVoice();
}

void AVendor::ApplyVoice()
{
	if (Voice && !VoiceName.IsEmpty())
	{
		Voice->SpeakerName = VoiceName;
	}
	if (!VoiceTag.IsNone())
	{
		Tags.AddUnique(VoiceTag);
	}
}

void AVendor::BeginPlay()
{
	Super::BeginPlay();
	// The build script sets its tags whole; this one it always carries (and the voice's, as set by then).
	Tags.AddUnique(VendorTag);
	ApplyVoice();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void AVendor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool AVendor::IsOpen() const
{
	if (OpenWhen.IsEmpty())
	{
		return true;
	}
	// The session's record (a test's in tests), as the encounters read theirs. Without a mission runner there's no story
	// here to follow, so a vendor waiting on the story stays shut.
	const UWorld* World = GetWorld();
	const UMissionRunner* Runner = World ? World->GetSubsystem<UMissionRunner>() : nullptr;
	return Runner && OpenWhen.IsMet(Runner->GetCampaign(), Runner);
}

FInteractionOptions AVendor::GetInteractionOptions(const UInteractionComponent& User) const
{
	// A mission to turn in or take here is the story's, said by the voice (its "Turn in" or "Talk"), even while the shop is
	// shut: the hatch opens for a bounty before it opens for trade.
	if (VoiceHasBusiness(Voice))
	{
		return Voice->GetInteractionOptions(User);
	}
	// Shut by the story, it isn't there to use at all (the Pruitts mutter through the door meanwhile).
	if (!IsOpen())
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	Options.bTap = true;
	Options.bHold = false;
	Options.TapPrompt = Prompt;
	Options.Reach = Reach;
	return Options;
}

bool AVendor::Interact(UInteractionComponent& User, bool bHeld)
{
	// The same choice as the options: what the key said is what it does.
	if (VoiceHasBusiness(Voice))
	{
		return Voice->Interact(User, bHeld);
	}
	return !bHeld && Use(User.GetOwner());
}

TOptional<FVector> AVendor::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorLocation());
}

bool AVendor::Use(AActor* Player)
{
	if (!IsOpen())
	{
		return false;
	}
	ALooterHUD* HUD = ALooterHUD::FindFor(Player);
	if (!HUD || !HUD->OpenVendor(this))
	{
		return false;
	}
	UE_LOG(LogLooter, Log, TEXT("%s: trading%s."), *GetActorNameOrLabel(), Player ? *FString::Printf(TEXT(" with %s"), *Player->GetName()) : TEXT(""));
	return true;
}
