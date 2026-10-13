// ARespawnMarker as a save point: walking up to a grave marks it, the last one marked in each area is where Continue wakes.

#include "World/RespawnMarker.h"
#include "AI_Looter_Shooter.h"
#include "Areas/AreaDefinition.h"
#include "Areas/AreaLandings.h"
#include "Areas/AreaTravelSubsystem.h"
#include "Audio/LooterSound.h"
#include "Audio/LooterSoundCues.h"
#include "Combat/HealthComponent.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Session/SessionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "World/GraveTravelSubsystem.h"
#include "World/WorldQueries.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The poll skips the rules until the player is within this far past the mark radius (cm), so a far-off grave costs one distance. */
	constexpr double NearPast = 400.0;

	/** A line to the grave's own spot is aimed this high over it (cm): the height of a standing man's chest. */
	constexpr double SpotHeight = 120.0;
}

// ---------------------------------------------------------------------------
// Where a Continue wakes
// ---------------------------------------------------------------------------

ARespawnMarker* ARespawnMarker::FindById(const UWorld* World, FName Id)
{
	if (!World || Id.IsNone())
	{
		return nullptr;
	}
	for (TActorIterator<ARespawnMarker> It(World); It; ++It)
	{
		ARespawnMarker* Marker = *It;
		if (Marker && !Marker->IsActorBeingDestroyed() && Marker->GetMarkerId() == Id)
		{
			return Marker;
		}
	}
	return nullptr;
}

FName ARespawnMarker::FindAreaId(const UWorld* World)
{
	if (!World)
	{
		return NAME_None;
	}
	const UMissionRunner* Runner = UMissionRunner::Get(World);
	if (Runner && !Runner->GetAreaHere().IsNone())
	{
		return Runner->GetAreaHere();
	}
	const UAreaDefinition* Area = UAreaDefinition::FindByMap(USessionSubsystem::MapOf(World));
	return Area ? Area->GetAreaId() : NAME_None;
}

const ARespawnMarker* ARespawnMarker::ChooseContinueGrave(const UWorld* World, const FVector& SavedLocation, const FCampaignRecord& Campaign,
	FName Area)
{
	// The grave the player last marked here, when it's in this level and open (a level rebuilt without it falls through).
	const ARespawnMarker* Marked = FindById(World, Campaign.GetLastGrave(Area));
	if (Marked && !AreaLandings::IsLanding(Marked) && Marked->IsActive(Campaign))
	{
		return Marked;
	}
	// None marked (a save from before save points, or a death before the first): the open grave nearest where they stood.
	return FindNearestActive(World, SavedLocation, Campaign);
}

// ---------------------------------------------------------------------------
// Marked or not
// ---------------------------------------------------------------------------

FName ARespawnMarker::GetMarkGate() const
{
	return MarkAfterMission.IsNone() ? ActiveAfterMission : MarkAfterMission;
}

bool ARespawnMarker::IsMarked(const FCampaignRecord& Campaign) const
{
	return GraveMarkRules::IsMarked(Campaign, GetMarkerId());
}

bool ARespawnMarker::IsSavePoint(const FCampaignRecord& Campaign, FName Area) const
{
	return GraveMarkRules::IsSavePoint(Campaign, Area, GetMarkerId());
}

bool ARespawnMarker::IsSavePoint() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Runner->IsActive() && IsSavePoint(Runner->GetCampaign(), FindAreaId(GetWorld()));
}

// ---------------------------------------------------------------------------
// Walking up to it
// ---------------------------------------------------------------------------

FGraveMarkSenses ARespawnMarker::GatherSenses(const AActor* Player) const
{
	FGraveMarkSenses Senses;
	Senses.bMarkOnApproach = bMarkOnApproach;
	Senses.bLanding = AreaLandings::IsLanding(this);
	Senses.Radius = MarkRadius;
	Senses.bHasPlayer = IsValid(Player);
	if (!Senses.bHasPlayer)
	{
		return Senses;
	}
	const UHealthComponent* Health = Player->FindComponentByClass<UHealthComponent>();
	Senses.bDying = Health && Health->IsDead();
	if (const USceneSubsystem* Scenes = USceneSubsystem::Get(this))
	{
		Senses.bInScene = Scenes->IsPlaying() || Scenes->IsHoldingPlayer();
	}
	const USessionSubsystem* Sessions = USessionSubsystem::Get(this);
	const UGraveTravelSubsystem* GraveTravel = UGraveTravelSubsystem::Get(this);
	const UAreaTravelSubsystem* Trips = UAreaTravelSubsystem::Get(this);
	Senses.bTravelling = (Sessions && Sessions->IsTravelling()) || (GraveTravel && GraveTravel->IsTravelling()) || (Trips && Trips->IsDeparting());

	const FName Gate = GetMarkGate();
	if (!Gate.IsNone())
	{
		const UMissionRunner* Runner = UMissionRunner::Get(this);
		Senses.bStoryAllows = Runner && Runner->GetCampaign().HasCompleted(Gate);
	}
	// Measured from their feet, through the air: a grave on the knoll is no nearer for a player on the cliff above it.
	const FVector Feet = Player->GetActorLocation() - FVector(0.0, 0.0, Player->GetSimpleCollisionHalfHeight());
	Senses.Distance = FVector::Dist(Feet, GetActorLocation());
	return Senses;
}

