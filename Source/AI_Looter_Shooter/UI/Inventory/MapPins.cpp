#include "UI/Inventory/MapPins.h"
#include "Loot/Chest.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "World/GunsmithBench.h"
#include "World/RespawnMarker.h"
#include "World/SkiffJetty.h"
#include "World/TrainStation.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#define LOCTEXT_NAMESPACE "MapPins"

namespace
{
	/** Closer than this to the tracked turn-in (cm), a giver's pin is the same pin. */
	constexpr double SamePinDistance = 150.0;
}

namespace MapPins
{
	TArray<FMapPin> Gather(const FMapPinSources& Sources)
	{
		TArray<FMapPin> Pins;
		const UWorld* World = Sources.World;
		if (!World)
		{
			return Pins;
		}

		// Chests: an opened one stays (dim), as the session keeps it open; a closed one once the player has come across it.
		// Only the treasure (the caches, the strongboxes): not every grave, coffin and mailbox in the lootable world.
		for (TActorIterator<AChest> It(World); It; ++It)
		{
			const AChest* Chest = *It;
			if (!IsValid(Chest) || !Chest->ShowsOnMap())
			{
				continue;
			}
			const bool bLooted = Chest->GetState() != EChestState::Closed;
			const bool bFound = Sources.FoundChests && Sources.FoundChests->Contains(Chest->GetSaveKey());
			if (!bLooted && !bFound)
			{
				continue;
			}
			FMapPin& Pin = Pins.AddDefaulted_GetRef();
			Pin.Kind = bLooted ? EMapPinKind::ChestLooted : EMapPinKind::Chest;
			Pin.Location = Chest->GetActorLocation();
			Pin.Name = AChest::KindName(Chest->Kind);
			Pin.Detail = bLooted ? LOCTEXT("Looted", "Looted") : LOCTEXT("NotOpened", "Not opened yet");
			Pin.Id = Chest->GetSaveKey();
		}

		for (TActorIterator<AGunsmithBench> It(World); It; ++It)
		{
			FMapPin& Pin = Pins.AddDefaulted_GetRef();
			Pin.Kind = EMapPinKind::Bench;
			Pin.Location = It->GetActorLocation();
			Pin.Name = LOCTEXT("Bench", "Gunsmith's bench");
			Pin.Detail = LOCTEXT("BenchDetail", "Swap and scrap gun parts");
			Pin.Id = It->GetFName();
		}

		// Where trips leave from: a station's board (the train), and Skyreach's jetty (the skiff).
		for (TActorIterator<ATrainStation> It(World); It; ++It)
		{
			FMapPin& Pin = Pins.AddDefaulted_GetRef();
			Pin.Kind = EMapPinKind::Station;
			Pin.Location = It->GetActorLocation();
			Pin.Name = LOCTEXT("Station", "Train station");
			Pin.Detail = LOCTEXT("StationDetail", "The departures board");
			Pin.Id = It->GetFName();
		}
		for (TActorIterator<ASkiffJetty> It(World); It; ++It)
		{
			FMapPin& Pin = Pins.AddDefaulted_GetRef();
			Pin.Kind = EMapPinKind::Station;
			Pin.Location = It->GetActorLocation();
			Pin.Name = LOCTEXT("Jetty", "Skiff jetty");
			Pin.Detail = LOCTEXT("JettyDetail", "The skiff's slate");
			Pin.Id = It->GetFName();
		}

		// The respawn graves open in this story: a closed one is the story's to reveal.
		for (TActorIterator<ARespawnMarker> It(World); It; ++It)
		{
			const ARespawnMarker* Grave = *It;
			if (!IsValid(Grave) || Grave->IsActorBeingDestroyed())
			{
				continue;
			}
			const bool bOpen = Sources.Campaign ? Grave->IsActive(*Sources.Campaign) : Grave->bStartActive;
			if (!bOpen)
			{
				continue;
			}
			FMapPin& Pin = Pins.AddDefaulted_GetRef();
			Pin.Kind = EMapPinKind::Grave;
			Pin.Location = Grave->GetActorLocation();
			Pin.Name = Grave->GetGraveName();
			Pin.Detail = LOCTEXT("GraveDetail", "Respawn grave: fast travel");
			Pin.Grave = Grave;
			Pin.Id = Grave->GetMarkerId();
		}

		// The tracked mission: its objective, or its giver once it waits to be turned in.
		const FMission* Tracked = Sources.Missions ? Sources.Missions->GetTracked() : nullptr;
		const bool bTrackedTurnIn = Tracked && Tracked->Waypoint.IsSet() && Tracked->Tracker.bTurnIn;
		if (Tracked && Tracked->Waypoint.IsSet())
		{
			FMapPin& Pin = Pins.AddDefaulted_GetRef();
			Pin.Kind = bTrackedTurnIn ? EMapPinKind::TurnIn : EMapPinKind::Objective;
			Pin.Location = Tracked->Waypoint.GetValue();
			Pin.Name = Tracked->Title;
			Pin.Detail = FText::FromString(Tracked->Tracker.Line.IsEmpty() ? Tracked->Objective.ToString() : Tracked->Tracker.Line);
		}

		// Every other mission waiting to be turned in here, at its giver.
		if (const UMissionRunner* Runner = Sources.Runner)
		{
			for (const UMissionDefinition* Mission : Runner->GetDefinitions())
			{
				if (!Mission || !Runner->IsRunning(Mission->GetMissionId()) || !Runner->IsReadyToTurnIn(Mission->GetMissionId()))
				{
					continue;
				}
				const TOptional<FVector> Giver = Runner->FindGiver(*Mission);
				if (!Giver.IsSet() || (bTrackedTurnIn && FVector::Dist(Giver.GetValue(), Tracked->Waypoint.GetValue()) < SamePinDistance))
				{
					continue;
				}
				FMapPin& Pin = Pins.AddDefaulted_GetRef();
				Pin.Kind = EMapPinKind::TurnIn;
				Pin.Location = Giver.GetValue();
				Pin.Name = Mission->Title;
				Pin.Detail = FText::FromString(Mission->GetTurnInText());
				Pin.Id = Mission->GetMissionId();
			}
		}

		// In drawing order: the later kinds on top (stable, so pins of a kind keep the level's order).
		Pins.StableSort([](const FMapPin& A, const FMapPin& B) { return static_cast<uint8>(A.Kind) < static_cast<uint8>(B.Kind); });
		return Pins;
	}

	FText KindName(EMapPinKind Kind)
	{
		switch (Kind)
		{
		case EMapPinKind::ChestLooted: return LOCTEXT("KindLooted", "Looted chest");
		case EMapPinKind::Chest:       return LOCTEXT("KindChest", "Chest");
		case EMapPinKind::Bench:       return LOCTEXT("KindBench", "Gunsmith's bench");
		case EMapPinKind::Station:     return LOCTEXT("KindStation", "Station");
		case EMapPinKind::Grave:       return LOCTEXT("KindGrave", "Respawn grave");
		case EMapPinKind::TurnIn:      return LOCTEXT("KindTurnIn", "Turn in");
		case EMapPinKind::Objective:   return LOCTEXT("KindObjective", "Objective");
		}
		return FText::GetEmpty();
	}

	int32 Count(TConstArrayView<FMapPin> Pins, EMapPinKind Kind)
	{
		int32 Found = 0;
		for (const FMapPin& Pin : Pins)
		{
			Found += Pin.Kind == Kind ? 1 : 0;
		}
		return Found;
	}
}

#undef LOCTEXT_NAMESPACE
