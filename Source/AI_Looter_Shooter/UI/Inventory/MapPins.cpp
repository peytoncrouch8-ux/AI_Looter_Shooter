#include "UI/Inventory/MapPins.h"
#include "Economy/Vendor.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Missions/MissionSubsystem.h"
#include "Session/CampaignRecord.h"
#include "UI/World/MissionOfferWidget.h"
#include "World/BossShrine.h"
#include "World/GunsmithBench.h"
#include "World/RespawnMarker.h"
#include "World/SkiffJetty.h"
#include "World/TrainStation.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

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

		// No chests, found or opened: the map shows where to go, not where the loot is (the user's call, 2026-10-27).
		for (TActorIterator<AGunsmithBench> It(World); It; ++It)
		{
			FMapPin& Pin = Pins.AddDefaulted_GetRef();
			Pin.Kind = EMapPinKind::Bench;
			Pin.Location = It->GetActorLocation();
			Pin.Name = LOCTEXT("Bench", "Gunsmith's bench");
			Pin.Detail = LOCTEXT("BenchDetail", "Swap and scrap gun parts");
			Pin.Id = It->GetFName();
		}

		// The vigil stones that have risen: a stone not yet risen isn't there to find (its boss is still unbeaten). The pin's
		// second line is the stone's own word on its boss: ready to call back, ready in mm:ss, or out there now.
		for (TActorIterator<ABossShrine> It(World); It; ++It)
		{
			const ABossShrine* Stone = *It;
			if (!IsValid(Stone) || Stone->IsActorBeingDestroyed() || !Stone->IsRisen())
			{
				continue;
			}
			FMapPin& Pin = Pins.AddDefaulted_GetRef();
			Pin.Kind = EMapPinKind::Shrine;
			Pin.Location = Stone->GetActorLocation();
			Pin.Name = LOCTEXT("Shrine", "Vigil Stone");
			Pin.Detail = Stone->GetMapDetail();
			Pin.Id = Stone->BossKey;
		}

		// The stores that trade now: a vendor waiting on the story (OpenWhen, read from the same record as the graves') isn't
		// there to use yet, so it stays off the map; one with no gate is always there.
		for (TActorIterator<AVendor> It(World); It; ++It)
		{
			const AVendor* Vendor = *It;
			if (!IsValid(Vendor) || Vendor->IsActorBeingDestroyed())
			{
				continue;
			}
			const bool bOpen = Vendor->OpenWhen.IsEmpty() || (Sources.Campaign && Vendor->OpenWhen.IsMet(*Sources.Campaign, Sources.Runner));
			if (!bOpen)
			{
				continue;
			}
			FMapPin& Pin = Pins.AddDefaulted_GetRef();
			Pin.Kind = EMapPinKind::Vendor;
			Pin.Location = Vendor->GetActorLocation();
			Pin.Name = Vendor->Title.IsEmpty() ? LOCTEXT("Store", "Store") : Vendor->Title;
			Pin.Detail = LOCTEXT("StoreDetail", "Buy ammo and salt, sell your guns");
			Pin.Id = Vendor->GetFName();
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

		// Hob's roosts found (or opened by the story) in this one: shown however far, a travel point each. One not found yet
		// is the player's to find, and stays off the map. A roost that is only where the player rises after a death (the
		// Sheriff's Office's: the user keeps two travel stations, 2026-10-27) is no travel point, so no pin either.
		for (TActorIterator<ARespawnMarker> It(World); It; ++It)
		{
			const ARespawnMarker* Grave = *It;
			if (!IsValid(Grave) || Grave->IsActorBeingDestroyed() || !Grave->bFastTravelStation)
			{
				continue;
			}
			const bool bFound = Sources.Campaign ? Grave->IsFound(*Sources.Campaign) : Grave->bStartActive;
			if (!bFound)
			{
				continue;
			}
			FMapPin& Pin = Pins.AddDefaulted_GetRef();
			Pin.Kind = EMapPinKind::Grave;
			Pin.Location = Grave->GetActorLocation();
			Pin.Name = Grave->GetGraveName();
			Pin.Detail = LOCTEXT("GraveDetail", "Hob's roost: fast travel");
			Pin.Grave = Grave;
			Pin.Id = Grave->GetMarkerId();
		}

		// The tracked mission: its objective, or its giver once it waits to be turned in.
		const FMission* Tracked = Sources.Missions ? Sources.Missions->GetTracked() : nullptr;
		const bool bTrackedTurnIn = Tracked && Tracked->Waypoint.IsSet() && Tracked->Tracker.bTurnIn;
		if (Tracked && Tracked->Waypoint.IsSet())
		{
			const UMissionDefinition* TrackedMission = Sources.Runner ? Sources.Runner->FindDefinition(Sources.Runner->GetTrackedMission()) : nullptr;
			FMapPin& Pin = Pins.AddDefaulted_GetRef();
			Pin.Kind = bTrackedTurnIn ? EMapPinKind::TurnIn : EMapPinKind::Objective;
			Pin.Location = Tracked->Waypoint.GetValue();
			Pin.Name = Tracked->Title;
			Pin.Detail = FText::FromString(Tracked->Tracker.Line.IsEmpty() ? Tracked->Objective.ToString() : Tracked->Tracker.Line);
			Pin.bMain = TrackedMission && TrackedMission->Kind == EMissionKind::Main;
		}

		// The givers with a mission to offer now: one pin a giver (its main mission's, when it has one), saying who and how
		// many. What isn't offered yet stays off the map, as the story's to reveal.
		if (const UMissionRunner* Runner = Sources.Runner)
		{
			struct FGiverPin
			{
				FVector Location;
				const UMissionDefinition* Shown;
				const AActor* Giver;
				int32 Count;
			};
			TArray<FGiverPin> Givers;
			for (const UMissionDefinition* Mission : Runner->GetDefinitions())
			{
				if (!Mission || !Runner->IsAvailable(Mission->GetMissionId()))
				{
					continue;
				}
				const AActor* Giver = Runner->FindOfferGiverActor(*Mission);
				if (!Giver)
				{
					continue;
				}
				FGiverPin* Same = Givers.FindByPredicate([Giver](const FGiverPin& Each) { return Each.Giver == Giver; });
				if (Same)
				{
					++Same->Count;
					Same->Shown = Mission->Kind == EMissionKind::Main && Same->Shown->Kind != EMissionKind::Main ? Mission : Same->Shown;
					continue;
				}
				Givers.Add({ Giver->GetActorLocation(), Mission, Giver, 1 });
			}
			for (const FGiverPin& Each : Givers)
			{
				FMapPin& Pin = Pins.AddDefaulted_GetRef();
				Pin.Kind = EMapPinKind::Offer;
				Pin.Location = Each.Location;
				Pin.Name = Each.Shown->Title;
				const FText Who = FMissionOfferCard::GiverNameOf(*Each.Shown, Each.Giver);
				FText Detail;
				switch (FMissionOfferCard::GiverKindOf(*Each.Shown, Each.Giver))
				{
				case EMissionGiverKind::Board:    Detail = FText::Format(LOCTEXT("OfferBoard", "Posted on {0}"), Who); break;
				case EMissionGiverKind::Artifact: Detail = FText::Format(LOCTEXT("OfferArtifact", "Something to find: {0}"), Who); break;
				case EMissionGiverKind::Someone:  Detail = FText::Format(LOCTEXT("OfferSomeone", "Offered by {0}"), Who); break;
				}
				Pin.Detail = Each.Count > 1 ? FText::Format(LOCTEXT("OfferMore", "{0}, and {1} more"), Detail, Each.Count - 1) : Detail;
				Pin.Id = Each.Shown->GetMissionId();
				Pin.bMain = Each.Shown->Kind == EMissionKind::Main;
			}
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
				Pin.bMain = Mission->Kind == EMissionKind::Main;
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
		case EMapPinKind::Bench:       return LOCTEXT("KindBench", "Gunsmith's bench");
		case EMapPinKind::Shrine:      return LOCTEXT("KindShrine", "Vigil Stone");
		case EMapPinKind::Vendor:     return LOCTEXT("KindVendor", "Store");
		case EMapPinKind::Station:    return LOCTEXT("KindStation", "Station");
		case EMapPinKind::Grave:       return LOCTEXT("KindGrave", "Hob's roost");
		case EMapPinKind::Offer:       return LOCTEXT("KindOffer", "Mission offered");
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