bool ARespawnMarker::HasSightLine(const AActor* Player) const
{
	const UWorld* World = GetWorld();
	if (!World || !Player)
	{
		return false;
	}
	const APawn* Pawn = Cast<APawn>(Player);
	const FVector Eyes = Pawn ? Pawn->GetPawnViewLocation() : Player->GetActorLocation();
	// The world's solid shapes only (walls, rocks, the ground; volumes are skipped), and the post itself and the player never
	// stand in the way. Either the flame or the grave's own spot in plain view is enough: a fence post between is no wall.
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("GraveMarkSight"), this, /*bTraceComplex*/ false);
	Params.AddIgnoredActor(Player);
	const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
	for (const FVector& Target : { GetLanternLocation(), GetActorLocation() + FVector(0.0, 0.0, SpotHeight) })
	{
		FHitResult Block;
		if (!World->LineTraceSingleByObjectType(Block, Eyes, Target, Solid, Params))
		{
			return true;
		}
	}
	return false;
}

EGraveMarkBlock ARespawnMarker::CheckMark(const AActor* Player) const
{
	const EGraveMarkBlock Block = GraveMarkRules::Check(GatherSenses(Player));
	if (Block != EGraveMarkBlock::None)
	{
		return Block;
	}
	// The line is the dearest question: asked last, and only of a player already near enough and free to mark.
	return HasSightLine(Player) ? EGraveMarkBlock::None : EGraveMarkBlock::NoSight;
}

void ARespawnMarker::PollApproach()
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}
	// Most of the time the player is nowhere near: one squared distance, and nothing else.
	const double Near = MarkRadius + NearPast;
	if (FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) > Near * Near)
	{
		return;
	}
	TryMark(Pawn);
}

EGraveMark ARespawnMarker::TryMark(AActor* Player, bool bForce)
{
	UMissionRunner* Runner = UMissionRunner::Get(this);
	// No one plays behind the main menu, and a level with no story keeps no save points.
	if (!Runner || !Runner->IsActive() || !IsValid(Player))
	{
		return EGraveMark::None;
	}
	if (bForce)
	{
		// The console marks a grave from anywhere, past its story and a wall: only a player who is down or none, and a landing, stop it.
		FGraveMarkSenses Senses = GatherSenses(Player);
		Senses.bMarkOnApproach = true;
		Senses.bInScene = false;
		Senses.bTravelling = false;
		Senses.bStoryAllows = true;
		Senses.Distance = 0.0;
		if (GraveMarkRules::Check(Senses) != EGraveMarkBlock::None)
		{
			return EGraveMark::None;
		}
	}
	else if (CheckMark(Player) != EGraveMarkBlock::None)
	{
		return EGraveMark::None;
	}

	FCampaignRecord& Campaign = Runner->GetCampaign();
	const EGraveMark Result = GraveMarkRules::Record(Campaign, FindAreaId(GetWorld()), GetMarkerId());
	if (Result == EGraveMark::None)
	{
		return Result;
	}
	if (Result == EGraveMark::First)
	{
		// A first mark opens the grave when the story hadn't (fast travel and a death's wake-up use it from now on).
		Activate(Campaign);
	}
	StartFlare(Result);
	Announce(Result, Player);
	UE_LOG(LogLooter, Log, TEXT("Respawn grave %s: %s, now the save point of %s."), *GetMarkerId().ToString(),
		Result == EGraveMark::First ? TEXT("marked") : TEXT("marked again"), *FindAreaId(GetWorld()).ToString());
	return Result;
}

void ARespawnMarker::Announce(EGraveMark Result, AActor* Player)
{
	const bool bFirst = Result == EGraveMark::First;
	// In the world, from the lantern, for as far as the grave is marked from.
	LooterSound::PlayAt(this, bFirst ? LooterSoundCue::GraveMark::Marked : LooterSoundCue::GraveMark::Remarked, GetLanternLocation());
	if (bFirst)
	{
		if (ALooterHUD* HUD = ALooterHUD::FindFor(Player))
		{
			HUD->ShowGraveMarked(GetGraveName());
		}
	}
	// Saved soon rather than at the next minute's autosave: a player who quits right after still wakes here.
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->SaveSoon();
	}
}
