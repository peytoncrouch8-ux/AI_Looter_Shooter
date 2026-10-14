#include "World/RespawnMarker.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaLandings.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/RoostBeacon.h"
#include "World/RoostCrows.h"
#include "World/RoostKit.h"
#include "World/RoostSettings.h"
#include "Audio/LooterSound.h"
#include "Components/ArrowComponent.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

namespace
{
	/** Seconds between looks at the player: a roost is a landmark, so a walk past it is noticed within a third of a second. */
	constexpr float PollSeconds = 0.3f;

	/** The light on the post: gone past this (cm, fading over the last stretch), so a roost across the valley costs nothing. */
	constexpr float LightDrawDistance = 4500.f;
	constexpr float LightFadeRange = 800.f;

	/** A greybox piece of the stand-in: solid, shadowed, never a navigation obstacle of its own. */
	UInstancedStaticMeshComponent* MakeGreybox(AActor& Owner, USceneComponent* Parent, const TCHAR* Name, UStaticMesh* Cube, UMaterialInterface* Material)
	{
		UInstancedStaticMeshComponent* Pieces = Owner.CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Pieces->SetupAttachment(Parent);
		Pieces->SetMobility(EComponentMobility::Movable);
		Pieces->SetStaticMesh(Cube);
		Pieces->SetMaterial(0, Material);
		Pieces->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Pieces->SetCanEverAffectNavigation(false);
		Pieces->SetCastShadow(true);
		return Pieces;
	}
}

ARespawnMarker::ARespawnMarker()
{
	// Ticks only for the second of its light's flare (AdvanceLook); the poll for the player is a timer.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Anchor->SetMobility(EComponentMobility::Movable);
	RootComponent = Anchor;

	// The models found only once they're in this checkout, so the class works, and its tests run, without them.
	static UStaticMesh* const RoostModel = RoostKit::FindIfMade<UStaticMesh>(RoostKit::RoostPath);
	static UStaticMesh* const BellModel = RoostKit::FindIfMade<UStaticMesh>(RoostKit::BellPath);
	static UStaticMesh* const CubeModel = RoostKit::FindIfMade<UStaticMesh>(RoostKit::CubePath);
	static UMaterialInterface* const WoodLook = RoostKit::FindIfMade<UMaterialInterface>(RoostKit::WoodPath);
	static UMaterialInterface* const IronLook = RoostKit::FindIfMade<UMaterialInterface>(RoostKit::IronPath);
	static UMaterialInterface* const StoneLook = RoostKit::FindIfMade<UMaterialInterface>(RoostKit::StonePath);
	RoostMesh = RoostModel;
	BellMesh = BellModel;

	Roost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Roost"));
	Roost->SetupAttachment(Anchor);
	Roost->SetMobility(EComponentMobility::Movable);
	Roost->SetStaticMesh(RoostModel);
	// A mast stops the player and shots as a mast does (its hulls: the mast and the cairn), and no creature plans round it.
	Roost->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Roost->SetCanEverAffectNavigation(false);
	Roost->SetCastShadow(true);

	// The bell swings free on its loop: no collision, so it never knocks the player or stops a shot.
	Bell = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bell"));
	Bell->SetupAttachment(Roost);
	Bell->SetMobility(EComponentMobility::Movable);
	Bell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bell->SetGenerateOverlapEvents(false);
	Bell->SetCanEverAffectNavigation(false);
	Bell->SetCastShadow(true);
	Bell->SetStaticMesh(BellModel ? BellModel : CubeModel);
	if (!BellModel)
	{
		Bell->SetMaterial(0, IronLook);
	}

	GreyboxWood = MakeGreybox(*this, Roost, TEXT("GreyboxWood"), CubeModel, WoodLook);
	GreyboxIron = MakeGreybox(*this, Roost, TEXT("GreyboxIron"), CubeModel, IronLook);
	GreyboxStone = MakeGreybox(*this, Roost, TEXT("GreyboxStone"), CubeModel, StoneLook);

	Beacon = CreateDefaultSubobject<URoostBeaconComponent>(TEXT("Beacon"));
	Beacon->SetupAttachment(Roost);

	BeaconLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeaconLight"));
	BeaconLight->SetupAttachment(Roost);
	BeaconLight->SetMobility(EComponentMobility::Movable);
	// The light on the post once found: the roost's colour on its own wood and the ground round it; never a shadow, gone
	// beyond a long walk (Medium's budget: a few shadowless lights in view at most, and roosts stand far apart).
	BeaconLight->SetCastShadows(false);
	BeaconLight->SetIntensityUnits(ELightUnits::Candelas);
	BeaconLight->SetMaxDrawDistance(LightDrawDistance);
	BeaconLight->SetMaxDistanceFadeRange(LightFadeRange);
	BeaconLight->SetVisibility(false);

#if WITH_EDITORONLY_DATA
	Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(Anchor);
		Arrow->SetRelativeLocation(FVector(0.0, 0.0, 30.0));
		Arrow->ArrowColor = FColor(140, 210, 255);
		Arrow->bTreatAsASprite = true;
		Arrow->bIsScreenSizeScaled = true;
	}
	// A death far away may wake the player here: in a partitioned world it never streams out.
	bIsSpatiallyLoaded = false;
