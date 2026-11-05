// ABargeBoarding: Vane's barge brought down over the chapel in Main 6: her life in the level (the hull, shown or not), her
// place in the story, her pieces, and the beats the furnace, valves and drum tell her. BargeBoardingSpots.cpp has the
// getters the sortie and the build scripts read, BargeBoardingKit.cpp her deck kit, BargeBoardingProps.cpp her props,
// BargeBoardingHall.cpp the hall below the deck, BargeBoardingDeck.cpp her tick, BargeBoardingStory.cpp her steps and state,
// BargeBoardingPushes.cpp the deck fight's pushes, BargeBoardingLights.cpp her lights by where the player is,
// BargeBoardingDying.cpp her end.

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
#include "World/PlayableArea.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
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

	static UStaticMesh* const HullMesh = BargeLooks::Find<UStaticMesh>(HullPath);
	static UStaticMesh* const CubeMesh = BargeLooks::Find<UStaticMesh>(BargeLooks::Cube);
	static UMaterialInterface* const Iron = BargeLooks::Find<UMaterialInterface>(BargeLooks::Iron);
	Hull = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hull"));
	Hull->SetupAttachment(Root);
	Hull->SetMobility(EComponentMobility::Movable);
	if (HullMesh)
	{
		// The jumboed Tripo hull carries its own collision (its UCX boxes: the decks, the rails round the berth's gap, the
		// coaming round the hatch's hole, the forecastle's step, the house, the casing, the stacks, the scoop; nothing in the
		// hollow below the well deck, where the kit's hall and office are): she's walked and shot on as she is.
		Hull->SetStaticMesh(HullMesh);
		Hull->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Hull->SetCollisionObjectType(ECC_WorldStatic);
	}
	else
	{
		// Without it: an iron box of her size up to her well deck, only to see; the kit lays its own deck plates, coaming and
		// rails on it (and builds the rooms below the deck inside it as in the hull).
		const double Length = BargeRules::HullFore - BargeRules::HullAft;
		Hull->SetStaticMesh(CubeMesh);
		Hull->SetMaterial(0, Iron);
		// Its top under the kit's plates' undersides (never in their plane: no flicker). It still roofs the rooms below seen
		// through the hatch's hole: a stand-in only.
		const double Top = BargeRules::WellDeckZ - BargeRules::PlateThickness;
		Hull->SetRelativeLocation(FVector((BargeRules::HullAft + BargeRules::HullFore) * 0.5, 0.0, Top * 0.5));
		Hull->SetRelativeScale3D(FVector(Length / 100.0, BargeRules::HullHalfBeam / 50.0, Top / 100.0));
		Hull->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
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
	// Her deck is playable wherever she hangs: over the chapel her bow lies past the boundary's closed edge east of the
	// knoll, where her fights' spots, her Dredgers' steps and the player's climbs would be turned down (IsAboardAt).
	if (APlayableArea* Area = APlayableArea::Find(GetWorld()))
	{
		Area->AddAloft(this);
	}
	if (UMissionRunner* Runner = UMissionRunner::Get(this))
	{
		BoundRunner = Runner;
		MissionsChangedHandle = Runner->OnChanged.AddUObject(this, &ABargeBoarding::HandleMissionsChanged);
	}
	RefreshStory();
}

void ABargeBoarding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayableArea* Area = APlayableArea::Find(GetWorld()))
	{
		Area->RemoveAloft(this);
	}
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

	// The furnace on the hall's floor, its cage's window facing the office's doorway (its grate ring turns with it).
	Furnace = Cast<ABargeFurnace>(Spawn(ABargeFurnace::StaticClass(), BargeRules::HallPoint(0.f, 0.f), BargeRules::OfficeAngle));
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
			BargeRules::HallPoint(Angle, BargeRules::HallRadius - BargeRules::ValveIn), BargeRules::OfficeAngle + Angle + 180.f)))
		{
			Valve->SetBarge(this);
			Valve->ShutWhen.DuringMission = BargeRules::Mission();
			Valve->ShutWhen.FromStep = BargeRules::FurnaceStep;
			Valve->ShutWhen.BeforeStep = BargeRules::CageStep;
			Valves.Add(Valve);
		}
	}
	// The drum's gantry on the well deck's aft port quarter (a drive shaft to the drum under her keel). Jammable through the
	// deck fight (the step's optional).
	HoseDrum = Cast<ABargeHoseDrum>(Spawn(ABargeHoseDrum::StaticClass(), BargeRules::DrumOnDeck, 0.f));
	if (HoseDrum)
	{
		HoseDrum->SetBarge(this);
		HoseDrum->JamWhen.DuringMission = BargeRules::Mission();
		HoseDrum->JamWhen.FromStep = BargeRules::DeckStep;
		HoseDrum->JamWhen.BeforeStep = BargeRules::ManifestStep;
	}
	// The hatch's marker (Main 6 step 3's Reach), riding her.
	HatchMarker = ALandfallMarker::SpawnTagged(World, HatchTag, ToWorld(BargeRules::HatchSpot() + FVector(0.0, 0.0, 50.0)), Kit);
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
