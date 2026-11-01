// ALamplighterShip: her life in the level, the flight's clock, sailing in along her course with the player on deck, and
// the jumps between her states. Her pieces are LamplighterShipParts.cpp's, the fight LamplighterShipBoarding.cpp's, the
// landing and the ledge LamplighterShipLanding.cpp's, the campaign and the lines LamplighterShipStory.cpp's.

#include "World/LamplighterShip.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Combat/HealthComponent.h"
#include "Creatures/CreatureBase.h"
#include "Missions/MissionRunner.h"
#include "Scenes/HardLanding.h"
#include "Scenes/SceneSubsystem.h"
#include "Story/WardenCrewMember.h"
#include "World/CompanyTender.h"
#include "World/LandfallGrapple.h"
#include "World/LandfallMarker.h"
#include "World/LanternFlame.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The bob comes in over this long as the flight begins, so she starts exactly where she was put. */
	constexpr float SwayInSeconds = 2.f;
	/** The grapples' jolt: her rock against the lines dies away over this long, then she's dead still. */
	constexpr float LurchSeconds = 1.4f;
	constexpr float LurchRoll = 4.f;
	constexpr float LurchPitch = 2.5f;
	/** If nothing else begins her, she begins herself this long after play does (the arrival's BeginLandfall is first). */
	constexpr float SelfBeginSeconds = 1.f;
	/** Off her deck this long while she flies, and the player is put back aboard. */
	constexpr float OffDeckGrace = 0.6f;
	/** The Great Lantern turns its flame toward the farm again this often while she moves. */
	constexpr float LanternEvery = 0.5f;
	/** Her deck's reach in her frame, rails and prow and all, and how high above it a jump still counts as aboard (cm). */
	constexpr float AboardAft = -1200.f;
	constexpr float AboardFore = 1300.f;
	constexpr float AboardAbove = 650.f;
	constexpr float AboardBelow = 150.f;
}

ALamplighterShip::ALamplighterShip()
{
	PrimaryActorTick.bCanEverTick = true;
	// Before the characters on her deck move: they ride her (a character based on her ticks after her, as it's based).
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetCanBeDamaged(false);
	LanternTarget = LandfallRules::LanternTargetTag();
	// The story's first arrival: the session finds her by this tag and stands the player on her LandingSpot.
	Tags.Add(LandfallRules::LandingTag());

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	BuildRails();
	ApplyLook();
}

ALamplighterShip* ALamplighterShip::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ALamplighterShip> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}
	return nullptr;
}

void ALamplighterShip::BeginPlay()
{
	Super::BeginPlay();
	SetupFromPlacement();
	SpawnPieces();
	BindMissions();
	BeganWait = 0.f;
}

void ALamplighterShip::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HoldSaves(false);
	LooterSound::Stop(EngineSound.Get(), 0.2f);
	LooterSound::Stop(CreakSound.Get(), 0.2f);
	EngineSound.Reset();
	CreakSound.Reset();
	ResetFight();
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	MissionsChangedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void ALamplighterShip::SetupFromPlacement()
{
	// Moored left unset: she lies where she was placed (WORLD can place her on the ledge and leave it).
	if (Moored.Equals(FTransform::Identity))
	{
		Moored = FTransform(GetActorQuat(), GetActorLocation());
	}
	Moored.SetScale3D(FVector::OneVector);
	if (!Path.Build(Course))
	{
		if (!Course.IsEmpty())
		{
			UE_LOG(LogLooter, Warning, TEXT("%s: her course needs two points or more (it has %d): she can only lie moored."), *GetActorNameOrLabel(), Course.Num());
		}
	}
	GrappleStop = FMath::Clamp(GrappleStop, 0, FMath::Max(Course.Num() - 1, 0));
	// Before the level begins her: where she'll begin, so the arrival stands the player on the right deck. Moored unless the
	// story is about to fly (BeginLandfall decides for good).
	SetActorTransform(Moored);
	CalmTransform = Moored;
}

float ALamplighterShip::GetGrappleStopDistance() const
{
	return Path.IsValid() ? Path.DistanceAtPoint(GrappleStop) : 0.f;
}

void ALamplighterShip::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Advance(DeltaSeconds);
}

void ALamplighterShip::Advance(float DeltaSeconds)
{
	if (!bBegun)
	{
		// Nothing began her (a level without the travel subsystem's arrival): she begins herself.
		BeganWait += DeltaSeconds;
		if (BeganWait >= SelfBeginSeconds)
		{
			BeginLandfall();
		}
		return;
	}
	switch (Flight)
	{
	case EShipFlight::Sailing:
		AdvanceSailing(DeltaSeconds);
		break;
	case EShipFlight::Grappled:
		AdvanceGrappled(DeltaSeconds);
		break;
	case EShipFlight::Breaking:
		AdvanceBreaking(DeltaSeconds);
		break;
	case EShipFlight::Landing:
		// The scene moves her (HardLanding): nothing here but her pieces.
		break;
	case EShipFlight::Moored:
		AdvanceMoored(DeltaSeconds);
		break;
	}
	AdvancePieces(DeltaSeconds);
	if (Flight != EShipFlight::Moored)
	{
		KeepPlayerAboard(DeltaSeconds);
	}
	LanternWait -= DeltaSeconds;
	if (LanternWait <= 0.f && Flight != EShipFlight::Moored)
	{
		LanternWait = LanternEvery;
		LeanLantern();
	}
}

