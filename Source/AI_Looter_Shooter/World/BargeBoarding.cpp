// ABargeBoarding: Vane's barge brought down over the chapel in Main 6: her life in the level, her place in the story, her
// pieces, the beats the furnace, valves and drum tell her, and the getters the sortie and the build script read.
// BargeBoardingKit.cpp builds her walkable kit, BargeBoardingDeck.cpp ticks her deck, BargeBoardingDying.cpp her end.

#include "World/BargeBoarding.h"
#include "AI_Looter_Shooter.h"
#include "Audio/LooterSound.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/BargeFurnace.h"
#include "World/BargeHoseDrum.h"
#include "World/BargeLooks.h"
#include "World/BargeRules.h"
#include "World/FurnaceDraftValve.h"
#include "World/LamplighterRules.h"
#include "World/LandfallMarker.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"

const FName ABargeBoarding::BargeTag(TEXT("Barge_Pilot"));
const FName ABargeBoarding::HatchTag(TEXT("Barge_FurnaceHatch"));
const FName ABargeBoarding::DeckFight(TEXT("BargeDeck"));
const FName ABargeBoarding::HallFight(TEXT("FurnaceHall"));
const TCHAR* const ABargeBoarding::HullPath = TEXT("/Game/Art/Tripo/SM_DredgeBarge.SM_DredgeBarge");

namespace
{
	/** The beats' line sets (create_story_lines.py, lane MAIN67's contract). */
	const FName FinchDimmingLines(TEXT("DA_Lines_Main6FinchDimming"));
	const FName NedRadioLines(TEXT("DA_Lines_Main6NedRadio"));
	const FName HallLines(TEXT("DA_Lines_Main6Hall"));
	const FName DrumJammedLines(TEXT("DA_Lines_Main6DrumJammed"));
	const FName ValveLines[] = { FName(TEXT("DA_Lines_Main6Valve1")), FName(TEXT("DA_Lines_Main6Valve2")), FName(TEXT("DA_Lines_Main6Valve3")) };
}

ABargeBoarding::ABargeBoarding()
{
	PrimaryActorTick.bCanEverTick = true;
	// Before the characters on her deck move: they ride her as she lists and drifts (as the Lamplighter's deck is ridden).
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	SetCanBeDamaged(false);
	Tags.Add(BargeTag);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	static UStaticMesh* const TripoHull = BargeLooks::Find<UStaticMesh>(HullPath);
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UMaterialInterface* const Iron = BargeLooks::Find<UMaterialInterface>(BargeLooks::Iron);
	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(Root);
	Hull->SetMobility(EComponentMobility::Movable);
	if (TripoHull)
	{
		Hull->SetStaticMesh(TripoHull);
	}
	else
	{
		// Without the Tripo barge: an iron box of her size up to her deck, so the kit stands on something.
		Hull->SetStaticMesh(CubeMesh);
		Hull->SetMaterial(0, Iron);
		Hull->SetRelativeLocation(FVector(0.0, 0.0, BargeRules::DeckTop * 0.5));
		Hull->SetRelativeScale3D(FVector(BargeRules::HullLength / 100.0, BargeRules::HullHalfBeam / 50.0, BargeRules::DeckTop / 100.0));
	}
	// Seen, never walked or shot: the kit carries every collision (her interior is a hall the hull's mesh would fill).
	Hull->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hull->SetCanEverAffectNavigation(false);
	Hull->bCastDynamicShadow = true;

	Kit = CreateDefaultSubobject<USceneComponent>(TEXT("Kit"));
	Kit->SetupAttachment(Root);
	Kit->SetMobility(EComponentMobility::Movable);

	BuildKit();

	// Main 6, until she falls.
	ShownWhen.DuringMission = BargeRules::Mission();
	ShownWhen.WithoutFlags = { BargeRules::BargeFellFlag() };
}

ABargeBoarding* ABargeBoarding::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ABargeBoarding> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}
	return nullptr;
}

void ABargeBoarding::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Kit->SetRelativeLocation(FVector(0.0, 0.0, KitLift));
	LayStaticLooks();
}

void ABargeBoarding::BeginPlay()
{
	Super::BeginPlay();
	Placed = GetActorTransform();
	bPlacedKnown = true;
	Kit->SetRelativeLocation(FVector(0.0, 0.0, KitLift));
	LayStaticLooks();
	if (const UStaticMesh* Section = HoseSections->GetStaticMesh())
	{
		HoseNativeLength = FMath::Max(static_cast<float>(Section->GetBoundingBox().GetSize().Z), 1.f);
	}
	SpawnPieces();
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &ABargeBoarding::HandleMissionsChanged);
	}
	RefreshStory();
}

