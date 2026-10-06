// ASkiffJetty: the jetty and its props, the story it reads, the gangplank's and the bell's motion, and boarding.

#include "World/SkiffJetty.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Core/LooterMenuGameMode.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionSubsystem.h"
#include "Missions/MissionRunner.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "UI/HUD/LooterHUD.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "SkiffJetty"

namespace
{
	/** While the plank is up, the story is looked at this often (s) for the tutorial's end. */
	constexpr float StoryCheckSeconds = 0.25f;

	/** The point looked at to board: this high over the plank's foot on the deck (cm). */
	constexpr double BoardPointHeight = 50.0;
}

ASkiffJetty::ASkiffJetty()
{
	// Ticks only while something moves (the plank, the bell, the lines) or it waits for the tutorial (RefreshTick).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// The imported models (Art/Models/Props/SkiffJetty.py, Art/Models/Vehicles/Skiff.py); each a setting.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> JettyAsset(TEXT("/Game/Art/Props/SM_SkiffJetty.SM_SkiffJetty"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SkiffAsset(TEXT("/Game/Art/Vehicles/SM_Skiff_A_Packet.SM_Skiff_A_Packet"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GangplankAsset(TEXT("/Game/Art/Vehicles/SM_Skiff_Gangplank.SM_Skiff_Gangplank"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BellPostAsset(TEXT("/Game/Art/Props/SM_JettyBellPost.SM_JettyBellPost"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BellAsset(TEXT("/Game/Art/Props/SM_JettyBell.SM_JettyBell"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SlateAsset(TEXT("/Game/Art/Props/SM_JettySlate.SM_JettySlate"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> LineAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> RopeAsset(TEXT("/Game/Art/Materials/MI_Canvas.MI_Canvas"));
	JettyMesh = JettyAsset.Object;
	SkiffMesh = SkiffAsset.Object;
	GangplankMesh = GangplankAsset.Object;
	BellPostMesh = BellPostAsset.Object;
	BellMesh = BellAsset.Object;
	SlateMesh = SlateAsset.Object;
	MooringLineMesh = LineAsset.Object;
	MooringLineMaterial = RopeAsset.Object;

	// Scenery that stands still; the bell swings, and the skiff and its lines are made in play. The deck is walked on, so
	// the minimap draws it as ground (no tag).
	Jetty = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Jetty"));
	Jetty->SetMobility(EComponentMobility::Static);
	Jetty->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Jetty->SetStaticMesh(JettyMesh);
	RootComponent = Jetty;

	BellPost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BellPost"));
	BellPost->SetMobility(EComponentMobility::Static);
	BellPost->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	BellPost->SetStaticMesh(BellPostMesh);
	BellPost->SetupAttachment(Jetty, BellPostSocket);

	// The bell swings high over everyone's head: no collision, so moving it costs no physics.
	Bell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bell"));
	Bell->SetMobility(EComponentMobility::Movable);
	Bell->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Bell->SetStaticMesh(BellMesh);
	Bell->SetupAttachment(BellPost, BellSocket);

	Slate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Slate"));
	Slate->SetMobility(EComponentMobility::Static);
	Slate->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Slate->SetStaticMesh(SlateMesh);
	Slate->SetupAttachment(Jetty, SlateSocket);

	Landing = CreateDefaultSubobject<USceneComponent>(TEXT("Landing"));
	Landing->SetMobility(EComponentMobility::Static);
	Landing->SetupAttachment(Jetty, LandingSocket);

#if WITH_EDITORONLY_DATA
	// Stand-ins for the editor: the skiff where it moors and its plank lowered, to judge the fit. Play spawns the real one.
	SkiffPreview = CreateEditorOnlyDefaultSubobject<UStaticMeshComponent>(TEXT("SkiffPreview"));
	if (SkiffPreview)
	{
		SkiffPreview->SetMobility(EComponentMobility::Static);
		SkiffPreview->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		SkiffPreview->SetHiddenInGame(true);
		SkiffPreview->SetStaticMesh(SkiffMesh);
		SkiffPreview->SetupAttachment(Jetty);
	}
	GangplankPreview = CreateEditorOnlyDefaultSubobject<UStaticMeshComponent>(TEXT("GangplankPreview"));
	if (GangplankPreview && SkiffPreview)
	{
		GangplankPreview->SetMobility(EComponentMobility::Static);
		GangplankPreview->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		GangplankPreview->SetHiddenInGame(true);
		GangplankPreview->SetStaticMesh(GangplankMesh);
		GangplankPreview->SetupAttachment(SkiffPreview, SkiffGangplankSocket);
	}
#endif

	// The stern line from the inner bollard, the bow line from the outer one, both to the skiff's jetty side.
	FSkiffMooringLine Stern;
	Stern.Bollard = TEXT("Bollard_1");
	Stern.SkiffEnd = TEXT("Mooring_3");
	FSkiffMooringLine Bow;
	Bow.Bollard = TEXT("Bollard_2");
	Bow.SkiffEnd = TEXT("Mooring_1");
	MooringLines = { Stern, Bow };

	// The imported models' fit (SkiffJetty.py): centreline 5.03 m off, keel 0.45 m under the deck's 0.30 m top, the
	// opening level with the landing 10 m out.
	FallbackMooring = FTransform(FVector(1060.0, 503.0, -15.0));
	Prompt = LOCTEXT("Prompt", "Board the skiff");
}

