// ALostPropertyWindow: the depot's LOST PROPERTY window: its prompt (with the count on the shelves), opening the screen, the
// shutter's sounds and the placeholder sign.

#include "World/LostPropertyWindow.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Core/LooterMenuGameMode.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Loot/LostLootSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "UI/World/LostLootWidget.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "LooterLostProperty"

const FName ALostPropertyWindow::WindowTag(TEXT("LostProperty"));

namespace
{
	/** The placeholder sign: over the window, its letters' height (cm), in the depot's painted signs' cream. */
	const FVector SignOffset(4.0, 0.0, 62.0);
	constexpr float SignLetterHeight = 13.f;
	const FColor SignPaint(232, 218, 182);
}

ALostPropertyWindow::ALostPropertyWindow()
{
	// Nothing about it moves or changes by itself: a screen opens.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	PlaceholderSign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PlaceholderSign"));
	PlaceholderSign->SetupAttachment(Root);
	PlaceholderSign->SetRelativeLocation(SignOffset);
	PlaceholderSign->SetHorizontalAlignment(EHTA_Center);
	PlaceholderSign->SetVerticalAlignment(EVRTA_TextCenter);
	PlaceholderSign->SetWorldSize(SignLetterHeight);
	PlaceholderSign->SetTextRenderColor(SignPaint);
	PlaceholderSign->SetText(LOCTEXT("Sign", "LOST PROPERTY"));
	PlaceholderSign->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	PlaceholderSign->SetCastShadow(false);
	PlaceholderSign->SetVisibility(false);

	Title = LOCTEXT("Title", "Lost Property");
	Notice = LOCTEXT("Notice", "Articles left on the line or about the town are brought here and held against a claim ticket. "
		"When the shelves are full the oldest unclaimed article is disposed of to make room. Claims at the window. "
		"By order of the Station Agent.");
	Prompt = LOCTEXT("Prompt", "Claim lost property");
	Tags.Add(WindowTag);
}

void ALostPropertyWindow::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplySign();
}

void ALostPropertyWindow::ApplySign()
{
	if (PlaceholderSign)
	{
		PlaceholderSign->SetVisibility(bShowPlaceholderSign);
	}
}

void ALostPropertyWindow::BeginPlay()
{
	Super::BeginPlay();
	// The build script sets its tags whole; this one it always carries.
	Tags.AddUnique(WindowTag);
	ApplySign();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}
}

void ALostPropertyWindow::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	Super::EndPlay(EndPlayReason);
}

bool ALostPropertyWindow::IsOpen() const
{
	const UWorld* World = GetWorld();
	// Behind the main menu nobody claims anything.
	if (!World || ALooterMenuGameMode::IsMenuWorld(World))
	{
		return false;
	}
	if (OpenWhen.IsEmpty())
	{
		return true;
	}
	// The session's record, as a vendor reads its own; without a mission runner there's no story to wait on, so it stays shut.
	const UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	return Runner && OpenWhen.IsMet(Runner->GetCampaign(), Runner);
}

bool ALostPropertyWindow::IsShutIn(const UWorld* World)
{
	if (!World)
	{
		return false;
	}
	// Any window open is enough (one a map today); a map with none has nowhere to be shut.
	bool bAnyWindow = false;
	for (TActorIterator<ALostPropertyWindow> It(World); It; ++It)
	{
		if (It->IsOpen())
		{
			return false;
		}
		bAnyWindow = true;
	}
	return bAnyWindow;
}

FInteractionOptions ALostPropertyWindow::GetInteractionOptions(const UInteractionComponent& User) const
{
	if (!IsOpen())
	{
		return FInteractionOptions::None();
	}
	FInteractionOptions Options;
	Options.bTap = true;
	Options.bHold = false;
	// The count on the shelves beside the prompt, so a glance says whether anything waits there.
	const ULostLootSubsystem* LostLoot = ULostLootSubsystem::Get(this);
	const int32 Waiting = LostLoot ? LostLoot->GetStore().Num() : 0;
	Options.TapPrompt = Waiting > 0 ? FText::Format(LOCTEXT("PromptCount", "{0} ({1})"), Prompt, FText::AsNumber(Waiting)) : Prompt;
	Options.Reach = Reach;
	return Options;
}

bool ALostPropertyWindow::Interact(UInteractionComponent& User, bool bHeld)
{
	return !bHeld && Use(User.GetOwner());
}

TOptional<FVector> ALostPropertyWindow::GetInteractionLocation() const
{
	return TOptional<FVector>(GetActorTransform().TransformPosition(InteractOffset));
}

bool ALostPropertyWindow::Use(AActor* Player)
{
	if (!IsOpen())
	{
		return false;
	}
	APlayerController* Controller = Cast<APlayerController>(Player);
	if (!Controller)
	{
		const APawn* Pawn = Cast<APawn>(Player);
		Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	}
	if (!ULostLootWidget::OpenFor(Controller, this))
	{
		return false;
	}
	PlayShutter(true);
	UE_LOG(LogLooter, Log, TEXT("%s: Lost Property open%s."), *GetActorNameOrLabel(), Player ? *FString::Printf(TEXT(" for %s"), *Player->GetName()) : TEXT(""));
	return true;
}

void ALostPropertyWindow::PlayShutter(bool bOpening) const
{
	LooterSound::PlayAt(this, bOpening ? LooterSoundCue::LostLoot::WindowOpen : LooterSoundCue::LostLoot::WindowClose,
		GetActorLocation());
}

#undef LOCTEXT_NAMESPACE
