// APilotSortie: the Lamplighter's part of Main 6, "The Pilot Light": its life in the level, reading Main 6's steps, and
// the phases' entries (what each one sets up). Her pose in each phase is PilotSortieFlight.cpp's; the tenders, the boarders,
// the launchers, the plank, the catch and the fall PilotSortieFight.cpp's.

#include "World/PilotSortie.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Missions/MissionRunner.h"
#include "Player/ViewKick.h"
#include "World/BargeBoarding.h"
#include "World/LamplighterEffects.h"
#include "World/LamplighterLauncher.h"
#include "World/LamplighterRules.h"
#include "World/LamplighterShip.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const TCHAR* PlankPath = TEXT("/Game/Art/Vehicles/SM_Skiff_Gangplank.SM_Skiff_Gangplank");

	/** Waiting for her to begin (or the missions), it looks again this often (s): nothing else ticks it. */
	constexpr float StoryLookEvery = 0.25f;
}

APilotSortie::APilotSortie()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = StoryLookEvery;
	SetCanBeDamaged(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlankMesh(PlankPath);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// The plank across to the barge: placed in the world's frame (hinged on her rail, its end on the barge's deck). Its mesh
	// is only looked at; the box under it is the walk.
	Plank = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plank"));
	Plank->SetupAttachment(Root);
	Plank->SetStaticMesh(PlankMesh.Object);
	Plank->SetUsingAbsoluteLocation(true);
	Plank->SetUsingAbsoluteRotation(true);
	Plank->SetUsingAbsoluteScale(true);
	Plank->SetMobility(EComponentMobility::Movable);
	Plank->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Plank->SetCanEverAffectNavigation(false);
	Plank->SetVisibility(false);

	PlankFloor = CreateDefaultSubobject<UBoxComponent>(TEXT("PlankFloor"));
	PlankFloor->SetupAttachment(Root);
	PlankFloor->SetUsingAbsoluteLocation(true);
	PlankFloor->SetUsingAbsoluteRotation(true);
	PlankFloor->SetUsingAbsoluteScale(true);
	PlankFloor->SetMobility(EComponentMobility::Movable);
	PlankFloor->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	// World static: the boarders' and the player's ground traces take it for a floor, as her deck.
	PlankFloor->SetCollisionObjectType(ECC_WorldStatic);
	PlankFloor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlankFloor->SetCanEverAffectNavigation(false);
	PlankFloor->SetGenerateOverlapEvents(false);
	PlankFloor->SetHiddenInGame(true);
}

APilotSortie* APilotSortie::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<APilotSortie> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}
	return nullptr;
}

void APilotSortie::BeginPlay()
{
	Super::BeginPlay();
	BuildCourses();
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &APilotSortie::HandleMissionsChanged);
		bBound = true;
	}
	bStoryPending = true;
}

void APilotSortie::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bBound)
	{
		if (UMissionRunner* Runner = UMissionRunner::Get(this))
		{
			Runner->OnChanged.Remove(MissionsChangedHandle);
		}
	}
	MissionsChangedHandle.Reset();
	bBound = false;
	DestroyFight();
	Super::EndPlay(EndPlayReason);
}

void APilotSortie::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// Only waiting for her to begin (and for the missions): her flight's frames come from her own tick (DriveShip).
	if (bStoryPending)
	{
		RefreshStory();
	}
}

ALamplighterShip* APilotSortie::FindShip()
{
	if (Ship)
	{
		return Ship;
	}
	if (!ShipNow.IsValid())
	{
		ShipNow = ALamplighterShip::Find(GetWorld());
	}
	return ShipNow.Get();
}

ABargeBoarding* APilotSortie::FindBarge() const
{
	return Barge ? Barge.Get() : ABargeBoarding::Find(GetWorld());
}

void APilotSortie::HandleMissionsChanged()
{
	RefreshStory();
}

bool APilotSortie::Agrees(EPilotPhase Current, EPilotPhase Wanted)
{
	switch (Wanted)
	{
	case EPilotPhase::Idle:
		// Main 6 ready to turn in (or done): the fall and the flight home carry on to the ledge by themselves.
		return Current == EPilotPhase::Idle || Current == EPilotPhase::Falling || Current == EPilotPhase::Home
			|| Current == EPilotPhase::Settling;
	case EPilotPhase::WaitCastOff:
		return Current == EPilotPhase::WaitCastOff;
	case EPilotPhase::LiftOff:
		return Current == EPilotPhase::LiftOff || Current == EPilotPhase::Cruise || Current == EPilotPhase::Approach
			|| Current == EPilotPhase::Alongside || Current == EPilotPhase::Pulling;
	case EPilotPhase::Boarded:
		return Current == EPilotPhase::Pulling || Current == EPilotPhase::Boarded;
	case EPilotPhase::Jump:
		return Current == EPilotPhase::Jump || Current == EPilotPhase::Falling || Current == EPilotPhase::Home
			|| Current == EPilotPhase::Settling;
	default:
		return Current == Wanted;
	}
}