void ASkiffJetty::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyArt();
}

void ASkiffJetty::ApplyArt()
{
	// Each mesh on its part, each prop on the socket it was made for.
	Jetty->SetStaticMesh(JettyMesh);
	BellPost->SetStaticMesh(BellPostMesh);
	Bell->SetStaticMesh(BellMesh);
	Slate->SetStaticMesh(SlateMesh);
	const FAttachmentTransformRules OnSocket = FAttachmentTransformRules::SnapToTargetNotIncludingScale;
	BellPost->AttachToComponent(Jetty, OnSocket, BellPostSocket);
	Bell->AttachToComponent(BellPost, OnSocket, BellSocket);
	Slate->AttachToComponent(Jetty, OnSocket, SlateSocket);
	Landing->AttachToComponent(Jetty, OnSocket, LandingSocket);

	// It carries its landing: the actor is found by the name, the spot on the deck is its Landing.
	Landing->ComponentTags = { LandingName };
	Tags.RemoveAll([](FName Tag) { return Tag.ToString().StartsWith(TEXT("Landing_")); });
	Tags.AddUnique(LandingName);

#if WITH_EDITORONLY_DATA
	if (SkiffPreview && GangplankPreview)
	{
		SkiffPreview->SetStaticMesh(SkiffMesh);
		SkiffPreview->SetRelativeTransform(ComputeMooring());
		GangplankPreview->SetStaticMesh(GangplankMesh);
		GangplankPreview->AttachToComponent(SkiffPreview, OnSocket, SkiffGangplankSocket);
	}
#endif
}

void ASkiffJetty::BeginPlay()
{
	Super::BeginPlay();
	bMenuWorld = ALooterMenuGameMode::IsMenuWorld(GetWorld());
	Tags.AddUnique(LandingName);

#if WITH_EDITORONLY_DATA
	// Play has the real skiff.
	if (GangplankPreview)
	{
		GangplankPreview->DestroyComponent();
		GangplankPreview = nullptr;
	}
	if (SkiffPreview)
	{
		SkiffPreview->DestroyComponent();
		SkiffPreview = nullptr;
	}
#endif

	SpawnSkiff();
	MakeLines();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Register(this);
	}

	// As the story stands, without a sound: down already on a practice visit, or for a session that finished the
	// tutorial before. Behind the main menu nobody plays, and the plank stays up.
	SetGangplankDown(!bMenuWorld && IsGangplankDownFor(IsTutorialDone(), HasCastOff()), /*bInstant*/ true);
	RefreshFromStory();
	RefreshTick();
}

void ASkiffJetty::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopListeningToScene();
	if (UInteractionSubsystem* Registry = UInteractionSubsystem::Get(this))
	{
		Registry->Unregister(this);
	}
	// The skiff is the jetty's: it goes with it (a level ending takes both anyway).
	AStaticMeshActor* Boat = Skiff.Get();
	if (EndPlayReason == EEndPlayReason::Destroyed && Boat && !Boat->IsActorBeingDestroyed())
	{
		Boat->Destroy();
	}
	Skiff = nullptr;
	Gangplank = nullptr;
	Super::EndPlay(EndPlayReason);
}