void ABargeBoarding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnChanged.Remove(MissionsChangedHandle);
	}
	BoundRunner.Reset();
	LooterSound::Stop(FurnaceDrone.Get(), 0.2f);
	FurnaceDrone.Reset();
	for (AActor* Piece : TArray<AActor*>{ Furnace.Get(), HoseDrum.Get(), HatchMarker.Get() })
	{
		if (IsValid(Piece))
		{
			Piece->Destroy();
		}
	}
	for (AFurnaceDraftValve* Valve : Valves)
	{
		if (IsValid(Valve))
		{
			Valve->Destroy();
		}
	}
	Valves.Reset();
	Super::EndPlay(EndPlayReason);
}

FVector ABargeBoarding::ToWorld(const FVector& Local) const
{
	return GetActorTransform().TransformPosition(Local + FVector(0.0, 0.0, KitLift));
}

FTransform ABargeBoarding::RestTransform() const
{
	return bPlacedKnown ? Placed : GetActorTransform();
}

void ABargeBoarding::SpawnPieces()
{
	UWorld* World = GetWorld();
	if (!World || Furnace)
	{
		return;
	}
	const auto Spawn = [this, World](UClass* Class, const FVector& Local, float Yaw) -> AActor*
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Piece = World->SpawnActor<AActor>(Class, ToWorld(Local), (GetActorQuat() * FQuat(FVector::UpVector, FMath::DegreesToRadians(Yaw))).Rotator(), Params);
		if (Piece)
		{
			// Carried by her (her list, her drift, her fall), and begun even in a level that isn't playing (the tests').
			Piece->AttachToComponent(Kit, FAttachmentTransformRules::KeepWorldTransform);
			if (!Piece->HasActorBegunPlay())
			{
				Piece->DispatchBeginPlay();
			}
		}
		return Piece;
	};

	// The furnace on the hall's floor, its cage's window facing the office.
	Furnace = Cast<ABargeFurnace>(Spawn(ABargeFurnace::StaticClass(), BargeRules::HallPoint(0.f, 0.f), 0.f));
	if (Furnace)
	{
		Furnace->SetBarge(this);
		Furnace->OpenWhen.DuringMission = BargeRules::Mission();
		Furnace->OpenWhen.FromStep = BargeRules::CageStep;
		Furnace->OpenWhen.BeforeStep = BargeRules::EscapeStep;
	}
	// The valves at the wall, facing into the hall.
	for (const float Angle : BargeRules::ValveAngles)
	{
		if (AFurnaceDraftValve* Valve = Cast<AFurnaceDraftValve>(Spawn(AFurnaceDraftValve::StaticClass(),
			BargeRules::HallPoint(Angle, BargeRules::HallRadius - 30.f), Angle + 180.f)))
		{
			Valve->SetBarge(this);
			Valve->ShutWhen.DuringMission = BargeRules::Mission();
			Valve->ShutWhen.FromStep = BargeRules::FurnaceStep;
			Valve->ShutWhen.BeforeStep = BargeRules::CageStep;
			Valves.Add(Valve);
		}
	}
	// The drum's gantry on the deck over the drum under her keel. Jammable through the deck fight (the step's optional).
	HoseDrum = Cast<ABargeHoseDrum>(Spawn(ABargeHoseDrum::StaticClass(), BargeRules::DrumOnDeck, 0.f));
	if (HoseDrum)
	{
		HoseDrum->SetBarge(this);
		HoseDrum->JamWhen.DuringMission = BargeRules::Mission();
		HoseDrum->JamWhen.FromStep = BargeRules::DeckStep;
		HoseDrum->JamWhen.BeforeStep = BargeRules::ManifestStep;
	}
	// The hatch's marker (Main 6 step 3's Reach), riding her.
	HatchMarker = ALandfallMarker::SpawnTagged(World, HatchTag, ToWorld(FVector(BargeRules::HatchX, 0.0, BargeRules::DeckTop + 50.0)), Kit);
}

// ---------------------------------------------------------------------------
// The story
// ---------------------------------------------------------------------------