void APilotSortie::RefreshStory()
{
	if (!bFollowStory)
	{
		bStoryPending = false;
		return;
	}
	ALamplighterShip* Boat = FindShip();
	if (Boat && Boat->IsGone())
	{
		bStoryPending = false;
		return;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Boat || !Boat->HasBegun() || !Runner)
	{
		// Too early: she hasn't begun yet (her first second), or the missions aren't up. Looked at again in a moment.
		bStoryPending = true;
		return;
	}
	bStoryPending = false;
	const FName Main = PilotLight::MissionId();
	const bool bRunning = Runner->IsRunning(Main);
	const int32 Step = bRunning ? Runner->GetStep(Main) : INDEX_NONE;
	const bool bReady = Runner->IsReadyToTurnIn(Main);
	const EPilotPhase Wanted = PilotLight::PhaseForStory(bRunning, Step, bReady);
	if (Agrees(Phase, Wanted))
	{
		return;
	}
	// The one beat the story starts in the middle of a live run: the cage burst (step 7) while she's in the berth.
	const bool bLive = Phase == EPilotPhase::Boarded && Wanted == EPilotPhase::Jump;
	if (Wanted == EPilotPhase::LiftOff && Step >= PilotLight::GrappleStep)
	{
		// The tenders were fought before this load: she flies straight to the barge.
		bTendersCalled = true;
		bTendersDown = true;
	}
	EnterPhase(Wanted, !bLive);
}

void APilotSortie::SetPhase(EPilotPhase NewPhase, bool bFresh)
{
	EnterPhase(NewPhase, bFresh);
}

bool APilotSortie::CastOff()
{
	if (Phase != EPilotPhase::WaitCastOff && !(Phase == EPilotPhase::Idle && !bFollowStory))
	{
		return false;
	}
	// A run of her own from the start: the tenders still to come.
	bTendersCalled = false;
	bTendersDown = false;
	EnterPhase(EPilotPhase::LiftOff, /*bFresh*/ false);
	return Phase == EPilotPhase::LiftOff;
}

