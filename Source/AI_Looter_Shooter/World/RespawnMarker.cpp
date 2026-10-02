#include "World/RespawnMarker.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaLandings.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

ARespawnMarker::ARespawnMarker()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* Anchor = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Anchor->SetMobility(EComponentMobility::Movable);
	RootComponent = Anchor;

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
	UMissionRunner* Runner = UMissionRunner::Get(this);
	if (!Runner || ActiveAfterMission.IsNone())
	{
		return;
	}
	// Its mission may have been finished before this level began (elsewhere, or before graves existed): recorded now.
	if (Runner->GetCampaign().HasCompleted(ActiveAfterMission) && Activate(Runner->GetCampaign()))
	{
		UE_LOG(LogLooter, Log, TEXT("Respawn grave %s is open: %s was finished."), *GetMarkerId().ToString(), *ActiveAfterMission.ToString());
	}
	BoundRunner = Runner;
	FinishedHandle = Runner->OnMissionFinished.AddUObject(this, &ARespawnMarker::HandleMissionFinished);
}

void ARespawnMarker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
			Spot.Grave = Grave;
			Spot.Location = Grave->GetActorLocation();
			Spot.Facing = FRotator(0.0, Grave->GetActorRotation().Yaw, 0.0);
			return Spot;
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
