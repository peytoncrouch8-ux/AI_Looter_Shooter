#include "World/RespawnMarker.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaLandings.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "World/RespawnMarkerLanternKit.h"
#include "Components/ArrowComponent.h"
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
	/** Seconds between looks at the player: a grave is a landmark, so a walk up to it is noticed within a third of a second. */
	constexpr float PollSeconds = 0.3f;
}

ARespawnMarker::ARespawnMarker()
{
	// Ticks only for the second of a lantern's flare (AdvanceLantern); the poll for the player is a timer.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Anchor->SetMobility(EComponentMobility::Movable);
	RootComponent = Anchor;

	// The lantern post beside the spot, the models found only once they're in this checkout so the class works, and its tests
	// run, without them.
	static UStaticMesh* const PostModel = RespawnLantern::FindIfMade<UStaticMesh>(RespawnLantern::PostPath);
	static UMaterialInterface* const TrimGlass = RespawnLantern::FindIfMade<UMaterialInterface>(RespawnLantern::DarkGlassPath);
	LanternMesh = PostModel;
	DarkGlass = TrimGlass;

	Lantern = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lantern"));
	Lantern->SetupAttachment(Anchor);
	Lantern->SetMobility(EComponentMobility::Movable);
	Lantern->SetStaticMesh(PostModel);
	// A post stops the player and shots as a post does, and no creature plans round it.
	Lantern->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Lantern->SetCanEverAffectNavigation(false);
	Lantern->SetCastShadow(true);

	LanternLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LanternLight"));
	LanternLight->SetupAttachment(Lantern);
	LanternLight->SetMobility(EComponentMobility::Movable);
	// A lantern's light: small, warm, never a shadow (the area's rule for every lantern), and gone beyond a long walk.
	LanternLight->SetCastShadows(false);
	LanternLight->SetIntensityUnits(ELightUnits::Candelas);
	LanternLight->SetIntensity(LanternCandela);
	LanternLight->SetAttenuationRadius(RespawnLantern::LightRadius);
	LanternLight->SetLightColor(RespawnLantern::Warm());
	LanternLight->SetMaxDrawDistance(RespawnLantern::LightDrawDistance);
	LanternLight->SetMaxDistanceFadeRange(RespawnLantern::LightFadeRange);
	LanternLight->SetVisibility(false);

	// The flame's glow cards (the gunfire's additive glow on the engine's plane), dark until the grave is marked.
	static UStaticMesh* const Plane = RespawnLantern::FindIfMade<UStaticMesh>(RespawnLantern::PlanePath);
	static UMaterialInterface* const Glow = RespawnLantern::FindIfMade<UMaterialInterface>(RespawnLantern::GlowPath);
	Flame = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Flame"));
	Flame->SetupAttachment(Lantern);
	Flame->SetMobility(EComponentMobility::Movable);
	Flame->SetStaticMesh(Plane);
	Flame->SetMaterial(0, Glow);
	Flame->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Flame->SetGenerateOverlapEvents(false);
	Flame->SetCanEverAffectNavigation(false);
	Flame->SetCastShadow(false);
	Flame->bReceivesDecals = false;
	Flame->SetNumCustomDataFloats(RespawnLantern::GlowFloats);
	Flame->SetVisibility(false);

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
		UE_LOG(LogLooter, Warning, TEXT("Respawn grave %s has no MarkerId: its opening is kept under its actor name, which a rebuilt level may change."),
			*GetActorNameOrLabel());
	}
	FitLantern();
	UMissionRunner* Runner = UMissionRunner::Get(this);
	// A grave marked in an earlier visit stands lit from the start, with no flare: the story is in the session's record.
	SetLit(Runner && Runner->IsActive() && IsMarked(Runner->GetCampaign()));
	if (bMarkOnApproach)
	{
		// A few looks a second, each grave off the others' beat; the work is one distance until the player is near.
		GetWorldTimerManager().SetTimer(PollTimer, this, &ARespawnMarker::PollApproach, PollSeconds, true, FMath::FRandRange(0.f, PollSeconds));
	}
	if (!Runner || ActiveAfterMission.IsNone())
	{
		return;
	}
	// Its mission may have been finished before this level began (elsewhere, or before graves existed): recorded now. That
	// opens the grave but never marks it: a save point is a grave the player has walked up to.
	if (Runner->GetCampaign().HasCompleted(ActiveAfterMission) && Activate(Runner->GetCampaign()))
	{
		UE_LOG(LogLooter, Log, TEXT("Respawn grave %s is open: %s was finished."), *GetMarkerId().ToString(), *ActiveAfterMission.ToString());
	}
	BoundRunner = Runner;
	FinishedHandle = Runner->OnMissionFinished.AddUObject(this, &ARespawnMarker::HandleMissionFinished);
}

void ARespawnMarker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
	Super::EndPlay(EndPlayReason);
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
		UE_LOG(LogLooter, Log, TEXT("Respawn grave %s opens: %s is finished."), *GetMarkerId().ToString(), *ActiveAfterMission.ToString());
	}
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
	// FamilyPlot -> "Family Plot", KeepersGrave -> "Keepers Grave": readable until the build gives it a name of its own.
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
	// Its mission counts as well as the record, so a grave opens even where nothing recorded it (finished in a level
	// played before this grave was placed).
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

ARespawnMarker* ARespawnMarker::FindNearestActive(const UWorld* World, const FVector& From, const FCampaignRecord& Campaign)
{
	if (!World)
	{
		return nullptr;
	}
	ARespawnMarker* Nearest = nullptr;
	double NearestDistance = TNumericLimits<double>::Max();
	for (TActorIterator<ARespawnMarker> It(World); It; ++It)
	{
		ARespawnMarker* Marker = *It;
		// A marker that is also a landing is where trips arrive: the dead never wake there.
		if (!Marker || Marker->IsActorBeingDestroyed() || AreaLandings::IsLanding(Marker) || !Marker->IsActive(Campaign))
		{
			continue;
		}
		// Measured through the air, height and all: a grave up on the knoll is farther from the Sink's floor than its map
		// distance says.
		const double Distance = FVector::DistSquared(Marker->GetActorLocation(), From);
		if (!Nearest || Distance < NearestDistance)
		{
			Nearest = Marker;
			NearestDistance = Distance;
		}
	}
	return Nearest;
}

FRespawnWakeSpot ARespawnMarker::ChooseWakeSpot(const UWorld* World, const FVector& DeathLocation, const FCampaignRecord* Campaign)
{
	FRespawnWakeSpot Spot;
	if (!World)
	{
		return Spot;
	}
	if (Campaign)
	{
		if (const ARespawnMarker* Grave = FindNearestActive(World, DeathLocation, *Campaign))
		{
			return Grave->GetWakeSpot();
		}
	}
	// No grave open: the level's own start, as before graves, never a trip's landing (a depot's or a jetty's player start).
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
	return Spot;
}