bool ABargeBoarding::StoryHolds(const FStoryCondition& Condition) const
{
	if (Condition.IsEmpty())
	{
		return true;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Condition.IsMet(Runner->GetCampaign(), Runner);
}

void ABargeBoarding::HandleMissionsChanged()
{
	RefreshStory();
}

void ABargeBoarding::ForceStep(int32 Step)
{
	bForced = Step != INDEX_NONE;
	ForcedStep = Step;
	bStoryRead = false;
	if (bForced && (State == EBargeState::Away || State == EBargeState::Fallen || Step < BargeRules::EscapeStep))
	{
		// Forced back into the sky or back before the escape (the console after a fall, a replay): whole again.
		SetState(EBargeState::Hanging);
		SetActorTransform(RestTransform());
		DyingClock = 0.f;
		bStackDown = false;
		bStackHurtDone = false;
		DropStack(0.f);
		if (Furnace)
		{
			Furnace->SetCageBurst(false);
		}
	}
	RefreshStory();
}

void ABargeBoarding::RefreshStory()
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	const bool bFell = Runner && Runner->HasFlag(BargeRules::BargeFellFlag());
	int32 Step = INDEX_NONE;
	if (bForced)
	{
		Step = ForcedStep;
	}
	else if (Runner && Runner->IsRunning(BargeRules::Mission()))
	{
		Step = Runner->GetStep(BargeRules::Mission());
	}
	const bool bWanted = bForced || (!bFell && StoryHolds(ShownWhen));
	if (!bWanted || (bFell && !bForced))
	{
		if (State != EBargeState::Fallen)
		{
			SetState(bFell ? EBargeState::Fallen : EBargeState::Away);
		}
		StoryStep = Step;
		bStoryRead = true;
		return;
	}
	if (State == EBargeState::Away || State == EBargeState::Fallen)
	{
		SetState(EBargeState::Hanging);
	}
	const bool bLive = bStoryRead && Step != StoryStep;
	const bool bChanged = !bStoryRead || Step != StoryStep;
	StoryStep = Step;
	bStoryRead = true;
	if (!bChanged)
	{
		return;
	}
	StepClock = 0.f;

	// The valves: open until the furnace's step, as the step's count starts over on a load; all shut past it.
	for (AFurnaceDraftValve* Valve : Valves)
	{
		if (!Valve)
		{
			continue;
		}
		if (Step >= BargeRules::CageStep)
		{
			Valve->SetShutNow(true);
		}
		else if (Step < BargeRules::FurnaceStep || !bLive)
		{
			Valve->SetShutNow(false);
		}
	}
	if (Furnace)
	{
		Furnace->SetValvesShut(CountValvesShut());
		// The ring heats from the office's step (the hall alive under the catwalk) until the furnace is cold.
		Furnace->SetRingRunning(Step >= BargeRules::ManifestStep && Step <= BargeRules::FurnaceStep);
		if (Step >= BargeRules::EscapeStep)
		{
			Furnace->SetCageBurst(true);
			Furnace->PutOut();
		}
	}
	if (HoseDrum)
	{
		HoseDrum->SetJammedNow(IsDrumJammed());
		HoseDrum->SetTurning(Step < BargeRules::CageStep);
	}
	bHoseFrozen = IsDrumJammed();

	if (Step == BargeRules::EscapeStep)
	{
		BeginDying();
	}
	else if (Step >= BargeRules::ReadyStep && State != EBargeState::Falling)
	{
		// Past the fall with no scene to have played it (a skip lost, a load): she's in the canyon.
		FinishFall();
	}
	if (bLive)
	{
		OnStepBegan(Step);
	}
}

void ABargeBoarding::OnStepBegan(int32 Step)
{
	if (Step == BargeRules::ManifestStep && !IsDrumJammed())
	{
		// Through the office's door: the saint dimming, on Finch's tube.
		LandfallRules::Say(this, FinchDimmingLines);
	}
	else if (Step == BargeRules::FurnaceStep)
	{
		// The manifest taken: Ned's call crackles out of the office's radio; then the hall wakes (the Stoker, Vane).
		if (Radio)
		{
			LooterSound::PlayAt(this, LooterSoundCue::VaneRadioIn, Radio->GetComponentLocation());
		}
		LandfallRules::Say(this, NedRadioLines);
		LandfallRules::Say(this, HallLines);
		if (!IsDrumJammed())
		{
			LandfallRules::Say(this, FinchDimmingLines);
		}
	}
}