void ASkiffJetty::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

// ---------------------------------------------------------------------------
// The rules
// ---------------------------------------------------------------------------

bool ASkiffJetty::IsGangplankDownFor(bool bTutorialDone, bool bFirstCastOff)
{
	return bTutorialDone || bFirstCastOff;
}

bool ASkiffJetty::ShowsBoardingFor(bool bTutorialDone, bool bFirstCastOff)
{
	return bTutorialDone && !bFirstCastOff;
}

bool ASkiffJetty::OfferBoarding(UMissionRunner& Runner, FName MissionId, bool bTutorialDone)
{
	if (Runner.IsRunning(MissionId))
	{
		return true;
	}
	if (!Runner.IsActive() || !ShowsBoardingFor(bTutorialDone, Runner.GetCampaign().bFirstCastOff))
	{
		return false;
	}
	if (!Runner.FindDefinition(MissionId))
	{
		UE_LOG(LogLooter, Warning, TEXT("Skiff jetty: no mission %s to offer (Tools/Unreal/create_mission_assets.py makes DA_Mission_BoardSkiff)."),
			*MissionId.ToString());
		return false;
	}
	// Not forced: finished once (the skiff boarded), it isn't offered again; only on Skyreach, its area.
	if (!Runner.StartMission(MissionId))
	{
		return false;
	}
	// What the player does next: the minimap's arrow points at the jetty.
	Runner.TrackMission(MissionId);
	return true;
}

// ---------------------------------------------------------------------------
// The story and the gangplank
// ---------------------------------------------------------------------------

FCampaignRecord* ASkiffJetty::FindCampaign() const
{
	return UAreaTravelSubsystem::FindCampaign(GetWorld());
}

bool ASkiffJetty::IsTutorialDone() const
{
	// The progress's flag (the director sets it, the skip does), or its mission finished: the director finishes the
	// mission a moment before it sets the flag.
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const ULocalPlayer* Player = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;
	const UPlayerProgressionSubsystem* Progression = Player ? Player->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
	if (Progression && Progression->IsTutorialDone())
	{
		return true;
	}
	const FCampaignRecord* Campaign = FindCampaign();
	return Campaign && Campaign->HasCompleted(TutorialMissionId);
}

bool ASkiffJetty::HasCastOff() const
{
	const FCampaignRecord* Campaign = FindCampaign();
	return Campaign && Campaign->bFirstCastOff;
}

void ASkiffJetty::RefreshFromStory()
{
	if (bMenuWorld || bGangplankForced || bCastingOff)
	{
		return;
	}
	const bool bTutorialDone = IsTutorialDone();
	if (!bGangplankDown && IsGangplankDownFor(bTutorialDone, HasCastOff()))
	{
		UE_LOG(LogLooter, Log, TEXT("%s: the skiff is ready, and its gangplank comes down."), *GetActorNameOrLabel());
		SetGangplankDown(true, /*bInstant*/ false);
		RingBell();
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		OfferBoarding(*Runner, BoardingMissionId, bTutorialDone);
	}
}

void ASkiffJetty::SetGangplankDown(bool bDown, bool bInstant)
{
	bGangplankDown = bDown;
	if (bInstant)
	{
		GangplankRaise = bDown ? 0.f : 1.f;
	}
	ApplyGangplank();
	RefreshTick();
}

void ASkiffJetty::ForceGangplank(bool bDown)
{
	bGangplankForced = true;
	const bool bLowering = bDown && !bGangplankDown;
	SetGangplankDown(bDown, /*bInstant*/ false);
	if (bLowering)
	{
		RingBell();
	}
}

void ASkiffJetty::RingBell()
{
	BellTime = 0.f;
	if (BellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BellSound, Bell ? Bell->GetComponentLocation() : GetActorLocation());
	}
	RefreshTick();
}