void APilotSortie::EnterPhase(EPilotPhase NewPhase, bool bFresh)
{
	ALamplighterShip* Boat = FindShip();
	if (!Boat)
	{
		UE_LOG(LogLooter, Warning, TEXT("The Pilot Light: no Lamplighter in the level: nothing flies."));
		return;
	}
	const EPilotPhase Old = Phase;
	if (NewPhase != EPilotPhase::Idle && NewPhase != EPilotPhase::WaitCastOff && Boat->GetFlight() != EShipFlight::Sortie
		&& !Boat->BeginSortie(*this))
	{
		// Not begun yet (her first second): the story asks again shortly.
		bStoryPending = true;
		return;
	}
	Phase = NewPhase;
	PhaseTime = 0.f;
	UE_LOG(LogLooter, Log, TEXT("The Pilot Light: %s -> %s%s."), PilotLight::PhaseName(Old), PilotLight::PhaseName(NewPhase),
		bFresh ? TEXT(" (as the story finds her)") : TEXT(""));
	if (NewPhase != EPilotPhase::Idle)
	{
		Boat->SetSortieKeepsAboard(PilotLight::KeepsPlayerAboard(NewPhase));
		if (Boat->GetFlight() == EShipFlight::Sortie)
		{
			Boat->SetSortieSavesHeld(PilotLight::IsFlying(NewPhase));
		}
	}
	switch (NewPhase)
	{
	case EPilotPhase::Idle:
		Stand();
		break;
	case EPilotPhase::WaitCastOff:
		if (Boat->GetFlight() == EShipFlight::Sortie)
		{
			Boat->EndSortie();
		}
		// Junie's launchers on the rail from the start of the mission: the player sees what the grapples are for.
		SpawnLaunchers();
		ShowPlank(false, true);
		break;
	case EPilotPhase::LiftOff:
	{
		SpawnLaunchers();
		SpawnDeckMarker();
		GlideFrom = Boat->GetActorTransform();
		GlideFrom.SetScale3D(FVector::OneVector);
		Distance = 0.f;
		Speed = 0.f;
		bPulled = false;
		// Sully over the gangway as the anchor comes up (a short black, so nobody's left on the ledge).
		if (!Boat->IsPlayerAboard())
		{
			const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
			if (Controller && Controller->PlayerCameraManager)
			{
				Controller->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.8f, FLinearColor::Black, false, false);
			}
			Boat->PutPlayerOnDeck();
		}
		// The bag takes her weight off the ledge: a lurch, the timbers groan, the crew grab hold.
		LooterSound::PlayAt(this, LooterSoundCue::ShipLurch, Boat->GetActorLocation(), 0.8f, 0.9f);
		LamplighterEffects::Shake(this, 0.35f, 0.9f);
		FViewKick Jolt;
		Jolt.Pitch = 1.8f;
		Jolt.Frequency = 4.f;
		Jolt.Damping = 0.35f;
		LamplighterEffects::Kick(this, Jolt);
		Boat->BraceCrew(2.5f);
		LandfallRules::Say(this, PilotLight::CastOffLines(), /*bInterrupt*/ true);
		break;
	}
	case EPilotPhase::Cruise:
		Distance = 0.f;
		break;
	case EPilotPhase::Approach:
		GlideFrom = LastCalm;
		break;
	case EPilotPhase::Alongside:
		SpawnLaunchers();
		LandfallRules::Say(this, PilotLight::LaunchersLines());
		break;
	case EPilotPhase::Pulling:
		GlideFrom = PilotLight::StandoffFor(GetBerth());
		LandfallRules::Say(this, PilotLight::FlickerLines());
		break;
	case EPilotPhase::Boarded:
	{
		// (Found so by a load or a console jump: no escort about. A live run's fallen wrecks smoke on in the fields.)
		if (bFresh)
		{
			DestroyTenders();
		}
		SpawnLaunchers();
		SpawnDeckMarker();
		const FTransform Berth = GetBerth();
		Boat->SetSortiePose(Berth, Berth);
		LastPose = LastCalm = Berth;
		for (int32 Index = 0; Index < Launchers.Num(); ++Index)
		{
			if (Launchers[Index] && !Launchers[Index]->IsBiting())
			{
				Launchers[Index]->BiteNow(GetGrappleAnchor(Index));
			}
		}
		Boat->OpenStarboardRail(true);
		ShowPlank(true, bFresh);
		if (!bFresh)
		{
			// Snubbed against the barge's side: a jolt through her, the chains groaning, the crew thrown against the rail.
			LooterSound::PlayAt(this, LooterSoundCue::ShipLurch, Boat->GetActorLocation());
			LooterSound::PlayAt(this, LooterSoundCue::Gunfeel::ImpactHeavy, Boat->DeckPoint(FVector(0.0, ALamplighterShip::RailOut, 0.0)), 0.9f, 0.7f);
			LamplighterEffects::Shake(this, 0.6f, 0.9f);
			FViewKick Jolt;
			Jolt.Pitch = -2.f;
			Jolt.Roll = 2.f;
			Jolt.Frequency = 6.f;
			Jolt.Damping = 0.4f;
			LamplighterEffects::Kick(this, Jolt);
			Boat->BraceCrew(2.f);
		}
		break;
	}
	case EPilotPhase::Jump:
		SpawnDeckMarker();
		CutLaunchers();
		ShowPlank(false, bFresh);
		Boat->OpenStarboardRail(false);
		GlideFrom = Boat->GetActorTransform();
		GlideFrom.SetScale3D(FVector::OneVector);
		bAtJump = false;
		LandedFor = 0.f;
		bFallAsked = false;
		if (bFresh)
		{
			// A load in the escape: under the gangway already, the chains long gone.
			const FTransform Under = GetJumpBerth();
			Boat->SetSortiePose(Under, Under);
			LastPose = LastCalm = Under;
			bAtJump = true;
		}
		break;
	case EPilotPhase::Falling:
		break;
	case EPilotPhase::Home:
		BuildHomeCourse();
		Distance = 0.f;
		Speed = 0.f;
		break;
	case EPilotPhase::Settling:
		GlideFrom = LastCalm;
		bSettleTouched = false;
		break;
	}
}

void APilotSortie::Stand()
{
	DestroyFight();
	ShowPlank(false, true);
	Phase = EPilotPhase::Idle;
	PhaseTime = 0.f;
	bAtJump = false;
	bFallAsked = false;
	LandedFor = 0.f;
	if (ALamplighterShip* Boat = FindShip(); Boat && Boat->GetFlight() == EShipFlight::Sortie)
	{
		Boat->EndSortie();
	}
}

void APilotSortie::DriveShip(float DeltaSeconds)
{
	ALamplighterShip* Boat = FindShip();
	if (!Boat)
	{
		return;
	}
	Clock += DeltaSeconds;
	PhaseTime += DeltaSeconds;
	FTransform Pose;
	FTransform Calm;
	AdvancePose(DeltaSeconds, Pose, Calm);
	if (Phase != EPilotPhase::Idle && Phase != EPilotPhase::WaitCastOff && Boat->GetFlight() == EShipFlight::Sortie)
	{
		Boat->SetSortiePose(Pose, Calm);
		LastPose = Pose;
		LastCalm = Calm;
	}
	// Her pieces after her, in the same frame: the escorts keep station on her calm pose, the hooks and the plank follow.
	AdvanceTenders(DeltaSeconds);
	AdvanceLaunchers(DeltaSeconds);
	AdvancePlank(DeltaSeconds);
	if (Phase == EPilotPhase::Jump)
	{
		AdvanceJump(DeltaSeconds);
	}
	else if (Phase == EPilotPhase::Falling)
	{
		AdvanceFalling();
	}
	CheckTendersDown();
	AdvanceBeats();
}