void ABargeBoarding::SetState(EBargeState NewState)
{
	if (State == NewState)
	{
		return;
	}
	const EBargeState Was = State;
	State = NewState;
	ApplyPresence(NewState != EBargeState::Away && NewState != EBargeState::Fallen);
	UE_LOG(LogLooter, Log, TEXT("%s: the barge is %s (was %s)."), *GetActorNameOrLabel(), *UEnum::GetValueAsString(NewState),
		*UEnum::GetValueAsString(Was));
}

void ABargeBoarding::ApplyPresence(bool bShown)
{
	// Out of the story she's out of the world: nothing seen, nothing to stand on, nothing ticking.
	SetActorHiddenInGame(!bShown);
	SetActorEnableCollision(bShown);
	SetActorTickEnabled(bShown);
	TArray<AActor*> Pieces = { Furnace.Get(), HoseDrum.Get(), HatchMarker.Get() };
	for (AFurnaceDraftValve* Valve : Valves)
	{
		Pieces.Add(Valve);
	}
	for (AActor* Piece : Pieces)
	{
		if (IsValid(Piece))
		{
			Piece->SetActorHiddenInGame(!bShown);
			Piece->SetActorEnableCollision(bShown);
			Piece->SetActorTickEnabled(bShown && Piece->PrimaryActorTick.bStartWithTickEnabled);
		}
	}
	if (bShown && !FurnaceDrone.IsValid() && Furnace && GetWorld() && GetWorld()->IsGameWorld())
	{
		// Her furnace's breath under the deck: heard round her, louder in the hall.
		FurnaceDrone = LooterSound::Start(this, LooterSoundCue::SmogAmbience, Furnace->GetRootComponent(), NAME_None, 0.9f, 0.7f);
	}
	else if (!bShown)
	{
		LooterSound::Stop(FurnaceDrone.Get(), 1.f);
		FurnaceDrone.Reset();
	}
}

// ---------------------------------------------------------------------------
// The beats
// ---------------------------------------------------------------------------

bool ABargeBoarding::IsDrumJammed() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return (Runner && Runner->HasFlag(BargeRules::DrumJammedFlag())) || (HoseDrum && HoseDrum->IsJammed());
}

void ABargeBoarding::NoteDrumJammed()
{
	bHoseFrozen = true;
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		Runner->SetFlag(BargeRules::DrumJammedFlag());
	}
	// Finch on the tube from the Reliquary: the hose stopped short of her roof.
	LandfallRules::Say(this, DrumJammedLines, /*bInterrupt*/ true);
}

int32 ABargeBoarding::CountValvesShut() const
{
	int32 Shut = 0;
	for (const AFurnaceDraftValve* Valve : Valves)
	{
		Shut += Valve && Valve->IsShut() ? 1 : 0;
	}
	return Shut;
}

void ABargeBoarding::NoteValveShut(AFurnaceDraftValve& Valve)
{
	const int32 Shut = CountValvesShut();
	if (Furnace)
	{
		Furnace->SetValvesShut(Shut);
	}
	if (Shut >= 1 && Shut <= static_cast<int32>(UE_ARRAY_COUNT(ValveLines)))
	{
		LandfallRules::Say(this, ValveLines[Shut - 1], /*bInterrupt*/ Shut == BargeRules::NumValves);
	}
	UE_LOG(LogLooter, Log, TEXT("%s: %d of %d draft valves shut."), *GetActorNameOrLabel(), Shut, BargeRules::NumValves);
}

void ABargeBoarding::NoteCageBurst()
{
	if (Furnace)
	{
		Furnace->SetCageBurst(true);
		Furnace->PutOut();
	}
	if (State == EBargeState::Hanging)
	{
		SetState(EBargeState::Burst);
	}
}

// ---------------------------------------------------------------------------
// The getters
// ---------------------------------------------------------------------------

FTransform ABargeBoarding::GetBerth() const
{
	const FVector Keel(BargeRules::BerthX, BargeRules::BerthSide * BargeRules::BerthOut,
		BargeRules::DeckTop - BargeRules::BerthUnderDeck - BargeRules::ShipDeckOverKeel);
	return FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), ToWorld(Keel));
}