void ASkiffJetty::Advance(float DeltaSeconds)
{
	// Waiting for the tutorial's end: the story is looked at a few times a second until the plank comes down.
	if (!bGangplankDown && !bGangplankForced && !bMenuWorld && !bCastingOff)
	{
		SinceStoryCheck += DeltaSeconds;
		if (SinceStoryCheck >= StoryCheckSeconds)
		{
			SinceStoryCheck = 0.f;
			RefreshFromStory();
		}
	}

	// The plank swings toward where it's wanted, a whole swing in GangplankSeconds.
	const float Wanted = bGangplankDown ? 0.f : 1.f;
	if (GangplankRaise != Wanted)
	{
		const float Step = DeltaSeconds / FMath::Max(GangplankSeconds, 0.1f);
		GangplankRaise = Wanted > GangplankRaise ? FMath::Min(GangplankRaise + Step, Wanted) : FMath::Max(GangplankRaise - Step, Wanted);
		ApplyGangplank();
	}

	// The bell swings and settles.
	if (BellTime >= 0.f)
	{
		BellTime += DeltaSeconds;
		if (BellTime >= BellRingSeconds)
		{
			BellTime = -1.f;
		}
		ApplyBell();
	}

	UpdateLines(DeltaSeconds);
	RefreshTick();
}

void ASkiffJetty::ApplyGangplank()
{
	if (!Gangplank)
	{
		return;
	}
	// Eased, so it starts and stops softly; it turns about its hinge's line until the free end points up.
	const float Eased = FMath::SmoothStep(0.f, 1.f, GangplankRaise);
	Gangplank->SetRelativeRotation(FQuat::Slerp(FQuat::Identity, GangplankRaiseTurn, Eased));
}

void ASkiffJetty::ApplyBell()
{
	if (!Bell)
	{
		return;
	}
	// A swing that dies away: a sine under a falling envelope, still by BellRingSeconds.
	float Angle = 0.f;
	if (BellTime >= 0.f)
	{
		const float Left = 1.f - FMath::Clamp(BellTime / BellRingSeconds, 0.f, 1.f);
		Angle = BellSwingDegrees * Left * Left * FMath::Sin(2.f * UE_PI * BellTime / FMath::Max(BellSwingSeconds, 0.1f));
	}
	const FVector Axis = BellSwingAxis.GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector);
	Bell->SetRelativeRotation(FQuat(Axis, FMath::DegreesToRadians(Angle)));
}

void ASkiffJetty::RefreshTick()
{
	const bool bPlankMoving = GangplankRaise != (bGangplankDown ? 0.f : 1.f);
	const bool bMoving = bPlankMoving || BellTime >= 0.f || bCastingOff || (bLinesSlipped && SlipTime < LineDropSeconds);
	const bool bWaiting = !bGangplankDown && !bGangplankForced && !bMenuWorld && !bCastingOff;
	SetActorTickInterval(bMoving ? 0.f : StoryCheckSeconds);
	SetActorTickEnabled(bMoving || bWaiting);
}

// ---------------------------------------------------------------------------
// Being used
// ---------------------------------------------------------------------------

FInteractionOptions ASkiffJetty::GetInteractionOptions(const UInteractionComponent& User) const
{
	FInteractionOptions Options;
	// Only with the plank down on the deck, and never while the skiff is leaving.
	Options.bUsable = bGangplankDown && GangplankRaise <= 0.f && !bCastingOff && !bMenuWorld && Skiff != nullptr;
	Options.bTap = false;
	Options.bHold = true;
	Options.HoldSeconds = HoldSeconds;
	Options.HoldPrompt = Prompt;
	Options.Reach = Reach;
	return Options;
}

bool ASkiffJetty::Interact(UInteractionComponent& User, bool bHeld)
{
	if (!bHeld || !GetInteractionOptions(User).bUsable)
	{
		return false;
	}
	ALooterHUD* HUD = ALooterHUD::FindFor(User.GetOwner());
	return HUD && HUD->OpenStationBoard(this, GetBoardWords());
}

TOptional<FVector> ASkiffJetty::GetInteractionLocation() const
{
	// The plank's foot on the deck, a little above it: where the player stands to board.
	if (Jetty && Jetty->DoesSocketExist(GangplankLandSocket))
	{
		return Jetty->GetSocketLocation(GangplankLandSocket) + FVector(0.0, 0.0, BoardPointHeight);
	}
	return TOptional<FVector>();
}

FStationBoardWords ASkiffJetty::GetBoardWords() const
{
	return FStationBoardWords::Jetty();
}

#undef LOCTEXT_NAMESPACE
