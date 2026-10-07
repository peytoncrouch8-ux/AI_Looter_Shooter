#include "Areas/StationBoard.h"
#include "Areas/AreaDefinition.h"
#include "Missions/MissionDefinition.h"
#include "Session/CampaignRecord.h"

#define LOCTEXT_NAMESPACE "StationBoard"

namespace
{
	const UAreaDefinition* FindArea(const TArray<UAreaDefinition*>& Areas, FName AreaId)
	{
		for (const UAreaDefinition* Area : Areas)
		{
			if (Area && Area->GetAreaId() == AreaId)
			{
				return Area;
			}
		}
		return nullptr;
	}

	/** A line going to Area, marked here when the board stands in it. */
	FStationBoardLine MakeAreaLine(UAreaDefinition& Area, EStationLine Kind, const FText& Name, FName Landing, FName HereAreaId)
	{
		FStationBoardLine Line;
		Line.Kind = Kind;
		Line.Area = &Area;
		Line.AreaId = Area.GetAreaId();
		Line.Name = Name;
		Line.SpokenName = Kind == EStationLine::Practice ? Name : StationBoard::SpokenName(Area);
		Line.Landing = Landing;
		Line.bHere = !HereAreaId.IsNone() && Line.AreaId == HereAreaId;
		Line.bLevelBuilt = Area.HasMap();
		return Line;
	}
}

FStationBoardWords FStationBoardWords::Station()
{
	FStationBoardWords Words;
	Words.Title = LOCTEXT("StationTitle", "Departures");
	Words.Depart = LOCTEXT("StationDepart", "Travel");
	Words.NotOpen = LOCTEXT("StationNotOpen", "The line to {0} isn't open yet.");
	return Words;
}

FStationBoardWords FStationBoardWords::Jetty()
{
	FStationBoardWords Words;
	Words.Title = LOCTEXT("JettyTitle", "Skiff jetty");
	Words.Depart = LOCTEXT("JettyDepart", "Cast off");
	Words.NotOpen = LOCTEXT("JettyNotOpen", "The skiff can't reach {0} yet.");
	return Words;
}

namespace StationBoard
{
	FName FirstAreaId()
	{
		return FName(TEXT("RansomsRest"));
	}

	FName FirstArrivalLanding()
	{
		return FName(TEXT("Landing_FamilyPlot"));
	}

	void RecordFirstCastOff(FCampaignRecord& Campaign)
	{
		Campaign.bFirstCastOff = true;
		Campaign.OpenArea(FirstAreaId());
	}

	bool IsAreaOpen(const FCampaignRecord& Campaign, FName AreaId)
	{
		// The first area is the story's start, so leaving Skyreach opens it even in a record that never wrote it down.
		return Campaign.IsAreaOpen(AreaId) || (Campaign.bFirstCastOff && !AreaId.IsNone() && AreaId == FirstAreaId());
	}

	const UMissionDefinition* FindNextMission(const FCampaignRecord& Campaign, const TArray<UMissionDefinition*>& Missions,
		const TArray<UAreaDefinition*>& Areas)
	{
		for (const UMissionDefinition* Mission : Missions)
		{
			if (!Mission || Campaign.HasCompleted(Mission->GetMissionId()))
			{
				continue;
			}
			for (const FName AreaId : Mission->Rewards.UnlockAreas)
			{
				// An area with no asset yet is still the next one: the line names the mission, never the place.
				const UAreaDefinition* Area = FindArea(Areas, AreaId);
				if (!AreaId.IsNone() && !IsAreaOpen(Campaign, AreaId) && !(Area && Area->bPractice))
				{
					return Mission;
				}
			}
		}
		return nullptr;
	}

	TArray<FStationBoardLine> BuildLines(const FCampaignRecord& Campaign, const TArray<UAreaDefinition*>& Areas,
		const TArray<UMissionDefinition*>& Missions, FName HereAreaId)
	{
		TArray<FStationBoardLine> Lines;

		// Before the first cast-off there is one trip: off Skyreach, to the story's first arrival.
		if (!Campaign.bFirstCastOff)
		{
			for (UAreaDefinition* Area : Areas)
			{
				if (Area && Area->GetAreaId() == FirstAreaId())
				{
					FStationBoardLine& Line = Lines.Add_GetRef(MakeAreaLine(*Area, EStationLine::Area, AreaName(*Area), FirstArrivalLanding(), HereAreaId));
					Line.bFirstCastOff = true;
				}
			}
			return Lines;
		}

		// Every opened area of the story, so travel never strands the player; trips arrive at each one's station.
		for (UAreaDefinition* Area : Areas)
		{
			if (Area && !Area->bPractice && IsAreaOpen(Campaign, Area->GetAreaId()))
			{
				Lines.Add(MakeAreaLine(*Area, EStationLine::Area, AreaName(*Area), Area->GetDefaultLanding(), HereAreaId));
			}
		}

		// The blank line: what opens the next area, without naming the place.
		if (const UMissionDefinition* Next = FindNextMission(Campaign, Missions, Areas))
		{
			FStationBoardLine& Line = Lines.AddDefaulted_GetRef();
			Line.Kind = EStationLine::NextMission;
			Line.MissionId = Next->GetMissionId();
			Line.Name = NextMissionText(*Next);
		}

		// Practice, from every board but its own: a plain trip to its jetty.
		for (UAreaDefinition* Area : Areas)
		{
			if (Area && Area->bPractice && Area->GetAreaId() != HereAreaId)
			{
				Lines.Add(MakeAreaLine(*Area, EStationLine::Practice, PracticeName(*Area), Area->GetDefaultLanding(), HereAreaId));
			}
		}
		return Lines;
	}

	FText AreaName(const UAreaDefinition& Area)
	{
		return Area.DisplayName.IsEmpty() ? FText::FromName(Area.GetAreaId()) : Area.DisplayName;
	}

	FText PracticeName(const UAreaDefinition& Area)
	{
		return FText::Format(LOCTEXT("PracticeName", "{0} (practice)"), AreaName(Area));
	}

	FText NextMissionText(const UMissionDefinition& Mission)
	{
		const FText Title = Mission.Title.IsEmpty() ? FText::FromName(Mission.GetMissionId()) : Mission.Title;
		return FText::Format(LOCTEXT("NextMission", "Finish ‘{0}’"), Title);
	}

	FText SpokenName(const UAreaDefinition& Area)
	{
		return Area.SpokenName.IsEmpty() ? AreaName(Area) : Area.SpokenName;
	}

	FText NotOpenText(const FStationBoardWords& Words, const FStationBoardLine& Line)
	{
		return FText::Format(Words.NotOpen, Line.SpokenName.IsEmpty() ? Line.Name : Line.SpokenName);
	}

	FName ReadEvent()
	{
		return FName(TEXT("StationBoard.Read"));
	}
}

#undef LOCTEXT_NAMESPACE
