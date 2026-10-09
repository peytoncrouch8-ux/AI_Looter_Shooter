#pragma once

#include "CoreMinimal.h"
#include "CampaignRecord.generated.h"

/**
 * The story so far, as a session saves it (ULooterSessionSave, from version 2): the missions finished (turned in), the
 * ones waiting to be turned in and the one being played, the areas open to travel, the bosses beaten, the respawn graves
 * opened, and the moments that happen once
 * (leaving Skyreach for the first time, the cold open). Missions, areas (UAreaDefinition::GetAreaId), bosses and graves
 * (ARespawnMarker::MarkerId) are named by id. The record is kept from step 8 on; the missions, the skiff and the station
 * board fill it in later steps.
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

	/**
	 * Respawn graves opened, by marker id, in the order they opened: a death wakes the player at the nearest open one. A
	 * grave open from the start, or whose mission is finished, is open whether or not it's listed. New within version 2:
	 * a save from before it reads as none opened, which is right, so it needs no upgrade.
	 */
	UPROPERTY()
	TArray<FName> ActiveRespawns;

	/**
	 * Missions whose objectives are all done, waiting to be turned in to their giver (UMissionDefinition::TurnIn), by id, in
	 * the order they got there: not finished yet (no rewards, not in CompletedMissions), and played on from there wherever
	 * the session goes on. The main one among them is also ActiveMission, its step one past its last. New within version 2:
	 * a save from before turn-ins reads as none waiting, which is right, so it needs no upgrade.
	 */
	UPROPERTY()
	TArray<FName> ReadyMissions;

	bool HasCompleted(FName Mission) const { return !Mission.IsNone() && CompletedMissions.Contains(Mission); }

	/** Records a mission as finished (once): it's no longer the one being played, nor waiting to be turned in. */
	void Complete(FName Mission)
	{
		if (Mission.IsNone())
		{
			return;
		}
		CompletedMissions.AddUnique(Mission);
		ReadyMissions.Remove(Mission);
		if (ActiveMission == Mission)
		{
			ActiveMission = NAME_None;
			ActiveMissionStep = 0;
		}
	}

	bool IsReadyToTurnIn(FName Mission) const { return !Mission.IsNone() && ReadyMissions.Contains(Mission); }

	/** Records a mission's objectives all done, waiting for its turn-in. False when it already was, or it's finished. */
	bool MarkReady(FName Mission)
	{
		if (Mission.IsNone() || ReadyMissions.Contains(Mission) || HasCompleted(Mission))
		{
			return false;
		}
		ReadyMissions.Add(Mission);
		return true;
	}

	/** Takes a mission off the turn-in list (played again from a step, or finished). False when it wasn't on it. */
	bool ClearReady(FName Mission)
	{
		return ReadyMissions.Remove(Mission) > 0;
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

	bool IsRespawnActive(FName Marker) const { return !Marker.IsNone() && ActiveRespawns.Contains(Marker); }

	/** Records a respawn grave opened. False when it already was. */
	bool ActivateRespawn(FName Marker)
	{
		if (Marker.IsNone() || ActiveRespawns.Contains(Marker))
		{
			return false;
		}
		ActiveRespawns.Add(Marker);
		return true;
	}
};
