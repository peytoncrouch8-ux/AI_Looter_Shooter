#pragma once

#include "CoreMinimal.h"
#include "CampaignRecord.generated.h"

/**
 * The story so far, as a session saves it (ULooterSessionSave, from version 2): the missions finished and the one being
 * played, the areas open to travel, the bosses beaten, and the moments that happen once (leaving Skyreach for the first
 * time, the cold open). Missions, areas (UAreaDefinition::GetAreaId) and bosses are named by id. The record is kept from
 * step 8 on; the missions, the skiff and the station board fill it in later steps.
 */
USTRUCT()
struct AI_LOOTER_SHOOTER_API FCampaignRecord
{
	GENERATED_BODY()

	/** Missions finished, by id, in the order they were finished. */
	UPROPERTY()
	TArray<FName> CompletedMissions;

	/** The story mission being played (None: none) and the step it's on. */
	UPROPERTY()
	FName ActiveMission;

	UPROPERTY()
	int32 ActiveMissionStep = 0;

	/** Areas the station boards list, by area id, in the order they opened. */
	UPROPERTY()
	TArray<FName> OpenedAreas;

	/** Bosses beaten at least once, by id, in the order of their first defeat: a first defeat's scene and reward come once. */
	UPROPERTY()
	TArray<FName> DefeatedBosses;

	/** The player has left Skyreach for the first time (the first cast-off, or "Skip the tutorial"): the story has begun. */
	UPROPERTY()
	bool bFirstCastOff = false;

	/** The cold open has played (once, on the first cast-off). */
	UPROPERTY()
	bool bColdOpenSeen = false;

	bool HasCompleted(FName Mission) const { return !Mission.IsNone() && CompletedMissions.Contains(Mission); }

	/** Records a mission as finished (once); it's no longer the one being played. */
	void Complete(FName Mission)
	{
		if (Mission.IsNone())
		{
			return;
		}
		CompletedMissions.AddUnique(Mission);
		if (ActiveMission == Mission)
		{
			ActiveMission = NAME_None;
			ActiveMissionStep = 0;
		}
	}

	bool IsAreaOpen(FName Area) const { return !Area.IsNone() && OpenedAreas.Contains(Area); }

	/** Opens an area to travel. False when it already was. */
	bool OpenArea(FName Area)
	{
		if (Area.IsNone() || OpenedAreas.Contains(Area))
		{
			return false;
		}
		OpenedAreas.Add(Area);
		return true;
	}

	/** Records a boss beaten. True only the first time: the first defeat's scene and reward. */
	bool RecordBossDefeat(FName Boss)
	{
		if (Boss.IsNone() || DefeatedBosses.Contains(Boss))
		{
			return false;
		}
		DefeatedBosses.Add(Boss);
		return true;
	}
};