#endif
}

void ARespawnMarker::BeginPlay()
{
	Super::BeginPlay();
	if (MarkerId.IsNone())
	{
		UE_LOG(LogLooter, Warning, TEXT("Roost %s has no MarkerId: its finding is kept under its actor name, which a rebuilt level may change."),
			*GetActorNameOrLabel());
	}
	bInPlay = true;
	FitRoost();
	SpawnCrows();
	UMissionRunner* Runner = UMissionRunner::Get(this);
	// A roost found in an earlier visit stands lit from the start, with no show (its circling crows already up): the story
	// is in the session's record.
	SetLit(Runner && Runner->IsActive() && IsMarked(Runner->GetCampaign()));
	if (bMarkOnApproach)
	{
		// A few looks a second, each roost off the others' beat; the work is one distance until the player is near.
		GetWorldTimerManager().SetTimer(PollTimer, this, &ARespawnMarker::PollApproach, PollSeconds, true, FMath::FRandRange(0.f, PollSeconds));
	}
	if (!Runner || ActiveAfterMission.IsNone())
	{
		return;
	}
	// Its mission may have been finished before this level began (elsewhere, or before roosts existed): recorded now. That
	// opens the roost but never finds it: a save point is a roost the player has walked up to.
	if (Runner->GetCampaign().HasCompleted(ActiveAfterMission) && Activate(Runner->GetCampaign()))
	{
		UE_LOG(LogLooter, Log, TEXT("Roost %s is open: %s was finished."), *GetMarkerId().ToString(), *ActiveAfterMission.ToString());
	}
	BoundRunner = Runner;
	FinishedHandle = Runner->OnMissionFinished.AddUObject(this, &ARespawnMarker::HandleMissionFinished);
}

void ARespawnMarker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bInPlay = false;
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PollTimer);
	}
	if (UMissionRunner* Runner = BoundRunner.Get())
	{
		Runner->OnMissionFinished.Remove(FinishedHandle);
	}
	BoundRunner.Reset();
	FinishedHandle.Reset();
	UpdateAmbient(false);
	DestroyCrows();
	Super::EndPlay(EndPlayReason);
}

void ARespawnMarker::Destroyed()
{
	// A roost removed without its play having begun (in the editor, or a test's level, where EndPlay never comes) still
	// takes its crows with it.
	DestroyCrows();
	Super::Destroyed();
}

void ARespawnMarker::DestroyCrows()
{
	for (TWeakObjectPtr<ARoostCrows>* Flock : { &Crows, &CirclingCrows })
	{
		if (ARoostCrows* Birds = Flock->Get())
		{
			Birds->Destroy();
		}
		Flock->Reset();
	}
}

void ARespawnMarker::HandleMissionFinished(const UMissionDefinition& Mission, bool bRewarded)
{
	UMissionRunner* Runner = BoundRunner.Get();
	if (!Runner || Mission.GetMissionId() != ActiveAfterMission)
	{
		return;
	}
	// The record is saved with the mission's finish a few seconds from now (the runner asks for that save).
	if (Activate(Runner->GetCampaign()))
	{
		UE_LOG(LogLooter, Log, TEXT("Roost %s opens: %s is finished."), *GetMarkerId().ToString(), *ActiveAfterMission.ToString());
	}
}

void ARespawnMarker::SpawnCrows()
{
	UWorld* World = GetWorld();
	if (!World || Crows.IsValid())
	{
		return;
	}
	// Made in play only and never saved: the roost's own crows on its perches, updated by the fauna's subsystem.
	const FTransform At(FRotator(0.0, GetActorRotation().Yaw, 0.0), GetActorLocation());
	ARoostCrows* Birds = World->SpawnActorDeferred<ARoostCrows>(ARoostCrows::StaticClass(), At, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Birds)
	{
		return;
	}
	Birds->SetRoostPerches(GetPerches());
	// Each roost's crows their own: seeded from where it stands, so a rebuild plays the same.
	Birds->Seed = static_cast<int32>(GetTypeHash(GetMarkerId())) | 1;
	Birds->FinishSpawning(At);
	Crows = Birds;
}