FTransform ABargeBoarding::GetJumpBerth() const
{
	// Under the gangway's broken end, as she lists and drifts: the ship keeps level (the barge's yaw only).
	const FVector Keel(BargeRules::BerthX, BargeRules::BerthSide * BargeRules::JumpOut,
		BargeRules::DeckTop - BargeRules::JumpUnderDeck - BargeRules::ShipDeckOverKeel);
	return FTransform(FRotator(0.f, GetActorRotation().Yaw, 0.f), ToWorld(Keel));
}

FVector ABargeBoarding::GetGrappleAnchor(int32 Index) const
{
	const float Along = Index == 0 ? BargeRules::GrappleSpread : -BargeRules::GrappleSpread;
	return ToWorld(FVector(BargeRules::BerthX + Along, BargeRules::BerthSide * (BargeRules::DeckHalfBeam + 20.0), BargeRules::DeckTop + 90.0));
}

FVector ABargeBoarding::GetBoardingPoint() const
{
	return ToWorld(FVector(BargeRules::BerthX, BargeRules::BerthSide * (BargeRules::DeckHalfBeam - 150.0), BargeRules::DeckTop));
}

FTransform ABargeBoarding::GetPlayerStart() const
{
	// On the deck, standing (a capsule's half height over it), facing inboard: by the plank until the hatch, by the hatch after.
	const bool bBelow = StoryStep >= BargeRules::ManifestStep;
	const FVector Local = bBelow ? FVector(BargeRules::HatchX + 300.0, 0.0, BargeRules::DeckTop + 100.0)
		: FVector(BargeRules::BerthX, BargeRules::BerthSide * (BargeRules::DeckHalfBeam - 200.0), BargeRules::DeckTop + 100.0);
	const float Yaw = static_cast<float>(GetActorRotation().Yaw) + (bBelow ? 180.f : (BargeRules::BerthSide < 0.f ? 90.f : -90.f));
	return FTransform(FRotator(0.f, Yaw, 0.f), ToWorld(Local));
}

TArray<FVector> ABargeBoarding::GetFightSpots(FName Fight) const
{
	TArray<FVector> Spots;
	if (Fight == DeckFight)
	{
		for (const FVector& Spot : BargeRules::DeckFightSpots)
		{
			Spots.Add(ToWorld(Spot + FVector(0.0, 0.0, 20.0)));
		}
	}
	else if (Fight == HallFight)
	{
		// Four at each stokehold's door, in two rows: the pushes come through them.
		for (const float Angle : BargeRules::StokeholdAngles)
		{
			for (int32 Index = 0; Index < BargeRules::HallFightSpots / 2; ++Index)
			{
				const float Across = (Index % 2 == 0 ? -1.f : 1.f) * 120.f;
				const float In = BargeRules::HallRadius - 150.f - (Index / 2) * 180.f;
				const float Turn = FMath::DegreesToRadians(Angle);
				const FVector Out(FMath::Cos(Turn), FMath::Sin(Turn), 0.0);
				const FVector Side(-Out.Y, Out.X, 0.0);
				Spots.Add(ToWorld(BargeRules::HallCenter + Out * In + Side * Across + FVector(0.0, 0.0, BargeRules::HallFloor + 20.0)));
			}
		}
	}
	return Spots;
}

FVector ABargeBoarding::GetOfficeSpot() const
{
	return ToWorld(BargeRules::DeskTop());
}

TArray<FTransform> ABargeBoarding::GetHornSpots() const
{
	TArray<FTransform> Spots;
	for (const FVector& Spot : BargeRules::HornSpots)
	{
		// Each post's horns turned in over the deck (toward her middle line).
		const float Yaw = static_cast<float>(GetActorRotation().Yaw) + (Spot.Y < 0.0 ? 90.f : -90.f);
		Spots.Add(FTransform(FRotator(0.f, Yaw, 0.f), ToWorld(Spot)));
	}
	return Spots;
}

FVector ABargeBoarding::GetHatchSpot() const
{
	return ToWorld(FVector(BargeRules::HatchX, 0.0, BargeRules::DeckTop));
}

FVector ABargeBoarding::GetDeckMiddle() const
{
	return ToWorld(FVector((BargeRules::DeckAft + BargeRules::DeckFore) * 0.5, 0.0, BargeRules::DeckTop + 20.0));
}

FVector ABargeBoarding::GetHallMiddle() const
{
	return ToWorld(BargeRules::HallPoint(0.f, 0.f, 20.f));
}

FVector ABargeBoarding::GetHoseTop() const
{
	return GetActorTransform().TransformPosition(BargeRules::DrumUnderKeel);
}