void ALamplighterShip::PlaceOnCourse(float AtDistance, float DeltaSeconds)
{
	if (!Path.IsValid())
	{
		return;
	}
	const float Sway = FMath::SmoothStep(0.f, SwayInSeconds, Clock);
	SetActorTransform(Path.PoseAt(AtDistance, Speed, Clock, Sway, GetActorScale3D()), /*bSweep*/ false, nullptr, ETeleportType::None);
	CalmTransform = Path.CalmPoseAt(AtDistance);
}

void ALamplighterShip::AdvanceSailing(float DeltaSeconds)
{
	Clock += DeltaSeconds;
	const float Stop = GetGrappleStopDistance();
	Speed = FLamplighterCourse::SpeedToward(Speed, Path.TargetSpeed(Distance, Stop), DeltaSeconds);
	// Never past where she holds: the last of the creep is eased to nothing there.
	Distance = FMath::Min(Distance + Speed * DeltaSeconds, Path.HoldDistance(Stop));
	PlaceOnCourse(Distance, DeltaSeconds);

	// The crew talk as she sails in, a moment after the title rises.
	if (!bDeckLinesSaid)
	{
		DeckLinesWait -= DeltaSeconds;
		if (DeckLinesWait <= 0.f)
		{
			bDeckLinesSaid = true;
			LandfallRules::Say(this, LandfallRules::DeckLines());
		}
	}
	// The player at the prow (Main 1 past its first step; with no mission, standing by the Lantern): the tender comes.
	if (!bTenderCalled)
	{
		const int32 Step = GetMainStep();
		bool bAtProw = Step > LandfallRules::StepProw;
		const APawn* Player = GetWorld() && GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
		if (!bAtProw && Step == INDEX_NONE && Player && ProwMarker)
		{
			bAtProw = FVector::Dist2D(Player->GetActorLocation(), ProwMarker->GetActorLocation()) <= 350.0;
		}
		if (bAtProw)
		{
			CallTender();
		}
	}
	AdvanceTender(DeltaSeconds);
}

void ALamplighterShip::AdvanceGrappled(float DeltaSeconds)
{
	GrappledClock += DeltaSeconds;
	if (GrappledClock < LurchSeconds + DeltaSeconds)
	{
		// Snubbed by the lines: from where she was to her still spot, rocking toward the tender and dying away.
		const float T = FMath::Min(GrappledClock, LurchSeconds);
		const float Settle = FMath::SmoothStep(0.f, LurchSeconds * 0.6f, T);
		const float Die = FMath::Exp(-3.2f * T) * (1.f - T / LurchSeconds);
		const float Roll = TenderSide * LurchRoll * Die * FMath::Sin(UE_TWO_PI * 1.6f * T);
		const float Pitch = LurchPitch * Die * FMath::Sin(UE_TWO_PI * 1.2f * T + 0.5f);
		FTransform Pose;
		Pose.Blend(LurchFrom, GrappleSpot, Settle);
		Pose.SetRotation(Pose.GetRotation() * FQuat(FVector::XAxisVector, FMath::DegreesToRadians(-Roll))
			* FQuat(FVector::YAxisVector, FMath::DegreesToRadians(-Pitch)));
		if (T >= LurchSeconds)
		{
			// Dead still from here: nothing moves her again until the lines part.
			Pose = GrappleSpot;
		}
		SetActorTransform(Pose, /*bSweep*/ false, nullptr, ETeleportType::None);
	}
	AdvanceBoarding(DeltaSeconds);
	AdvanceTender(DeltaSeconds);
}

bool ALamplighterShip::IsPlayerAboard() const
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	if (!Player)
	{
		return false;
	}
	const FVector Local = GetActorTransform().InverseTransformPositionNoScale(Player->GetActorLocation());
	return Local.X >= AboardAft && Local.X <= AboardFore && FMath::Abs(Local.Y) <= RailOut + 30.f
		&& Local.Z >= DeckTop - AboardBelow && Local.Z <= DeckTop + AboardAbove;
}

void ALamplighterShip::KeepPlayerAboard(float DeltaSeconds)
{
	const UWorld* World = GetWorld();
	APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Player = Controller ? Controller->GetPawn() : nullptr;
	const UHealthComponent* Health = Player ? Player->FindComponentByClass<UHealthComponent>() : nullptr;
	const USceneSubsystem* Scenes = USceneSubsystem::Get(this);
	if (!Player || (Health && Health->IsDead()) || (Scenes && Scenes->IsHoldingPlayer()) || IsPlayerAboard())
	{
		OffDeckFor = 0.f;
		return;
	}
	OffDeckFor += DeltaSeconds;
	if (OffDeckFor < OffDeckGrace)
	{
		return;
	}
	// Off her while she flies (woken at a roost on the ground after a death, or over the side): back on her deck, out of a
	// short black, with nothing else changed.
	OffDeckFor = 0.f;
	if (PutPlayerOnDeck())
	{
		UE_LOG(LogLooter, Log, TEXT("Landfall: the player was off the Lamplighter while she flies; back on her deck."));
		if (Controller->PlayerCameraManager)
		{
			Controller->PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.6f, FLinearColor::Black, false, false);
		}
	}
}