// ---------------------------------------------------------------------------
// Open or closed
// ---------------------------------------------------------------------------

FName ARespawnMarker::GetMarkerId() const
{
	return MarkerId.IsNone() ? GetFName() : MarkerId;
}

FText ARespawnMarker::GetGraveName() const
{
	if (!DisplayName.IsEmpty())
	{
		return DisplayName;
	}
	// MainStreet -> "Main Street", KeepersGrave -> "Keepers Grave": readable until the build gives it a name of its own.
	return FText::FromString(FName::NameToDisplayString(GetMarkerId().ToString(), /*bIsBool*/ false));
}

FRespawnWakeSpot ARespawnMarker::GetWakeSpot() const
{
	FRespawnWakeSpot Spot;
	Spot.Grave = this;
	Spot.Location = GetActorLocation();
	Spot.Facing = FRotator(0.0, GetActorRotation().Yaw, 0.0);
	return Spot;
}

bool ARespawnMarker::IsActive(const FCampaignRecord& Campaign) const
{
	// Its mission counts as well as the record, so a roost opens even where nothing recorded it (finished in a level
	// played before this roost was placed).
	return bStartActive || Campaign.IsRespawnActive(GetMarkerId()) || (!ActiveAfterMission.IsNone() && Campaign.HasCompleted(ActiveAfterMission));
}

bool ARespawnMarker::Activate(FCampaignRecord& Campaign) const
{
	return Campaign.ActivateRespawn(GetMarkerId());
}

FString ARespawnMarker::DescribeState(const FCampaignRecord& Campaign) const
{
	if (bStartActive)
	{
		return TEXT("open from the start");
	}
	if (!ActiveAfterMission.IsNone() && Campaign.HasCompleted(ActiveAfterMission))
	{
		return FString::Printf(TEXT("open: %s finished"), *ActiveAfterMission.ToString());
	}
	if (Campaign.IsRespawnActive(GetMarkerId()))
	{
		return TEXT("opened");
	}
	return ActiveAfterMission.IsNone() ? FString(TEXT("closed")) : FString::Printf(TEXT("closed until %s is finished"), *ActiveAfterMission.ToString());
}

// ---------------------------------------------------------------------------
// Where a death wakes the player
// ---------------------------------------------------------------------------

bool ARespawnMarker::HasRoosts(const UWorld* World)
{
	if (!World)
	{
		return false;
	}
	for (TActorIterator<ARespawnMarker> It(World); It; ++It)
	{
		// A marker that is also a landing is where trips arrive, not one of Hob's.
		if (*It && !It->IsActorBeingDestroyed() && !AreaLandings::IsLanding(*It))
		{
			return true;
		}
	}
	return false;
}

FRespawnWakeSpot ARespawnMarker::ChooseWakeSpot(const UWorld* World, const FCampaignRecord* Campaign)
{
	FRespawnWakeSpot Spot;
	if (!World)
	{
		return Spot;
	}
	// The user's rule (2026-10-14, with two roosts a map): the roost they last found here, wherever they fell. The nearest
	// open one is never asked for: with two a map, the nearest could be one they had never walked up to.
	if (Campaign)
	{
		if (const ARespawnMarker* Last = FindLastFound(World, *Campaign, FindAreaId(World)))
		{
			Spot = Last->GetWakeSpot();
			Spot.bByCrows = true;
			return Spot;
		}
	}
	// None found yet (a new game before its first roost, a story never played here): the level's own start, where the
	// game begins (Ransom's Rest's is Ellis's family plot, the story's first wake-up), on its ground and inside the level;
	// never a trip's landing (a depot's or a jetty's player start).
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		const APlayerStart* Start = *It;
		if (!Start || Start->IsActorBeingDestroyed() || AreaLandings::IsLanding(Start))
		{
			continue;
		}
		Spot.Start = Start;
		Spot.Location = Start->GetActorLocation();
		Spot.Facing = FRotator(0.0, Start->GetActorRotation().Yaw, 0.0);
		break;
	}
	// Hob's crows still drag Ellis there on a map that has his roosts (his word comes at every death in the story); a map
	// with none (Skyreach) keeps its plain fade.
	Spot.bByCrows = Spot.Start && Campaign && HasRoosts(World);
	return Spot;
}
