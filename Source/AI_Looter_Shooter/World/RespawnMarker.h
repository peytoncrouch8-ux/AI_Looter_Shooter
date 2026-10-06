#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RespawnMarker.generated.h"

class ARespawnMarker;
class UArrowComponent;
class UMissionDefinition;
class UMissionRunner;
struct FCampaignRecord;

/** Where a death wakes the player (ARespawnMarker::ChooseWakeSpot): a grave, or the level's own start. */
struct AI_LOOTER_SHOOTER_API FRespawnWakeSpot
{
	/** A grave's spot on the ground (the player stands on it), or a player start's (the middle of the player goes there). */
	FVector Location = FVector::ZeroVector;

	/** Which way they face: the grave's or the start's, level. */
	FRotator Facing = FRotator::ZeroRotator;

	/** The grave they wake at; null at the level's start. */
	const ARespawnMarker* Grave = nullptr;

	/** The level's start they wake at when no grave is open; null at a grave. */
	const AActor* Start = nullptr;

	/** Somewhere was found (a level with neither a grave open nor a start of its own has nowhere). */
	bool IsSet() const { return Grave || Start; }
};

/**
 * A respawn grave (the family plot, the chapel yard, boot hill, the keeper's grave): a death wakes the player at the open
 * one nearest where they fell, standing on it and facing its arrow, or else at the level's own start as before, never at
 * a trip's landing (UPlayerVitalsSubsystem). The wake-up is the death fade for now; the grave wake-up camera is a scene.
 *
 * A grave opens with the story: it's open from the start (bStartActive), opens when ActiveAfterMission is finished (Main 1
 * opens the family plot, Main 4 the chapel yard), or is opened from the console. Openings are kept in the session's
 * campaign record by MarkerId, so they last through saves and trips. Place it on the ground beside its grave, where the
 * player gets up; it's never streamed out, since the nearest open grave can be far from where the player fell.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ARespawnMarker : public AActor
{
	GENERATED_BODY()

public:
	ARespawnMarker();

	/**
	 * Its name in the campaign record ("FamilyPlot", "KeepersGrave"): unique in the campaign, and never changed once a
	 * session may have saved it. None: the actor's name, which a rebuilt level may change.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn")
	FName MarkerId;

	/** Open from the start. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn")
	bool bStartActive = false;

	/** Opens when this mission is finished (its id). None: only the start or the console opens it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Respawn")
	FName ActiveAfterMission;

	/** MarkerId, or the actor's name without one. */
	FName GetMarkerId() const;

	/** It's open in this story: from the start, recorded open, or its mission finished. */
	bool IsActive(const FCampaignRecord& Campaign) const;

	/** Records it open in Campaign. True when it wasn't recorded before. */
	bool Activate(FCampaignRecord& Campaign) const;

	/** Why it's open ("open from the start", "open: Main1 finished", "opened"), or "closed", for logs and the console. */
	FString DescribeState(const FCampaignRecord& Campaign) const;

	/** The open grave nearest From in World, never one that is also a trip's landing; null when none is open. */
	static ARespawnMarker* FindNearestActive(const UWorld* World, const FVector& From, const FCampaignRecord& Campaign);

	/**
	 * Where a death at DeathLocation wakes the player: the nearest open grave, else the level's own start, never a trip's
	 * landing. Without a campaign (no story to have opened anything) only the start. Unset when the level has neither.
	 */
	static FRespawnWakeSpot ChooseWakeSpot(const UWorld* World, const FVector& DeathLocation, const FCampaignRecord* Campaign);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void HandleMissionFinished(const UMissionDefinition& Mission, bool bRewarded);

	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle FinishedHandle;

#if WITH_EDITORONLY_DATA
	/** The way the player faces on waking (the editor shows it; the game never does). */
	UPROPERTY()
	TObjectPtr<UArrowComponent> Arrow;
#endif
};