void ALamplighterShip::EnterFlight(EShipFlight State)
{
	Flight = State;
	switch (State)
	{
	case EShipFlight::Sailing:
		PlaceOnCourse(Distance, 0.f);
		break;
	case EShipFlight::Moored:
		SetActorTransform(Moored, /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
		CalmTransform = Moored;
		Slid = 0.f;
		HoldSaves(false);
		LeanLantern();
		break;
	default:
		break;
	}
	ShowGangway(State == EShipFlight::Moored);
	UpdateSounds();
	UE_LOG(LogLooter, Log, TEXT("Landfall: the Lamplighter is %s."), *UEnum::GetValueAsString(State));
}

void ALamplighterShip::UpdateSounds()
{
	// Her engine and her timbers: sailing, straining against the lines, labouring as she goes down; quiet once anchored.
	const bool bRunning = Flight != EShipFlight::Moored || !bAnchored;
	if (bRunning && !EngineSound.IsValid())
	{
		EngineSound = LooterSound::Start(this, LooterSoundCue::ShipEngine, Root);
	}
	if (bRunning && !CreakSound.IsValid())
	{
		CreakSound = LooterSound::Start(this, LooterSoundCue::ShipCreak, Root);
	}
	if (UAudioComponent* Running = EngineSound.Get())
	{
		const float Pitch = Flight == EShipFlight::Grappled ? 1.18f : Flight == EShipFlight::Breaking || Flight == EShipFlight::Landing ? 0.8f : 1.f;
		Running->SetPitchMultiplier(Pitch);
	}
	if (!bRunning)
	{
		LooterSound::Stop(EngineSound.Get(), 2.5f);
		LooterSound::Stop(CreakSound.Get(), 1.5f);
		EngineSound.Reset();
		CreakSound.Reset();
	}
}

void ALamplighterShip::SetFlight(EShipFlight State)
{
	bBegun = true;
	switch (State)
	{
	case EShipFlight::Sailing:
		ResetFight();
		Distance = 0.f;
		Speed = FLamplighterCourse::CruiseSpeed;
		Clock = 0.f;
		bDeckLinesSaid = true;
		EnterFlight(EShipFlight::Sailing);
		HoldSaves(true);
		PutPlayerOnDeck();
		break;
	case EShipFlight::Grappled:
		ResetFight();
		Distance = GetGrappleStopDistance();
		Speed = 0.f;
		EnterFlight(EShipFlight::Sailing);
		HoldSaves(true);
		bTenderCalled = true;
		GrappleNow();
		PutPlayerOnDeck();
		break;
	case EShipFlight::Breaking:
	case EShipFlight::Landing:
	{
		// (Braced: the copy below is a local, and the next case label may not jump past its initialisation.)
		if (Flight != EShipFlight::Grappled)
		{
			SetFlight(EShipFlight::Grappled);
		}
		ClearDeck();
		// A copy: the second cut breaks her free, which changes what she holds.
		const TArray<TObjectPtr<ALandfallGrapple>> Lines = Grapples;
		for (ALandfallGrapple* Grapple : Lines)
		{
			if (Grapple)
			{
				Grapple->Cut();
			}
		}
		if (Flight != EShipFlight::Breaking)
		{
			BreakFree();
		}
		if (State == EShipFlight::Landing)
		{
			BreakClock = HardLanding::BreakSeconds;
			StartHardLanding();
		}
		break;
	}
	case EShipFlight::Moored:
		ResetFight();
		bAnchored = false;
		EnterFlight(EShipFlight::Moored);
		FindOrSpawnWinch();
		break;
	}
}

void ALamplighterShip::ResetFight()
{
	for (const TWeakObjectPtr<ACreatureBase>& Boarder : Boarders)
	{
		if (AActor* Gone = Boarder.Get())
		{
			Gone->Destroy();
		}
	}
	Boarders.Reset();
	for (ALandfallGrapple* Grapple : Grapples)
	{
		if (IsValid(Grapple))
		{
			Grapple->Destroy();
		}
	}
	Grapples.Reset();
	if (IsValid(Tender))
	{
		Tender->Destroy();
	}
	Tender = nullptr;
	bTenderCalled = false;
	SinceCalled = 0.f;
	bGrapplesFired = false;
	GrappledClock = 0.f;
	WavesSpawned = 0;
	WaveClock = 0.f;
	NextWaveAt = -1.f;
	PendingBoarders.Reset();
	bDeckClear = false;
	bRiflemenOut = false;
	bPlankDown = false;
	BreakClock = 0.f;
	bTouched = false;
	VaneHornConversation = INDEX_NONE;
}
