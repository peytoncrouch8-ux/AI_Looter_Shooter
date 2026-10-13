// ARespawnMarker as a save point: walking up to a roost finds it, the last one found in each area is where Continue wakes.

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
#include "Story/CaptionSubsystem.h"
#include "UI/HUD/LooterHUD.h"
#include "World/GraveTravelSubsystem.h"
#include "World/RoostBeacon.h"
#include "World/RoostCrows.h"
#include "World/RoostKit.h"
#include "World/RoostSettings.h"
#include "World/WorldQueries.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** The poll skips the rules until the player is within this far past the find radius (cm), so a far-off roost costs one distance. */
	constexpr double NearPast = 400.0;

	/** A line to the roost's post is aimed this high over its foot (cm): a standing man's chest, under the crossbar. */
	constexpr double PostChest = 140.0;

	/** The beacon follows the camera's nearness from this far past the settings' NearFull (cm): it starts fading in time. */
	constexpr double WatchPast = 1500.0;

	/** The roost's sound stops this far past where it starts (cm): no stutter at the edge. */
	constexpr double AmbientHysteresis = 400.0;
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
	// The roost the player last found here, when it's in this level and open (a level rebuilt without it falls through).
	const ARespawnMarker* Marked = FindById(World, Campaign.GetLastGrave(Area));
	if (Marked && !AreaLandings::IsLanding(Marked) && Marked->IsActive(Campaign))
	{
		return Marked;
	}
	// None found (a save from before save points, or a death before the first): the open roost nearest where they stood.
	return FindNearestActive(World, SavedLocation, Campaign);
}

// ---------------------------------------------------------------------------
// Found or not
// ---------------------------------------------------------------------------

FName ARespawnMarker::GetMarkGate() const
{
	return MarkAfterMission.IsNone() ? ActiveAfterMission : MarkAfterMission;
}

bool ARespawnMarker::IsMarked(const FCampaignRecord& Campaign) const
{
	return GraveMarkRules::IsMarked(Campaign, GetMarkerId());
}

bool ARespawnMarker::IsFound(const FCampaignRecord& Campaign) const
{
	return IsMarked(Campaign) || IsActive(Campaign);
}

bool ARespawnMarker::IsFound() const
{
	const UMissionRunner* Runner = UMissionRunner::Get(this);
	return Runner && Runner->IsActive() ? IsFound(Runner->GetCampaign()) : bStartActive;
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
	// Measured from their feet, through the air: a roost on the knoll is no nearer for a player on the cliff above it.
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
	// The world's solid shapes only (walls, rocks, the ground; volumes are skipped), and the roost itself and the player
	// never stand in the way. Either the beacon on its top or the post at chest height in plain view is enough: a fence
	// post between is no wall.
	FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("RoostSight"), this, /*bTraceComplex*/ false);
	Params.AddIgnoredActor(Player);
	FTransform Foot;
	const FVector Post = GetRoostSocket(NAME_None, Foot) ? Foot.GetLocation() + FVector(0.0, 0.0, PostChest)
		: GetActorLocation() + FVector(0.0, 0.0, PostChest);
	const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
	for (const FVector& Target : { GetBeaconLocation(), Post })
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
	// The line is the dearest question: asked last, and only of a player already near enough and free to find it.
	return HasSightLine(Player) ? EGraveMarkBlock::None : EGraveMarkBlock::NoSight;
}

void ARespawnMarker::PollApproach()
{
	const UWorld* World = GetWorld();
	const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn)
	{
		UpdateAmbient(false);
		return;
	}
	// Most of the time the player is nowhere near: one squared distance, and nothing else.
	const URoostSettings& Settings = URoostSettings::Get();
	const double Away = FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation());
	const double Watch = Settings.NearFull + WatchPast;
	const double Ambient = Settings.AmbientRadius + (IsAmbientPlaying() ? AmbientHysteresis : 0.0);
	const double Near = MarkRadius + NearPast;
	const double Farthest = FMath::Max3(Watch, Ambient, Near);
	if (Away > Farthest * Farthest)
	{
		UpdateAmbient(false);
		if (Beacon)
		{
			Beacon->WatchView(false);
		}
		return;
	}
	// Near: its crows heard, and its beacon fading as the camera comes under it.
	UpdateAmbient(Away <= Ambient * Ambient);
	if (Beacon)
	{
		Beacon->WatchView(bLit && Away <= Watch * Watch);
	}
	if (Away <= Near * Near)
	{
		TryMark(Pawn);
	}
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
		// The console finds a roost from anywhere, past its story and a wall: only a player who is down or none, and a landing, stop it.
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
		// A first find opens the roost when the story hadn't (fast travel and a death's wake-up use it from now on).
		Activate(Campaign);
	}
	StartFlare(Result);
	Announce(Result, Player);
	UE_LOG(LogLooter, Log, TEXT("Roost %s: %s, now the save point of %s."), *GetMarkerId().ToString(),
		Result == EGraveMark::First ? TEXT("found") : TEXT("found again"), *FindAreaId(GetWorld()).ToString());
	return Result;
}

void ARespawnMarker::Announce(EGraveMark Result, AActor* Player)
{
	const bool bFirst = Result == EGraveMark::First;
	// In the world, from the bell, carried as far as the roost is found from.
	FTransform BellSpot;
	const FVector Ring = GetRoostSocket(RoostKit::BellSocket(), BellSpot) ? BellSpot.GetLocation() : GetBeaconLocation();
	LooterSound::PlayAt(this, bFirst ? LooterSoundCue::RoostMark::Found : LooterSoundCue::RoostMark::Remarked, Ring);
	if (bFirst)
	{
		// The crows lift off as its light catches, wheel once and settle.
		if (ARoostCrows* Birds = Crows.Get())
		{
			Birds->Lift(Player ? Player->GetActorLocation() : GetActorLocation());
		}
		if (ALooterHUD* HUD = ALooterHUD::FindFor(Player))
		{
			HUD->ShowGraveMarked(GetGraveName());
		}
		// The story's first roost: Hob says what they're for, once (the story keeps it said).
		UMissionRunner* Runner = UMissionRunner::Get(this);
		if (Runner && Runner->IsActive() && Runner->GetCampaign().SetFlag(RoostRules::FirstRoostFlag))
		{
			if (UCaptionSubsystem* Captions = UCaptionSubsystem::Get(this))
			{
				Captions->Play({ RoostRules::FirstRoostLine() }, ECaptionPlay::Queue);
			}
		}
	}
	// Saved soon rather than at the next minute's autosave: a player who quits right after still wakes here.
	if (USessionSubsystem* Sessions = USessionSubsystem::Get(this))
	{
		Sessions->SaveSoon();
	}
}
