// AVendor: a shop's door the player trades at, and opening its screen.

#include "Economy/Vendor.h"
#include "AI_Looter_Shooter.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "UI/HUD/LooterHUD.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

const FName AVendor::VendorTag(TEXT("Vendor"));

AVendor::AVendor()
{
	// Nothing about it moves or changes by itself: a screen opens.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Title = NSLOCTEXT("LooterVendor", "DefaultTitle", "General Store");
	Prompt = NSLOCTEXT("LooterVendor", "DefaultPrompt", "Trade");
	Tags.Add(VendorTag);
}

void AVendor::BeginPlay()
{
	Super::BeginPlay();
	// The build script sets its tags whole; this one it always carries.
	Tags.AddUnique(VendorTag);
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
